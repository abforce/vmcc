#include "monitorpanel.h"
#include "ui_monitorpanel.h"

// Columns indices of the site information table
#define COL_SITE_NAME       0
#define COL_CONNECTIVITY    1
#define COL_STATUS          2
#define COL_POWER           3

// Columns indices of the site data structures table.
#define COL_DS_SITE_NAME    0
#define COL_DS_SN           1
#define COL_DS_SV           2

MonitorPanel::MonitorPanel(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MonitorPanel)
{
    ui->setupUi(this);
    timerFlasher = new QTimer();
    timerFlasher->setInterval(150);
    connect(timerFlasher, SIGNAL(timeout()), this, SLOT(onFlashTimerTimeout()));
    // Register vixCallback static method as a delegation for all asynchronous vix calls.
    VmVixHelperAsync::registerCallback(MonitorPanel::vixCallback);

    // vixCallbackSignal will be emitted inside vixCallback method, which is run on background thread, so we
    // in order to run on main thread, should emit a signal and catch it on a main thread slot.
    connect(this, SIGNAL(vixCallbackSignal(UserData*,int)), this, SLOT(vixCallbackGuiThread(UserData*,int)));

    // A delegation for click signals of all sites' action button.
    sitesActionClickedMapper = new QSignalMapper(this);
    connect(sitesActionClickedMapper, SIGNAL(mapped(QObject*)), this, SLOT(siteActionClicked(QObject*)));

    // List of all added sites.
    listSites = new QList<SiteInfo*>();

    // An auto incrementing id for sites' sockets.
    nextSiteId = 0;

    // SocketPool manages all sites' sockets and also handles their signals
    socketPool = new SocketPool();
    connect(socketPool, SIGNAL(socketReadyRead(QTcpSocket*,int)), this, SLOT(onSocketReadyRead(QTcpSocket*,int)));
    connect(socketPool, SIGNAL(socketDisconnected(QTcpSocket*,int)), this, SLOT(onSocketDisconnected(QTcpSocket*,int)));

    QHeaderView* header = ui->tblSitesInfo->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header = ui->tblSitesDataStructures->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);

    graph = new GraphVisualizer(ui->qgvGraph);
    timeline = new TimeLineVisualizer(ui->qgvTimeLine);
    watchdog = new QFileSystemWatcher();
    connect(watchdog, SIGNAL(fileChanged(QString)), this, SLOT(onWatchdogBark(QString)));
    setStatusTip("The algorihm is not running");
}

MonitorPanel::~MonitorPanel()
{
    delete ui;
}

// ==============================================================================================================
// ==============================================================================================================
//
//                                  N E T W O R K I N G --- S T U F F
//
// ==============================================================================================================
// ==============================================================================================================

/**
 * @brief MonitorPanel::startServer
 *  This method starts the network server of this program, in order to establish
 *  a communication channel between sites and this program.
 * @param ifName
 *  Network interface to start server on.
 * @param port
 *  Listening port of our server.
 * @return
 *  true if success otherwise false.
 */
bool MonitorPanel::startServer(QString ifName, int port)
{
    server = new QTcpServer(this);
    connect(server, SIGNAL(newConnection()), this, SLOT(requestForConnectionReceived()));

    // Getting address of this interface
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(ifName);
    QList<QNetworkAddressEntry> addresses = interface.addressEntries();

    // If no address is assigned to this interface
    if(addresses.size() == 0){
        emit logcat("Network", "Interface '" + ifName +"' is not connected.");
        return false;
    }

    // Get first (main not aliases) available IP address of this interface.
    // Also we have not interested in IPv6.
    QHostAddress address = addresses.at(0).ip();
    if(address.protocol() != QAbstractSocket::IPv4Protocol){
        emit logcat("Network", "Interface '" + ifName + "' does not have IPv4 address");
        return false;
    }

    // Store specification of this entry point in class members
    serverAddress = address;
    serverPort = port;

    // Listen now!
    if(server->listen(serverAddress, port)){
        emit logcat("Network", "Server is listening on " + serverAddress.toString() + ":" + QString::number(port));
        return true;
    } else {
        emit logcat("Network", "Server failed to listen");
        return false;
    }
}

