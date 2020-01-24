#include <QString>
#include <QtTest>
#include <QCoreApplication>

#include "deferredtask.h"

#include <QByteArray>
#include <QString>

#include <QTcpSocket>
#include <QByteArray>
#include <QEventLoop>

#include <QSharedPointer>
#include <QScopedPointer>
#include <QJsonDocument>
#include <QJsonObject>


#include "tcpserverwrapper.h"
#include "deferredtask.h"
#include "logger.hpp"

class Deferedtask_testTest : public QObject
{
    Q_OBJECT

public:
    Deferedtask_testTest();

private Q_SLOTS:
    void testCase1();
};


static int _trace_tcp_package(DeferredTask<QByteArray, QString>::Deferred &&def, QByteArray incoming_data)
{
    LOGGING_LEVEL("debug")<<"CALL  def= "<<&def;
    LOGGING_LEVEL("debug")<<"CALL  _incomingdata "<<incoming_data;
    def.slove(incoming_data);
    return  0;
}

Deferedtask_testTest::Deferedtask_testTest()
{


}

void Deferedtask_testTest::testCase1()
{
    QScopedPointer<QEventLoop> _looper(new QEventLoop);
    QSharedPointer<DeferredTask<QByteArray,QString>> deferredTask(new DeferredTask<QByteArray, QString>());

    TcpConnectionDelegate tcpConnection;

    QObject::connect(&tcpConnection, &TcpConnectionDelegate::sigHasReadyClient,[&](QTcpSocket* _tcpClient){
            LOGGING_LEVEL("debug")<<"on new connection "<<_tcpClient;
            deferredTask->observeOn([_tcpClient](DeferredTask<QByteArray, QString>::Deferred &def){
                 QByteArray _data = _tcpClient->readAll();
                 def.slove(_data);
            })
            .then([](DeferredTask<QByteArray, QString>::Deferred &def, QByteArray _incomingdata){
                LOGGING_LEVEL("debug")<<"CALL NDK def= "<<&def;
                LOGGING_LEVEL("debug")<<"CALL NDK _incomingdata "<<_incomingdata;
                _trace_tcp_package(std::move(def), std::move(_incomingdata));

            })
            .then([_tcpClient](DeferredTask<QByteArray, QString>::Deferred &def, QByteArray _incomingdata){
                _tcpClient->write(_incomingdata);
                _tcpClient->flush();
                _tcpClient->close();
            })
            .catching([](QString errorMessage){
                LOGGING_LEVEL("ERROR")<<"errorMessage "<<errorMessage;
            })
           .start();
    });

    QObject::connect(&tcpConnection, &TcpConnectionDelegate::sigOnError,[&](QAbstractSocket::SocketError errors){
            LOGGING_LEVEL("debug")<<"on accept error code "<<errors;
            _looper->exit(-1);
    });

    if(!tcpConnection.listen(QHostAddress::Any, 5544)){
        LOGGING_LEVEL("fatal")<<"listen 5544 failure! server exit from listen loop!";
        return;
    }

    _looper->exec();
    return;
}






QTEST_MAIN(Deferedtask_testTest)

#include "tst_deferedtask_testtest.moc"
