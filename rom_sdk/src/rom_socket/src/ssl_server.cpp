#include "ssl_server.h"

QString video_directory = "/home/buc_robot/data/upload/videos/";
QString audio_directory = "/home/buc_robot/data/upload/audio/";
QString waypoints_directory = "/home/buc_robot/data/waypoints/";

// Upload limit constants
static const int MAX_VIDEO_FILES = 5;
static const int MAX_AUDIO_FILES = 10;

// Helper: directory ထဲမှာ media file ဘယ်နှစ်ခုရှိလဲ ရေတွက်ခြင်း
static int countMediaFiles(const QString &directory, const QStringList &filters)
{
    QDir dir(directory);
    if (!dir.exists()) return 0;
    dir.setNameFilters(filters);
    return dir.entryList(QDir::Files).count();
}

// Embedded SSL Certificate (ဒီမှာ သင့် certificate content ထည့်ပါ)
const char* EMBEDDED_CERTIFICATE = R"(
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

// Embedded Private Key (ဒီမှာ သင့် private key content ထည့်ပါ)
const char* EMBEDDED_PRIVATE_KEY = R"(
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC09kVsc+U9mfSA
p4e+EWb3uWSaDTGHUWjjWvEJRsuwm5ITJdKbPQhTtCeYS2WYgEUJwaBZMJgxJUqX
FFSo2f6XZmehGQpQDiGnt7Ozhq4nknO8E7PgtDMuqFqyrWksK2UzZSHr2fAWWsIh
iGWUK1HABf0Go5zGsvEsf5Rt2N0VpJggcOAHypR2XhCTV86l+QOJUHX375/6k6ps
Gyxf8cHreuvubL0IDP/5hJCmjm19RFcYr4UzLRCsztso2uMFMAM9PmaF3xexWsWO
ZmITlBFighp7lz4FajZARlBG5JbjgA8jx+5LM0EvKLtHttplTAzp0a3yK6jLxIXs
hTBWashpAgMBAAECggEAIrP3rEytcZrMoL9/8Js1u9v5xvEwxlp+WttdzgGdnki3
QqTGNq8FoceysCoFWbF/i9duAKypAwO7wi1L7vWfNTmWNfavW5raKWe5NnxleYFE
YJUGsdjc193BRvKqcKYBylFip/Arcp4FDJkzoa6NBt4fH97QeepnBbmRpV8pRrTu
yqoN2iW1kDQNGoBD9xyCXJdeV9IDIlFWKi8qeWpqm368F62S/XJfgyAVdmEECpFc
38n2hX7k+sUMXNoj8KPiqO6v4oPOzfqCRBivzjWBS8yqG+dTYiaOSQ4z/TlUVTDa
ioEAKcM1cIYYKHG2B48CdlnhBTM4urTwZBlO+8KX4QKBgQDwNQilqaYAB+gOdXGG
LStuDsyh+adDBZqFwQZnA1SR2Lu7s84MD2+U3qVcbqK4G2XrThb0SKRhdN50JFjL
sZDjJhNlrfyosGw4GdD/0VdYXhkcummB9tE0toA1ONq9hlQI4WKgwmYOq0sDDMBV
UDRrl4umPB7YYF5f722A203CPwKBgQDA3BKBAQYdB3EC9geOjfo0AovAyAMMNOxC
lHwmy8/IzaCPwIijYmzrkDTn0xVgFajSGk3OCOVaxlG0DlDc7sHEaZhUFovg3yx4
TMNB2n2Wm4J0X4pOha1dxx2sA4BI2rQYms6t8lOmdkTa+B0rwgvbOUDUEtjfx4Pn
4dg71/b7VwKBgQC6H43Ut5BNw9KWqX/OhN97BvKeq1BkSUpDS57HYTg9Tl+hAKCu
jaNbCe29omhpGamuWzLEFCly7liUS7mWE799knpDNj5pA1LHYZGlNzNj4H262eJ4
9qOCIctT8frkEdq5itKeWCM2SJn2AgJh2KTVnXZy13DbHkjiMyZ5SvSEhwKBgGCC
wM/Fz2Vff/JXZFi2O+sjCwSiEsRdB44Z+DcB7y0xmZPWaYo5iwAm3hLU0vGOZTke
6KieUwgmDmTodRbadCTyIsSRs9YIWJyq7VtbF1Xy5EmQNgotYyB2sCaQafYLW+yk
K6FojuvSa4qYdyCarow6DnMSK21wzlWP80GfRX1pAoGAXnVnF1Y2ncu2D5YdFSGa
KJU+r14OaTUpAVLdSyhcp0gYSE2rGQuf8+Nc0SqfoehWo1tDP4Q1pZ/M/TM2WaB6
AYfQleUbAbWQNjLvpPlnxb3wecKSxxvaiccGV/u67IdCydf0ssTJvf1zb4lA7bMp
KzyBByrueDl2o25D6iWm1so=
-----END PRIVATE KEY-----
)";

