#ifndef DIALOGCOREDIR_H
#define DIALOGCOREDIR_H

#include <QDialog>
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
#include <QString>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QPalette>

namespace Ui {
class DialogCoreDir;
}

class DialogCoreDir : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCoreDir(QWidget *parent = 0);
    ~DialogCoreDir();

    QString get_core();
    QString get_PATH();
    QString ContinueApp();

private slots:
    void dialogButtonBox();

    void on_pushButton_select_clicked();

private:
    Ui::DialogCoreDir *ui;
};

#endif // DIALOGCOREDIR_H
