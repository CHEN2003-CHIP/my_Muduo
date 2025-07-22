#include "Logger.h"
#include <string>
#include "Timestamp.h"
#include <iostream>
using namespace std;

using namespace std;

Logger& Logger::instance(){
    static Logger logger_;
    return logger_;
}

void Logger::setLogLevel(int level){
    this->LoggerLevel_=level;
}

//[info] time xxxxxxxx
void Logger::log(std::string msg){
    switch (this->LoggerLevel_)
    {
    case INFO:
        /* code */
        cout<<"[INFO] ";
        break;
    case ERROR:
        cout<<"[ERROR] ";
        break;
    case FATAL:
        cout<<"[FATAL] ";
        break;
    case DEBUG:
        cout<<"[DEBUG] ";
    default:
        
        break;
    }
    cout<<Timestamp::now().Timestamp::toString()<<" : "<<msg<<endl;

}