SslServer::SslServer(QObject *parent) : QTcpServer(parent) 
{
    qDebug() << "Initializing SslServer...";
    
    // Initialize audio mpv player
    audioMpv_ = mpv_create();
    if (!audioMpv_) {
        qCritical() << "Failed to create audio MPV instance";
    } else {
        qDebug() << "Audio MPV instance created";
        
        mpv_set_option_string(audioMpv_, "vo", "null");
        mpv_set_option_string(audioMpv_, "video", "no");
        mpv_set_option_string(audioMpv_, "audio-display", "no");
        mpv_set_option_string(audioMpv_, "force-window", "no");
        mpv_set_option_string(audioMpv_, "ytdl", "no");
        mpv_set_option_string(audioMpv_, "terminal", "no");
        mpv_set_option_string(audioMpv_, "input-terminal", "no");
        mpv_set_option_string(audioMpv_, "volume", "70");
        
        int result = mpv_initialize(audioMpv_);
        if (result < 0) {
            qCritical() << "Failed to initialize audio MPV:" << mpv_error_string(result);
            mpv_terminate_destroy(audioMpv_);
            audioMpv_ = nullptr;
        } else {
            qDebug() << "Audio MPV player initialized successfully";
        }
    }
    
    // Initialize video mpv player
    videoMpv_ = mpv_create();
    if (!videoMpv_) {
        qCritical() << "Failed to create video MPV instance";
    } else {
        qDebug() << "Video MPV instance created";
        
        mpv_set_option_string(videoMpv_, "terminal", "no");
        mpv_set_option_string(videoMpv_, "input-terminal", "no");
        mpv_set_option_string(videoMpv_, "input-default-bindings", "no");
        mpv_set_option_string(videoMpv_, "vo", "drm,gpu,null");
        mpv_set_option_string(videoMpv_, "volume", "70");
        
        int result = mpv_initialize(videoMpv_);
        if (result < 0) {
            qCritical() << "Failed to initialize video MPV:" << mpv_error_string(result);
            mpv_terminate_destroy(videoMpv_);
            videoMpv_ = nullptr;
        } else {
            qDebug() << "Video MPV player initialized successfully";
        }
    }
    
    qDebug() << "SslServer initialization complete - Audio:" << (audioMpv_ ? "OK" : "FAILED") 
             << "Video:" << (videoMpv_ ? "OK" : "FAILED");
}

SslServer::~SslServer()
{
    // Clean up audio mpv player
    if (audioMpv_) {
        mpv_terminate_destroy(audioMpv_);
        audioMpv_ = nullptr;
        qDebug() << "Audio MPV player destroyed";
    }
    
    // Clean up video mpv player
    if (videoMpv_) {
        mpv_terminate_destroy(videoMpv_);
        videoMpv_ = nullptr;
        qDebug() << "Video MPV player destroyed";
    }
}

