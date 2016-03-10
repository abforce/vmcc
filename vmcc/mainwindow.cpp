#include "mainwindow.h"
#include "ui_mainwindow.h"

#define CONNECTED_STATUS    "Connected to Workstation"
#define DISCONNECTED_STATUS "Disconnected from Workstation"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new UiHelper(this))
{
    monitorPanel = new MonitorPanel(this);
    listLogcat = new QList<LogcatEntry>();
    listLogcatWidgets = new QList<LogcatWidget *>();

    registerLogcatWidget(ui->ui->logcatWidget);
    registerLogcatWidget(monitorPanel->getLogcatWidget());

    connect(this, SIGNAL(connectedToWorkstation()), ui, SLOT(on_connected_to_workstation()));
    connect(this, SIGNAL(disconnectedFromWorkstation()), ui, SLOT(on_disconnected_from_workstation()));
    connect(this, SIGNAL(machinePoweredOn()), ui, SLOT(on_machine_powered_on()));
    connect(this, SIGNAL(machinePoweredOff()), ui, SLOT(on_machine_powered_off()));
    connect(this, SIGNAL(numberOfSnapshotsRetrieved(int)), ui, SLOT(on_number_of_snapshots_retrieved(int)));
    connect(this, SIGNAL(revertedToSnapshot()), ui, SLOT(on_reverted_to_snapshot()));

    connect(monitorPanel, SIGNAL(logcat(QString,QString)), this, SLOT(logcat(QString,QString)));

    setStatusTip(DISCONNECTED_STATUS);
    logcat("App", "Application started.");

    // Retrieve list of interfaces and populate corresponding spin box
    foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces()){
        ui->ui->spnInterfaces->addItem(interface.name());

        // Set defaulf interface to 'vmnet1', if exists
        if(interface.name() == "vmnet1"){
            ui->ui->spnInterfaces->setCurrentIndex(ui->ui->spnInterfaces->count() - 1);
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "About this app", "Virtual Machines Controling Client.\nWritten by Ali Reza Barkhordari\nUsing source codes of this program is subjected to copyrights.\n\n(C) Copyright Ali Reza Barkhordari");
}

void MainWindow::on_btnVmxBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select VMX file...", QDir::homePath(), "Virtual Machine Executable file (*.vmx)");
    ui->ui->txtVmxFile->setText(fileName);
}

