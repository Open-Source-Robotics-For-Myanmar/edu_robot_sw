#ifndef WIFI_CONFIGURATOR_H
#define WIFI_CONFIGURATOR_H

#include <QString>
#include <QStringList>
#include <QObject>

/**
 * @brief WiFi configuration utility class using NetworkManager (nmcli)
 * 
 * Provides methods to search, connect, disconnect, and query WiFi networks
 * on Linux systems using nmcli command-line tool.
 */
class WiFiConfigurator : public QObject
{
    Q_OBJECT

public:
    struct NetworkInfo {
        QString ssid;
        int signal;      // Signal strength (0-100)
        QString security; // Security type (WPA2, WEP, --, etc.)
        bool isConnected;
    };

    explicit WiFiConfigurator(QObject *parent = nullptr);
    ~WiFiConfigurator();

    /**
     * @brief Search for available WiFi networks
     * @return List of available networks with signal and security info
     */
    QList<NetworkInfo> searchWiFi();

    /**
     * @brief Connect to a WiFi network
     * @param ssid Network SSID to connect to
     * @param password Network password (empty for open networks)
     * @return true if connection successful, false otherwise
     */
    bool connectWiFi(const QString &ssid, const QString &password = "");

    /**
     * @brief Disconnect from current WiFi network
     * @return true if disconnection successful, false otherwise
     */
    bool disconnectWiFi();

    /**
     * @brief Get currently connected WiFi SSID
     * @return SSID of connected network, empty string if not connected
     */
    QString getCurrentSSID();

    /**
     * @brief Get IP address of current WiFi connection
     * @return IP address string, empty if not connected
     */
    QString getCurrentIP();

    /**
     * @brief Get last error message
     * @return Error message from last operation
     */
    QString getLastError() const { return m_lastError; }

private:
    QString m_lastError;

    /**
     * @brief Execute nmcli command and return output
     * @param args Command arguments
     * @param timeoutMs Timeout in milliseconds
     * @return Command output, empty string on failure
     */
    QString executeNmcli(const QStringList &args, int timeoutMs = 10000);
};

#endif // WIFI_CONFIGURATOR_H