/**
 * @brief MonitorPanel::getLogcatWidget
 *  Returns a reference to the logcat widget supplied in this window.
 *  Actually this logcat widget and the one shown in main window are populated by a single model.
 * @return
 */
LogcatWidget *MonitorPanel::getLogcatWidget()
{
    return ui->logcatWidget;
}

/**
 * @brief MonitorPanel::requestForConnectionReceived
 *  Our server object got a socket connection on the specified interface.
 */
void MonitorPanel::requestForConnectionReceived()
{
    // Logcat the event.
    emit logcat("Network", "A client socket was connected. Socket ID=" + QString::number(nextSiteId));

    // It's up to the socket pool to handle this socket.
    // Auto incrementing site socket id.
    socketPool->addSocket(server->nextPendingConnection(), nextSiteId++);
}

/**
 * @brief MonitorPanel::onSocketReadyRead
 *  A socket callback as getting called whenever some data received by a socket.
 * @param socket
 *  Pointer to the ready-to-read socket.
 * @param id
 *  Socket ID for this socket. Should not be confused with site ID.
 */
void MonitorPanel::onSocketReadyRead(QTcpSocket *socket, int id)
{
    try{
        QByteArray dataBytes = socket->readAll();
        std::vector<std::string> messagesList = MessageForger::splitMessages(dataBytes.toStdString());

        for(int i = 0, count = messagesList.size(); i < count; i += 1){
            string msgstr = messagesList[i];

            // Try to parse this message
            MessageForger::Message message = MessageForger::parse(msgstr);

            // If a parse-level error occurred...
            if(message.type == MessageForger::ERROR){
                throw 1;
            }

            handleMessage(socket, id, message, QByteArray::fromStdString(msgstr));
        }

    } catch(int code){
        // Any error would not be tolerated. Abandon the socket.
        socketPool->removeSocket(socket);
        socket->disconnectFromHost();
        emit logcat("Network", "There found a problem with socket " + QString::number(id) + ". Socket forcefully closed. code=" + QString::number(code));
    }
}

/**
 * @brief MonitorPanel::handleMessage
 * @param socket
 * @param socketId
 * @param message
 * @param dataBytes
 */
