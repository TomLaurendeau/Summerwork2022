#ifndef MYTHREADPLOT_H
#define MYTHREADPLOT_H

#include <QThread>
#include <QtCore>
#include <papi.h>

class MyThreadPlot : public QThread
{
    Q_OBJECT
public:
    explicit MyThreadPlot(QObject *parent = 0);

    void run();
    bool stop = false;

    char inCounter1[PAPI_MAX_STR_LEN];
    char inCounter2[PAPI_MAX_STR_LEN];
    char inCounter3[PAPI_MAX_STR_LEN];
    char inCounter4[PAPI_MAX_STR_LEN];
    char app[60];

    std::vector<double> instr_ret_threadPlot;
    std::vector<double> counter_1_threadPlot; //Counter_1
    std::vector<double> counter_2_threadPlot; //Counter_2
    std::vector<double> counter_3_threadPlot; //Counter_3
    std::vector<double> counter_4_threadPlot; //Counter_4

signals:
    void runStartAndStop(char *inCounter1,
                         char *inCounter2,
                         char *inCounter3,
                         char *inCounter4,
                         char *app,
                         std::vector<double> instr_ret_threadPlot,
                         std::vector<double> counter_1_threadPlot,
                         std::vector<double> counter_2_threadPlot,
                         std::vector<double> counter_3_threadPlot,
                         std::vector<double> counter_4_threadPlot);
public slots:

};

#endif // MYTHREADPLOT_H
