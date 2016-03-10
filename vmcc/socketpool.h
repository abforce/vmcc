#ifndef SOCKETPOOL_H
#define SOCKETPOOL_H

#include <QSignalMapper>
#include <QTcpSocket>
#include <QMap>

class SocketPool : public QObject
{
    Q_OBJECT
public:
    SocketPool();
    void addSocket(QTcpSocket *socket, int id);
    void removeSocket(QTcpSocket *socket);
    QTcpSocket *getSocketById(int id);

private:
    QSignalMapper *mapperReadyRead;
    QSignalMapper *mapperDisconnected;
    QMap<int, QTcpSocket*> *socketMapping;

private slots:
    void onSocketReadyRead(int id);
    void onSocketDisconnected(int id);

signals:
    void socketReadyRead(QTcpSocket *socket,int id);
    void socketDisconnected(QTcpSocket *socket,int id);
};

#endif // SOCKETPOOL_H