void MonitorPanel::handleMessage(QTcpSocket *socket, int socketId, MessageForger::Message message, QByteArray dataBytes) throw(int)
{
    // Retrive entry of this site from the list.
    // Fields sender and receiver are of type site id.
    SiteInfo *sender = listSites->at(message.sender);

    // Each newbie MUST say hello first, It's the manner.
    if(sender->state == SiteInfo::STATE_NEW){
        if(message.type != MessageForger::HELLO){
            throw 3;
        }
        sender->socketId = socketId;
        sender->state = SiteInfo::STATE_HELLO_SAIED;
        sender->connected = true;
        updateTableSiteInfo(sender->id, COL_CONNECTIVITY);
        emit logcat(sender->name, "The site saied Hello");

        // Sending a hello back
        MessageForger::Message serverHello(MessageForger::HELLO, MessageForger::CONTROLER_SITE_ID, message.sender);
        serverHello.initialTockenHolder = sender->holder;
        serverHello.sitesCount = listSites->count();
        socket->write(MessageForger::serialize(serverHello));
        emit logcat("VMCC", "VMCC sent a Hello back to " + sender->name);
    } else {
        if(message.receiver == MessageForger::CONTROLER_SITE_ID){
            if(message.type == MessageForger::WRITE){
                timeline->addCriticalSectionEvent(sender->id);
                emit logcat("Network", "Write message received from site " + sender->name);
                emit logcat(sender->name, "Doing critical section");
                emit logcat("VMCC", "Request for critical operation received from " + sender->name);

                // Whether this file should be added to the watchdog to be watched
                QString filename = QString::fromStdString(message.fileName);
                if(!watchdog->files().contains(filename)){
                    watchdog->addPath(filename);
                    ui->lstWatchedFiles->addItem(filename);
                }

                // Write file
                writeFile(filename, QString::fromStdString(message.fileContent));
            }
        } else {
            SiteInfo *receiver = listSites->at(message.receiver);
            if(message.type == MessageForger::TOKEN){
                receiver->holder = true;
                sender->holder = false;
                QString tsn = "[";
                QString tsv = "[";
                for(int i = 0, count = listSites->size(); i < count; i += 1){
                    tsn += " " + QString::number(message.TSN[i]) + " ";
                    QString state;
                    switch(message.TSV[i]){
                    case STATE_R: state = "R"; break;
                    case STATE_E: state = "E"; break;
                    case STATE_H: state = "H"; break;
                    case STATE_N: state = "N"; break;
                    }
                    tsv += " " + state + " ";
                }
                tsn += "]";
                tsv += "]";
                ui->lblTSN->setText(tsn);
                ui->lblTSV->setText(tsv);
                delete[] message.TSN;
                delete[] message.TSV;
                graph->invalidateScene();
                timeline->addTransitionEvent(sender->id, receiver->id, TimeLineVisualizer::TOKEN);
                emit logcat("Singhal", "Token has been sent from " + sender->name + " to " + receiver->name);
                emit logcat(sender->name, "The site gives the token to " + receiver->name);
                emit logcat(receiver->name, "The site gets the token from " + sender->name);
            }
            if(message.type == MessageForger::REQUEST){
                timeline->addTransitionEvent(sender->id, receiver->id, TimeLineVisualizer::REQUEST);
                emit logcat("Singhal", "A request has been sent from " + sender->name + " to " + receiver->name);
                emit logcat(sender->name, "The site sends a request to " + receiver->name);
                emit logcat(receiver->name, "The site receives a request from " + sender->name);
            }
            if(message.type == MessageForger::TOKEN || message.type == MessageForger::REQUEST){
                QString sn = "[";
                QString sv = "[";
                for(int i = 0, count = listSites->size(); i < count; i += 1){
                    sn += " " + QString::number(message.SN[i]) + " ";
                    QString state;
                    switch(message.SV[i]){
                    case STATE_R: state = "R"; break;
                    case STATE_E: state = "E"; break;
                    case STATE_H: state = "H"; break;
                    case STATE_N: state = "N"; break;
                    }
                    sv += " " + state + " ";
                }
                sn += "]";
                sv += "]";
                sender->SN = sn;
                sender->SV = sv;
                delete[] message.SN;
                delete[] message.SV;

                updateTableSiteDataStructures(sender->id, COL_DS_SN);
                updateTableSiteDataStructures(sender->id, COL_DS_SV);
            }

            // Relay the message to its receiver
            QTcpSocket *socket = socketPool->getSocketById(receiver->socketId);
            if(socket != 0){
                if(socket->write(dataBytes) <= 0){
                    emit logcat("Network", "FATAL ERROR: Could not write data on socket " + QString::number(socketId));
                } else {
                    socket->flush();
                }
            }
        }
    }
}

/**
 * @brief MonitorPanel::writeFile
 * @param filename
 * @param content
 */
void MonitorPanel::writeFile(QString filename, QString content)
{
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly)){
        QTextStream stream(&file);
        stream << content;
        stream.flush();
        file.flush();
        file.close();
    } else {
        emit logcat("App", "Failed to write file as a critical operation");
    }
}

/**
 * @brief MonitorPanel::onSocketDisconnected
 * @param socket
 * @param id
 */
void MonitorPanel::onSocketDisconnected(QTcpSocket *socket, int id)
{
    // Logcat the event
    emit logcat("Network", "A client socket was disconnected. Socket ID=" + QString::number(id));

    // Remove it from our socket pool
    socketPool->removeSocket(socket);

    // We've got only it's socket id, we've to search for it's site id
    for(int i = 0, count = listSites->count(); i < count; i += 1){
        if(listSites->at(i)->socketId == id){
            SiteInfo *site = listSites->at(i);

            // Site is now offline.
            site->connected = false;

            // Update it's entry in the table
            updateTableSiteInfo(site->id, COL_CONNECTIVITY);
            break;
        }
    }
}

// ==============================================================================================================
// ==============================================================================================================
//
//                                    V I X --- A S Y N C --- S T U F F
//
// ==============================================================================================================
// ==============================================================================================================

