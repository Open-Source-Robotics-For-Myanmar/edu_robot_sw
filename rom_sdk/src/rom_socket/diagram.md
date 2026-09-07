# ROM Socket - Architecture & Flow Diagrams

## 1. Overall System Architecture

```mermaid
graph TB
    subgraph "Remote Clients"
        GUI["Qt GUI App<br/>(SslClient)"]
        CTRL["Control App<br/>(WiFi Config)"]
    end

    subgraph "rom_socket Process (main.cpp)"
        QApp["QCoreApplication Event Loop"]
        SSL["SslServer<br/>0.0.0.0:8765<br/>(SSL/TLS Encrypted)"]
        WIFI["WiFiTcpServer<br/>10.0.0.100:7358<br/>(Plain TCP)"]
    end

    subgraph "System Dependencies"
        MPV_A["libmpv<br/>(Audio Player)"]
        MPV_V["libmpv<br/>(Video Player)"]
        NMCLI["nmcli<br/>(NetworkManager CLI)"]
    end

    subgraph "File System"
        VID["/home/buc_robot/data/upload/videos/<br/>(max 5 files)"]
        AUD["/home/buc_robot/data/upload/audio/<br/>(max 10 files)"]
    end

    GUI -->|"SSL/TLS :8765"| SSL
    CTRL -->|"TCP :7358"| WIFI

    QApp --> SSL
    QApp --> WIFI

    SSL --> MPV_A
    SSL --> MPV_V
    SSL --> VID
    SSL --> AUD

    WIFI --> NMCLI
```

---

## 2. Application Startup Flow

```mermaid
flowchart TD
    A["main() Entry Point"] --> B["setlocale(LC_NUMERIC, 'C')<br/>MPV compatibility"]
    B --> C["QCoreApplication app(argc, argv)"]
    C --> D["Create SslServer<br/>listen 0.0.0.0:8765"]
    D --> E{"SSL Server<br/>started?"}
    E -->|Yes| F["Create WiFiTcpServer<br/>listen 10.0.0.100:7358"]
    E -->|No| X1["Error: SSL Server failed to start"]
    F --> G{"WiFi Server<br/>started?"}
    G -->|Yes| H["app.exec()<br/>Event Loop Running"]
    G -->|No| X2["Error: WiFi Server failed to start"]
    H --> I["Shutdown & Cleanup"]
```

---

## 3. SslServer - Class Structure & Functions

```mermaid
classDiagram
    class SslServer {
        -mpv_handle* audioMpv_
        -mpv_handle* videoMpv_
        -QString currentAudioFile_
        -QString currentVideoFile_
        -QMap~QSslSocket*, QByteArray~ socketBuffers
        -QMap~QSslSocket*, quint32~ socketExpectedSizes
        +SslServer(QObject* parent)
        #incomingConnection(qintptr socketDescriptor)
        -processPackets(QSslSocket* socket)
        -playAudio(QString filename)
        -playVideo(QString filename)
        -toggleAudioPlayback()
        -toggleVideoPlayback()
        -setAudioVolume(int volume)
        -setVideoVolume(int volume)
        -getAudioVolume() int
        -getVideoVolume() int
        -getVideoList() QString
        -getAudioList() QString
        -saveUploadedFile(QString filename, QByteArray data, QString subDir)
        -sendResponse(QSslSocket* socket, QString response)
        -llmResponse() QString
    }
    SslServer --|> QTcpServer

    class SslClient {
        -QSslSocket* socket_
        -QByteArray receiveBuffer_
        -quint32 expectedSize_
        +connectToServer(QString host, quint16 port)
        +disconnectFromServer()
        +isConnected() bool
        +sendShowVideoCommand()
        +sendShowAudioCommand()
        +uploadVideoFile(QString filePath)
        +uploadAudioFile(QString filePath)
        +sendCustomCommand(QString command)
        -sendPacket(QString type, QString data)
        -sendFilePacket(QString type, QString filename, QByteArray fileData)
        -processIncomingPackets()
        ~connected()
        ~disconnected()
        ~connectionError(QString)
        ~sslError(QString)
        ~responseReceived(QString)
        ~uploadProgress(qint64, qint64)
        ~uploadFinished(bool, QString)
    }
    SslClient --|> QObject
```

