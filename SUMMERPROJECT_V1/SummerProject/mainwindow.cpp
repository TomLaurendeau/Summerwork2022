#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mythread.h"
#include "mythreadplot.h"
#include "mainwindow.h"

// Init values
int rowPP = 0;
int k = 0;
int checkValue = 0;
int comeFromTry = 0;
int tooMushCounters = 0;
int mon_pid = 0;
int STOP = 0;

unsigned int SLEEP_TIME = 10000; // default value

std::vector<char *> PAPI_EVENTS;
QVector<QString> counters_vector;
QVector<QString> number_counters_vector;
QString Path_selected;
QString Core_selected;
QString SOURCE_DIR;
QString FreeCoreIsSelected;

//var charac

pid_t my_pid;
int status /* unused ifdef WAIT_FOR_COMPLETION */;
int ev_set = PAPI_NULL;
long long values[7];
// initialization the PAPI library
int retval = PAPI_library_init(PAPI_VER_CURRENT);
std::vector<double> instr_ret;
PAPI_option_t opt;
std::vector<double> counter_1; //Counter_1
std::vector<double> counter_2; //Counter_2
std::vector<double> counter_3; //Counter_3
std::vector<double> counter_4; //Counter_4
auto timestamp_start = std::chrono::high_resolution_clock::now();

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // open dialog to select PROCESSOR / CORE / PATH
    DialogCoreDir dialogCoreDir;
    dialogCoreDir.setModal(true);
    dialogCoreDir.exec();
    Core_selected = dialogCoreDir.get_core();
    if(Core_selected == "FREE"){
        // get the number of logical CPUs.
        int core = sysconf(_SC_NPROCESSORS_CONF);
        FreeCoreIsSelected = QString::number(core);
    }
    Path_selected = dialogCoreDir.get_PATH();
    int  valueContinueApp = dialogCoreDir.ContinueApp().toInt();

    if(valueContinueApp == 0){
        qDebug() << "Quit app";
        //NEED TO CLOSE MAINWINDOW
    }
    else{ // button "OK" pressed
        this->setWindowTitle("User Interface");
        ui->setupUi(this);

        StartAndStopThread = new Mythread(this);
        connect(StartAndStopThread,&Mythread::startPlot,
                this, &MainWindow::startPlotThread);
        connect(StartAndStopThread, &Mythread::time_exec,
                this, &MainWindow::time_exec);
        /*connect(StartAndStopThread, &Mythread::runStartAndStop,
                this, &MainWindow::runThread);*/

        PlotThread = new MyThreadPlot(this);
        connect(PlotThread, &MyThreadPlot::runStartAndStop,
                this, &MainWindow::runThread);

        SOURCE_DIR  = Path_selected + "/Resource_graphs/";

        // read file papi_avail and create file papi_avail.txt
        QProcess::execute( Path_selected + "/executable/papi_avail -a");  // here
        file_path = "";

        // read line by line papi_avail.txt
        int line_count = 0;
        QString Counters[10000];
        QFile file( Path_selected + "/executable/papi_avail.txt");
        if(!file.open(QFile::ReadOnly | QFile::Text)){
               QMessageBox::information(this, "tilte", "No counter available, try to fix the perf_event_paranoid to 0 "
                                                       "in a terninal: ~$ sudo nano /proc/sys/kernel/perf_event_paranoid\n\n"
                                                       "If the problem persists, please check the 'papi_avail.cpp' file");
               return;
        }

        // set listWidget_Available (left list) from papi_avail.txt
        QTextStream in(&file);
        while(!in.atEnd())
        {
                Counters[line_count] = in.readLine();
                line_count++;
        }
        for(int i=0; i<line_count; i++){
            ui->listWidget_Available->addItem(Counters[i]);
        }
        file.close();

        // set current row to 0 for see the first line selected
        ui->listWidget_Available->setCurrentRow(0);

        // set the header name for the argument table
        this->createUI(QStringList() << "*" << "File(s)"
                                     << "Core"
                                     << "Argument(s)"
                                     << "");

        // set all the Item for the number of core
        ui->comboBox_core->addItem("1");
        ui->comboBox_core->addItem("2");
        ui->comboBox_core->addItem("4");
        ui->comboBox_core->addItem("8");
        ui->comboBox_core->addItem("16");
        ui->comboBox_core->addItem("32");
        ui->comboBox_core->addItem("64");
        ui->comboBox_core->insertSeparator(8);
        ui->comboBox_core->addItem("FREE"); // if free is selected use -> cat proc/cpuinfo | grep 'cpu cores' and return the value !
        ui->comboBox_core->setStyleSheet("combobox-popup: 0;"); // for a maximum number of visible items
        ui->comboBox_core->setCurrentText(Core_selected);

        // set all the Item for the sampling combo box
        ui->comboBox->addItem("DEFAULT");
        for(int i=1; i<50; i++){
            QString SLEEP_TIME_box = QString::number(i*1000);
            ui->comboBox->addItem(SLEEP_TIME_box);
        }
        ui->comboBox->setStyleSheet("combobox-popup: 0;"); // for a maximum number of visible items
        ui->comboBox->setCurrentIndex(0);
    }
}

