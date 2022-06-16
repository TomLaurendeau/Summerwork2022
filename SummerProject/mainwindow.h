#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_add_clicked();

    void on_pushButton_delete_clicked();

    void on_pushButton_delete_all_clicked();

    void counter_description(int value, QString Item_Counter);

    void on_listWidget_Available_clicked();

    void on_listWidget_Available_doubleClicked();

    void on_listWidget_selected_clicked();

    void on_actionExit_triggered();

    void on_actionSave_Profil_triggered();

    void on_actionDelete_profil_triggered();

    void on_pushButton_all_clicked();

    void on_actionOpen_profil_triggered();

    void on_actionAbout_triggered();

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

private:
    Ui::MainWindow *ui;
    QString file_path;
};

#endif // MAINWINDOW_H
