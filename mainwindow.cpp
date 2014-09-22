#include "mainwindow.h"
#include "ui_mainwindow.h"
NamGenerator* NamGenerator::instance = NULL;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)  //definir constructor de ventana principal
{
    ui->setupUi(this);QSize(840,470);
    usim = new USim();

    //Directorios
    QString absoluteTempPath=execRoute.absolutePath()+"/temp";

    QDir dir(absoluteTempPath);
    if (!dir.exists()){
        QDir().mkpath(absoluteTempPath);
        qDebug() << "Creando carpeta temporal";
    }
    fileRoute="temp/temp.usim";
    tempPath = "temp/";

    //Set Eventos
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(botonNewClicked()));
    connect(ui->actionGuardar, SIGNAL(triggered()), this, SLOT(botonGuardarClicked()));
    connect(ui->experiments , SIGNAL(currentIndexChanged(int)),this,SLOT(experimentsChanged()));


}

MainWindow::~MainWindow()
{
    delete ui;
}

float MainWindow::calcNdAck(float n,float fs,float cwndo){

    float dack;
    dack= (pow ( (fs/cwndo),1/n) ) -1 ;
    return dack;

}
void MainWindow::botonNewClicked(){ //Abrir o crear nuevo archivo

    fileRoute=QFileDialog::getSaveFileName(0,"Abre o Crea un archivo","/home","tipo uSim (*.usim)",new QString("tipo uSim (*.usim)"));

    QFile file(fileRoute);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text )){
        //qDebug() << "No se pudo crear el archivo 1";
    }else{
        if(!sesion.setContent(&file)){
            qDebug() << "Archivo vacio";
        }else{
            QDomElement simulations = sesion.firstChildElement();
            QDomNodeList simList = simulations.elementsByTagName("simulation");
            for(int i = 0; i < simList.count(); i++){

                QDomElement itemele = simList.at(i).toElement();
                ui->experiments->addItem(itemele.attribute("Date"));

            }
        }
        file.close();
    }
}
void MainWindow::refreshWindow(QString date){

    QDomElement simulations = sesion.firstChildElement();
    QDomNodeList simList = simulations.elementsByTagName("simulation");
    for(int i = 0; i <= simList.count(); i++){

        QDomElement itemele = simList.at(i).toElement();

        if(itemele.attribute("Date")==date){

            QDomNodeList inDList=itemele.elementsByTagName("indata");
            QDomElement data = inDList.at(0).toElement();

            usim = new USim(data.attribute("tsim").toFloat(),data.attribute("appr").toInt(),
                            data.attribute("bn1").toInt(),data.attribute("bn2").toInt(),
                            data.attribute("ndivack").toInt(),data.attribute("ssthresh").toInt(),
                            data.attribute("buffer").toInt(),data.attribute("delay1").toFloat(),data.attribute("delay2").toFloat(),
                            data.attribute("timeout").toFloat(),data.attribute("delta").toFloat());
            usim->run_sim();
            usim->plot_cwnd();
            usim->plot_gp();
            usim->plot_tp();

            showPlots();
            break;
        }else{
            //do nothing
        }
    }
}

void MainWindow::experimentsChanged(){

    //qDebug() <<  ui->experiments->currentText();
    refreshWindow( ui->experiments->currentText());

}

void MainWindow::botonGuardarClicked(){ //evento guardar simulacion
    if(!(usim->isEmpty())){
        saveToFile();
    }else{
        qDebug() << "uSim vacio";
        QMessageBox Msgbox;
        Msgbox.setText("No se puede guardar experimento vacio");
        Msgbox.exec();
    }
}
void MainWindow::on_saveB_clicked()
{
    if(!(usim->isEmpty())){
        saveToFile();
    }else{
        qDebug() << "uSim vacio";
        QMessageBox Msgbox;
        Msgbox.setText("No se puede guardar experimento vacio");
        Msgbox.exec();
    }
}

void MainWindow::showPlots(){  //muestra graficas en mainwindow
    QIcon icon;
    QSize size;
    char *auxPath =new char[100];

    sprintf(auxPath,"%suSim_cwnd.jpeg", tempPath);
    QPixmap background(auxPath);
    icon.addPixmap(background, QIcon::Normal, QIcon::On);
    size = QSize(840,470);
    QPixmap pixmap = icon.pixmap(size, QIcon::Normal, QIcon::On);
    ui->cwnd_area->setPixmap(pixmap);

    sprintf(auxPath,"%suSim_gp.jpeg", tempPath);
    QPixmap background2(auxPath);
    icon.addPixmap(background2, QIcon::Normal, QIcon::On);
    size = QSize(840,470);
    QPixmap pixmapgp = icon.pixmap(size, QIcon::Normal, QIcon::On);
    ui->gp_area->setPixmap(pixmapgp);

    sprintf(auxPath,"%suSim_tp.jpeg", tempPath);
    QPixmap background3(auxPath);
    icon.addPixmap(background3, QIcon::Normal, QIcon::On);
    size = QSize(840,470);
    QPixmap pixmaptp = icon.pixmap(size, QIcon::Normal, QIcon::On);
    ui->tp_area->setPixmap(pixmaptp);

    sprintf(auxPath,"%suSim_tp&gp.jpeg", tempPath);
    QPixmap background4(auxPath);
    icon.addPixmap(background4, QIcon::Normal, QIcon::On);
    size = QSize(840,470);
    QPixmap pixmaptpandgp = icon.pixmap(size, QIcon::Normal, QIcon::On);
    ui->tpygp_area->setPixmap(pixmaptpandgp);
    delete []auxPath;
}

