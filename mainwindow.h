//uSim global variables, TODO: shouldn't be using global variables.  >>>  almost there  16/06/2014
//#define TSIM       4
#define PACKETSIZE 1000     //bytes
//#define APP_RATE   5000000  //it was 400000 for every 0.1 secs....bit per seconds
//#define BOTTLENECK 700000   //BW from src to router
//#define BOTTLENECK2 350000  //BW from router to dst
//#define DIVACKS 0
//#define SSTHRESH 400 // segments
#define ACKSIZE 54


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolButton>
#include <QPixmap>
#include <QToolBar>
#include <QLabel>
#include <QMessageBox>
#include <QtCore>
#include <QtXml>
#include <QFileDialog>
#include <QBrush>
#include <QDir>
#include "usim.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    Ui::MainWindow *ui;
    QToolButton *botonNew;
    bool bandNew;
    QString fileRoute;

    USim * usim;
    QDomDocument sesion;    //sesion actual de experimentos
    QDir execRoute;         //Ruta del ejecutable
    char *tempPath;         //Ruta de carpeta temporal

    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_exe_clicked();
    void showPlots();
    void saveToFile();
    float calcNdAck(float n,float fs,float cwndo);
    void botonNewClicked();
    void botonGuardarClicked();
    void refreshWindow(QString date);
    void experimentsChanged();

    void on_saveB_clicked();
    void on_run_nam_clicked();
};



#endif // MAINWINDOW_H
