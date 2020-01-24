#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>
#include <stdarg.h>
#include <QString>
#include <QDateTime>
#include <QMessageLogContext>
//#include "qblockingqueue.hpp"
#include <QThread>
#include <QTextStream>
#include <QFile>


class QDebug;

class Logger {
public:
    enum LEVEL{
      LEVEL_NOTHING =0,
      LEVEL_DEBUG   =1,
      LEVEL_WARN,
      LEVEL_CRITICAL,
      LEVEL_FATAL,
    };

    struct TraceInfo{
        const char*   file;
        const char*   fucntion;
        size_t        line;

        QString FILE()const{return QString::fromLocal8Bit(file);}
        QString FUNCTION()const{return QString::fromLocal8Bit(fucntion);}
        size_t  LINE()const{ return line;}
    };

    Logger(QtMsgType qtType):mDebugDevice(new QDebug(qtType)){}

    /// delegate constructor. please refer to cplusplus syntax.
    Logger(LEVEL level):Logger(toQtMsaType(level)){}
    Logger(QString level):Logger(fromString(level)){}

    ~Logger(){
        if(mDebugDevice) delete mDebugDevice;
    }

    inline QDebug traceAt(TraceInfo trace){
            qint64 passedTimeInMS= QDateTime::currentMSecsSinceEpoch() - QDateTime(QDate::currentDate()).toMSecsSinceEpoch();
            QString dateTime=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
             mDebugDevice->nospace()<<"["<<dateTime<<":"<<passedTimeInMS;

             mDebugDevice->space()<<"@F:="<<trace.FILE()
                                  <<",Fn="<<trace.FUNCTION()
                                  <<",line="<<trace.LINE();
            *mDebugDevice<<"]";
            return mDebugDevice->space();
    }

private:
    static inline QtMsgType toQtMsaType(LEVEL level){
        const QtMsgType typesMapper[]={
            QtDebugMsg,     // DEFAULT,
            QtDebugMsg,     // LEVEL_DEBUG   =1,
            QtWarningMsg,   // LEVEL_WARN,
            QtCriticalMsg,  // LEVEL_CRITICAL,
            QtFatalMsg,     // LEVEL_FATAL,
        };

        return typesMapper[level];
    }

    static QtMsgType fromString(const QString& level){
            QString _l  =level.toLower();
            if(_l == "debug")return QtDebugMsg;
            else if(_l == "warn") return QtWarningMsg;
            else if(_l == "critical") return QtCriticalMsg;
            else if(_l == "fatal") return QtFatalMsg;
            else return QtDebugMsg;
    }
    QDebug*   mDebugDevice;
};


static inline void __install_loggerHandler()
{
    printf("@__install_loggerHandler\n");

}

#define INSTALL_LOG_HANDLER()  __install_loggerHandler()
#define LOGGER_DEBUG_DEVICE()  Logger(Logger::LEVEL_DEBUG).traceAt(Logger::TraceInfo{__FILE__,__FUNCTION__,static_cast<size_t>(__LINE__)})
#define LOGGING_LEVEL(LEVEL) Logger((LEVEL)).traceAt(Logger::TraceInfo{__FILE__,__FUNCTION__,static_cast<size_t>(__LINE__)})
#endif // LOGGER_H
