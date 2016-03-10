#include "logcatwidget.h"
#include "ui_logcatwidget.h"

LogcatWidget::LogcatWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogcatWidget)
{
    ui->setupUi(this);
}

LogcatWidget::~LogcatWidget()
{
    delete ui;
}

void LogcatWidget::registerLogcatEntryList(QList<LogcatEntry> *&list)
{
    listLogcat = list;
}

void LogcatWidget::notifyRowAdded()
{
    // Retrieve last logcat entry
    LogcatEntry entry = listLogcat->last();

    // Add this tag to the filter spinbox if needed
    bool found = false;
    for(int i = 1, count = ui->spnFilterByTag->count(); i < count; i += 1){
        if(ui->spnFilterByTag->itemText(i) == entry.getTag()){
            found = true;
            break;
        }
    }
    if(!found){
        ui->spnFilterByTag->addItem(entry.getTag());
    }

    // Add this row to the table
    int row = ui->tblLog->rowCount();
    ui->tblLog->insertRow(row);
    ui->tblLog->setItem(row, 0, new QTableWidgetItem(entry.getTime()));
    ui->tblLog->setItem(row, 1, new QTableWidgetItem(entry.getTag()));
    ui->tblLog->setItem(row, 2, new QTableWidgetItem(entry.getContent()));

    // Whether this row should be visible or not.
    updateLogcatEntryVisibility(row);

    // Auto scroll down
    if(ui->chkAutoScroll->isChecked()){
        ui->tblLog->scrollToBottom();
    }
}

void LogcatWidget::updateLogcatEntryVisibility(int index)
{
    if(ui->spnFilterByTag->currentIndex() == 0){
        ui->tblLog->showRow(index);
    } else {
        ui->tblLog->setRowHidden(index, ui->spnFilterByTag->currentText() != listLogcat->at(index).getTag());
    }
}

void LogcatWidget::on_spnFilterByTag_currentIndexChanged(int index)
{
    for(int i = 0, count = listLogcat->count(); i < count; i += 1){
        updateLogcatEntryVisibility(i);
    }
}