void MainWindow::saveToFile(){ //guarda en fileRoute simulacion reciente

        QFile file(fileRoute);
        QDomDocument document;

        if(!file.exists() )
        {
            //agregar raiz
            QDomElement root= document.createElement("simulations");
            document.appendChild(root);
            //agregar simulacion
            QDomElement simulation = document.createElement("simulation");
            simulation.setAttribute("ID",QString::number(1));

            QString currentD=QDateTime::currentDateTime().toString();
            ui->experiments->addItem(currentD);
            simulation.setAttribute("Date",currentD);

            root.appendChild(simulation);
            //agregar datos de simulacion
            QDomElement indata = document.createElement("indata");
            indata.setAttribute("tsim",usim->getTSIM());
            indata.setAttribute("appr",usim->getAPP_RATE());
            indata.setAttribute("bn1",usim->getBOTTLENECK());
            indata.setAttribute("bn2",usim->getBOTTLENECK2());
            indata.setAttribute("ndivack",usim->getDIVACKS());
            indata.setAttribute("ssthresh",usim->getSSTHRESH());
            indata.setAttribute("buffer",usim->getBUFFERSIZE());
            indata.setAttribute("delay1",usim->getDELAY1());
            indata.setAttribute("delay2",usim->getDELAY2());
            indata.setAttribute("timeout",usim->getTIMEOUT());
            indata.setAttribute("delta",usim->getDELTA());
            simulation.appendChild(indata);

            if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                qDebug() << "No se pudo crear el archivo 2";
            }
            else
            {
                QTextStream stream(&file);
                stream << document.toString();
                sesion=document;
                file.close();
                qDebug() << "se creo archivo en "+fileRoute;
            }
        }else{

            if(!file.open(QIODevice::ReadOnly | QIODevice::Text ))
            {
                qDebug() << "No se pudo crear el archivo";

            }
            else
                {
                    if(!document.setContent(&file))
                    {
                        qDebug() << "Failed to load document";
                    }
                    file.close();
                }

                QDomElement simulations = document.firstChildElement();

                //agregar simulacion
                QDomElement simulation = document.createElement("simulation");
                simulation.setAttribute("ID",QString::number(simulations.childNodes().count()+1));

                QString currentD=QDateTime::currentDateTime().toString();
                ui->experiments->addItem(currentD);
                simulation.setAttribute("Date",currentD);

                simulations.appendChild(simulation);
                //agregar datos de simulacion
                QDomElement indata = document.createElement("indata");
                indata.setAttribute("tsim",usim->getTSIM());
                indata.setAttribute("appr",usim->getAPP_RATE());
                indata.setAttribute("bn1",usim->getBOTTLENECK());
                indata.setAttribute("bn2",usim->getBOTTLENECK2());
                indata.setAttribute("ndivack",usim->getDIVACKS());
                indata.setAttribute("ssthresh",usim->getSSTHRESH());
                indata.setAttribute("buffer",usim->getBUFFERSIZE());
                indata.setAttribute("delay1",usim->getDELAY1());
                indata.setAttribute("delay2",usim->getDELAY2());
                indata.setAttribute("timeout",usim->getTIMEOUT());
                indata.setAttribute("delta",usim->getDELTA());
                simulation.appendChild(indata);


                if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QFile::Truncate))
                {
                    qDebug() << "No se pudo crear el archivo 4";
                }
                else
                {
                    QTextStream stream(&file);
                    sesion=document;
                    stream << document.toString();
                    file.close();
                    qDebug() << "se creo archivo en "+fileRoute;
                }
        }
}


void MainWindow::on_exe_clicked() //inicio de simulacion
{


    int ps, ar, bw1, bw2, da, sst, acksize, bfs;
    float d1,d2,to,tsim,del;


    tsim = ui->tsim->text().toInt();
//    ps = ui->packetsize->text().toInt();
    ar = ui->app_rate->text().toInt();
    bw1 = ui->bw1->text().toInt();
    bw2 = ui->bw2->text().toInt();
    da= ui->ndivacks->text().toInt();
    sst = ui->ssthresh->text().toInt();
    bfs = ui->bfs->text().toInt();
    d1 = ui->delay1->text().toFloat();
    d2 = ui->delay2->text().toFloat();
    to = ui->timeout->text().toFloat();

    del = ui->delta->text().toFloat();
//    acksize = ui->acksize->text().toInt();


    usim = new USim(tsim,ar,bw1,bw2,da,sst,bfs,d1,d2,to,del);
    usim->run_sim();
    usim->plot_cwnd();
    usim->plot_gp();
    usim->plot_tp();
    usim->plot_tp_and_gp();


//    //qDebug() << sim->getlog();
    ui->logTextEdit->clear();
    ui->logTextEdit->appendPlainText(usim->getlog());

    showPlots();

}



void MainWindow::on_run_nam_clicked()
{
    QFile file("temp/usim.nam");
    if(!file.exists() )
    {
        qDebug() << "no file";
        QMessageBox Msgbox;
        Msgbox.setText("Debe correr el experimento primero");
        Msgbox.exec();

    }else{
        QString route=QFileDialog::getSaveFileName(0,"Donde desea guardar","/home","tipo NAM (*.nam)",new QString("tipo NAM (*.nam)"));
        QFile::copy("temp/usim.nam", route);

    }
}
