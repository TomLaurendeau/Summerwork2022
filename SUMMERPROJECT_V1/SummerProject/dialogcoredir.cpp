#include "dialogcoredir.h"
#include "ui_dialogcoredir.h"

DialogCoreDir::DialogCoreDir(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCoreDir)
{
    this->setWindowTitle("User Interface");
    ui->setupUi(this);

    //set all the Item for the number of core
    ui->comboBox_core->addItem("");
    ui->comboBox_core->addItem("1");
    ui->comboBox_core->addItem("2");
    ui->comboBox_core->addItem("4");
    ui->comboBox_core->addItem("8");
    ui->comboBox_core->addItem("16");
    ui->comboBox_core->addItem("32");
    ui->comboBox_core->addItem("64");
    ui->comboBox_core->insertSeparator(8);
    ui->comboBox_core->addItem("FREE");
    ui->comboBox_core->setStyleSheet("combobox-popup: 0;");

    ui->comboBox_processor->addItem("");
    ui->comboBox_processor->addItem("ARM");
    ui->comboBox_processor->addItem("INTEL");

    connect(ui->pushButton_ok, SIGNAL(clicked()), this, SLOT(dialogButtonBox()));
    connect(ui->pushButton_cancel, SIGNAL(clicked()), this, SLOT(dialogButtonBox()));
}

DialogCoreDir::~DialogCoreDir()
{
    delete ui;
}

QString DialogCoreDir::get_core()
{
    return ui->comboBox_core->currentText();
}

QString DialogCoreDir::get_PATH()
{
    return ui->label_path->text();
}

QString DialogCoreDir::ContinueApp()
{
    return ui->lineEdit_SetValueContinueApp->text();
}

void DialogCoreDir::dialogButtonBox()
{
    QObject* button = QObject::sender();
    if(button == ui->pushButton_ok){
        if(ui->label_path->text() != "" && ui->comboBox_core->currentText() != "" && ui->comboBox_processor->currentText() != ""){
            ui->lineEdit_SetValueContinueApp->setText("1");
            this->close();
        }
        else{
            if(ui->label_path->text() == "" && ui->comboBox_core->currentText() != "" && ui->comboBox_processor->currentText() != "")
                QMessageBox::warning(this, "WARNING", "Path is empty");
            else if(ui->comboBox_core->currentText() == "" && ui->label_path->text() != "" && ui->comboBox_processor->currentText() != "")
                QMessageBox::warning(this, "WARNING", "Core is empty");
            else if(ui->comboBox_core->currentText() != "" && ui->label_path->text() != "" && ui->comboBox_processor->currentText() == "")
                QMessageBox::warning(this, "WARNING", "Processor is empty");
            else
                QMessageBox::warning(this, "WARNING", "All are empty");
        }
    }
    else{
        ui->lineEdit_SetValueContinueApp->setText("0");
        this->close();
    }
}


void DialogCoreDir::on_pushButton_select_clicked()
{
    QString dir_name = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->label_path->setText(dir_name);
}
