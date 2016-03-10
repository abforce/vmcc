#ifndef MONITORPANEL_H
#define MONITORPANEL_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QMessageBox>
#include <QSignalMapper>
#include <QFileSystemWatcher>
#include <QTimer>

#include "socketpool.h"
#include "messageforger.h"
#include "addsitedialog.h"
#include "siteinfo.h"
#include "vmvixhelperasync.h"
#include "graphvisualizer.h"
#include "timelinevisualizer.h"
#include "logcatwidget.h"

namespace Ui {
class MonitorPanel;
}

class MonitorPanel : public QMainWindow
{
    Q_OBJECT

public:
    explicit MonitorPanel(QWidget *parent = 0);
    ~MonitorPanel();
    bool startServer(QString ifName, int port);
    LogcatWidget *getLogcatWidget();

private:
    Ui::MonitorPanel *ui;
    QSignalMapper *sitesActionClickedMapper;
    QFileSystemWatcher *watchdog;
    QTimer *timerFlasher;
    QTcpServer *server;
    QHostAddress serverAddress;
    int serverPort;
    SocketPool *socketPool;
    int nextSiteId;
    QList<SiteInfo*> *listSites;
    GraphVisualizer *graph;
    TimeLineVisualizer *timeline;

    static void vixCallback(UserData *data, int result);
    void emitSignalVixCallback(UserData *data, int result);
    void handleMessage(QTcpSocket *socket, int socketId, MessageForger::Message message, QByteArray dataBytes) throw(int);
    void writeFile(QString filename, QString content);

    void showMessage(QString message);
    void updateTableSiteInfo(int row, int column);
    void addTableSiteInfoEntry(int siteId);
    void updateTableSiteDataStructures(int row, int column);
    void addTableSiteDataStructuresEntry(int siteId);

signals:
    void logcat(QString tag, QString message);
    void vixCallbackSignal(UserData *data , int result);

private slots:
    void requestForConnectionReceived();
    void siteActionClicked(QObject *s);

    void onSocketReadyRead(QTcpSocket *socket, int id);
    void onSocketDisconnected(QTcpSocket *socket, int id);

    void vixCallbackGuiThread(UserData *data, int result);
    void on_actionAdd_triggered();
    void on_actionLaunch_triggered();

    void onWatchdogBark(QString file);
    void onFlashTimerTimeout();
};

#endif // MONITORPANEL_H