MainWindow::~MainWindow()
{
    // clear the current txt files in the folder
    for(int j=0; j<counters_vector.size(); j++){
        if(counters_vector[j]== "PAPI_NULL") break;
        else{
            QString file_name = Path_selected + "/Resource_graphs/" + counters_vector[j] + "_data.txt";
            qDebug() << file_name + " is deleted";
            QDir file;
            file.remove(file_name);
        }
    }
    delete ui;
}

// plot in txt file the value of number of instruction / number of cache misses
void MainWindow::plot_txt(task_characteristic *output, char *counter, int number, char *app)
{
    char data_file[130];
    char * source = SOURCE_DIR.toLatin1().data(); // WARNING !!!
    std::ofstream myfile;
    sprintf(data_file, "%s/%s_%s_data.txt", source, app, counter);
    myfile.open(data_file);
    for (int i = 0; i <  (*output).event_counter_sets[0].size(); i ++){
        myfile << std::fixed << i << '\t' <<  (*output).event_counter_sets[0][i] << '\t' << (*output).event_counter_sets[number][i] << "\n";
    }
    myfile.close();
}

int MainWindow::characterize_program(char **argument, char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4)
{
    int event_1 = 0, event_2 = 0, event_3 = 0, event_4 = 0;

    counter_1.clear();
    counter_2.clear();
    counter_3.clear();
    counter_4.clear();
    instr_ret.clear();
    ev_set = PAPI_NULL;

    // initialization the PAPI library
    retval = PAPI_library_init(PAPI_VER_CURRENT);

    // create an event
    if ((retval = PAPI_create_eventset(&ev_set)) != PAPI_OK){
        printf("ERROR: PAPI_create_eventset %d: %s\n", retval,
               PAPI_strerror(retval));
        exit(-1);
    }

    //Aassign it to CPU compoment
    if ((retval = PAPI_assign_eventset_component((ev_set), 0)) != PAPI_OK)
    {
        printf("ERROR: PAPI_assign_eventset_component %d: %s\n", retval,
               PAPI_strerror(retval));
        exit(-1);
    }

    if((PAPI_event_name_to_code(("%s",inCounter1), &event_1)!= PAPI_OK) && (strcmp(inCounter1,"PAPI_NULL") == -2)){} //std::cout << std::hex << event_1 << "\n";
    if((PAPI_event_name_to_code(("%s",inCounter2), &event_2)!= PAPI_OK) && (strcmp(inCounter2,"PAPI_NULL") == -2)){} //std::cout << std::hex << event_2 << "\n";
    if((PAPI_event_name_to_code(("%s",inCounter3), &event_3)!= PAPI_OK) && (strcmp(inCounter3,"PAPI_NULL") == -2)){} //std::cout << std::hex << event_3 << "\n";
    if((PAPI_event_name_to_code(("%s",inCounter4), &event_4)!= PAPI_OK) && (strcmp(inCounter4,"PAPI_NULL") == -2)){} //std::cout << std::hex << event_4 << "\n";

    memset(&opt, 0x0, sizeof(PAPI_option_t));
    opt.inherit.inherit = PAPI_INHERIT_ALL;
    opt.inherit.eventset = ev_set;
    if ((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK)
    { // most important, the papi inheritance!!!!
        printf("PAPI_set_opt error %d: %s0", retval, PAPI_strerror(retval));
        exit(-1);
    }
    if ((retval = PAPI_add_event(ev_set, PAPI_TOT_INS)) != PAPI_OK) //PAPI_TOT_INS
    {
        printf("failed to retvalattach instructions retired");
        printf("Wrapper create: %s\n", strerror(errno));
    }
    if (((retval = PAPI_add_event(ev_set, event_1)) != PAPI_OK) && (strcmp(inCounter1,"PAPI_NULL") == -2))
    {
        printf("failed to attach event_1 ");
        printf("Wrapper create: %s\n", strerror(errno));
    }
    if (((retval = PAPI_add_event(ev_set, event_2)) != PAPI_OK) && (strcmp(inCounter2,"PAPI_NULL") == -2))
    {
        printf("failed to attach event_2 ");
        printf("Wrapper create: %s\n", strerror(errno));
    }
    if (((retval = PAPI_add_event(ev_set, event_3)) != PAPI_OK) && (strcmp(inCounter3,"PAPI_NULL") == -2))
    {
        printf("failed to attach event_3 ");
        printf("Wrapper create: %s\n", strerror(errno));
    }
    if (((retval = PAPI_add_event(ev_set, event_4)) != PAPI_OK) && (strcmp(inCounter4,"PAPI_NULL") == -2))
    {
        printf("failed to attach event_4 ");
        printf("Wrapper create: %s\n", strerror(errno));
    }
    std::string path = argument[0];
    std::string name;
    size_t sep = path.find_last_of("\\/");
    if (sep != std::string::npos)
            path = path.substr(sep + 1, path.size() - sep - 1);

    size_t dot = path.find_last_of(".");
    if (dot != std::string::npos){
        name = path.substr(0, dot);
        //std::string ext  = path.substr(dot, path.size() - dot);
    }
    else{
        name = path;
        //std::string ext  = "";
    }
    char* name_app = strcpy(new char[name.length() + 1], name.c_str());

    timestamp_start = std::chrono::high_resolution_clock::now();

    //Started thread
    StartAndStopThread->stop = false;
    strcpy(StartAndStopThread->inCounter1,inCounter1);
    strcpy(StartAndStopThread->inCounter2,inCounter2);
    strcpy(StartAndStopThread->inCounter3,inCounter3);
    strcpy(StartAndStopThread->inCounter4,inCounter4);
    strcpy(StartAndStopThread->app,name_app);

    int AAA = 0;
    for(AAA = 0; argument[AAA] != NULL; AAA++){
        //StartAndStopThread->argument[AAA] = malloc(strlen(argument[AAA] + 1));
        StartAndStopThread->argument[AAA] = strdup(argument[AAA]);
        //strcpy(StartAndStopThread->argument[AAA],argument[AAA]);
    }
    argument[AAA] = NULL;
    StartAndStopThread->start();
    return 0;
}

void MainWindow::mainProg(char **argv)
{
    // initialization
    using namespace std;
    FILE * fp;
    char filepath [60];
        memset(filepath,0,60);
    char argument[60];
        memset(argument,0,60);
    char counter1[PAPI_MAX_STR_LEN] = "PAPI_NULL";	char counter2[PAPI_MAX_STR_LEN] = "PAPI_NULL"; char counter3[PAPI_MAX_STR_LEN] = "PAPI_NULL"; char counter4[PAPI_MAX_STR_LEN] = "PAPI_NULL";
    int core = -1;
    // affectation
    for (int i=0; i<8; i++)
        qDebug() << argv[i];
    strcpy(filepath,argv[1]);
    strcpy(counter1,argv[2]);
    strcpy(counter2,argv[3]);
    strcpy(counter3,argv[4]);
    strcpy(counter4,argv[5]);
    strcpy(argument,argv[6]);
    core = atoi(argv[7]);

    /****************************************************************************************************************************
     * 	EXAMPLE	(OLD)																												*
     *  filepath										Counters	argument											  core 	*
     *	/home/jakob/Desktop/arm_tests/SUSAN_bench/susan 1 24 37 -1  /home/jakob/Desktop/arm_tests/SUSAN_bench/landing.bmp 0"   	*
     ****************************************************************************************************************************/

    // open file with the filepath
    fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        qDebug()<< "Could not read file";
        exit(EXIT_FAILURE);
    }
    char * argument_exe[] = {argv[1], argv[6], argv[7], "1", NULL}; // !!! warning'

    characterize_program(argument_exe, (char*)counter1, (char*)counter2, (char*)counter3, (char*)counter4);
}