/**
 * @brief MonitorPanel::vixCallback
 *  This is a static method of this class that is delegated for all asych VIX calls. Note that this method always
 *  run on a background thread created by VIX libraries. Hence we are not allowed here to do GUI stuff.
 * @param data
 *  A data structure that is used to store state among various VIX calls
 * @param result
 *  Result of the ordered VIX operation. Of type VmVixHelper::OperationResult
 */
void MonitorPanel::vixCallback(UserData *data, int result)
{
    // Extract 'this' reference
    MonitorPanel *pThis = (MonitorPanel *)data->context;

    // Send a signal for informing VIX call has finished.
    pThis->emitSignalVixCallback(data, result);
}

/**
 * @brief MonitorPanel::emitSignalVixCallback
 *  We are still on a background thread created by VIX functions.
 * @param data
 *  Bundle of our identities!
 * @param result
 *  Operation result
 */
void MonitorPanel::emitSignalVixCallback(UserData *data, int result)
{
    // Emit the job ready signal from the background thread to be catch on a UI thread slot.
    emit vixCallbackSignal(data, result);
}

/**
 * @brief MonitorPanel::vixCallbackGuiThread
 *  Member method, not static method, of this class and is intended to run on UI thread and handle
 *  VIX callbacks.
 * @param data
 *  The bundle
 * @param result
 *  Operaton result
 */
void MonitorPanel::vixCallbackGuiThread(UserData *data, int result)
{
    // We had passed site id as a request code for this async VIX call.
    // So we now treat this request code as site id.
    SiteInfo *site = listSites->at(data->requestCode);

    // Site id also represents row index of corresponding site entry in the table.
    int row = data->requestCode;

    // Which method has been called ?
    switch (data->callee) {
    case VmVixHelperAsync::CALL_POWER_ON:{
        if(result == VmVixHelper::OperationResult::SUCCESS){

            // Site is now running.
            site->running = true;

            // Inform the logcat
            emit logcat("VIX", "The site " + site->name + " powered on");

            // Store Virtual Machine Handle for this site
            // That's used for futher uses.
            site->vmHandle = data->vmHandle;

            // Update it's row on the table
            updateTableSiteInfo(row, COL_STATUS);

            // We should now run Singhal client on the site.

            // Full address to its executable file.
            const QString program = "/root/SinghalClient";

            // Command line arguments
            // Args: <server ip in dotted notation> <server port> <site id>
            const QString args = serverAddress.toString() + " " + QString::number(serverPort) + " " + QString::number(site->id);

            // Issue the request.
            VmVixHelperAsync::asyncRunProgramInGuest(this, data->vmHandle, site->username, site->password, data->requestCode, program, args);

        } else {
            QString message = "Could not power on site " + site->name;
            emit logcat("VIX", message);
            showMessage(message);
        }
        updateTableSiteInfo(row, COL_POWER);
        break;
    }

    case VmVixHelperAsync::CALL_POWER_OFF:{
        if(result == VmVixHelper::OperationResult::SUCCESS){
            site->running = false;
            site->connected = false;
            updateTableSiteInfo(row, COL_STATUS);
            updateTableSiteInfo(row, COL_CONNECTIVITY);
        } else {
            QString message = "Could not stop site " + site->name;
            emit logcat("VIX", message);
            showMessage(message);
        }
        updateTableSiteInfo(row, COL_POWER);
        break;
    }

    case VmVixHelperAsync::CALL_RUN_PROGRAM_IN_GUEST:{
        if(result == VmVixHelper::OperationResult::SUCCESS){
            emit logcat("VIX", "Singhal client of site " + site->name + " run.");
        } else {
            QString message = "Could not run Singhal client in site " + site->name;
            emit logcat("VIX", message);
            showMessage(message);
        }
        break;
    }
    }
}

// ==============================================================================================================
// ==============================================================================================================
//
//                                           G U I --- S T U F F
//
// ==============================================================================================================
// ==============================================================================================================

/**
 * @brief MonitorPanel::siteActionClicked
 *  Power button of a site listed in the site information table has been clicked.
 * @param s
 *  A poiter to the SiteInfo structure of the clicked entry.
 */
