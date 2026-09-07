#include "ssl_client.h"
#include <QDebug>
#include <QFileInfo>
#include <QSslConfiguration>

// Embedded CA Certificate (optional - server ရဲ့ CA certificate)
const char* EMBEDDED_CA_CERTIFICATE = R"(
-----BEGIN CERTIFICATE-----
MIIEBTCCAu2gAwIBAgIULI+4AvkXolMpumQu5G4/FQK4+oQwDQYJKoZIhvcNAQEL
BQAwgZExCzAJBgNVBAYTAk1NMQwwCgYDVQQIDANZR04xDDAKBgNVBAcMA1lHTjEV
MBMGA1UECgwMUk9NLVJvYm90aWNzMREwDwYDVQQLDAhSb2JvdGljczERMA8GA1UE
AwwIR2hvc3RNYW4xKTAnBgkqhkiG9w0BCQEWGnNlcnZlcjAxLnBzYTE5ODFAZ21h
aWwuY29tMB4XDTI2MDEwMjA4MTUwMFoXDTI3MDEwMjA4MTUwMFowgZExCzAJBgNV
BAYTAk1NMQwwCgYDVQQIDANZR04xDDAKBgNVBAcMA1lHTjEVMBMGA1UECgwMUk9N
LVJvYm90aWNzMREwDwYDVQQLDAhSb2JvdGljczERMA8GA1UEAwwIR2hvc3RNYW4x
KTAnBgkqhkiG9w0BCQEWGnNlcnZlcjAxLnBzYTE5ODFAZ21haWwuY29tMIIBIjAN
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtPZFbHPlPZn0gKeHvhFm97lkmg0x
h1Fo41rxCUbLsJuSEyXSmz0IU7QnmEtlmIBFCcGgWTCYMSVKlxRUqNn+l2ZnoRkK
UA4hp7ezs4auJ5JzvBOz4LQzLqhasq1pLCtlM2Uh69nwFlrCIYhllCtRwAX9BqOc
xrLxLH+UbdjdFaSYIHDgB8qUdl4Qk1fOpfkDiVB19++f+pOqbBssX/HB63rr7my9
CAz/+YSQpo5tfURXGK+FMy0QrM7bKNrjBTADPT5mhd8XsVrFjmZiE5QRYoIae5c+
BWo2QEZQRuSW44API8fuSzNBLyi7R7baZUwM6dGt8iuoy8SF7IUwVmrIaQIDAQAB
o1MwUTAdBgNVHQ4EFgQURssjEMz0z6C/SzzLqlndlkrGJjowHwYDVR0jBBgwFoAU
RssjEMz0z6C/SzzLqlndlkrGJjowDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B
AQsFAAOCAQEADhkoMGDiYHjfnLOMw27qDHc7kx0rvaOPoGyWkjNHVXV4RCoBa7gZ
/+7WTSIEiNT3t/WlSr5bSoFl7BiU0qMMC9K+eus5zeB7a8JBvFd/1gGZMys8selh
DqgqOYyOiQxEOrOqUxzV9fpC7m0NePcGL9GkETm/Y6ACbk6C8bg7WKd/wh2TLaKS
wyo0/IKYDUj2xfoqxNKPREGt/OwGn82EfxC2MjHwxeoGSNP2qBY34UrQJz1wW7q+
ThYTlH/6b/hvHkVxhRPz4Yu/0o7F60s7df738ZZN7kiIPAvJquyH2rp8s6tyexRW
bzvmOOpVNw+tclNNJnmTKJafuiu6o8ZfdQ==
-----END CERTIFICATE-----
)";

