#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include <QTableWidget>
#include "QListWidget"
#include "QDir"
#include "QFile"
#include "QTextStream"
#include "QFileDialog"
#include "QDebug"

int row_pp = 0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);    
    file_path = "";

    //*******************************************************************
    int line_count=0;
    QString line[100];
    QString Counters[100];

    QFile file("/home/jakob/Desktop/Summerwork/QtCreator/SummerProject/PAPI_EVENT_Available.txt");
    if(!file.open(QFile::ReadOnly | QFile::Text)){
           QMessageBox::information(this, "tilte", "file not open");
           return;
    }
    QTextStream in(&file);
    while(!in.atEnd())
    {
        if(in.readLine() != "\n")
        {
            line[line_count]=in.readLine();
            line_count++;
        }
    }
    line_count--; // to skip the last carriage return ("...\n")
    for(int i=0; i<line_count; i++){
        Counters[i] = line[i];
        ui->listWidget_Available->addItem(Counters[i]);
    }
    file.close();
    //*******************************************************************
    ui->listWidget_Available->setCurrentRow(0);

    //ui->listWidget_Available->setSelectionMode(QAbstractItemView::MultiSelection);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_add_clicked()
{
    int already_exist;
    int row = ui->listWidget_selected->count();
    int row_Available = ui->listWidget_Available->count();
    if(ui->listWidget_Available->currentItem())
    {
        QString selected_item = ui->listWidget_Available->currentItem()->text();
        QListWidgetItem *add_counter = new QListWidgetItem(selected_item);
        if(row == 0){
            ui->listWidget_selected->addItem(add_counter);
            ui->listWidget_selected->setCurrentRow(0);

        }
        else{
            for(int i=0; i<row; i++){
                QString otherItem = ui->listWidget_selected->item(i)->text();
                if(selected_item == otherItem){
                    already_exist = 1;
                    break;
                }
                else
                    already_exist = 0;
            }
            if (already_exist == 0){
                ui->listWidget_selected->addItem(add_counter);
                ui->listWidget_selected->setCurrentRow(row);
            }
        }
        row_pp = ui->listWidget_Available->currentRow()+1;
        ui->listWidget_Available->setCurrentRow(row_pp % row_Available);
        on_listWidget_Available_clicked();
        on_listWidget_selected_clicked();

    }
    else{
        QMessageBox::warning(this, "WARNING", "No Counter Selected");
    }
}

void MainWindow::on_pushButton_delete_clicked()
{
    delete ui->listWidget_selected->currentItem();
    int row = ui->listWidget_selected->count();
    if(row == 0){
        ui->plainTextEdit_Selected_Counters->clear();
    }
    else{
        QString Item_Counter = ui->listWidget_selected->currentItem()->text();
        counter_description(0, Item_Counter);
    }

}

void MainWindow::on_pushButton_delete_all_clicked()
{
    ui->listWidget_selected->clear();
    ui->plainTextEdit_Available_Counters->clear();
    ui->plainTextEdit_Selected_Counters->clear();
    ui->listWidget_selected->setCurrentRow(0);
    ui->listWidget_Available->setCurrentRow(-1);
    row_pp = 0;
}

void MainWindow::counter_description(int value, QString Item_Counter)
{
    QFile file("/home/jakob/Desktop/Summerwork/QtCreator/SummerProject/Explanatory/PAPI_EVENT_LIST.txt");
    if(!file.open(QFile::ReadOnly | QFile::Text)){
       QMessageBox::warning(this, "title", "file not open");
    }
    QTextStream in(&file);
    QString text; // = in.readAll();
    QString line = in.readLine();
    do{
        line = in.readLine();
        if (line.contains(Item_Counter, Qt::CaseSensitive)) {
            text = line;
        }
    }
    while(!line.isNull());
    if (value == 1)
        ui->plainTextEdit_Available_Counters->setPlainText("PAPI preset events:\n\n"+ text);
    if (value == 0)
        ui->plainTextEdit_Selected_Counters->setPlainText("PAPI preset events:\n\n" + text);
    file.close();
}



void MainWindow::on_listWidget_Available_clicked()
{
    QString Item_Counter = ui->listWidget_Available->currentItem()->text();
    counter_description(1, Item_Counter);
}

