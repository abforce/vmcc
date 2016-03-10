#ifndef UIHELPER_H
#define UIHELPER_H

#include <QMainWindow>

namespace Ui {
    class MainWindow;
}

class UiHelper : public QObject
{
    Q_OBJECT

public:
    UiHelper(QMainWindow *window);
    ~UiHelper();
    Ui::MainWindow *ui;

private slots:
   void on_connected_to_workstation();
   void on_disconnected_from_workstation();
   void on_machine_powered_on();
   void on_machine_powered_off();
   void on_number_of_snapshots_retrieved(int num);
   void on_reverted_to_snapshot();
};

#endif // UIHELPER_H
