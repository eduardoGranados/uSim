/**
 *  namgenerator.h
 *  Class NamGenerator
 *
 **/

#ifndef NAMGENERATOR_H_
#define NAMGENERATOR_H_

#include <stddef.h>  // defines NULL
#include <fstream>
#include <string>
using namespace std;

class NamGenerator {

private:
    ofstream namFile;
    NamGenerator (){}
    static NamGenerator* instance;

protected:
    NamGenerator (char* filename){
        namFile.open(filename,ios::out);
    }

public:
    static NamGenerator* Inst(char* filename){
        if(instance == NULL){
            instance = new NamGenerator(filename);
          }
        return instance;
    }
    static NamGenerator* Inst(){
        return instance;
    }
    ~NamGenerator(){
        namFile.close();
    }
    void addEvent(string namLine){
        namFile<<namLine<<endl;
    }
};
NamGenerator* NamGenerator::instance = NULL;
#endif
