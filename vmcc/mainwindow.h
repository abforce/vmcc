#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QTime>
#include <QNetworkInterface>
#include "vmvixhelper.h"
#include "uihelper.h"
#include "monitorpanel.h"
#include "logcatwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    // GUI Slots
    void on_actionAbout_triggered();
    void on_btnVmxBrowse_clicked();
    void on_actionConnect_to_Workstation_triggered();
    void on_actionDisconnect_from_Workstation_triggered();
    void on_btnPowerOn_clicked();
    void on_btnPowerOff_clicked();
    void on_actionExit_triggered();
    void on_btnRetrieveNumOfSnapshots_clicked();
    void on_btnCreateSnapshot_clicked();
    void on_btnRevertToSnapshot_clicked();
    void on_btnApplyIPConfig_clicked();
    void on_btnLaunchAgent_clicked();
    void on_btnSwitchToAlgorithmPanel_clicked();
    void on_btnRunServer_clicked();

    // Other slots
    void logcat(QString tag, QString message);

private:
    UiHelper *ui;
    MonitorPanel *monitorPanel;
    QList<LogcatEntry> *listLogcat;
    QList<LogcatWidget *> *listLogcatWidgets;

    void showMessage(QString message);
    void registerLogcatWidget(LogcatWidget *widget);

signals:
    void connectedToWorkstation();
    void disconnectedFromWorkstation();
    void machinePoweredOn();
    void machinePoweredOff();
    void numberOfSnapshotsRetrieved(int num);
    void revertedToSnapshot();
};

#endif // MAINWINDOW_H
