#ifndef TCPSERVERWRAPPER_H
#define TCPSERVERWRAPPER_H

#include <QObject>
#include <QTcpServer>
#include <QThread>


class TcpServerWrapper : public QTcpServer{
    Q_OBJECT
public:
    inline   TcpServerWrapper(QObject *parent = 0):QTcpServer(parent){}
    virtual ~TcpServerWrapper(){}

private:
    virtual void incomingConnection(qintptr handle)override;
};

class TcpConnectionDelegate : public QObject
{
    Q_OBJECT
public:
    explicit TcpConnectionDelegate(QObject *parent = nullptr);
    virtual ~TcpConnectionDelegate();
    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
signals:
    void sigHasReadyClient(QTcpSocket *);
    void sigOnError(QAbstractSocket::SocketError );

private slots:
    void onReadyRead();
    void onReadyForConnecting();
    void onAcceptError(QAbstractSocket::SocketError socketError);
    void onClientDestory(QObject* );
private:
    QTcpServer* mTcpServer;};

#endif // TCPSERVERWRAPPER_H
