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
#include <QTimer>

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include </home/jakob/Desktop/Summerwork/files/papi.h> // !!!!!!!!!!!!!!!!!!!
#include <math.h>
#include <vector>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <papiStdEventDefs.h>
#include <papi.h>
#include <chrono>
#include <cstring>

#include "mythread.h"
#include "mythreadplot.h"

#define TASK_ACTIVE_MS 120000 // 120s
#define CACHE_LOWEST_SIZE 0
#define VALUE_SIZE 400			  //Samples for correlations
#define TARGET_PERCENTAGE 0.01	  //This can be discussed
#define CORRELATION_THRESHOLD 0.5 //Corr
#define SAMPLING_FREQ 1			  //ms, think about this....---???
#define CACHE_LOW_THRESHOLD 0.3
#define CACHE_HIGH_THRESHOLD 0.65
#define LLC_PC_ON 1
#define REPARTITION_LLC_PC 1
#define PART_TEST 0
#define SAMPLES 500
#define CUTOFF_STD_FACTOR 0.7
#define WINDOW_SIZE_DEF 100
#define JUNK_CONTAINER_SIZE 80
#define LLM_SHARK_PART 1
#define PROFILING_PART_SIZE 15

typedef struct characteristics
{
    char *fp;
    char *name;
    int counters;
    int pid;
    int priority;
    double instr_hist[WINDOW_SIZE_DEF];
    double cache_hist[WINDOW_SIZE_DEF];
    double execution_time;
    double current_performance;
    double current_cache;
    double diff;
    double normal_diff;
    double max_performance;
    double desired_performance;
    int frequency;
    char *app_arg[64];
    std::vector<std::vector<double>> event_counter_sets;
    std::vector<char *> event_name;
    std::vector<double> total_correlations;
    std::vector<std::vector<double>> segment_correlations;
    double baseline_correlation[16];
    double std_instr;
    double median_instr;

    std::vector<int> break_point_1;
    std::vector<int> break_point_2;

} task_characteristic;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
     Mythread *StartAndStopThread;
     MyThreadPlot *PlotThread;

private slots:

    //------------------------------------------------

    static void plot_txt(task_characteristic *output, char *counter, int number, char *app);
    int characterize_program(char **argument, char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4);
    void mainProg(char **argv);

    //------------------------------------------------

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

    void on_pushButton_Arguments_Run_clicked();

    void createPlot(QVector<int> *x_, QVector<float> *y_, QString file_name);

    void readFilesPlots(QString counter);

    void on_pushButton_Plot_To_File_clicked();

    void on_pushButton_savePlot_clicked();

    void on_comboBox_currentIndexChanged();

    void on_pushButton_Plot_Stop_clicked();

    void on_pushButton_Plot_Clear_clicked();

public slots:
    int runThread(char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4, char *app,
                  std::vector<double> instr_ret_threadPlot,
                  std::vector<double> counter_1_threadPlot,
                  std::vector<double> counter_2_threadPlot,
                  std::vector<double> counter_3_threadPlot,
                  std::vector<double> counter_4_threadPlot);
    void time_exec(int time);
    void startPlotThread(std::vector<double> instr_ret, std::vector<double> counter_1, std::vector<double> counter_2, std::vector<double> counter_3, std::vector<double> counter_4, char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4, char *app);

private:
    Ui::MainWindow *ui;
    QString file_path;
    DialogCoreDir *dialogCoreDir;

};

#endif // MAINWINDOW_H