void SslServer::incomingConnection(qintptr socketDescriptor) 
{
    qDebug() << "=== New Incoming Connection ===";
    qDebug() << "Socket Descriptor:" << socketDescriptor;
    
    QSslSocket *socket = new QSslSocket(this);

    if (socket->setSocketDescriptor(socketDescriptor)) 
    {
        qDebug() << "Socket descriptor set successfully";
        qDebug() << "Client address:" << socket->peerAddress().toString();
        qDebug() << "Client port:" << socket->peerPort();
        
        // Embedded certificate နဲ့ key ကို load လုပ်ခြင်း
        qDebug() << "Loading SSL certificate and private key...";
        QSslCertificate certificate(QByteArray(EMBEDDED_CERTIFICATE), QSsl::Pem);
        QSslKey privateKey(QByteArray(EMBEDDED_PRIVATE_KEY), QSsl::Rsa, QSsl::Pem);
        
        if (certificate.isNull()) {
            qCritical() << "Failed to load embedded certificate!";
        } else if (privateKey.isNull()) {
            qCritical() << "Failed to load embedded private key!";
        } else {
            qDebug() << "Certificate Subject:" << certificate.subjectDisplayName();
            qDebug() << "Certificate Issuer:" << certificate.issuerDisplayName();
            socket->setLocalCertificate(certificate);
            socket->setPrivateKey(privateKey);
            qDebug() << "SSL certificate and key loaded successfully";
        }
        
        socket->setProtocol(QSsl::TlsV1_2OrLater);
        qDebug() << "SSL protocol set to TLS 1.2 or later";

        // Encryption စတင်ခြင်း
        qDebug() << "Starting server-side SSL encryption...";
        socket->startServerEncryption();
        qDebug() << "Waiting for SSL handshake...";
        
        // Add encrypted signal handler
        connect(socket, &QSslSocket::encrypted, [socket]() {
            qDebug() << "=== SSL Handshake Completed Successfully ===";
            qDebug() << "Encrypted connection established with" << socket->peerAddress().toString();
            qDebug() << "Cipher:" << socket->sessionCipher().name();
        });
        
        // Add error signal handler
        connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
                [socket](const QList<QSslError> &errors) {
            qWarning() << "=== SSL Errors on Server ===";
            for (const QSslError &error : errors) {
                qWarning() << "SSL Error:" << error.errorString();
            }
            // Ignore errors for self-signed certificate
            socket->ignoreSslErrors();
        });
        
        connect(socket, &QSslSocket::errorOccurred,
                [socket](QAbstractSocket::SocketError error) {
            qCritical() << "=== Socket Error on Server ===";
            qCritical() << "Error code:" << error;
            qCritical() << "Error string:" << socket->errorString();
        });

        connect(socket, &QSslSocket::readyRead, [this, socket]() 
        {
            qDebug() << "=== Data Ready to Read ===";
            qint64 bytesAvailable = socket->bytesAvailable();
            qDebug() << "Bytes available:" << bytesAvailable;
            
            // Get or create buffer for this socket
            QByteArray &buffer = socketBuffers[socket];
            quint32 &expectedSize = socketExpectedSizes[socket];
            
            // Append incoming data to buffer
            QByteArray newData = socket->readAll();
            qDebug() << "Read" << newData.size() << "bytes from socket";
            buffer.append(newData);
            qDebug() << "Total buffer size:" << buffer.size();
            
            // Process all complete packets
            processPackets(socket, buffer, expectedSize);
        });
        
        connect(socket, &QSslSocket::disconnected, [this, socket]() 
        {
            qDebug() << "=== Client Disconnected ===";
            qDebug() << "Client was:" << socket->peerAddress().toString();
            
            // Clean up buffers when socket disconnects
            socketBuffers.remove(socket);
            socketExpectedSizes.remove(socket);
            socket->deleteLater();
        });
    } 
    else 
    {
        delete socket;
    }
}

QString SslServer::getVideoList()
{
    QDir videoDir(video_directory);
    
    if (!videoDir.exists()) {
        return "Error: Video directory does not exist";
    }
    
    // Video file extensions
    QStringList filters;
    filters << "*.mp4" << "*.avi" << "*.mkv" << "*.mov" << "*.wmv" << "*.flv" << "*.webm";
    videoDir.setNameFilters(filters);
    
    QStringList videoFiles = videoDir.entryList(QDir::Files);
    
    if (videoFiles.isEmpty()) {
        return "No video files found";
    }
    
    return videoFiles.join(",");
}

QString SslServer::getAudioList()
{
    QDir audioDir(audio_directory);
    
    if (!audioDir.exists()) {
        return "Error: Audio directory does not exist";
    }
    
    // Audio file extensions
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.aac" << "*.ogg" << "*.m4a" << "*.wma";
    audioDir.setNameFilters(filters);
    
    QStringList audioFiles = audioDir.entryList(QDir::Files);
    
    if (audioFiles.isEmpty()) {
        return "No audio files found";
    }
    
    return audioFiles.join(",");
}

