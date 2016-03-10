#include "uihelper.h"
#include "ui_mainwindow.h"

UiHelper::UiHelper(QMainWindow *window):ui(new Ui::MainWindow)
{
    ui->setupUi(window);
}

UiHelper::~UiHelper()
{
    delete ui;
}

void UiHelper::on_connected_to_workstation()
{
    ui->actionConnect_to_Workstation->setEnabled(false);
    ui->actionDisconnect_from_Workstation->setEnabled(true);
    ui->btnPowerOn->setEnabled(true);
    ui->btnRetrieveNumOfSnapshots->setEnabled(true);
    ui->btnRevertToSnapshot->setEnabled(true);
    ui->spnSnapshotIndex->setEnabled(true);
    ui->btnSwitchToAlgorithmPanel->setEnabled(true);
}

void UiHelper::on_disconnected_from_workstation()
{
    ui->actionConnect_to_Workstation->setEnabled(true);
    ui->actionDisconnect_from_Workstation->setEnabled(false);
    ui->btnPowerOn->setEnabled(false);
    ui->btnRetrieveNumOfSnapshots->setEnabled(false);
    ui->btnRevertToSnapshot->setEnabled(false);
    ui->spnSnapshotIndex->setEnabled(false);
    ui->btnSwitchToAlgorithmPanel->setEnabled(false);
}

void UiHelper::on_machine_powered_on()
{
    ui->btnPowerOff->setEnabled(true);
    ui->txtVmxFile->setEnabled(false);
    ui->btnVmxBrowse->setEnabled(false);
    ui->btnPowerOn->setEnabled(false);
    ui->btnCreateSnapshot->setEnabled(true);
    ui->btnApplyIPConfig->setEnabled(true);
    ui->btnLaunchAgent->setEnabled(true);
}

void UiHelper::on_machine_powered_off()
{
    ui->btnPowerOff->setEnabled(false);
    ui->txtVmxFile->setEnabled(true);
    ui->btnVmxBrowse->setEnabled(true);
    ui->btnPowerOn->setEnabled(true);
    ui->btnCreateSnapshot->setEnabled(false);
    ui->btnApplyIPConfig->setEnabled(false);
    ui->btnLaunchAgent->setEnabled(false);
}

void UiHelper::on_number_of_snapshots_retrieved(int num)
{
    ui->lblNumOfSnapshots->setText(QString::number(num));
    ui->spnSnapshotIndex->clear();
    for(int i = 0; i < num; i += 1){
        ui->spnSnapshotIndex->addItem(QString::number(i));
    }
}

void UiHelper::on_reverted_to_snapshot()
{
    ui->btnPowerOff->setEnabled(true);
    ui->txtVmxFile->setEnabled(false);
    ui->btnVmxBrowse->setEnabled(false);
    ui->btnPowerOn->setEnabled(false);
    ui->btnCreateSnapshot->setEnabled(true);
    ui->btnApplyIPConfig->setEnabled(true);
    ui->btnLaunchAgent->setEnabled(true);
}
