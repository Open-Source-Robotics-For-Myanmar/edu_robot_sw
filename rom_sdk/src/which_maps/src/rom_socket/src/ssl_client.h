#ifndef SSL_CLIENT_H
#define SSL_CLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QDataStream>

class SslClient : public QObject
{
    Q_OBJECT

public:
    explicit SslClient(QObject *parent = nullptr);
    ~SslClient();

    // Connection methods
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // API Methods - Commands
    void sendShowVideoCommand();
    void sendShowAudioCommand();
    void sendLlmResponseCommand(const QString &query);
    void sendCustomCommand(const QString &command);

    // API Methods - File Upload
    void uploadVideoFile(const QString &filePath);
    void uploadAudioFile(const QString &filePath);

    // Configuration
    void setCertificatePath(const QString &certPath);
    void setIgnoreSslErrors(bool ignore);

signals:
    // Connection signals
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void sslError(const QString &error);

    // Response signals
    void responseReceived(const QString &response);
    
    // Progress signals
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void uploadFinished(bool success, const QString &filename);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSslErrors(const QList<QSslError> &errors);
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QSslSocket *socket;
    QString certificatePath;
    bool ignoreSslErrors;
    
    // Buffering for packet reassembly
    QByteArray receiveBuffer;
    quint32 expectedPacketSize;

    // Helper methods
    void sendPacket(const QString &type, const QByteArray &data);
    void sendFilePacket(const QString &type, const QString &filename, const QByteArray &fileData);
    QByteArray readFileData(const QString &filePath, bool &success);
    void processIncomingPackets();
};

#endif // SSL_CLIENT_H
