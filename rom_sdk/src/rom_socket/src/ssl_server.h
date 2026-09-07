#include <QTcpServer>
#include <QSslSocket>
#include <QSslKey>
#include <QSslCertificate>
#include <QSslCipher>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include <QMap>
#include <QProcess>
#include <mpv/client.h>
#include "settings_manager.h"
#include "behavior_tree_updater.h"

class SslServer : public QTcpServer 
{
    Q_OBJECT
public:
    explicit SslServer(QObject *parent = nullptr);
    ~SslServer();
protected:
    void incomingConnection(qintptr socketDescriptor) override; // Connection လက်ခံဖို့ override လုပ်ခြင်း

private:
    QString getVideoList();
    QString getAudioList();
    QString llmResponse(const QString &query);
    QString getWaypointNames(const QString &modeFile);
    void sendResponse(QSslSocket *socket, const QString &response);
    bool saveUploadedFile(const QString &directory, const QString &filename, const QByteArray &fileData);
    void processPackets(QSslSocket *socket, QByteArray &buffer, quint32 &expectedSize);
    
    // Media playback
    void playAudio(const QString &filename);
    void playVideo(const QString &filename);
    void toggleAudioPlayback();
    void toggleVideoPlayback();
    void setAudioVolume(int volume);
    void setVideoVolume(int volume);
    int getAudioVolume();
    int getVideoVolume();
    
    // Track buffers per socket
    QMap<QSslSocket*, QByteArray> socketBuffers;
    QMap<QSslSocket*, quint32> socketExpectedSizes;
    
    // MPV players
    mpv_handle* audioMpv_ = nullptr;
    mpv_handle* videoMpv_ = nullptr;
    QString currentAudioFile_;
    QString currentVideoFile_;
    
    // Read/Update ROM environment variables in .rom_environment.sh
    QString readRomEnvironment(const QString &key);
    bool updateRomEnvironment(const QString &key, const QString &value);

    // Settings manager for YAML read/write
    SettingsManager settingsManager_;

    // BT XML updater — syncs YAML changes to behavior tree files
    BehaviorTreeUpdater btUpdater_;
};