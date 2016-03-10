#ifndef LOGCATWIDGET_H
#define LOGCATWIDGET_H

#include <QWidget>
#include <QList>
#include "logcatentry.h"

namespace Ui {
class LogcatWidget;
}

class LogcatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogcatWidget(QWidget *parent = 0);
    ~LogcatWidget();
    void registerLogcatEntryList(QList<LogcatEntry> *&list);
    void notifyRowAdded();

private:
    Ui::LogcatWidget *ui;
    QList<LogcatEntry> *listLogcat;

    void updateLogcatEntryVisibility(int index);

private slots:
    void on_spnFilterByTag_currentIndexChanged(int index);

};

#endif // LOGCATWIDGET_H