void MainWindow::on_actionConnect_to_Workstation_triggered()
{
    int result = VmVixHelperAsync::connectToWorkstation();
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Successfully connected to Workstation.";
        setStatusTip(CONNECTED_STATUS);
        emit connectedToWorkstation();
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_CONNECT){
        message = "Could not connect to Workstation.";
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::showMessage(QString message)
{
    QMessageBox::information(this, "Message", message);
}

void MainWindow::registerLogcatWidget(LogcatWidget *widget)
{
    widget->registerLogcatEntryList(listLogcat);
    listLogcatWidgets->append(widget);
}

void MainWindow::logcat(QString tag, QString message)
{
    // Logcat entry model
    LogcatEntry entry(QTime::currentTime().toString(), tag, message);

    // Add to logcat list
    listLogcat->append(entry);

    // Notify logcat widgets
    foreach(LogcatWidget *widget, *listLogcatWidgets){
        widget->notifyRowAdded();
    }
}

void MainWindow::on_actionDisconnect_from_Workstation_triggered()
{
    int result = VmVixHelper::disconnectFromWorkstation();
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Successfully disconnected from Workstation.";
        setStatusTip(DISCONNECTED_STATUS);
        emit disconnectedFromWorkstation();
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_btnPowerOn_clicked()
{
    QString vmxPath = ui->ui->txtVmxFile->text();
    if(vmxPath.isEmpty()){
        showMessage("Please specify virtual machine executable file (.vmx)");
        return;
    }
    int result = VmVixHelper::powerOn(vmxPath);
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Virtual machine successfully powered on.";
        emit machinePoweredOn();
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_OPEN_MACHINE){
        message = "Could not open specified virtual machine.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_POWER_ON){
        message = "Could not power on specified virtual machine.";
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_btnPowerOff_clicked()
{
    int result = VmVixHelper::powerOff();
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Virtual machine successfully powered off.";
        emit machinePoweredOff();
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_POWER_OFF){
        message = "Could not power off specified virtual machine";
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_actionExit_triggered()
{
    VmVixHelper::cleanup();
    QApplication::quit();
}

void MainWindow::on_btnRetrieveNumOfSnapshots_clicked()
{
    QString vmxPath = ui->ui->txtVmxFile->text();
    if(vmxPath.isEmpty()){
        showMessage("Please specify virtual machine executable file (.vmx)");
        return;
    }
    int numberOfSnapshots;
    int result = VmVixHelper::getNumberOfSnapshots(vmxPath, &numberOfSnapshots);
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Successfully number of snapshots queried";
        emit numberOfSnapshotsRetrieved(numberOfSnapshots);
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_OPEN_MACHINE){
        message = "Could not open specified virtual machine.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_RETRIEVE_NUMBER_OF_SNAPSHOTS){
        message = "Could not query number of snapshots for the specified virtual machine.";
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_btnCreateSnapshot_clicked()
{
    int result = VmVixHelper::createSnapshot();
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Successfully created a snapshot.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_CREATE_SNAPSHOT){
        message = "Could not create a snapshot.";
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_btnRevertToSnapshot_clicked()
{
    QString vmxPath = ui->ui->txtVmxFile->text();
    if(vmxPath.isEmpty()){
        showMessage("Please specify virtual machine executable file (.vmx)");
        return;
    }
    if(ui->ui->spnSnapshotIndex->count() == 0){
        showMessage("There is no already taken snapshot for this virtual machine.");
        return;
    }
    int index = ui->ui->spnSnapshotIndex->currentIndex();
    int result = VmVixHelper::revertToSnapshot(vmxPath, index);
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Successfully reverted to the specified snapshot.";
        emit revertedToSnapshot();
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_OPEN_MACHINE){
        message = "Could not open specified virtual machine.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_RETRIEVE_SNAPSHOT){
        message = "Could not retrieve snapshot handle.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_REVERT_TO_SNAPSHOT){
        message = "Could not revert to the specified snapshot.";
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_btnApplyIPConfig_clicked()
{
    QString interface = ui->ui->txtIPInterface->text();
    QString ip = ui->ui->txtIPAddress->text();
    QString mask = ui->ui->txtIPMask->text();
    QString gateway = ui->ui->txtIPGateway->text();

    if(interface.isEmpty() || ip.isEmpty() || mask.isEmpty()){
        showMessage("Please fill the required fields.");
        return;
    }
    int result = VmVixHelper::setIpAddress(interface, ip, mask, gateway);
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        message = "Successfully dispatched request for IP configuration setting.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_WAIT_FOR_TOOLS){
        message = "Could not wait for guest to be fully booted.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_LOGIN){
        message = "Could not login in the guest OS.";
    }
    if(result == VmVixHelper::OperationResult::COULD_NOT_SET_IP_ADDRESS){
        message = "Could not set IP configuration.";
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_btnLaunchAgent_clicked()
{
    VmVixHelper::ShellOutput *output;
    int result =  VmVixHelper::runProgramInGuest("c:\\mystuff\\guest_agent.exe", QString(), &output);
    QString message;
    if(result == VmVixHelper::OperationResult::SUCCESS){
        QString filename = (*output).txtStderr;
        QString content = (*output).txtStdout;
        QFile file(filename);
        if(file.open(QFile::WriteOnly)){
            file.write(content.toUtf8());
            file.close();
            message = "Execution of guest agent successfully finished.";
        } else {
            message = "Could not open file in host.";
        }
        ui->ui->lblTargetFile->setText(filename);
        ui->ui->lblBytesOverwritten->setText(QString::number(content.toUtf8().size()));
    } else {
        message = "Error occurred. code:" + QString::number(result);
    }
    logcat("VIX", message);
    showMessage(message);
}

void MainWindow::on_btnSwitchToAlgorithmPanel_clicked()
{
    monitorPanel->showMaximized();
}

void MainWindow::on_btnRunServer_clicked()
{
    if(monitorPanel->startServer(ui->ui->spnInterfaces->currentText(), ui->ui->nbrTcpPort->value())){
        ui->ui->btnRunServer->setText("Server is running...");
        ui->ui->btnRunServer->setEnabled(false);
    } else {
        showMessage("Server failed to start. See more info in logcat.");
    }
}