QString SslServer::llmResponse(const QString &query)
{
    // TODO: LLM integration ဒီနေရာမှာ လုပ်ရမယ်
    // ဥပမာ: OpenAI API, Ollama, သို့မဟုတ် အခြား LLM service ကို ခေါ်ရမယ်
    
    qDebug() << "LLM query received:" << query;
    
    // Placeholder response
    return "LLM response not implemented yet. Query was: " + query;
}

/**
 * Read waypoint names from a mode YAML file.
 * File path: /home/buc_robot/data/waypoints/<modeFile>.yaml
 *
 * YAML format:
 *   waypoints:
 *     - name: <string>
 *       ...
 *
 * Returns comma-separated names: "WAYPOINTS:a,b,c"
 * Returns "WAYPOINTS:" if no waypoints found or file missing.
 */
QString SslServer::getWaypointNames(const QString &modeFile)
{
    // Sanitize: prevent path traversal
    if (modeFile.contains("..") || modeFile.contains('/') || modeFile.contains('\\')) {
        qWarning() << "Invalid waypoint file name:" << modeFile;
        return "WAYPOINTS:";
    }

    QString filePath = waypoints_directory + modeFile + ".yaml";
    QFile file(filePath);

    if (!file.exists()) {
        qDebug() << "Waypoint file not found:" << filePath;
        return "WAYPOINTS:";
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open waypoint file:" << filePath;
        return "WAYPOINTS:";
    }

    // Simple line-by-line YAML parsing — extract "name:" values under "waypoints:"
    QStringList names;
    QTextStream in(&file);
    bool inWaypoints = false;

    while (!in.atEnd()) {
        QString line = in.readLine();
        QString trimmed = line.trimmed();

        if (trimmed == "waypoints:") {
            inWaypoints = true;
            continue;
        }

        if (inWaypoints && trimmed.startsWith("- name:")) {
            QString name = trimmed.mid(7).trimmed(); // Remove "- name:" prefix
            // Remove surrounding quotes if present
            if ((name.startsWith('"') && name.endsWith('"')) ||
                (name.startsWith('\'') && name.endsWith('\''))) {
                name = name.mid(1, name.length() - 2);
            }
            if (!name.isEmpty()) {
                names.append(name);
            }
        }
    }
    file.close();

    qDebug() << "Waypoint names from" << modeFile << ":" << names;
    return "WAYPOINTS:" + names.join(",");
}

void SslServer::sendResponse(QSslSocket *socket, const QString &response)
{
    if (!socket || !socket->isOpen()) {
        qWarning() << "Cannot send response: socket is not open";
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
    
    qDebug() << "Response sent:" << response.left(100) << "..."; // First 100 chars
}

bool SslServer::saveUploadedFile(const QString &directory, const QString &filename, const QByteArray &fileData)
{
    // Directory ရှိမရှိ စစ်ဆေးပြီး မရှိရင် ဖန်တီးပါမယ်
    QDir dir(directory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qCritical() << "Failed to create directory:" << directory;
            return false;
        }
        qDebug() << "Created directory:" << directory;
    }
    
    // Full file path
    QString filePath = directory + "/" + filename;
    
    // File save လုပ်ခြင်း
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    qint64 bytesWritten = file.write(fileData);
    file.close();
    
    if (bytesWritten != fileData.size()) {
        qCritical() << "File write incomplete:" << bytesWritten << "of" << fileData.size();
        return false;
    }
    
    qDebug() << "File saved successfully:" << filePath << "(" << bytesWritten << "bytes)";
    return true;
}