void MainWindow::createUI(const QStringList &headers)
{
    //set the UI environnement
    ui->tableWidget->setColumnCount(5);
    ui->tableWidget->setShowGrid(true);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    //ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);

    //set ColumnWidth
    ui->tableWidget->setColumnWidth(0,11);
    ui->tableWidget->setColumnWidth(1,470);
    ui->tableWidget->setColumnWidth(2,50);
    ui->tableWidget->setColumnWidth(3,250);
    ui->tableWidget->setColumnWidth(4,20);
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
        rowPP = ui->listWidget_Available->currentRow()+1;
        ui->listWidget_Available->setCurrentRow(rowPP % row_Available);
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
    // clear right list and the description of the counter
    ui->listWidget_selected->clear();
    ui->plainTextEdit_Available_Counters->clear();
    ui->plainTextEdit_Selected_Counters->clear();
    ui->listWidget_selected->setCurrentRow(0);
    ui->listWidget_Available->setCurrentRow(-1);
    rowPP = 0;
}

void MainWindow::counter_description(int value, QString Item_Counter)
{
    // set description of the differents counters from a file .txt to a plaintext
    QFile file( Path_selected + "/QtCreator/SummerProject/Explanatory/PAPI_EVENT_LIST.txt");
    if(!file.open(QFile::ReadOnly | QFile::Text)){
       QMessageBox::warning(this, "title", "file not open");
    }
    QTextStream in(&file);
    QString text;
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
    //add the double clicked from the available list to the selected list
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
    //save counters selected in a file .txt
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
             QString file_name = QFileDialog::getSaveFileName(this, "Open file", Path_selected + "/QtCreator/SummerProject/Profile");
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
    //delete profil
    QString file_name = QFileDialog::getOpenFileName(this, "Open file", Path_selected + "/QtCreator/SummerProject/Profile");
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
    //read .txt and add the selected counters
    int line_count=0;
    QString line[100];

    QString file_name = QFileDialog::getOpenFileName(this, "Open file", Path_selected + "/QtCreator/SummerProject/Profile");
    QFile file(file_name);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
           QMessageBox::information(this, "tilte", "file not open");
           return;
    }
    ui->listWidget_selected->clear();
    ui->plainTextEdit_Available_Counters->clear();
    ui->plainTextEdit_Selected_Counters->clear();


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

    //set the cursor at the first row and show the description
    ui->listWidget_selected->setCurrentRow(0);
    on_listWidget_selected_clicked();
    ui->listWidget_Available->setCurrentRow(0);
    on_listWidget_Available_clicked();
}