---

## 4. WiFi Server - Class Structure

```mermaid
classDiagram
    class WiFiTcpServer {
        -QMap~QTcpSocket*, QByteArray~ socketBuffers_
        -QMap~QTcpSocket*, quint32~ expectedSizes_
        -WiFiConfigurator* wifiConfig_
        #incomingConnection(qintptr socketDescriptor)
        -onReadyRead()
        -onDisconnected()
        -processPackets(QTcpSocket* socket)
        -handleSearchWiFi(QTcpSocket* socket)
        -handleConnectWiFi(QTcpSocket* socket, QString ssid, QString pwd)
        -handleDisconnectWiFi(QTcpSocket* socket)
        -handleCurrentWiFi(QTcpSocket* socket)
        -sendResponse(QTcpSocket* socket, QString response)
    }
    WiFiTcpServer --|> QTcpServer

    class WiFiConfigurator {
        -QString lastError_
        +searchWiFi() QList~NetworkInfo~
        +connectWiFi(QString ssid, QString password) bool
        +disconnectWiFi() bool
        +getCurrentSSID() QString
        +getCurrentIP() QString
        +getLastError() QString
        -executeNmcli(QStringList args, int timeout) QString
    }
    WiFiConfigurator --|> QObject

    class NetworkInfo {
        +QString ssid
        +int signal
        +QString security
        +bool isConnected
    }

    WiFiTcpServer --> WiFiConfigurator : uses
    WiFiConfigurator --> NetworkInfo : returns
```

---

## 5. SSL Connection & Handshake Flow

```mermaid
sequenceDiagram
    participant Client as Qt GUI (SslClient)
    participant Server as SslServer :8765

    Client->>Server: TCP Connect
    Server->>Server: incomingConnection()<br/>Create QSslSocket
    Server->>Server: Load embedded cert & key<br/>(server.crt / server.key)
    Server->>Client: SSL Handshake (TLS 1.2+)
    Client->>Client: Ignore SSL errors<br/>(self-signed cert)
    Client-->>Server: Handshake Complete ✓

    Note over Client,Server: Encrypted channel established

    Client->>Server: [4-byte size] + [Packet Data]
    Server->>Server: processPackets()
    Server->>Client: [4-byte size] + [Response Data]
```

---

## 6. Packet Protocol Structure

```mermaid
graph LR
    subgraph "Packet Format (QDataStream Qt_6_0)"
        SIZE["4 bytes<br/>quint32<br/>Packet Size"]
        TYPE["QString<br/>Packet Type"]
        PAYLOAD["Variable<br/>Payload Data"]
    end

    SIZE --> TYPE --> PAYLOAD

    subgraph "Packet Types"
        CMD["COMMAND<br/>type + command_string"]
        UV["UPLOAD_VIDEO<br/>type + filename + fileData"]
        UA["UPLOAD_AUDIO<br/>type + filename + fileData"]
        RSP["RESPONSE<br/>type + response_text"]
    end
```

---

## 7. Media Command Flow (SSL Server)

```mermaid
flowchart TD
    A["Client sends COMMAND packet"] --> B["processPackets()"]
    B --> C{"Command Type?"}

    C -->|"show_video"| D["getVideoList()<br/>scan /data/upload/videos/"]
    C -->|"show_audio"| E["getAudioList()<br/>scan /data/upload/audio/"]
    C -->|"play_video:name"| F["playVideo(name)<br/>mpv_command_async(videoMpv_)"]
    C -->|"play_audio:name"| G["playAudio(name)<br/>mpv_command_async(audioMpv_)"]
    C -->|"toggle_video"| H["toggleVideoPlayback()<br/>mpv_set_property pause"]
    C -->|"toggle_audio"| I["toggleAudioPlayback()<br/>mpv_set_property pause"]
    C -->|"set_video_volume:N"| J["setVideoVolume(N)<br/>mpv_set_property volume"]
    C -->|"set_audio_volume:N"| K["setAudioVolume(N)<br/>mpv_set_property volume"]
    C -->|"get_video_volume"| L["getVideoVolume()<br/>mpv_get_property volume"]
    C -->|"get_audio_volume"| M["getAudioVolume()<br/>mpv_get_property volume"]
    C -->|"llm_response"| N["llmResponse()<br/>TODO: Not implemented"]

    D --> R["sendResponse(socket, result)"]
    E --> R
    F --> R
    G --> R
    H --> R
    I --> R
    J --> R
    K --> R
    L --> R
    M --> R
    N --> R
```

