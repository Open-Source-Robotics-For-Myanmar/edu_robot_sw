# ROM Socket - API Reference & Architecture

## System Overview

```mermaid
graph TB
    subgraph "Android Kotlin App"
        KA["SSL Client<br/>(Port 8765)"]
        UI["Settings UI<br/>(Fragments)"]
    end

    subgraph "rom_socket Process"
        SSL["SslServer :8765<br/>(SSL/TLS)"]
        WIFI["WiFiTcpServer :7358<br/>(Plain TCP)"]
        SM["SettingsManager"]
    end

    subgraph "Storage"
        YAML["data/app/app_settings/<br/>*.yaml files"]
        MEDIA["data/upload/<br/>videos/ audio/"]
    end

    subgraph "System"
        MPV["libmpv<br/>(Audio/Video)"]
        NM["nmcli<br/>(NetworkManager)"]
    end

    UI -->|user input| KA
    KA -->|"SSL/TLS"| SSL

    SSL --> SM
    SSL --> MPV
    SM -->|"read/write"| YAML
    SSL --> MEDIA

    WIFI --> NM

    style SM fill:#e76f51,color:#fff
    style YAML fill:#2a9d8f,color:#fff
```

---

## Packet Protocol

```mermaid
graph LR
    subgraph "Packet Format (QDataStream Qt_6_0)"
        SIZE["4 bytes<br/>quint32<br/>Packet Size"]
        TYPE["QString<br/>Packet Type"]
        PAYLOAD["QByteArray<br/>Payload"]
    end
    SIZE --> TYPE --> PAYLOAD
```

**Packet Types:**

| Type | Direction | Payload |
|------|-----------|---------|
| `COMMAND` | Client → Server | Command string (UTF-8) |
| `UPLOAD_VIDEO` | Client → Server | filename (QString) + fileData (QByteArray) |
| `UPLOAD_AUDIO` | Client → Server | filename (QString) + fileData (QByteArray) |
| `RESPONSE` | Server → Client | Response string (UTF-8) |

---

## Settings API Commands

### `list_settings`
List all available settings files.

```
→ COMMAND: "list_settings"
← RESPONSE: "basic_settings,advertising_settings,language_settings,...,meal_delivery_mode_settings,..."
```

```mermaid
sequenceDiagram
    participant C as Android Client
    participant S as SslServer
    participant SM as SettingsManager
    participant FS as File System

    C->>S: COMMAND: "list_settings"
    S->>SM: listSettings()
    SM->>FS: Scan app_settings/*.yaml<br/>+ distribution_modes/*.yaml
    FS-->>SM: File list
    SM-->>S: "basic_settings,cruise_mode_settings,..."
    S-->>C: RESPONSE: file list (comma-separated)
```

---

### `get_settings:<name>`
Read all key-value pairs from a settings file.

```
→ COMMAND: "get_settings:basic_settings"
← RESPONSE: "robot_name:ROM-Robot-01\nmedia_volume:50\nscreen_brightness:70\n..."
```

```mermaid
sequenceDiagram
    participant C as Android Client
    participant S as SslServer
    participant SM as SettingsManager

    C->>S: COMMAND: "get_settings:basic_settings"
    S->>SM: readAll("basic_settings")
    SM->>SM: resolveFilePath()<br/>→ app_settings/basic_settings.yaml
    SM->>SM: parseYaml() → key:value map
    SM-->>S: "robot_name:ROM-Robot-01\nmedia_volume:50\n..."
    S-->>C: RESPONSE: all settings as key:value pairs
```

---

### `get_setting_value:<name>:<key>`
Read a single setting value.

```
→ COMMAND: "get_setting_value:basic_settings:robot_name"
← RESPONSE: "ROM-Robot-01"
```

---

### `set_setting:<name>:<key>:<value>`
Write/update a single setting.

```
→ COMMAND: "set_setting:basic_settings:robot_name:MyRobot-02"
← RESPONSE: "OK:Setting updated: robot_name=MyRobot-02"
```