void MainWindow::on_actionAbout_triggered()
{
    QString about_text;
    about_text = "Auther : Tom Laurendeau \n";
    about_text += "Collaborator : Regis Jubeau \n";
    about_text += "Tutors : Mr and Mrs Seceleanu \n";
    QMessageBox::about(this, "Information", about_text);
}

void MainWindow::validateCounters()
{
    int row = ui->listWidget_selected->count();
    int row_avail = ui->listWidget_Available->count();
    counters_vector.clear();
    number_counters_vector.clear();
    if(row<5 && row !=0){
        for(int i=0; i<row;i++)
        {
            QString counter = ui->listWidget_selected->item(i)->text();
              counters_vector.append(counter);//put in some vector the selected counters
              for(int j=0; j<row_avail; j++)
                if(counter == (ui->listWidget_Available->item(j)->text())){
                    number_counters_vector.append(QString::number(j));
                }
        }
        while(counters_vector.size()<4){
            counters_vector.append("PAPI_NULL");
            number_counters_vector.append("-1");
        }
        tooMushCounters = 0;
    }
    else{
        if(row == 0)
            tooMushCounters = 0;
        else
            tooMushCounters = 1;
    }
}



void MainWindow::on_pushButton_File_Dialogue_New_clicked() //add row with the new application
{
    int newFile = 1;
    int newFileArg = 1;

   //Open file to get the full path
    QString file_name = QFileDialog::getOpenFileName(this, "Select an application", QDir::homePath());
    QFile file(file_name);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
        newFile = 0;
    }
    if(newFile){
        // Insert row
        ui->tableWidget->insertRow(0);

        // Create a String, which will serve as a name in a line edit
        //QString name_apllication = file_name.section("/", -1,-1);

        // Create an element, which will serve as a line edit
        QLabel *lineedit = new QLabel();
        lineedit->setText(file_name);
        //Set the edit line in the column 2
        ui->tableWidget->setCellWidget(0, 1, lineedit);

        file.close();

        // Create an element, which will serve as a checkbox
        QTableWidgetItem *item_checkbox = new QTableWidgetItem();
        item_checkbox->data(Qt::CheckStateRole);
        item_checkbox->setCheckState(Qt::Checked);

        // Create an element, which will serve as a combo box
        QComboBox *combobox = new QComboBox();
        QString OS_core = ui->comboBox_core->currentText();
        int core;
        if(OS_core == "FREE"){
            core = FreeCoreIsSelected.toInt();
        }
        else
            core = OS_core.toInt();
        for(int i=0; i<core; i++){
            combobox->addItem(QString::number(i));
        }
        combobox->setStyleSheet("combobox-popup: 0;");

        // Create an element, which will serve as a line edit
        QTableWidgetItem *argument = new QTableWidgetItem();
        argument->data(Qt::EditRole);

        //Open file to get the full path argument
        QString file_name_argument = QFileDialog::getOpenFileName(this, "Select an Argument", file_name);
        QFile file(file_name_argument);
        if(!file.open(QFile::ReadOnly | QFile::Text)){
         newFileArg = 0;
        }
        if(newFileArg)
            argument->setText(file_name_argument.section("/", -1,-1));
        else
            argument->setText("");

        // Create an element, which will serve as a push buton
        QPushButton *button_file_argument = new QPushButton();
        button_file_argument->setText("...");
        connect(button_file_argument, SIGNAL(clicked(bool)), this, SLOT(onClicked()));

        // Set the checkbox in the column 1
        ui->tableWidget->setItem(0,0, item_checkbox);

        //Set the combo box in the column 3
        ui->tableWidget->setCellWidget(0, 2, combobox);

        //Set the edit line in the column 4
        ui->tableWidget->setItem(0, 3, argument);

        //Set the edit line in the column 5
        ui->tableWidget->setCellWidget(0, 4, button_file_argument);
    }
}

void MainWindow::on_pushButton_File_Dialogue_Delete_clicked() //delete currwent row
{
    int i = ui->tableWidget->currentRow();
    if(i != -1){
        ui->tableWidget->removeRow(i);
    }
}

void MainWindow::on_comboBox_core_currentIndexChanged() //set the different combo box in tablewidget with number of core
{
    for(int j=0; j<ui->tableWidget->rowCount(); j++){

        QComboBox *combobox = new QComboBox();
        QString OS_core = ui->comboBox_core->currentText();
        int core;
        if(OS_core == "FREE"){
            core = FreeCoreIsSelected.toInt();
        }
        else
            core = OS_core.toInt();

        for(int i=0; i<core; i++){
                combobox->addItem(QString::number(i));
        }
        //Set the combo box in the column 3
        ui->tableWidget->setCellWidget(j, 2, combobox);
        combobox->setStyleSheet("combobox-popup: 0;");
    }
}

