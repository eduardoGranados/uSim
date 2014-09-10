#ifndef SSSIM_H
#define SSSIM_H

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <string>
#include <math.h>
#include <sstream>
#include <QDebug>
using namespace std;

class SSsim
{
private:
    int nRTT;
    float ndivAcks;
    float fileSize;
    float initCwnd;
    float delay;
    float ssthresh;
    float rwnd;
    float initThresh;
    QString log;
    char* tempPath;


    void graficar(float p1)
    {
        FILE *pipe = popen("gnuplot -persist","w");

        fprintf(pipe, "set grid\n");
        fprintf(pipe, "set boxwidth 1 absolute\n");
        fprintf(pipe, "set style fill solid 1.00 border -1\n");
        fprintf(pipe, "set terminal jpeg medium\n");
        fprintf(pipe, "set output '%sgrafica.jpeg'\n",tempPath);
        fprintf(pipe, "set xlabel 'Tiempo(ms)'\n");
        fprintf(pipe, "set ylabel 'CWND(segmentos)'\n");
        fprintf(pipe, "set key bottom right\n");
        fprintf(pipe, "ssthresh=%5.2f \n", p1);
        fprintf(pipe, "plot ssthresh , \"%site.dat\" using 1:2 with linespoints title \" cwnd \"  \n",tempPath);
        fclose(pipe);
    }
    void graficarTP(float p1)
    {
        FILE *pipe = popen("gnuplot -persist","w");

        fprintf(pipe, "set grid\n");
        fprintf(pipe, "set boxwidth 1 absolute\n");
        fprintf(pipe, "set style fill solid 1.00 border -1\n");
        fprintf(pipe, "set terminal jpeg medium\n");
        fprintf(pipe, "set output '%sgraficaTP.jpeg'\n",tempPath);
        fprintf(pipe, "set xlabel 'Tiempo(ms)'\n");
        fprintf(pipe, "set ylabel 'Throughput(segmentos)'\n");
        fprintf(pipe, "set key bottom right\n");

        fprintf(pipe, "ssthresh=%5.2f \n", p1);

        fprintf(pipe, "plot ssthresh , \"%siteTP.dat\" using 1:2 with linespoints title \" Throughput \"  \n",tempPath);
        fclose(pipe);

    }

public:

    SSsim (){
        nRTT=0;
        ndivAcks=0;
        fileSize=0;
        initCwnd=0;
        delay=0;
        ssthresh=0;
        rwnd=0;
        log="";
        initThresh=ssthresh;
    }

    SSsim (int n,float nda, float fs, float cwndo, float d,float thrsh, float rw, char* _tempPath){
        nRTT=n;
        ndivAcks=nda;
        fileSize=fs;
        initCwnd=cwndo;
        delay=d;
        ssthresh=thrsh;
        rwnd=rw;
        initThresh=ssthresh;
        tempPath=_tempPath;
    }
    ~SSsim(){}

    void init(){

        FILE *file;
        FILE *fileTP;
        char *auxPath =new char[100];
        sprintf(auxPath,"%site.dat", tempPath);

        file=fopen(auxPath,"w");

        sprintf(auxPath,"%siteTP.dat", tempPath);
        fileTP=fopen(auxPath,"w"); //grafica TP

        float cwnd=initCwnd;

        float p=0;  //progreso en segmentos de la transferencia
        int iterations=0;
        bool end=false;
        bool inCA=false;

        float rttIterator;
        float cwndAux;
        log  = "******************************************************************************";
        log += "\nLog de Simulacion \n Cwnd (segmentos) Progreso de transferencia (segmentos)";
        log += "\n******************************************************************************";

        log += "\n nRTT: "+QString::number(nRTT) +" ndivAcks: "+QString::number(ndivAcks)+" initCwnd: "
                + QString::number(initCwnd)+"\n one way delay: "+ QString::number(delay)
                + " ssthresh: "+QString::number(ssthresh)+" rwnd: "+QString::number(rwnd)+" fileSize: "+QString::number(fileSize);
        log += "\n******************************************************************************";
        log += "\n inicio--->cwnd:";
        log= log.append(QString::number(cwnd));


        fprintf(file,"0 %f %f\n",cwnd,cwnd);

        fprintf(fileTP,"%f 0 \n",delay);

        cwndAux=cwnd;
        p=p+cwnd;

//-------------------------------------------------------Simulacion---------------------------------------------------------
        while(end==false){
            iterations++;
            rttIterator=0;

            //El emisor recibe los divacks y acks
            while((rttIterator<cwnd)&&(inCA==false)){ //Crecimiento de cwnd en 1 rtt en SS
                rttIterator++;
                cwndAux=cwndAux+ndivAcks;
                if(cwndAux>=ssthresh){
                    cwndAux=ssthresh;
                    inCA=true;
                    log+="\n Inicio de Congestion Avoidance";
                }
                if(cwndAux>rwnd){
                    fprintf(file,"%f %f \n",((iterations*delay)-(rttIterator/cwnd)*delay),cwndAux);
                    ssthresh=rwnd/2;  //setea el valor de ssthresh a la mitad de ventana anunciada
                    cwndAux=initCwnd; //se reinicia el valor de cwnd
                    fprintf(file,"%f %f \n",((iterations*delay)-((rttIterator/cwnd)*delay)+0.1),cwndAux);
                }
            }
            while((rttIterator<cwnd)&&(inCA==true)){ //Crecimiento de cwnd en 1 rtt en CA
                rttIterator++;
                cwndAux=cwndAux+(ndivAcks/cwndAux);
                if(cwndAux>rwnd){
                    fprintf(file,"%f %f \n",((iterations*delay)-(rttIterator/cwnd)*delay),cwndAux);
                    cwndAux=cwndAux/2; //setea cwnd a la mitad actual
                    fprintf(file,"%f %f \n",((iterations*delay)-((rttIterator/cwnd)*delay)+0.1),cwndAux);
                }
            }
            cwnd=cwndAux;
            //Se envian los datos al receptor
            p=p+cwnd;

            if(p>=fileSize){  //condicion de fin de transferencia
                p=fileSize;
                end=true;
            }
            log+="\n RTT: "+QString::number(iterations)+"--->cwnd:"+QString::number(cwnd)+"--->Progreso de Transferencia:"+QString::number(p);
            //cout<<"iteracion:"<<iterations<<"--->cwnd:"<<cwnd<<"--->p:"<<p<<endl;
            fprintf(file,"%f %f \n",(iterations*delay),cwnd);
            fprintf(fileTP,"%f %f \n",((iterations*delay)+delay),p);

        }
//----------------------------------------------------------------------------------------------------------------
        fclose(file);
        fclose(fileTP);

        delete []auxPath;

        this->graficar(ssthresh);
        this->graficarTP(ssthresh);

    }
    bool isEmpty(){
        if (nRTT==0 && ndivAcks==0 && fileSize==0 && initCwnd==0 && delay==0 && ssthresh==0 && rwnd==0){
            return true;
        }
        return false;
    }

//GETTERS

    int getnRTT(){return nRTT;}
    float getndivAcks(){return ndivAcks;}
    float getfileSize(){return fileSize;}
    float getinitCwnd(){return initCwnd;}
    float getdelay(){return delay;}
    float getssthresh(){return ssthresh;}
    float getinitthresh(){return initThresh;}
    float getrwnd(){return rwnd;}
    QString getlog(){return log;}


};


#endif // SSSIM_H


