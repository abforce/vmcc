#ifndef ADDSITEDIALOG_H
#define ADDSITEDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include "siteinfo.h"

namespace Ui {
class AddSiteDialog;
}

class AddSiteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddSiteDialog(QWidget *parent = 0, bool tokenOptionEnabled = false, bool isTokenHolder = false);
    ~AddSiteDialog();
    SiteInfo* getSiteInfo();

private slots:
    void on_btnAdd_clicked();
    void on_btnCancel_clicked();
    void on_btnBrowse_clicked();

private:
    Ui::AddSiteDialog *ui;
};

#endif // ADDSITEDIALOG_H