void MainWindow::on_tableWidget_doubleClicked() //set the new path application when double clic on the right cell
{
    int currentRow = ui->tableWidget->currentRow();
    int newFileName = 1;
    QLabel *lineEdit = (QLabel*)ui->tableWidget->cellWidget(currentRow,1);
    QString pathClick = lineEdit->text();
    if(ui->tableWidget->selectionModel()->currentIndex().column() == 1){ // == 1 && !(ui->tableWidget->focusWidget())
        QString file_name = QFileDialog::getOpenFileName(this, "Open a file", pathClick);
        QFile file(file_name);
        if(!file.open(QFile::ReadOnly | QFile::Text)){
          // QMessageBox::warning(this, "title", "file not change");
           newFileName = 0;
        }
        if (newFileName){
            // Create a String, which will serve as a name in a line edit
            //QString name_apllication = file_name.section("/", -1,-1);

            // Create an element, which will serve as a line edit
            QLabel *lineedit = new QLabel();
            lineedit->setText(file_name);
            //Set the edit line in the column 2
            ui->tableWidget->setCellWidget(currentRow, 1, lineedit);
        }
    }
}

void MainWindow::onClicked() //fonction connect to the push button in tablewidget and set the argument (line edit) with the new path
{
    int newFileName = 1;
    QPushButton *w = qobject_cast<QPushButton *>(QObject::sender());

    if(w){
        int row = getCurrentRow();
        QString file_name = QFileDialog::getOpenFileName(this, "Open a file", QDir::homePath());
        QFile file(file_name);
        if(!file.open(QFile::ReadOnly | QFile::Text)){
            newFileName = 0;
        }
        if (newFileName){
            // Create a String, which will serve as a name in a line edit
            //QString name_arg = file_name.section("/", -1,-1);

            // Create an element, which will serve as a line edit
            QTableWidgetItem *lineedit = new QTableWidgetItem();
            lineedit->data(Qt::EditRole);
            lineedit->setText(file_name);

            //Set the edit line in the column 2
            ui->tableWidget->setItem(row, 3, lineedit);
        }
    }
}

int MainWindow::getCurrentRow() //return the current Row of the focus Widget else return -1
{
    for(int i=0; i<ui->tableWidget->rowCount(); i++){
        for(int j=0; j<ui->tableWidget->columnCount(); j++) {
            if(ui->tableWidget->cellWidget(i,j) == ui->tableWidget->focusWidget()) {
                return i;
            }
        }
    }
    return -1;
}

void MainWindow::on_pushButton_Arguments_Check_clicked()
{
    int currentRow = -1;
    int checkState = 0;
    int row = ui->tableWidget->rowCount();
    validateCounters();
    if(row != 0 && number_counters_vector.size() != 0){
        for(int i=0; i<row; i++){
            if(ui->tableWidget->item(i,0)->checkState()){
                currentRow = i;
                checkState++;
            }
        }

        if(checkState == 1 && currentRow != -1){
            QLabel *lineEdit = (QLabel*)ui->tableWidget->cellWidget(currentRow,1);
            QString full_Path = lineEdit->text();
            QComboBox* combox=(QComboBox*)ui->tableWidget->cellWidget(currentRow,2);
            QString argument = ui->tableWidget->item(currentRow,3)->text();
            QString core = combox->currentText();
            //QString number_counters = "";
            QString StringListVectorCounters = "";
            for(int k=0; k<number_counters_vector.count(); k++)
            {
                if(counters_vector[k]!= "PAPI_NULL")
                //number_counters += number_counters_vector[k] + " ";
                StringListVectorCounters += counters_vector[k] + " ";
            }
            if(!comeFromTry)QMessageBox::information(this, "Checking", "Counter selected : " + StringListVectorCounters
                                                                     +"\nFull Path Application: " + full_Path
                                                                     + "\nCore selected: " + core
                                                                     + "\nArgument selected: " + argument);
             checkValue = 1;
        }
        else{
            QMessageBox::warning(this,"Warning","Too mush application selected");
            checkValue = 0;
        }
    }
    else {
        checkValue = 0;
        if(row == 0 && number_counters_vector.size() != 0)
            QMessageBox::warning(this,"WARNING","No Application in the list,\n please retry when you have added at least one ");
        else if(row != 0 && number_counters_vector.size() == 0)
            QMessageBox::warning(this,"WARNING","No Vectors selected,\nPlease retry when you have added at least one ");
        else if(tooMushCounters && row == 0)
            QMessageBox::warning(this, "WARNING", "Too much Counters selected (max: 4)\nNo Application in the list,\nPlease retry when you have added at least one and delete some counter ");
        else if(tooMushCounters && row != 0)
            QMessageBox::warning(this, "WARNING", "Too much Counters selected (max: 4),\nPlease delete some counter");
        else
            QMessageBox::warning(this,"WARNING","No Application in the list,\nNo Vectors selected,\nPlease retry when you have added at least one of each");
    }
}