void SslServer::processPackets(QSslSocket *socket, QByteArray &buffer, quint32 &expectedSize)
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
        
        qDebug() << "Packet Type:" << type;

        if (type == "COMMAND") 
        {
            // Command packets: type + command string
            QByteArray data;
            stream >> data;
            QString command = QString::fromUtf8(data);
            qDebug() << "Received Command:" << command;
            
            QString response;
            if (command == "show_video") {
                response = getVideoList();
                qDebug() << "Sending video list to client";
            } else if (command == "show_audio") {
                response = getAudioList();
                qDebug() << "Sending audio list to client";
            } else if (command == "choose_music") {
                // List only mp3 files from audio directory for background music selection
                QDir audioDir(audio_directory);
                QStringList filters;
                filters << "*.mp3";
                audioDir.setNameFilters(filters);
                QStringList mp3Files = audioDir.entryList(QDir::Files, QDir::Name);
                if (mp3Files.isEmpty()) {
                    response = "MUSIC_LIST:";
                } else {
                    response = "MUSIC_LIST:" + mp3Files.join(",");
                }
                qDebug() << "Sending music list to client:" << mp3Files.size() << "files";
            } else if (command.startsWith("play_video:")) {
                QString videoName = command.mid(11); // Remove "play_video:" prefix
                qDebug() << "Play video requested:" << videoName;
                playVideo(videoName);
                response = "Playing video: " + videoName;
            } else if (command.startsWith("play_audio:")) {
                QString audioName = command.mid(11); // Remove "play_audio:" prefix
                qDebug() << "Play audio requested:" << audioName;
                playAudio(audioName);
                response = "Playing audio: " + audioName;
            } else if (command == "toggle_video") {
                qDebug() << "Toggle video play/pause requested";
                toggleVideoPlayback();
                response = "Video playback toggled";
            } else if (command == "toggle_audio") {
                qDebug() << "Toggle audio play/pause requested";
                toggleAudioPlayback();
                response = "Audio playback toggled";
            } else if (command.startsWith("set_audio_volume:")) {
                int volume = command.mid(17).toInt(); // Remove "set_audio_volume:" prefix
                qDebug() << "Set audio volume requested:" << volume;
                setAudioVolume(volume);
                response = "Audio volume set to: " + QString::number(volume);
            } else if (command.startsWith("set_video_volume:")) {
                int volume = command.mid(17).toInt(); // Remove "set_video_volume:" prefix
                qDebug() << "Set video volume requested:" << volume;
                setVideoVolume(volume);
                response = "Video volume set to: " + QString::number(volume);
            } else if (command == "get_audio_volume") {
                int volume = getAudioVolume();
                qDebug() << "Get audio volume requested";
                response = "Audio volume: " + QString::number(volume);
            } else if (command == "get_video_volume") {
                int volume = getVideoVolume();
                qDebug() << "Get video volume requested";
                response = "Video volume: " + QString::number(volume);
            } else if (command == "llm_response") {
                response = llmResponse(command);
                qDebug() << "Sending LLM response to client";
            } 
            // ===== Settings Commands =====
            else if (command == "system_ready") {
                // Android app checks if server is alive
                response = "OK";
                qDebug() << "System ready check — OK";
            } else if (command == "get_robot_name") {
                // Read ROM_ROBOT_NAMESPACE from .rom_environment.sh
                QString name = readRomEnvironment("ROM_ROBOT_NAMESPACE");
                response = name.isEmpty() ? "ROBOT_NAME:unknown" : "ROBOT_NAME:" + name;
                qDebug() << "Robot name requested:" << response;
            } else if (command == "list_settings") {
                // List all available settings files
                response = settingsManager_.listSettings();
                qDebug() << "Sending settings list to client";
            } else if (command.startsWith("get_settings:")) {
                // Get all key-value pairs from a settings file
                // Format: "get_settings:<settings_name>"
                QString settingsName = command.mid(13); // Remove "get_settings:" prefix
                qDebug() << "Get settings requested:" << settingsName;
                response = settingsManager_.readAll(settingsName);
            } else if (command.startsWith("get_setting_value:")) {
                // Get a single value from a settings file
                // Format: "get_setting_value:<settings_name>:<key>"
                QString params = command.mid(18); // Remove "get_setting_value:" prefix
                int sepIdx = params.indexOf(':');
                if (sepIdx > 0) {
                    QString settingsName = params.left(sepIdx);
                    QString key = params.mid(sepIdx + 1);
                    qDebug() << "Get setting value:" << settingsName << "/" << key;
                    response = settingsManager_.readValue(settingsName, key);
                } else {
                    response = "ERROR:Invalid format. Use get_setting_value:<settings_name>:<key>";
                }
            } else if (command.startsWith("set_setting:")) {
                // Set a single key-value pair in a settings file
                // Format: "set_setting:<settings_name>:<key>:<value>"
                QString params = command.mid(12); // Remove "set_setting:" prefix
                int firstColon = params.indexOf(':');
                int secondColon = params.indexOf(':', firstColon + 1);
                if (firstColon > 0 && secondColon > firstColon) {
                    QString settingsName = params.left(firstColon);
                    QString key = params.mid(firstColon + 1, secondColon - firstColon - 1);
                    QString value = params.mid(secondColon + 1);
                    qDebug() << "Set setting:" << settingsName << "/" << key << "=" << value;
                    bool success = settingsManager_.writeValue(settingsName, key, value);
                    // robot_name ပြောင်းရင် .rom_environment.sh ကိုပါ update
                    if (success && settingsName == "basic_settings" && key == "robot_name") {
                        updateRomEnvironment("ROM_ROBOT_NAMESPACE", value);
                    }
                    // YAML ပြင်ပြီးရင် သက်ဆိုင်ရာ BT XML ကိုပါ update
                    if (success) {
                        btUpdater_.updateBehaviorTree(settingsName, key, value);
                    }
                    response = success ? "OK:Setting updated: " + key + "=" + value
                                       : "ERROR:Failed to update: " + key;
                } else {
                    response = "ERROR:Invalid format. Use set_setting:<settings_name>:<key>:<value>";
                }
            } else if (command.startsWith("set_settings:")) {
                // Set multiple key-value pairs at once
                // Format: "set_settings:<settings_name>:<key1>:<value1>\n<key2>:<value2>"
                QString params = command.mid(13); // Remove "set_settings:" prefix
                int firstColon = params.indexOf(':');
                if (firstColon > 0) {
                    QString settingsName = params.left(firstColon);
                    QString keyValues = params.mid(firstColon + 1);
                    qDebug() << "Set multiple settings:" << settingsName;
                    bool success = settingsManager_.writeMultiple(settingsName, keyValues);
                    // bulk update ထဲမှာ robot_name ပါရင် .rom_environment.sh ပါ update
                    if (success && settingsName == "basic_settings") {
                        const QStringList lines = keyValues.split('\n', Qt::SkipEmptyParts);
                        for (const QString &line : lines) {
                            int sep = line.indexOf(':');
                            if (sep > 0 && line.left(sep).trimmed() == "robot_name") {
                                updateRomEnvironment("ROM_ROBOT_NAMESPACE", line.mid(sep + 1).trimmed());
                                break;
                            }
                        }
                    }
                    // YAML ပြင်ပြီးရင် သက်ဆိုင်ရာ BT XML တွေကိုပါ update
                    if (success) {
                        const QStringList lines = keyValues.split('\n', Qt::SkipEmptyParts);
                        for (const QString &line : lines) {
                            int sep = line.indexOf(':');
                            if (sep > 0) {
                                QString k = line.left(sep).trimmed();
                                QString v = line.mid(sep + 1).trimmed();
                                btUpdater_.updateBehaviorTree(settingsName, k, v);
                            }
                        }
                    }
                    response = success ? "OK:Settings updated for " + settingsName
                                       : "ERROR:Failed to update settings for " + settingsName;
                } else {
                    response = "ERROR:Invalid format. Use set_settings:<settings_name>:<key1>:<value1>\\n<key2>:<value2>";
                }
            }
            // ===== Waypoints Command =====
            else if (command.startsWith("get_waypoints:")) {
                // Get waypoint names from a mode YAML file
                // Format: "get_waypoints:<mode_file>"  e.g. "get_waypoints:service_mode"
                QString modeFile = command.mid(14); // Remove "get_waypoints:" prefix
                qDebug() << "Get waypoints requested for:" << modeFile;
                response = getWaypointNames(modeFile);
            }
            else {
                response = "Unknown command: " + command;
                qDebug() << "Unknown command received";
            }
            
            sendResponse(socket, response);
            
        } 
        else if (type == "UPLOAD_VIDEO") 
        {
            // Video upload packets: type + filename + file data
            QString filename;
            QByteArray fileData;
            stream >> filename >> fileData;
            
            qDebug() << "Uploading video:" << filename << "Size:" << fileData.size();
            
            // Upload limit စစ်ဆေးခြင်း — video အများဆုံး MAX_VIDEO_FILES ခုသာ ခွင့်ပြု
            QStringList videoFilters;
            videoFilters << "*.mp4" << "*.avi" << "*.mkv" << "*.mov" << "*.wmv" << "*.flv" << "*.webm";
            int currentVideoCount = countMediaFiles(video_directory, videoFilters);
            
            if (currentVideoCount >= MAX_VIDEO_FILES) {
                QString response = QString("Upload rejected: Video limit reached (%1/%1). Delete existing videos first.")
                    .arg(MAX_VIDEO_FILES);
                qWarning() << response;
                sendResponse(socket, response);
            } else {
                bool success = saveUploadedFile(video_directory, filename, fileData);
                QString response = success ? 
                    QString("Video uploaded successfully: %1 (%2/%3)").arg(filename).arg(currentVideoCount + 1).arg(MAX_VIDEO_FILES) : 
                    "Failed to upload video: " + filename;
                sendResponse(socket, response);
            }
            
        } 
        else if (type == "UPLOAD_AUDIO") 
        {
            // Audio upload packets: type + filename + file data
            QString filename;
            QByteArray fileData;
            stream >> filename >> fileData;
            
            qDebug() << "Uploading audio:" << filename << "Size:" << fileData.size();
            
            // Upload limit စစ်ဆေးခြင်း — audio အများဆုံး MAX_AUDIO_FILES ခုသာ ခွင့်ပြု
            QStringList audioFilters;
            audioFilters << "*.mp3" << "*.wav" << "*.flac" << "*.aac" << "*.ogg" << "*.m4a" << "*.wma";
            int currentAudioCount = countMediaFiles(audio_directory, audioFilters);
            
            if (currentAudioCount >= MAX_AUDIO_FILES) {
                QString response = QString("Upload rejected: Audio limit reached (%1/%1). Delete existing audio files first.")
                    .arg(MAX_AUDIO_FILES);
                qWarning() << response;
                sendResponse(socket, response);
            } else {
                bool success = saveUploadedFile(audio_directory, filename, fileData);
                QString response = success ? 
                    QString("Audio uploaded successfully: %1 (%2/%3)").arg(filename).arg(currentAudioCount + 1).arg(MAX_AUDIO_FILES) : 
                    "Failed to upload audio: " + filename;
                sendResponse(socket, response);
            }
            
        } else {
            qDebug() << "Unknown packet type:" << type;
        }
    }
}