void MonitorPanel::siteActionClicked(QObject *s)
{
    SiteInfo *site = (SiteInfo *)s;
    if(site->running){
        VmVixHelperAsync::asyncPowerOff(this, site->vmHandle, site->id);
        site->btnAction->setText("Stopping...");
        site->btnAction->setEnabled(false);
    } else {
        VmVixHelperAsync::asyncPowerOn(this, site->vmxPath, site->id);
        site->btnAction->setText("Starting...");
        site->btnAction->setEnabled(false);
    }
}

/**
 * @brief MonitorPanel::showMessage
 *  Shows a message box to inform user of a particular event.
 * @param message
 *  Message to be poped up.
 */
void MonitorPanel::showMessage(QString message)
{
    QMessageBox::information(this, "Message", message);
}

/**
 * @brief MonitorPanel::updateTableSiteInfo
 *  Given row and column indices, updates a sepecified cell of the sites information table.
 * @param row
 *  The row index of the cell to be updated.
 * @param column
 *  The column index of the cell to be updated.
 */
void MonitorPanel::updateTableSiteInfo(int row, int column)
{
    QTableWidget *tbl = ui->tblSitesInfo;

    // Row indices and site identifiers coincide.
    SiteInfo *site = listSites->at(row);

    // This is a new entry.
    if(row == tbl->rowCount()){
        tbl->insertRow(row);
    }

    QTableWidgetItem *cell;
    switch (column) {
    case COL_SITE_NAME:
        cell = new QTableWidgetItem(site->name);
        cell->setTextAlignment(Qt::AlignCenter);
        tbl->setItem(row, COL_SITE_NAME, cell);
        break;
    case COL_CONNECTIVITY:
        cell = new QTableWidgetItem(site->connected ? "Online" : "Offline");
        cell->setTextAlignment(Qt::AlignCenter);
        cell->setBackgroundColor(site->connected ? QColor::fromRgb(192, 255, 148) : QColor::fromRgb(255, 170, 167));
        tbl->setItem(row, COL_CONNECTIVITY, cell);
        break;
    case COL_STATUS:
        cell = new QTableWidgetItem(site->running ? "Running" : "Stopped");
        cell->setTextAlignment(Qt::AlignCenter);
        cell->setBackgroundColor(site->running ? QColor::fromRgb(192, 255, 148) : QColor::fromRgb(255, 170, 167));
        tbl->setItem(row, COL_STATUS, cell);
        break;
    case COL_POWER:
        QPushButton *btn = site->btnAction;

        // If this is the first time.
        if(btn == 0){
            btn = site->btnAction = new QPushButton(this);

            // Delegate click signal to the signal mapper.
            connect(btn, SIGNAL(clicked()), sitesActionClickedMapper, SLOT(map()));
            sitesActionClickedMapper->setMapping(btn, site);
        }
        if(site->running){
            btn->setText("Stop");
        } else {
            btn->setText("Start");
        }

        // Under some circumstances these buttons may become disabled.
        btn->setEnabled(true);

        // Paste the button on the cell
        tbl->setCellWidget(row, COL_POWER, btn);
        break;
    }
}

/**
 * @brief MonitorPanel::addTableSiteInfoEntry
 *  A syntax sugar for adding a new entry to the table
 * @param siteId
 */
void MonitorPanel::addTableSiteInfoEntry(int siteId)
{
    updateTableSiteInfo(siteId, 0);
    updateTableSiteInfo(siteId, 1);
    updateTableSiteInfo(siteId, 2);
    updateTableSiteInfo(siteId, 3);
}

/**
 * @brief MonitorPanel::updateTableSiteDataStructures
 *  This method updates sites data structures table. Given row and column index of a cell this method updates that cell.
 * @param row
 *  Index of the row to be updated.
 * @param column
 *  Index of the column to be updated.
 */
void MonitorPanel::updateTableSiteDataStructures(int row, int column)
{
    QTableWidget *tbl = ui->tblSitesDataStructures;
    SiteInfo *site = listSites->at(row);

    // New entry
    if(row == tbl->rowCount()){
        tbl->insertRow(row);
    }

    switch (column) {
    case COL_DS_SITE_NAME:
        tbl->setItem(row, COL_DS_SITE_NAME, new QTableWidgetItem(site->name));
        break;
    case COL_DS_SN:
        tbl->setItem(row, COL_DS_SN, new QTableWidgetItem(site->SN));
        break;
    case COL_DS_SV:
        tbl->setItem(row, COL_DS_SV, new QTableWidgetItem(site->SV));
        break;
    }
}