void MainWindow::on_pushButton_Arguments_Run_clicked()
{
    on_pushButton_Plot_Clear_clicked();
    int currentRow = -1;
    int checkState = 0;
    int row = ui->tableWidget->rowCount();
    comeFromTry = 1;
    on_pushButton_Arguments_Check_clicked();
    comeFromTry = 0;
    if(row != 0 && number_counters_vector.size() != 0){
        for(int i=0; i<row; i++){
            if(ui->tableWidget->item(i,0)->checkState()){
                currentRow = i;
                checkState++;
            }
        }
        if(checkState == 1 && currentRow != -1 && checkValue == 1){
            QLabel *lineEdit = (QLabel*)ui->tableWidget->cellWidget(currentRow,1);
            std::string utf8_full_Path = lineEdit->text().toUtf8().constData();
                const char* full_Path = utf8_full_Path.c_str();

            std::string utf8_argument = ui->tableWidget->item(currentRow,3)->text().toUtf8().constData();
                const char* argument = utf8_argument.c_str();

            QComboBox* combox=(QComboBox*)ui->tableWidget->cellWidget(currentRow,2);
            std::string  utf8_core = combox->currentText().toUtf8().constData();
                const char* core = utf8_core.c_str();

            std::string utf8_counter0 = counters_vector[0].toUtf8().constData();
                const char* counter0 = utf8_counter0.c_str();
            std::string utf8_counter1 = counters_vector[1].toUtf8().constData();
                const char* counter1 = utf8_counter1.c_str();
            std::string utf8_counter2 = counters_vector[2].toUtf8().constData();
                const char* counter2 = utf8_counter2.c_str();
            std::string utf8_counter3 = counters_vector[3].toUtf8().constData();
                const char* counter3 = utf8_counter3.c_str();

            char* argumentProg[] = {"a.out",
                                    (char*)full_Path,
                                    (char*)counter0,
                                    (char*)counter1,
                                    (char*)counter2,
                                    (char*)counter3,
                                    (char*)argument,
                                    (char*)core,
                                    "1"};
            mainProg(argumentProg);
            qDebug() << "End mainProg";
        }

        /*on_pushButton_Plot_Clear_clicked();

        for(int j=0; j<counters_vector.size(); j++){
            if(counters_vector[j]== "PAPI_NULL") break;
            else{
                QString file_name = Path_selected + "/Resource_graphs/" + counters_vector[j] + "_data.txt";
                readFilesPlots(file_name);
            }
        }*/
    }
}

void MainWindow::createPlot(QVector<int> *x_, QVector<float> *y_, QString file_name)
{
        ui->tableWidget_2->insertRow(0);
        ui->tableWidget_2->setColumnCount(1);
        ui->tableWidget_2->setShowGrid(true);
        ui->tableWidget_2->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tableWidget_2->setHorizontalHeaderLabels(QStringList() << "plot" );
        ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);
        ui->tableWidget_2->setRowHeight(0, 366);

        QCustomPlot *customPlot1 = new QCustomPlot(QCUSTOMPLOT_H);

        customPlot1->clearGraphs();

        // generate some points of data (y0 for first, y1 for second graph):
        QVector<double> x(x_->size()), y(y_->size());// y2(x_->size()); // initialize with entries 0..100

        for (int i=0; i<x_->size(); i++)
        {
          x[i] = x_->at(i);
          y[i] = y_->at(i);
        }

        // create graph and assign data to it:
        customPlot1->addGraph();

        customPlot1->graph(0)->setData(x, y);

        // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
        customPlot1->graph(0)->rescaleAxes();

        // give the axes some labels
        customPlot1->xAxis->setLabel("Time ms (approxiation)");
        customPlot1->yAxis->setLabel("Cache misses");

        //legend
        QFont legendFont = font();  // start out with MainWindow's font..
        customPlot1->legend->setVisible(true);
        customPlot1->legend->setFont(legendFont);
        customPlot1->legend->setBrush(QBrush(QColor(255,255,255,230)));

        customPlot1->graph(0)->setLineStyle(QCPGraph::lsNone);
        customPlot1->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 3));
        // by default, the legend is in the inset layout of the main axis rect. So this is how we access it to change legend placement:
        customPlot1->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignRight);

        // add two new graphs and set their look:
        customPlot1->graph(0)->setPen(QPen(Qt::red)); // line color blue for first graph
        customPlot1->graph(0)->setName("Caches misses " + file_name);

        // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
        customPlot1->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
        customPlot1->replot();

        ui->tableWidget_2->setCellWidget(0, 0, customPlot1);
}

void MainWindow::readFilesPlots(QString file_name)
{
    //read .txt and values in vectors
    int i = 0;
    QVector<int> iTime;
    QVector<float> cacheMisses, numberInstruction;
    bool ok;
    QFile file(file_name);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
           //QMessageBox::information(this, "tilte", "file not open");
           return;
    }
    QTextStream in(&file);
    while( !in.atEnd())
    {
        QString line =in.readLine();
        QStringList split = line.split("\t");
        foreach(QString word, split)
        {
            if (i == 0)
            {
                iTime.append(word.toInt(&ok));
                i=1;
            }
            else if (i == 1)
            {
                numberInstruction.append(word.toFloat(&ok));
                i=2;
            }
            else if (i == 2)
            {
                cacheMisses.append(word.toFloat(&ok));
                i = 0;
            }
        }
    }
    file.close();
    createPlot(&iTime, &cacheMisses, file_name.section("/", -1,-1));
}