void SslServer::playAudio(const QString &filename)
{
    if (!audioMpv_) {
        qWarning() << "Audio MPV player not initialized";
        return;
    }
    
    QString filePath = audio_directory + filename;
    QFileInfo fileInfo(filePath);
    
    if (!fileInfo.exists()) {
        qWarning() << "Audio file does not exist:" << filePath;
        return;
    }
    
    currentAudioFile_ = filename;
    
    // Store file path in a persistent QByteArray
    QByteArray pathData = filePath.toUtf8();
    const char* cmd[] = {"loadfile", pathData.constData(), NULL};
    
    int result = mpv_command_async(audioMpv_, 0, cmd);
    if (result < 0) {
        qCritical() << "Failed to play audio:" << mpv_error_string(result);
    } else {
        qDebug() << "Playing audio file:" << filePath;
    }
}

void SslServer::playVideo(const QString &filename)
{
    if (!videoMpv_) {
        qWarning() << "Video MPV player not initialized";
        return;
    }
    
    QString filePath = video_directory + filename;
    QFileInfo fileInfo(filePath);
    
    if (!fileInfo.exists()) {
        qWarning() << "Video file does not exist:" << filePath;
        return;
    }
    
    currentVideoFile_ = filename;
    
    // Store file path in a persistent QByteArray
    QByteArray pathData = filePath.toUtf8();
    const char* cmd[] = {"loadfile", pathData.constData(), NULL};
    
    int result = mpv_command_async(videoMpv_, 0, cmd);
    if (result < 0) {
        qCritical() << "Failed to play video:" << mpv_error_string(result);
    } else {
        qDebug() << "Playing video file:" << filePath;
    }
}