```mermaid
sequenceDiagram
    participant C as Android Client
    participant S as SslServer
    participant SM as SettingsManager
    participant FS as YAML File

    C->>S: COMMAND: "set_setting:basic_settings:robot_name:MyRobot-02"
    S->>SM: writeValue("basic_settings", "robot_name", "MyRobot-02")
    SM->>FS: Read basic_settings.yaml
    FS-->>SM: File content (lines)
    SM->>SM: Find line with "robot_name:"<br/>Replace value, preserve comments
    SM->>FS: Write to .tmp → atomic rename
    FS-->>SM: Success
    SM-->>S: true
    S-->>C: RESPONSE: "OK:Setting updated: robot_name=MyRobot-02"
```

---

### `set_settings:<name>:<key1>:<value1>\n<key2>:<value2>`
Write multiple settings at once.

```
→ COMMAND: "set_settings:basic_settings:robot_name:NewBot\nmedia_volume:80"
← RESPONSE: "OK:Settings updated for basic_settings"
```

---

## Media API Commands

### Video Commands

| Command | Response | Description |
|---------|----------|-------------|
| `show_video` | `"video1.mp4,video2.mp4"` | List video files |
| `play_video:<filename>` | `"Playing video: filename"` | Play via mpv |
| `toggle_video` | `"Video playback toggled"` | Pause / Resume |
| `set_video_volume:<0-100>` | `"Video volume set to: N"` | Set volume |
| `get_video_volume` | `"Video volume: N"` | Get current volume |

### Audio Commands

| Command | Response | Description |
|---------|----------|-------------|
| `show_audio` | `"song1.mp3,song2.mp3"` | List audio files |
| `play_audio:<filename>` | `"Playing audio: filename"` | Play via mpv |
| `toggle_audio` | `"Audio playback toggled"` | Pause / Resume |
| `set_audio_volume:<0-100>` | `"Audio volume set to: N"` | Set volume |
| `get_audio_volume` | `"Audio volume: N"` | Get current volume |

### Upload Commands

```mermaid
sequenceDiagram
    participant C as Client
    participant S as SslServer
    participant FS as File System

    C->>S: UPLOAD_VIDEO [filename, fileData]
    S->>FS: countMediaFiles() → check limit
    alt Count < 5
        S->>FS: saveUploadedFile()
        S-->>C: "Video uploaded: file.mp4 (3/5)"
    else Count >= 5
        S-->>C: "Upload rejected: limit reached (5/5)"
    end
```

| Upload Type | Max Files | Storage Path |
|-------------|-----------|-------------|
| `UPLOAD_VIDEO` | 5 | `/home/mr_robot/data/upload/videos/` |
| `UPLOAD_AUDIO` | 10 | `/home/mr_robot/data/upload/audio/` |

---

## WiFi API Commands (Port 7358, Plain TCP)

| Command | Response | Description |
|---------|----------|-------------|
| `SEARCH_WIFI` | `"WIFI_LIST:ssid:signal:security:active,..."` | Scan WiFi networks |
| `CONNECT_WIFI:<ssid>:<pwd>` | `"CONNECT_OK:<ssid>"` or `"ERROR:msg"` | Connect to WiFi |
| `DISCONNECT_WIFI` | `"DISCONNECT_OK"` or `"ERROR:msg"` | Disconnect WiFi |
| `CURRENT_WIFI` | `"CURRENT_WIFI:<ssid>:<ip>"` | Get current WiFi info |

---

## Settings Files Reference

```mermaid
graph TD
    subgraph "data/app/app_settings/"
        BS["basic_settings.yaml<br/>14 settings"]
        AS["advertising_settings.yaml"]
        LS["language_settings.yaml"]
        NS["network_settings.yaml"]
        VS["version_settings.yaml"]
        
        subgraph "distribution_modes/"
            MD["meal_delivery_mode_settings"]
            CD["cruise_mode_settings"]
            RD["recycling_mode_settings"]
            BD["birthday_mode_settings"]
            FD["free_distribution_mode_settings"]
            PD["patrol_mode_settings"]
            SD["service_mode_settings"]
            WD["waypoints_mode_settings"]
        end
    end

    style BS fill:#2a9d8f,color:#fff
    style MD fill:#264653,color:#fff
    style CD fill:#264653,color:#fff
    style RD fill:#264653,color:#fff
    style BD fill:#264653,color:#fff
    style FD fill:#264653,color:#fff
    style PD fill:#264653,color:#fff
    style SD fill:#264653,color:#fff
    style WD fill:#264653,color:#fff
```