void MainWindow::on_pushButton_Plot_To_File_clicked()
{
    on_pushButton_Plot_Clear_clicked();

    QString file_name = QFileDialog::getOpenFileName(this, "Open file", Path_selected + "/Resource_graphs_saved");
    QFile file(file_name);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
           QMessageBox::information(this, "tilte", "file not open");
           return;
    }
    readFilesPlots(file_name);
}

void MainWindow::on_pushButton_savePlot_clicked()
{
    //if dir is empty Qmessage
    QDir dir(Path_selected + "/Resource_graphs");
    if (dir.isEmpty()){
        QMessageBox::warning(this, "", "No Plot are available to be saved");
        return;
    }
    QString file_save = QFileDialog::getOpenFileName(this, "Open file", Path_selected + "/Resource_graphs");
    QFile file_saved(file_save);
    if(file_saved.open(QFile::ReadOnly | QFile::Text)){
        QTextStream in(&file_saved);
        QString fileSaveName =  file_save.section("/", -1,-1);
        if(fileSaveName.contains(".txt"))
        {
            fileSaveName.replace(".txt","");
        }
        QString sDate = QDateTime::currentDateTime().toString("_dd-MM-yyyy_hh:mm:ss");
        QFile file(Path_selected + "/Resource_graphs_saved/" + fileSaveName + sDate + ".txt");
        file_path = Path_selected + "/Resource_graphs_saved/" + fileSaveName + sDate + ".txt";
        if(!file.open(QFile::ReadWrite | QFile::Text)){
               QMessageBox::warning(this, "WARNING", "The file wasn't saved");
               return;
        }
        QTextStream out(&file);
        while(!in.atEnd()){
           out << in.readLine();
           out << "\n";
        }
        file.close();
    }
}

void MainWindow::on_comboBox_currentIndexChanged()
{
    if(ui->comboBox->currentText() == "DEFAULT")
        SLEEP_TIME = 10000;
    else
        SLEEP_TIME = ui->comboBox->currentText().toInt();

}

void MainWindow::on_pushButton_Plot_Stop_clicked()
{
    //Stopped
    StartAndStopThread->stop = true;
    PlotThread->stop = true;
}

void MainWindow::on_pushButton_Plot_Clear_clicked()
{
    // Remove all items
    ui->tableWidget_2->clear();

    // Set row count to 0 (remove rows)
    ui->tableWidget_2->setRowCount(0);
}

int MainWindow::runThread(char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4, char *app, std::vector<double> instr_ret_threadPlot,
                          std::vector<double> counter_1_threadPlot,
                          std::vector<double> counter_2_threadPlot,
                          std::vector<double> counter_3_threadPlot,
                          std::vector<double> counter_4_threadPlot)
{
    task_characteristic output;

    output.event_counter_sets.push_back(instr_ret_threadPlot);
    output.event_name.push_back((char *)malloc(sizeof(char) * strlen("PAPI_TOT_INS")));
    strcpy(output.event_name[0], "PAPI_TOT_INS");

    if(strcmp(inCounter1,"PAPI_NULL") == -2){
        output.event_counter_sets.push_back(counter_1_threadPlot);
        output.event_name.push_back((char*)malloc(sizeof(char)*strlen(inCounter1)));
        strcpy(output.event_name[1], inCounter1);
        plot_txt(&output, inCounter1, 1, app);
    }
    if(strcmp(inCounter2,"PAPI_NULL") == -2){
        output.event_counter_sets.push_back(counter_2_threadPlot);
        output.event_name.push_back((char*)malloc(sizeof(char)*strlen(inCounter2)));
        strcpy(output.event_name[2], inCounter2);
        plot_txt(&output, inCounter2, 2, app);
    }
    if(strcmp(inCounter3,"PAPI_NULL") == -2){
        output.event_counter_sets.push_back(counter_3_threadPlot);
        output.event_name.push_back((char*)malloc(sizeof(char)*strlen(inCounter3)));
        strcpy(output.event_name[3], inCounter3);
        plot_txt(&output, inCounter3, 3, app);
    }
    if(strcmp(inCounter4,"PAPI_NULL") == -2){
        output.event_counter_sets.push_back(counter_4_threadPlot);
        output.event_name.push_back((char*)malloc(sizeof(char)*strlen(inCounter4)));
        strcpy(output.event_name[4], inCounter4);
        plot_txt(&output, inCounter4, 4, app);
    }
    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status))	{
        qDebug() << "Task 5 second characterization done!";
        return -1;
    }

    on_pushButton_Plot_Clear_clicked();

    for(int j=0; j<counters_vector.size(); j++){
        if(counters_vector[j]!= "PAPI_NULL"){
            QString file_name = Path_selected + "/Resource_graphs/" + app + "_" + counters_vector[j] + "_data.txt";
            readFilesPlots(file_name);
        }
    }
    return 0;
}