void SslServer::toggleAudioPlayback()
{
    if (!audioMpv_) return;
    
    // Get pause state
    int pause = 0;
    mpv_get_property(audioMpv_, "pause", MPV_FORMAT_FLAG, &pause);
    
    // Toggle pause
    pause = !pause;
    mpv_set_property(audioMpv_, "pause", MPV_FORMAT_FLAG, &pause);
    
    qDebug() << (pause ? "Audio paused" : "Audio resumed");
}

void SslServer::toggleVideoPlayback()
{
    if (!videoMpv_) return;
    
    // Get pause state
    int pause = 0;
    mpv_get_property(videoMpv_, "pause", MPV_FORMAT_FLAG, &pause);
    
    // Toggle pause
    pause = !pause;
    mpv_set_property(videoMpv_, "pause", MPV_FORMAT_FLAG, &pause);
    
    qDebug() << (pause ? "Video paused" : "Video resumed");
}

void SslServer::setAudioVolume(int volume)
{
    if (!audioMpv_) return;
    
    // Clamp volume between 0 and 100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    int64_t vol = volume;
    mpv_set_property(audioMpv_, "volume", MPV_FORMAT_INT64, &vol);
    
    qDebug() << "Audio volume set to:" << volume;
}

void SslServer::setVideoVolume(int volume)
{
    if (!videoMpv_) return;
    
    // Clamp volume between 0 and 100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    int64_t vol = volume;
    mpv_set_property(videoMpv_, "volume", MPV_FORMAT_INT64, &vol);
    
    qDebug() << "Video volume set to:" << volume;
}