---

## 8. File Upload Flow (SSL Server)

```mermaid
sequenceDiagram
    participant Client as SslClient
    participant Server as SslServer
    participant FS as File System

    Client->>Client: Read file from disk
    Client->>Server: UPLOAD_VIDEO packet<br/>[type, filename, fileData]
    
    Server->>Server: processPackets()<br/>type == "UPLOAD_VIDEO"

    Server->>FS: Check /data/upload/videos/<br/>file count
    
    alt File count >= 5
        Server->>Client: RESPONSE: "Upload limit reached (5/5)"
    else File count < 5
        Server->>FS: saveUploadedFile()<br/>mkdir -p if needed<br/>write file data
        alt Save success
            Server->>Client: RESPONSE: "Video uploaded: file.mp4 (3/5)"
        else Save failed
            Server->>Client: RESPONSE: "Error saving file"
        end
    end
```

---

## 9. WiFi Configuration Flow

```mermaid
sequenceDiagram
    participant App as Control App
    participant WTS as WiFiTcpServer :7358
    participant WC as WiFiConfigurator
    participant NM as nmcli (NetworkManager)

    Note over App,NM: === Search WiFi Networks ===
    App->>WTS: COMMAND: "SEARCH_WIFI"
    WTS->>WC: searchWiFi()
    WC->>NM: nmcli device wifi list --rescan yes
    NM-->>WC: Raw output (ssid:signal:security:active)
    WC-->>WTS: QList<NetworkInfo>
    WTS-->>App: "WIFI_LIST:SSID1:85:WPA2:yes,SSID2:60:--:no"

    Note over App,NM: === Connect to WiFi ===
    App->>WTS: COMMAND: "CONNECT_WIFI:MySSID:password123"
    WTS->>WC: connectWiFi("MySSID", "password123")
    WC->>NM: nmcli device wifi connect MySSID password password123
    NM-->>WC: Success / Failure
    WC-->>WTS: bool result
    WTS-->>App: "CONNECT_OK:MySSID" or "ERROR:message"

    Note over App,NM: === Get Current WiFi ===
    App->>WTS: COMMAND: "CURRENT_WIFI"
    WTS->>WC: getCurrentSSID() + getCurrentIP()
    WC->>NM: nmcli device wifi / nmcli connection show
    NM-->>WC: SSID + IP address
    WC-->>WTS: ssid, ip
    WTS-->>App: "CURRENT_WIFI:MySSID:192.168.1.100"

    Note over App,NM: === Disconnect WiFi ===
    App->>WTS: COMMAND: "DISCONNECT_WIFI"
    WTS->>WC: disconnectWiFi()
    WC->>NM: nmcli connection down <ssid>
    NM-->>WC: Success / Failure
    WC-->>WTS: bool result
    WTS-->>App: "DISCONNECT_OK" or "ERROR:message"
```

---

## 10. Network Topology

```mermaid
graph TB
    subgraph "Robot Internal Network (10.0.0.x)"
        RPC["Robot PC<br/>10.0.0.1<br/>SSL :8765"]
        RPC2["Robot PC<br/>10.0.0.100<br/>WiFi TCP :7358"]
        L1["Laser1 (Front)<br/>10.0.0.2"]
        L2["Laser2 (Back)<br/>10.0.0.3"]
        MINI["Mini PC / Jetson<br/>10.0.0.4"]
        ANDROID["Android ETH<br/>10.0.0.5"]
    end

    subgraph "External WiFi Network"
        ROUTER["WiFi Router<br/>192.168.x.x"]
        CLOUD["Internet / Cloud"]
    end

    RPC <--> L1
    RPC <--> L2
    RPC <--> MINI
    RPC <--> ANDROID
    RPC2 -.->|"nmcli manage"| ROUTER
    ROUTER <--> CLOUD

    style RPC fill:#2d6a4f,color:#fff
    style RPC2 fill:#2d6a4f,color:#fff
    style ROUTER fill:#e76f51,color:#fff
```

