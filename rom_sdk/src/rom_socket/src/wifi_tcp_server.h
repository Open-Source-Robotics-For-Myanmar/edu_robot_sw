#ifndef WIFI_TCP_SERVER_H
#define WIFI_TCP_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QByteArray>
#include "wifi_configurator.h"

/**
 * @brief Plain TCP Server for WiFi configuration (no SSL)
 * 
 * Listens on 10.0.0.1:7358 for WiFi management commands
 * Protocol: Simple packet-based with size header
 */
class WiFiTcpServer : public QTcpServer 
{
    Q_OBJECT
    
public:
    explicit WiFiTcpServer(QObject *parent = nullptr);
    ~WiFiTcpServer();
    
protected:
    void incomingConnection(qintptr socketDescriptor) override;
    
private slots:
    void onReadyRead();
    void onDisconnected();
    
private:
    void processPackets(QTcpSocket *socket, QByteArray &buffer, quint32 &expectedSize);
    void sendResponse(QTcpSocket *socket, const QString &response);
    
    // WiFi command handlers
    QString handleSearchWiFi();
    QString handleConnectWiFi(const QString &ssid, const QString &password);
    QString handleDisconnectWiFi();
    QString handleCurrentWiFi();
    QString handleGetServerIp();
    
    // WiFi configurator instance
    WiFiConfigurator *m_wifiConfig;
    
    // Track buffers per socket
    QMap<QTcpSocket*, QByteArray> socketBuffers;
    QMap<QTcpSocket*, quint32> socketExpectedSizes;
};

#endif // WIFI_TCP_SERVER_H
