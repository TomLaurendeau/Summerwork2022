#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "QMessageBox"
#include "QTableWidget"
#include "QListWidget"
#include "QDir"
#include "QFile"
#include "QTextStream"
#include "QFileDialog"
#include "QDebug"
#include "QProcess"
#include "QVector"
#include <QDialog>
#include <QString>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QPalette>
#include <dialogcoredir.h>
#include <QPen>


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

    void createUI(const QStringList &headers);

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

    void validateCounters();

    void on_pushButton_File_Dialogue_New_clicked();

    void on_pushButton_File_Dialogue_Delete_clicked();

    void on_comboBox_core_currentIndexChanged();

    void on_tableWidget_doubleClicked();

    void onClicked();

    int getCurrentRow();

    void on_pushButton_Arguments_Check_clicked();

    void on_pushButton_Plot_Restart_clicked();

    void on_pushButton_Arguments_Run_clicked();

    void createPlot(QVector<int> *x_, QVector<float> *y_, QString file_name);

    void readFilesPlots(QString counter);

    void on_pushButton_Plot_To_File_clicked();

    void on_pushButton_savePlot_clicked();

private:
    Ui::MainWindow *ui;
    QString file_path;
    DialogCoreDir *dialogCoreDir;

};

#endif // MAINWINDOW_H
