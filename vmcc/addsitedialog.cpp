#include "addsitedialog.h"
#include "ui_addsitedialog.h"

AddSiteDialog::AddSiteDialog(QWidget *parent, bool tokenOptionEnabled, bool isTokenHolder) :
    QDialog(parent),
    ui(new Ui::AddSiteDialog)
{
    ui->setupUi(this);
    ui->chkInitialHolder->setEnabled(tokenOptionEnabled);
    ui->chkInitialHolder->setChecked(isTokenHolder);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setResult(QDialog::Rejected);
}

AddSiteDialog::~AddSiteDialog()
{
    delete ui;
}

SiteInfo* AddSiteDialog::getSiteInfo()
{
    QString name = ui->txtSiteName->text();
    QString vmx = ui->txtVmxAddress->text();
    bool holder = ui->chkInitialHolder->isChecked();
    QString username = ui->txtUsername->text();
    QString password = ui->txtPassword->text();

    return new SiteInfo(name, vmx, username, password, holder);
}

void AddSiteDialog::on_btnAdd_clicked()
{
    setResult(QDialog::Accepted);
    hide();
}

void AddSiteDialog::on_btnCancel_clicked()
{
    hide();
}

void AddSiteDialog::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select VMX file...", QDir::homePath(), "Virtual Machine Executable file (*.vmx)");
    ui->txtVmxAddress->setText(fileName);
}