void MainWindow::on_listWidget_selected_clicked()
{
    QString Item_Counter = ui->listWidget_selected->currentItem()->text();
    counter_description(0, Item_Counter);
}

void MainWindow::on_listWidget_Available_doubleClicked()
{
    int already_exist;
    int row = ui->listWidget_selected->count();
    if(ui->listWidget_Available->currentItem())
    {
        QString selected_item = ui->listWidget_Available->currentItem()->text();
        QListWidgetItem *add_counter = new QListWidgetItem(selected_item);
        if(row == 0){
            ui->listWidget_selected->addItem(add_counter);
            ui->listWidget_selected->setCurrentRow(0);
        }
        else{
            for(int i=0; i<row; i++){
                QString otherItem = ui->listWidget_selected->item(i)->text();
                if(selected_item == otherItem){
                    already_exist = 1;
                    break;
                }
                else
                    already_exist = 0;
            }
            if (already_exist == 0){
                ui->listWidget_selected->addItem(add_counter);
                ui->listWidget_selected->setCurrentRow(row);
            }
        }
    }
    on_listWidget_selected_clicked();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionSave_Profil_triggered()
{
    int row = ui->listWidget_selected->count();
    QString counter_selected = "";
    if(row != 0){
        for(int i=0; i<row; i++){
            counter_selected = counter_selected + ui->listWidget_selected->item(i)->text() + "\n";
        }
        QMessageBox::StandardButton reply = QMessageBox::question(this,"QUESTION", "You'll save: \n" + counter_selected,
                              QMessageBox::Yes | QMessageBox::No);
         if (reply == QMessageBox::Yes)
         {
             QString file_name = QFileDialog::getSaveFileName(this, "Open file", "/home/jakob/Desktop/Summerwork/QtCreator/SummerProject/Profile");
             QFile file(file_name);
             file_path = file_name;
             if(!file.open(QFile::WriteOnly | QFile::Text)){
                    QMessageBox::warning(this, "WARNING", "The file wasn't saved");
                    return;
             }
             QTextStream out(&file);
             out << counter_selected;
             file.flush();
             file.close();
         }
    }
    else{
        QMessageBox::warning(this, "WARNING", "No counter selected \n"
                                              "Can't saved, please retry");
    }
}

void MainWindow::on_actionDelete_profil_triggered()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Open file", "/home/jakob/Desktop/Summerwork/QtCreator/SummerProject/Profile");
    QDir dir;
    dir.remove(file_name);
}


void MainWindow::on_pushButton_all_clicked()
{
    int row_Available = ui->listWidget_Available->count();
    ui->listWidget_selected->clear();
    for(int i=0; i<row_Available; i++){
            QString selected_item = ui->listWidget_Available->item(i)->text();
            QListWidgetItem *add_counter = new QListWidgetItem(selected_item);
            ui->listWidget_selected->addItem(add_counter);
            ui->listWidget_selected->setCurrentRow(i);
        }
    on_listWidget_selected_clicked();

}

void MainWindow::on_actionOpen_profil_triggered()
{
    int line_count=0;
    QString line[100];

    QString file_name = QFileDialog::getOpenFileName(this, "Open file", "/home/jakob/Desktop/Summerwork/QtCreator/SummerProject/Profile");
    QFile file(file_name);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
           QMessageBox::information(this, "tilte", "file not open");
           return;
    }
    QTextStream in(&file);
    while( !in.atEnd())
    {
        line[line_count]=in.readLine();
        line_count++;
    }
   // QString text = in.readAll();
    for(int i=0; i<line_count; i++){
        QString selected_item = line[i];
        QListWidgetItem *add_counter = new QListWidgetItem(selected_item);
        ui->listWidget_selected->addItem(add_counter);
        ui->listWidget_selected->setCurrentRow(i);
    }
    file.close();
    on_listWidget_selected_clicked();

}

void MainWindow::on_actionAbout_triggered()
{
    QString about_text;
    about_text = "Auther : Tom Laurendeau \n";
    about_text += "Collaborator : Regis Jubeau \n";
    about_text += "Tutors : Mr and Mrs Seceleanu \n";
    QMessageBox::about(this, "Information", about_text);
}

void MainWindow::on_actionUndo_triggered()
{
    ui->plainTextEdit->undo();
}


void MainWindow::on_actionRedo_triggered()
{
    ui->plainTextEdit->redo();
}