SslClient::SslClient(QObject *parent) 
    : QObject(parent)
    , socket(new QSslSocket(this))
    , ignoreSslErrors(false)
    , expectedPacketSize(0)
{
    // Signal connections
    connect(socket, &QSslSocket::connected, this, &SslClient::onConnected);
    connect(socket, &QSslSocket::encrypted, this, &SslClient::onConnected); // Wait for encryption
    connect(socket, &QSslSocket::disconnected, this, &SslClient::onDisconnected);
    connect(socket, &QSslSocket::readyRead, this, &SslClient::onReadyRead);
    connect(socket, &QSslSocket::sslErrors, this, &SslClient::onSslErrors);
    connect(socket, &QSslSocket::errorOccurred, this, &SslClient::onErrorOccurred);
}

SslClient::~SslClient()
{
    if (socket->isOpen()) {
        socket->close();
    }
}

// ==================== Connection Methods ====================

void SslClient::connectToServer(const QString &host, quint16 port)
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        qWarning() << "Already connected to server";
        return;
    }

    qDebug() << "Connecting to" << host << ":" << port;
    
    // SSL configuration
    socket->setProtocol(QSsl::TlsV1_2OrLater);
    
    // Connect and start encryption
    socket->connectToHostEncrypted(host, port);
}

void SslClient::disconnectFromServer()
{
    if (socket->isOpen()) {
        socket->disconnectFromHost();
    }
}

bool SslClient::isConnected() const
{
    return socket->state() == QAbstractSocket::ConnectedState && socket->isEncrypted();
}

// ==================== API Methods - Commands ====================

void SslClient::sendShowVideoCommand()
{
    sendPacket("COMMAND", QString("show_video").toUtf8());
    qDebug() << "Sent: show_video command";
}

void SslClient::sendShowAudioCommand()
{
    sendPacket("COMMAND", QString("show_audio").toUtf8());
    qDebug() << "Sent: show_audio command";
}

void SslClient::sendLlmResponseCommand(const QString &query)
{
    sendPacket("COMMAND", query.toUtf8());
    qDebug() << "Sent: llm_response command with query:" << query;
}

void SslClient::sendCustomCommand(const QString &command)
{
    sendPacket("COMMAND", command.toUtf8());
    qDebug() << "Sent custom command:" << command;
}

// ==================== API Methods - File Upload ====================

void SslClient::uploadVideoFile(const QString &filePath)
{
    bool success = false;
    QByteArray fileData = readFileData(filePath, success);
    
    if (!success) {
        emit uploadFinished(false, filePath);
        return;
    }

    QFileInfo fileInfo(filePath);
    QString filename = fileInfo.fileName();
    
    qDebug() << "Uploading video:" << filename << "Size:" << fileData.size();
    sendFilePacket("UPLOAD_VIDEO", filename, fileData);
    
    emit uploadProgress(fileData.size(), fileData.size());
}

void SslClient::uploadAudioFile(const QString &filePath)
{
    bool success = false;
    QByteArray fileData = readFileData(filePath, success);
    
    if (!success) {
        emit uploadFinished(false, filePath);
        return;
    }

    QFileInfo fileInfo(filePath);
    QString filename = fileInfo.fileName();
    
    qDebug() << "Uploading audio:" << filename << "Size:" << fileData.size();
    sendFilePacket("UPLOAD_AUDIO", filename, fileData);
    
    emit uploadProgress(fileData.size(), fileData.size());
}

// ==================== Configuration ====================

void SslClient::setCertificatePath(const QString &certPath)
{
    certificatePath = certPath;
    
    // Load from file or use embedded certificate
    if (!certPath.isEmpty()) {
        QList<QSslCertificate> certs = QSslCertificate::fromPath(certPath);
        if (!certs.isEmpty()) {
            QSslConfiguration sslConfig = socket->sslConfiguration();
            sslConfig.addCaCertificates(certs);
            socket->setSslConfiguration(sslConfig);
            qDebug() << "Loaded certificate from:" << certPath;
        }
    } else {
        // Use embedded CA certificate
        QSslCertificate caCert(QByteArray(EMBEDDED_CA_CERTIFICATE), QSsl::Pem);
        if (!caCert.isNull()) {
            QSslConfiguration sslConfig = socket->sslConfiguration();
            sslConfig.addCaCertificate(caCert);
            socket->setSslConfiguration(sslConfig);
            qDebug() << "Loaded embedded CA certificate";
        }
    }
}

