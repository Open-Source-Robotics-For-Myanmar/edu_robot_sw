#include "wifi_tcp_server.h"
#include <QDataStream>
#include <QDebug>

WiFiTcpServer::WiFiTcpServer(QObject *parent) 
    : QTcpServer(parent)
{
    qDebug() << "Initializing WiFi TCP Server (no SSL)...";
    
    // Initialize WiFi configurator
    m_wifiConfig = new WiFiConfigurator(this);
    qDebug() << "WiFi configurator initialized for TCP server";
}

WiFiTcpServer::~WiFiTcpServer()
{
    qDebug() << "WiFi TCP Server destroyed";
}

void WiFiTcpServer::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "=== New WiFi TCP Connection ===";
    qDebug() << "Socket Descriptor:" << socketDescriptor;
    
    QTcpSocket *socket = new QTcpSocket(this);
    
    if (socket->setSocketDescriptor(socketDescriptor)) 
    {
        qDebug() << "Socket descriptor set successfully";
        qDebug() << "Client address:" << socket->peerAddress().toString();
        qDebug() << "Client port:" << socket->peerPort();
        
        // Connect socket signals
        connect(socket, &QTcpSocket::readyRead, this, &WiFiTcpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &WiFiTcpServer::onDisconnected);
        
        qDebug() << "WiFi TCP client connected successfully";
    } 
    else 
    {
        qWarning() << "Failed to set socket descriptor";
        delete socket;
    }
}

void WiFiTcpServer::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    qDebug() << "=== Data Ready to Read from WiFi Client ===";
    qint64 bytesAvailable = socket->bytesAvailable();
    qDebug() << "Bytes available:" << bytesAvailable;
    
    // Get or create buffer for this socket
    QByteArray &buffer = socketBuffers[socket];
    quint32 &expectedSize = socketExpectedSizes[socket];
    
    // Append incoming data to buffer
    QByteArray newData = socket->readAll();
    qDebug() << "Read" << newData.size() << "bytes from WiFi client";
    buffer.append(newData);
    qDebug() << "Total buffer size:" << buffer.size();
    
    // Process all complete packets
    processPackets(socket, buffer, expectedSize);
}

void WiFiTcpServer::onDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    qDebug() << "=== WiFi TCP Client Disconnected ===";
    qDebug() << "Client was:" << socket->peerAddress().toString();
    
    // Clean up buffers when socket disconnects
    socketBuffers.remove(socket);
    socketExpectedSizes.remove(socket);
    socket->deleteLater();
}

void WiFiTcpServer::processPackets(QTcpSocket *socket, QByteArray &buffer, quint32 &expectedSize)
{
    while (true) {
        // If we don't know the packet size yet, try to read it
        if (expectedSize == 0) {
            if (buffer.size() < (int)sizeof(quint32)) {
                return; // Need more data for size header
            }
            
            // Read packet size from buffer
            QDataStream sizeStream(buffer);
            sizeStream.setVersion(QDataStream::Qt_6_0);
            sizeStream >> expectedSize;
            
            // Remove size header from buffer
            buffer.remove(0, sizeof(quint32));
        }
        
        // Check if we have the complete packet
        if (buffer.size() < (int)expectedSize) {
            return; // Need more data for complete packet
        }
        
        // Extract one complete packet
        QByteArray packet = buffer.left(expectedSize);
        buffer.remove(0, expectedSize);
        expectedSize = 0; // Reset for next packet
        
        // Parse the packet
        QDataStream stream(&packet, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_0);
        
        QString type;
        stream >> type;
        
        qDebug() << "WiFi Packet Type:" << type;

        if (type == "COMMAND") 
        {
            // Command packets: type + command string
            QByteArray data;
            stream >> data;
            QString command = QString::fromUtf8(data);
            qDebug() << "Received WiFi Command:" << command;
            
            QString response;
            
            if (command == "SEARCH_WIFI") {
                response = handleSearchWiFi();
                qDebug() << "Sending WiFi search results to client";
            } 
            else if (command.startsWith("CONNECT_WIFI:")) {
                QString params = command.mid(13); // Remove "CONNECT_WIFI:" prefix
                QStringList parts = params.split(":");
                if (parts.size() >= 1) {
                    QString ssid = parts[0];
                    QString password = parts.size() > 1 ? parts[1] : "";
                    response = handleConnectWiFi(ssid, password);
                } else {
                    response = "ERROR:Invalid CONNECT_WIFI format";
                }
                qDebug() << "Sending WiFi connect response to client";
            } 
            else if (command == "DISCONNECT_WIFI") {
                response = handleDisconnectWiFi();
                qDebug() << "Sending WiFi disconnect response to client";
            } 
            else if (command == "CURRENT_WIFI") {
                response = handleCurrentWiFi();
                qDebug() << "Sending current WiFi info to client";
            } 
            else if (command == "GET_SERVER_IP") {
                response = handleGetServerIp();
                qDebug() << "Sending server IP to client";
            } 
            else {
                response = "ERROR:Unknown WiFi command: " + command;
                qDebug() << "Unknown WiFi command received";
            }
            
            sendResponse(socket, response);
        } 
        else {
            qWarning() << "Unknown WiFi packet type:" << type;
            sendResponse(socket, "ERROR:Unknown packet type");
        }
    }
}

