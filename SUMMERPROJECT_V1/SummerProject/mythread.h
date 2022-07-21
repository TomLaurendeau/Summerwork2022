#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QtCore>
#include <papi.h>

class Mythread : public QThread
{
    Q_OBJECT
public:
    explicit Mythread(QObject *parent = 0);

    void run();
    bool stop = false;

    char inCounter1[PAPI_MAX_STR_LEN];
    char inCounter2[PAPI_MAX_STR_LEN];
    char inCounter3[PAPI_MAX_STR_LEN];
    char inCounter4[PAPI_MAX_STR_LEN];
    char * argument[5] = {NULL, NULL, NULL, NULL, NULL};
    char app[60];

signals:
    void time_exec(int time);
    void startPlot(std::vector<double> instr_ret, std::vector<double> counter_1, std::vector<double> counter_2, std::vector<double> counter_3, std::vector<double> counter_4, char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4, char *app);
    void runStartAndStop(char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4, char *app);
public slots:

};

#endif // MYTHREAD_H