### basic_settings.yaml

| Key | Default | Type | Range |
|-----|---------|------|-------|
| `robot_name` | `"ROM-Robot-01"` | string | - |
| `media_volume` | `50` | int | 1–100 |
| `screen_brightness` | `70` | int | 1–100 |
| `low_battery_setting` | `24` | int | 0–48 |
| `obstacle_avoidance_prompt` | `"Excuse me..."` | string | - |
| `administrator_password` | `1` | int | 0 / 1 |
| `display_content_during_delivery` | `0` | int | 0 / 1 |
| `emoticon_animation` | `"style1"` | string | style1 / style2 |
| `table_distribution` | `"three_columns"` | string | three_columns / four_columns |
| `data_synchronization` | `"myanmar"` | string | myanmar / other |
| `relocate` | `false` | bool | true / false |
| `switch_map` | `false` | bool | true / false |
| `call_module_configuration` | `false` | bool | true / false |
| `multi_machine_configuration` | `false` | bool | true / false |

### Distribution Mode Settings

| Mode | Key Settings |
|------|-------------|
| **meal_delivery** | operating_speed_ms, waiting_time_for_meal_taking, food_delivery_arrival_reminder, select_background_music |
| **cruise** | operating_speed_ms, select_background_music, circular_broadcast, broadcast_interval |
| **recycling** | operating_speed_ms, residence_time, prompt_for_placing_recyclables, prompt_after_placing_recyclables |
| **birthday** | operating_speed_ms, waiting_time_for_meal_taking, prompt_for_delivery_arrival, prompt_after_taking_meal, select_background_music, background_music_playing_time_point |
| **free_distribution** | operating_speed_ms, waiting_time_for_meal_taking, food_delivery_arrival_reminder, select_background_music |
| **patrol** | num_cycles, replanning_rate_hz, planner_id, controller_id, server_timeout, recovery params |
| **service** | delay_between_waypoints_ms, replanning_rate_hz, planner_id, controller_id, server_timeout, recovery params |
| **waypoints** | replanning_rate_hz, planner_id, controller_id, server_timeout, recovery params, time_allowance |

---

## Complete Data Flow

```mermaid
flowchart LR
    subgraph "Android App"
        A1["Settings UI"]
        A2["SSL Client"]
    end

    subgraph "rom_socket"
        B1["SslServer :8765"]
        B2["processPackets()"]
        B3["SettingsManager"]
        B4["Media Player (mpv)"]
    end

    subgraph "Storage"
        C1["YAML Settings Files"]
        C2["Media Files"]
    end

    A1 -->|"user edits"| A2
    A2 -->|"SSL/TLS"| B1
    B1 --> B2

    B2 -->|"get_settings / set_setting"| B3
    B2 -->|"play/toggle/volume"| B4
    B2 -->|"upload"| C2

    B3 -->|"read/write"| C1

    C1 -.->|"get_settings response"| B3
    B3 -.-> B2
    B2 -.-> B1
    B1 -.->|"SSL/TLS"| A2
    A2 -.->|"update UI"| A1
```

---

## Error Responses

All error responses start with `ERROR:` prefix:

| Error | Cause |
|-------|-------|
| `ERROR:Settings not found: <name>` | YAML file does not exist |
| `ERROR:Empty or invalid settings file: <name>` | YAML file is empty or unparseable |
| `ERROR:Key not found: <key>` | Key does not exist in the YAML file |
| `ERROR:Failed to update: <key>` | File write/permission error |
| `ERROR:Invalid format. Use ...` | Malformed command string |

---

## Security Notes

- SSL/TLS 1.2+ with self-signed certificate (CN=GhostMan, O=ROM-Robotics)
- Path traversal protection: `..`, `/`, `\` in settings names are rejected
- Atomic file writes: write to `.tmp` then rename (prevents corruption)
- Thread-safe: QMutex protects all file R/W operations
- WiFi server on Ethernet-only interface (10.0.0.100) — not exposed over WiFi