void MainWindow::time_exec(int time)
{
    ui->label_time_exec->setText(QString::number(time));
    if(time > TASK_ACTIVE_MS *1000)
        PlotThread->stop = true;
}

void MainWindow::startPlotThread(std::vector<double> instr_ret, std::vector<double> counter_1, std::vector<double> counter_2, std::vector<double> counter_3, std::vector<double> counter_4, char *inCounter1, char *inCounter2, char *inCounter3, char *inCounter4, char *app)
{ // vector<double> to double
    PlotThread->instr_ret_threadPlot = instr_ret;
    PlotThread->counter_1_threadPlot = counter_1;
    PlotThread->counter_2_threadPlot = counter_2;
    PlotThread->counter_3_threadPlot = counter_3;
    PlotThread->counter_4_threadPlot = counter_4;
    strcpy(PlotThread->inCounter1,inCounter1);
    strcpy(PlotThread->inCounter2,inCounter2);
    strcpy(PlotThread->inCounter3,inCounter3);
    strcpy(PlotThread->inCounter4,inCounter4);
    strcpy(PlotThread->app,app);

    PlotThread->start();
}

//--------------------------------------------------------------------------------------------------------------------------------
//mythread cpp
//--------------------------------------------------------------------------------------------------------------------------------

Mythread::Mythread(QObject *parent):
    QThread(parent)
{

}
void Mythread::run()
{    
    int i = 0;
    int skipped = 0;
    int totalValues = 0;
    if (0 == (my_pid = fork())){
        i++;
        mon_pid = syscall(SYS_gettid);
        fflush(stdout);

        if (i == 1){
            //printf("Child pid is: %d\n", mon_pid);
        }
        if( -1 == execve(argument[0], argument, NULL)){
            exit(-1);
        }
    }
    if ((retval = PAPI_attach(ev_set, my_pid)) != PAPI_OK){
        printf("Monitor pid is: %d\n", my_pid);
        printf("Papi error: %s\n", strerror(errno));
        exit(-1);
    }
    if ((retval = PAPI_start(ev_set)) != PAPI_OK){
        printf("PAPI_start error %d: %s0\n", retval, PAPI_strerror(retval));
        exit(-1);
    }
    if (PAPI_reset(ev_set) != PAPI_OK){
        printf("PAPI_reset error %d: %s0\n", retval, PAPI_strerror(retval));
        exit(-1);
    }
    //clear graph
    while((0 == waitpid(my_pid, &status, WNOHANG))){
        QMutex mutex;
        mutex.lock();
        if ((retval = PAPI_read(ev_set, values)) != PAPI_OK){
          qDebug("Failed to read the events");
        }
        if(values[0] == 0){
            skipped ++;
        }
        PAPI_reset(ev_set);
        instr_ret.push_back(values[0]);
        if(strcmp(inCounter1,"PAPI_NULL") == -2)   counter_1.push_back(values[1]);
        if(strcmp(inCounter2,"PAPI_NULL") == -2)   counter_2.push_back(values[2]);
        if(strcmp(inCounter3,"PAPI_NULL") == -2)   counter_3.push_back(values[3]);
        if(strcmp(inCounter4,"PAPI_NULL") == -2)   counter_4.push_back(values[4]);

        totalValues++;

        auto timestamp_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> test = timestamp_end - timestamp_start;
        //qDebug() << test.count();
        emit time_exec(test.count());
        qRegisterMetaType<std::vector<double>>();

        //just send the lqst value (counter_1.last)
        emit startPlot(instr_ret, counter_1, counter_2, counter_3, counter_4, inCounter1, inCounter2, inCounter3, inCounter4, app);

        //emit runStartAndStop(inCounter1, inCounter2, inCounter3, inCounter4, app);

        if (((test.count() * 1000 > TASK_ACTIVE_MS)) || this->stop){
            kill(my_pid, SIGKILL);
            PAPI_reset(ev_set);
            PAPI_stop(ev_set, values);
            qDebug() << "There are " << skipped << " value(s)" << " out of " << totalValues << " equal 0";
            if(this->stop)
                qDebug() << "Kill Process with STOP";
            else
                qDebug() << "Kill Process, time limit reached";
            //emit runStartAndStop(inCounter1, inCounter2, inCounter3, inCounter4, app);
            break;
        }
        mutex.unlock();
        usleep(SLEEP_TIME); //sampling
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
//mythreadPlot cpp
//--------------------------------------------------------------------------------------------------------------------------------

MyThreadPlot::MyThreadPlot(QObject *parent):
    QThread(parent)
{

}

void MyThreadPlot::run()
{
    QMutex mutex;
    mutex.lock();
    emit runStartAndStop(inCounter1, inCounter2, inCounter3, inCounter4, app, instr_ret_threadPlot, counter_1_threadPlot, counter_2_threadPlot, counter_3_threadPlot, counter_4_threadPlot);
    mutex.unlock();
    this->usleep(SLEEP_TIME);
}