/**
 * @brief MonitorPanel::addTableSiteDataStructuresEntry
 *  Syntax sugar similar to that of site information table.
 * @param siteId
 */
void MonitorPanel::addTableSiteDataStructuresEntry(int siteId)
{
    updateTableSiteDataStructures(siteId, 0);
    updateTableSiteDataStructures(siteId, 1);
    updateTableSiteDataStructures(siteId, 2);
}

/**
 * @brief MonitorPanel::onWatchdogBark
 * @param file
 */
void MonitorPanel::onWatchdogBark(QString file)
{
    ui->txtChangedFileName->setText(file);
    QFile f(file);
    if(f.open(QIODevice::ReadOnly)){
        ui->txtChangedFileContent->setPlainText(QTextStream(&f).readAll());
        f.close();
    } else {
        ui->txtChangedFileContent->setPlainText("The file has been changed but could not read the file");
    }
    ui->tabWidget->tabBar()->setTabTextColor(1, QColor(Qt::red));
    timerFlasher->start();
}

/**
 * @brief MonitorPanel::onFlashTimerTimeout
 */
void MonitorPanel::onFlashTimerTimeout()
{
    ui->tabWidget->tabBar()->setTabTextColor(1, QColor(Qt::black));
}

/**
 * @brief MonitorPanel::on_actionAdd_triggered
 */
void MonitorPanel::on_actionAdd_triggered()
{
    // The first site only can be an initial token holder
    bool isTokenHolder = listSites->count() == 0;

    AddSiteDialog *dialog = new AddSiteDialog(this, false, isTokenHolder);
    dialog->exec();
    if(dialog->result() == QDialog::Accepted){
        SiteInfo * site = dialog->getSiteInfo();
        site->id = listSites->count();
        listSites->append(site);
        addTableSiteInfoEntry(site->id);
        addTableSiteDataStructuresEntry(site->id);
        graph->addSite(site);
        timeline->addSite(site);
    }
    dialog->deleteLater();
}

/**
 * @brief MonitorPanel::on_actionLaunch_triggered
 */
void MonitorPanel::on_actionLaunch_triggered()
{
    if(ui->actionLaunch->text() == "Stop"){
        MessageForger::Message stopMessage(MessageForger::STOP, MessageForger::CONTROLER_SITE_ID, MessageForger::INVALID_SITE_ID);

        // Send stop message to all sites
        for(int i = 0, count = listSites->count(); i < count; i += 1){
            SiteInfo *site = listSites->at(i);
            stopMessage.receiver = site->id;
            socketPool->getSocketById(site->socketId)->write(MessageForger::serialize(stopMessage));
        }

        ui->actionLaunch->setText("Stopped");
        ui->actionLaunch->setEnabled(false);
        setStatusTip("The algorithm has stopped running");

        return;
    }

    // Check whether all sites are ready to launch
    bool ready = true;

    // Check whether there is at least one initial token holder
    bool initTokenHolder = false;

    for(int i = 0, count = listSites->count(); i < count; i += 1){
        SiteInfo *site = listSites->at(i);

        // Each site should be connected and running
        if(!site->connected){
            ready = false;
            break;
        }

        // If this site is the initial token holder
        if(site->holder){
            initTokenHolder = true;
        }
    }

    if(!ready){
        showMessage("All sites must be online and running in order to launch the algorithm.");
        return;
    }

    if(!initTokenHolder){
        showMessage("There is no initial token holder");
        return;
    }

    // Forge a launch message
    MessageForger::Message message(MessageForger::LAUNCH, MessageForger::CONTROLER_SITE_ID, MessageForger::INVALID_SITE_ID);

    // Send launch message to all sites
    for(int i = 0, count = listSites->count(); i < count; i += 1){
        SiteInfo *site = listSites->at(i);
        message.receiver = site->id;
        socketPool->getSocketById(site->socketId)->write(MessageForger::serialize(message));
    }

    ui->actionLaunch->setText("Stop");
    setStatusTip("The algorithm is running");
}