void SslClient::setIgnoreSslErrors(bool ignore)
{
    ignoreSslErrors = ignore;
}

// ==================== Private Slots ====================

void SslClient::onConnected()
{
    qDebug() << "Connected to server";
    emit connected();
}

void SslClient::onDisconnected()
{
    qDebug() << "Disconnected from server";
    emit disconnected();
}

void SslClient::onReadyRead()
{
    // Append incoming data to buffer
    receiveBuffer.append(socket->readAll());
    
    // Process all complete packets
    processIncomingPackets();
}

void SslClient::processIncomingPackets()
{
    while (true) {
        // If we don't know the packet size yet, try to read it
        if (expectedPacketSize == 0) {
            if (receiveBuffer.size() < (int)sizeof(quint32)) {
                return; // Need more data for size header
            }
            
            // Read packet size from buffer
            QDataStream sizeStream(receiveBuffer);
            sizeStream.setVersion(QDataStream::Qt_6_0);
            sizeStream >> expectedPacketSize;
            
            // Remove size header from buffer
            receiveBuffer.remove(0, sizeof(quint32));
        }
        
        // Check if we have the complete packet
        if (receiveBuffer.size() < (int)expectedPacketSize) {
            return; // Need more data for complete packet
        }
        
        // Extract one complete packet
        QByteArray packet = receiveBuffer.left(expectedPacketSize);
        receiveBuffer.remove(0, expectedPacketSize);
        expectedPacketSize = 0; // Reset for next packet
        
        // Parse the packet
        QDataStream stream(&packet, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_0);
        
        QString type;
        stream >> type;

        if (type == "RESPONSE") {
            QByteArray data;
            stream >> data;
            QString response = QString::fromUtf8(data);
            
            qDebug().noquote() << "Received response:" << response;
            emit responseReceived(response);
            
            // Check if it's an upload response
            if (response.contains("uploaded successfully")) {
                emit uploadFinished(true, response);
            } else if (response.contains("Failed to upload")) {
                emit uploadFinished(false, response);
            }
        } else {
            qWarning() << "Unknown packet type:" << type;
        }
    }
}

void SslClient::onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        qWarning() << "SSL Error:" << error.errorString();
        emit sslError(error.errorString());
    }

    if (ignoreSslErrors) {
        qDebug() << "Ignoring SSL errors (as configured)";
        socket->ignoreSslErrors();
    }
}

void SslClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    QString errorString = socket->errorString();
    qCritical() << "Socket error:" << socketError << "-" << errorString;
    emit connectionError(errorString);
}

// ==================== Helper Methods ====================

void SslClient::sendPacket(const QString &type, const QByteArray &data)
{
    if (!isConnected()) {
        qWarning() << "Cannot send packet: not connected";
        return;
    }

    // Build packet in memory first
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << type << data;
    
    // Send packet size first, then packet data
    QDataStream out(socket);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint32(packet.size());
    socket->write(packet);
    socket->flush();
}

void SslClient::sendFilePacket(const QString &type, const QString &filename, const QByteArray &fileData)
{
    if (!isConnected()) {
        qWarning() << "Cannot send file: not connected";
        return;
    }

    // Build packet in memory first
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << type << filename << fileData;
    
    // Send packet size first, then packet data
    QDataStream out(socket);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint32(packet.size());
    socket->write(packet);
    socket->flush();
}

QByteArray SslClient::readFileData(const QString &filePath, bool &success)
{
    QFile file(filePath);
    
    if (!file.exists()) {
        qCritical() << "File does not exist:" << filePath;
        success = false;
        return QByteArray();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open file:" << filePath;
        success = false;
        return QByteArray();
    }

    QByteArray data = file.readAll();
    file.close();

    success = true;
    return data;
}
