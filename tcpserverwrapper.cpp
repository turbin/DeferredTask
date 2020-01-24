#include "tcpserverwrapper.h"
#include <QTcpSocket>
#include "logger.hpp"


void TcpServerWrapper::incomingConnection(qintptr handle)
{
     LOGGING_LEVEL("debug")<<"@incomingConnection() handle"<<handle;
     QTcpServer::incomingConnection(handle);
}


TcpConnectionDelegate::TcpConnectionDelegate(QObject *parent)
                :QObject(parent)
                ,mTcpServer((QTcpServer*)(new TcpServerWrapper(this)))
{
    connect(mTcpServer, SIGNAL(newConnection()), this, SLOT(onReadyForConnecting()));
    connect(mTcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(onAcceptError(QAbstractSocket::SocketError)));
    mTcpServer->setMaxPendingConnections(90);
}

TcpConnectionDelegate::~TcpConnectionDelegate()
{

}

bool TcpConnectionDelegate::listen(const QHostAddress &address, quint16 port)
{
     return mTcpServer->listen(address, port);
}

void TcpConnectionDelegate::onReadyRead()
{
  QTcpSocket * _client =  (QTcpSocket *)(sender());
  emit sigHasReadyClient(_client);
}

void TcpConnectionDelegate::onReadyForConnecting()
{


   QTcpSocket * _client = mTcpServer->nextPendingConnection();

   connect(_client, SIGNAL(readyRead()),     this,   SLOT(onReadyRead()));
   connect(_client, SIGNAL(disconnected()), _client, SLOT(deleteLater()));
//   connect(_client, SIGNAL(destroyed(QObject *)),    this, SLOT(onClientDestory(QObject *)));
}
void TcpConnectionDelegate::onAcceptError(QAbstractSocket::SocketError socketError)
{
    LOGGING_LEVEL("debug")<<"@onAcceptError() "<<mTcpServer->errorString();
    emit sigOnError(socketError);
}

void TcpConnectionDelegate::onClientDestory(QObject * obj)
{
//    LOGGING_LEVEL("debug")<<"@onClientDestory() "<<obj;
}