---

## 11. Complete Data Flow Summary

```mermaid
flowchart LR
    subgraph "Inputs"
        A1["Qt GUI<br/>(Media Commands)"]
        A2["Control App<br/>(WiFi Commands)"]
    end

    subgraph "rom_socket Process"
        B1["SslServer :8765<br/>SSL/TLS Encrypted"]
        B2["WiFiTcpServer :7358<br/>Plain TCP"]
    end

    subgraph "Processing"
        C1["processPackets()<br/>COMMAND / UPLOAD"]
        C2["processPackets()<br/>WiFi COMMAND"]
    end

    subgraph "Backends"
        D1["libmpv<br/>Audio Player"]
        D2["libmpv<br/>Video Player"]
        D3["File System<br/>/data/upload/"]
        D4["WiFiConfigurator"]
        D5["nmcli<br/>NetworkManager"]
    end

    subgraph "Outputs"
        E1["Audio Output<br/>(Speaker)"]
        E2["Video Output<br/>(Monitor/DRM)"]
        E3["Saved Files<br/>videos/ audio/"]
        E4["WiFi Connection<br/>Status"]
    end

    A1 -->|SSL| B1
    A2 -->|TCP| B2

    B1 --> C1
    B2 --> C2

    C1 --> D1
    C1 --> D2
    C1 --> D3

    C2 --> D4
    D4 --> D5

    D1 --> E1
    D2 --> E2
    D3 --> E3
    D5 --> E4
```

---

## Function Reference Table

| Server | Command | Function Called | Description |
|--------|---------|---------------|-------------|
| SSL :8765 | `show_video` | `getVideoList()` | Video file list ပြန်ပေးသည် |
| SSL :8765 | `show_audio` | `getAudioList()` | Audio file list ပြန်ပေးသည် |
| SSL :8765 | `play_video:<name>` | `playVideo()` | MPV ဖြင့် video ဖွင့်သည် |
| SSL :8765 | `play_audio:<name>` | `playAudio()` | MPV ဖြင့် audio ဖွင့်သည် |
| SSL :8765 | `toggle_video` | `toggleVideoPlayback()` | Video ရပ်/ဆက်ဖွင့် |
| SSL :8765 | `toggle_audio` | `toggleAudioPlayback()` | Audio ရပ်/ဆက်ဖွင့် |
| SSL :8765 | `set_video_volume:<N>` | `setVideoVolume()` | Video volume (0-100) |
| SSL :8765 | `set_audio_volume:<N>` | `setAudioVolume()` | Audio volume (0-100) |
| SSL :8765 | `get_video_volume` | `getVideoVolume()` | Video volume ပြန်ဖတ် |
| SSL :8765 | `get_audio_volume` | `getAudioVolume()` | Audio volume ပြန်ဖတ် |
| SSL :8765 | `UPLOAD_VIDEO` | `saveUploadedFile()` | Video file upload (max 5) |
| SSL :8765 | `UPLOAD_AUDIO` | `saveUploadedFile()` | Audio file upload (max 10) |
| SSL :8765 | `llm_response` | `llmResponse()` | TODO: LLM integration |
| WiFi :7358 | `SEARCH_WIFI` | `handleSearchWiFi()` | WiFi network scan |
| WiFi :7358 | `CONNECT_WIFI:<ssid>:<pwd>` | `handleConnectWiFi()` | WiFi connect |
| WiFi :7358 | `DISCONNECT_WIFI` | `handleDisconnectWiFi()` | WiFi disconnect |
| WiFi :7358 | `CURRENT_WIFI` | `handleCurrentWiFi()` | Current WiFi info |
