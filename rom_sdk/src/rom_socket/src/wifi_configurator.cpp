#include "wifi_configurator.h"
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

WiFiConfigurator::WiFiConfigurator(QObject *parent)
    : QObject(parent)
{
}

WiFiConfigurator::~WiFiConfigurator()
{
}

QString WiFiConfigurator::executeNmcli(const QStringList &args, int timeoutMs)
{
    QProcess process;
    process.start("nmcli", args);
    
    if (!process.waitForFinished(timeoutMs)) {
        m_lastError = "Command timeout";
        qWarning() << "nmcli command timeout:" << args;
        process.kill();
        return "";
    }
    
    if (process.exitCode() != 0) {
        m_lastError = process.readAllStandardError().trimmed();
        qWarning() << "nmcli command failed:" << args << "-" << m_lastError;
        return "";
    }
    
    return process.readAllStandardOutput().trimmed();
}

QList<WiFiConfigurator::NetworkInfo> WiFiConfigurator::searchWiFi()
{
    qDebug() << "Searching for WiFi networks...";
    
    QList<NetworkInfo> networks;
    m_lastError.clear();
    
    // Enable WiFi if disabled
    executeNmcli(QStringList() << "radio" << "wifi" << "on", 3000);
    
    // Scan for networks: nmcli -t -f SSID,SIGNAL,SECURITY,ACTIVE device wifi list
    QString output = executeNmcli(QStringList() 
        << "-t" << "-f" << "SSID,SIGNAL,SECURITY,ACTIVE" 
        << "device" << "wifi" << "list" << "--rescan" << "yes", 20000);
    
    if (output.isEmpty()) {
        m_lastError = "WiFi scan failed or no networks found";
        return networks;
    }
    
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    QSet<QString> seenSSIDs;
    
    for (const QString &line : lines) {
        QStringList parts = line.split(":");
        if (parts.size() >= 4) {
            QString ssid = parts[0].trimmed();
            QString signalStr = parts[1].trimmed();
            QString security = parts[2].trimmed();
            QString active = parts[3].trimmed();
            
            // Skip empty SSIDs and duplicates
            if (ssid.isEmpty() || seenSSIDs.contains(ssid)) {
                continue;
            }
            
            seenSSIDs.insert(ssid);
            
            NetworkInfo info;
            info.ssid = ssid;
            info.signal = signalStr.toInt();
            info.security = security.isEmpty() ? "--" : security;
            info.isConnected = (active == "yes" || active == "*");
            
            networks.append(info);
        }
    }
    
    qDebug() << "Found" << networks.size() << "WiFi networks";
    return networks;
}

bool WiFiConfigurator::connectWiFi(const QString &ssid, const QString &password)
{
    qDebug() << "Connecting to WiFi:" << ssid;
    
    m_lastError.clear();
    
    QStringList args;
    args << "device" << "wifi" << "connect" << ssid;
    
    if (!password.isEmpty()) {
        args << "password" << password;
    }
    
    QString output = executeNmcli(args, 30000);
    
    if (m_lastError.isEmpty()) {
        qDebug() << "Successfully connected to" << ssid;
        return true;
    }
    
    qWarning() << "Failed to connect to" << ssid << ":" << m_lastError;
    return false;
}

bool WiFiConfigurator::disconnectWiFi()
{
    qDebug() << "Disconnecting WiFi...";
    
    m_lastError.clear();
    
    // Get current connection
    QString currentSSID = getCurrentSSID();
    if (currentSSID.isEmpty()) {
        m_lastError = "No active WiFi connection";
        return false;
    }
    
    QString output = executeNmcli(QStringList() << "connection" << "down" << currentSSID, 10000);
    
    if (m_lastError.isEmpty()) {
        qDebug() << "Successfully disconnected from" << currentSSID;
        return true;
    }
    
    qWarning() << "Failed to disconnect:" << m_lastError;
    return false;
}

QString WiFiConfigurator::getCurrentSSID()
{
    m_lastError.clear();
    
    // nmcli -t -f ACTIVE,SSID device wifi
    QString output = executeNmcli(QStringList() 
        << "-t" << "-f" << "ACTIVE,SSID" << "device" << "wifi", 5000);
    
    if (output.isEmpty()) {
        return "";
    }
    
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QStringList parts = line.split(":");
        if (parts.size() >= 2 && (parts[0] == "yes" || parts[0] == "*")) {
            return parts[1].trimmed();
        }
    }
    
    return "";
}

QString WiFiConfigurator::getCurrentIP()
{
    m_lastError.clear();
    
    // Get active WiFi connection name first
    QString ssid = getCurrentSSID();
    if (ssid.isEmpty()) {
        return "";
    }
    
    // Get IP address: nmcli -t -f IP4.ADDRESS connection show <ssid>
    QString output = executeNmcli(QStringList() 
        << "-t" << "-f" << "IP4.ADDRESS" << "connection" << "show" << ssid, 5000);
    
    if (output.isEmpty()) {
        return "";
    }
    
    // Parse IP4.ADDRESS:192.168.1.100/24
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.contains("IP4.ADDRESS")) {
            QStringList parts = line.split(":");
            if (parts.size() >= 2) {
                QString ipWithMask = parts[1].trimmed();
                // Remove subnet mask (/24)
                int slashPos = ipWithMask.indexOf('/');
                if (slashPos > 0) {
                    return ipWithMask.left(slashPos);
                }
                return ipWithMask;
            }
        }
    }
    
    return "";
}