int SslServer::getAudioVolume()
{
    if (!audioMpv_) return 0;
    
    int64_t volume = 0;
    mpv_get_property(audioMpv_, "volume", MPV_FORMAT_INT64, &volume);
    
    return static_cast<int>(volume);
}

int SslServer::getVideoVolume()
{
    if (!videoMpv_) return 0;
    
    int64_t volume = 0;
    mpv_get_property(videoMpv_, "volume", MPV_FORMAT_INT64, &volume);
    
    return static_cast<int>(volume);
}

// .rom_environment.sh ထဲက export variable value ကို ဖတ်ခြင်း
QString SslServer::readRomEnvironment(const QString &key)
{
    const QString envPath = "/home/buc_robot/data/systemd/.rom_environment.sh";
    QFile file(envPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open .rom_environment.sh for reading:" << file.errorString();
        return QString();
    }

    const QString prefix = "export " + key + "=";
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Skip commented-out lines
        if (line.startsWith('#')) continue;
        if (line.startsWith(prefix)) {
            file.close();
            QString value = line.mid(prefix.length()).trimmed();
            // Remove inline comments
            int commentIdx = value.indexOf(" #");
            if (commentIdx >= 0) value = value.left(commentIdx).trimmed();
            qDebug() << "Read" << key << "=" << value << "from .rom_environment.sh";
            return value;
        }
    }
    file.close();

    qWarning() << "Key" << key << "not found in .rom_environment.sh";
    return QString();
}

// .rom_environment.sh ထဲက export variable ကို update လုပ်ခြင်း
// e.g. updateRomEnvironment("ROM_ROBOT_NAMESPACE", "my_robot")
//      → "export ROM_ROBOT_NAMESPACE=my_robot"
bool SslServer::updateRomEnvironment(const QString &key, const QString &value)
{
    const QString envPath = "/home/buc_robot/data/systemd/.rom_environment.sh";
    QFile file(envPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open .rom_environment.sh for reading:" << file.errorString();
        return false;
    }

    // File ကို line-by-line ဖတ်ပြီး target line ကို replace
    QStringList lines;
    QTextStream in(&file);
    bool found = false;
    const QString prefix = "export " + key + "=";

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().startsWith(prefix)) {
            lines.append(prefix + value);
            found = true;
            qDebug() << "Updated" << key << "to" << value << "in .rom_environment.sh";
        } else {
            lines.append(line);
        }
    }
    file.close();

    if (!found) {
        qWarning() << "Key" << key << "not found in .rom_environment.sh";
        return false;
    }

    // Write back
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "Failed to open .rom_environment.sh for writing:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    for (int i = 0; i < lines.size(); ++i) {
        out << lines[i];
        if (i < lines.size() - 1) out << "\n";
    }
    file.close();

    // Force reboot to apply new ROM_ROBOT_NAMESPACE
    QProcess::execute("sudo", QStringList() << "reboot" << "-f");

    return true;
}