void WiFiTcpServer::sendResponse(QTcpSocket *socket, const QString &response)
{
    if (!socket || !socket->isOpen()) {
        qWarning() << "Cannot send WiFi response: socket is not open";
        return;
    }
    
    // Build packet in memory first
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << QString("RESPONSE") << response.toUtf8();
    
    // Send packet size first, then packet data
    QDataStream out(socket);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint32(packet.size());
    socket->write(packet);
    socket->flush();
    
    qDebug() << "WiFi Response sent:" << response.left(100) << "..."; // First 100 chars
}

QString WiFiTcpServer::handleSearchWiFi()
{
    qDebug() << "Searching for WiFi networks...";
    QList<WiFiConfigurator::NetworkInfo> networks = m_wifiConfig->searchWiFi();
    
    if (networks.isEmpty()) {
        QString error = m_wifiConfig->getLastError();
        if (!error.isEmpty()) {
            return "ERROR:" + error;
        }
        return "WIFI_LIST:";
    }
    
    // Format: WIFI_LIST:ssid1:signal1:security1:connected,ssid2:signal2:security2:connected,...
    QStringList networkList;
    for (const auto &network : networks) {
        QString networkStr = QString("%1:%2:%3:%4")
            .arg(network.ssid)
            .arg(network.signal)
            .arg(network.security)
            .arg(network.isConnected ? "yes" : "no");
        networkList.append(networkStr);
    }
    
    return "WIFI_LIST:" + networkList.join(",");
}

QString WiFiTcpServer::handleConnectWiFi(const QString &ssid, const QString &password)
{
    qDebug() << "Connecting to WiFi:" << ssid;
    bool success = m_wifiConfig->connectWiFi(ssid, password);
    
    if (success) {
        return "CONNECT_OK:" + ssid;
    } else {
        QString error = m_wifiConfig->getLastError();
        return "ERROR:" + (error.isEmpty() ? "Connection failed" : error);
    }
}

QString WiFiTcpServer::handleDisconnectWiFi()
{
    qDebug() << "Disconnecting WiFi...";
    bool success = m_wifiConfig->disconnectWiFi();
    
    if (success) {
        return "DISCONNECT_OK";
    } else {
        QString error = m_wifiConfig->getLastError();
        return "ERROR:" + (error.isEmpty() ? "Disconnection failed" : error);
    }
}

QString WiFiTcpServer::handleCurrentWiFi()
{
    qDebug() << "Getting current WiFi info...";
    QString ssid = m_wifiConfig->getCurrentSSID();
    QString ip = m_wifiConfig->getCurrentIP();
    
    if (ssid.isEmpty()) {
        return "CURRENT_WIFI:Not connected::";
    }
    
    // Format: CURRENT_WIFI:ssid:ip
    return QString("CURRENT_WIFI:%1:%2").arg(ssid).arg(ip);
}

QString WiFiTcpServer::handleGetServerIp()
{
    qDebug() << "Getting server IP for SSL client...";
    QString ip = m_wifiConfig->getCurrentIP();
    
    if (ip.isEmpty()) {
        return "SERVER_IP:";
    }
    
    // Format: SERVER_IP:<ip>
    return QString("SERVER_IP:%1").arg(ip);
}
