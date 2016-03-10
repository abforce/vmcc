#include "socketpool.h"

SocketPool::SocketPool()
{
    socketMapping = new QMap<int, QTcpSocket*>();
    mapperReadyRead = new QSignalMapper(this);
    mapperDisconnected = new QSignalMapper(this);

    QObject::connect(mapperReadyRead, SIGNAL(mapped(int)), this, SLOT(onSocketReadyRead(int)));
    QObject::connect(mapperDisconnected, SIGNAL(mapped(int)), this, SLOT(onSocketDisconnected(int)));
}

void SocketPool::addSocket(QTcpSocket *socket, int id)
{
    QObject::connect(socket, SIGNAL(readyRead()), mapperReadyRead, SLOT(map()));
    mapperReadyRead->setMapping(socket, id);

    QObject::connect(socket, SIGNAL(disconnected()), mapperDisconnected, SLOT(map()));
    mapperDisconnected->setMapping(socket, id);

    socketMapping->insert(id, socket);
}

void SocketPool::removeSocket(QTcpSocket *socket)
{
    QObject::disconnect(socket, SIGNAL(readyRead()), mapperReadyRead, SLOT(map()));
    mapperReadyRead->removeMappings(socket);

    QObject::disconnect(socket, SIGNAL(disconnected()), mapperDisconnected, SLOT(map()));
    mapperDisconnected->removeMappings(socket);

    socketMapping->remove(socketMapping->key(socket));
}

QTcpSocket *SocketPool::getSocketById(int id)
{
    return socketMapping->value(id, 0);
}

void SocketPool::onSocketReadyRead(int id)
{
    emit socketReadyRead(socketMapping->value(id), id);
}

void SocketPool::onSocketDisconnected(int id)
{
    emit socketDisconnected(socketMapping->value(id), id);
}
