# ROM Socket - SSL Server/Client for Qt

Qt6 based SSL encrypted TCP server/client implementation for secure file transfer and command execution.

## Features

- ✅ SSL/TLS encrypted communication (TLS 1.2+)
- ✅ Multiple client support (concurrent connections)
- ✅ File upload (video/audio)
- ✅ Command execution (show_video, show_audio, llm_response)
- ✅ WiFi configuration and management (scan, connect, disconnect)
- ✅ Media playback control (play, pause, volume)
- ✅ Embedded SSL certificates (no external files needed)
- ✅ Cross-platform (Linux, Windows, macOS)

---

## System Requirements

### Minimum Requirements
- **OS:** Ubuntu 20.04+ / Debian 11+ / macOS / Windows 10+
- **Compiler:** GCC 9+ / Clang 10+ / MSVC 2019+
- **CMake:** 3.16+
- **Qt:** 6.2+ (Qt 6.8 recommended)
- **OpenSSL:** 1.1.1+ or 3.0+

### Hardware
- **CPU:** 2 cores minimum
- **RAM:** 512MB minimum (2GB recommended)
- **Disk:** 100MB free space

---

## Installation

### 1. Install Qt6 (Ubuntu/Debian)

#### Option A: APT Package Manager (Easiest)

```bash
# Update system
sudo apt update

# Install Qt6 base packages
sudo apt install -y qt6-base-dev libqt6network6

# Install development tools
sudo apt install -y qt6-tools-dev cmake build-essential
sudo apt install qml6-module-qtmultimedia libqt6multimedia6*
# Install OpenSSL
sudo apt install -y libssl-dev
sudo apt update && sudo apt install -y libmpv-dev

# Verify installation
qmake6 --version
```

---

### 2. Install Build Dependencies

```bash
# Essential build tools
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# SSL/TLS libraries
sudo apt install -y \
    libssl-dev \
    openssl

# Optional: Certificate generation tools
sudo apt install -y \
    ca-certificates
```

---

## Build Instructions

### Clone Repository

```bash
git clone <your-repo-url>
cd rom_socket
```

### Generate SSL Certificates (Optional)

If you don't have certificates, generate self-signed ones:

```bash
mkdir -p secret
cd secret

# Generate private key
openssl genrsa -out server.key 2048

# Generate certificate
openssl req -new -x509 -key server.key -out server.crt -days 365 \
    -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"

cd ..
```

### Compile Project

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make

# Check executable
ls -lh rom_socket
```

### Alternative: Clean Build

```bash
rm -rf build && mkdir build && cd build && cmake .. && make
```

---

## Running the Server

```bash
cd build
./rom_socket
```

**Expected Output:**
```
SSL Server is running on port 12345
Loaded embedded SSL certificate
```

---

## Configuration

### Server Configuration

Edit `src/ssl_server.cpp`:

```cpp
// Change upload directories
QString video_directory = "/home/mr_robot/data/upload/videos/";
QString audio_directory = "/home/mr_robot/data/upload/audio/";
```

Edit `src/main.cpp`:

```cpp
// Change port
if (!server.listen(QHostAddress::Any, 12345)) {
    // Change 12345 to your preferred port
}
```

### Embedded Certificates

Replace placeholder certificates in `src/ssl_server.cpp`:

```cpp
const char* EMBEDDED_CERTIFICATE = R"(
-----BEGIN CERTIFICATE-----
... paste your certificate here ...
-----END CERTIFICATE-----
)";

const char* EMBEDDED_PRIVATE_KEY = R"(
-----BEGIN PRIVATE KEY-----
... paste your private key here ...
-----END PRIVATE KEY-----
)";
```

---

## Client Usage

### Include in Your Qt Project

Copy `ssl_client.h` and `ssl_client.cpp` to your project.

### Example Code

```cpp
#include "ssl_client.h"

// Create client
SslClient *client = new SslClient(this);

// Ignore SSL errors for self-signed certificates
client->setIgnoreSslErrors(true);

// Connect signals
connect(client, &SslClient::connected, []() {
    qDebug() << "Connected!";
});

connect(client, &SslClient::responseReceived, [](const QString &response) {
    qDebug() << "Response:" << response;
});

// Connect to server
client->connectToServer("192.168.1.100", 12345);

// Send commands
client->sendShowVideoCommand();
client->sendShowAudioCommand();

// Upload file
client->uploadVideoFile("/path/to/video.mp4");
```

---

## Protocol Documentation

### Supported Packet Types

| Type | Description | Format |
|------|-------------|--------|
| `COMMAND` | Execute server command | type + command string |
| `UPLOAD_VIDEO` | Upload video file | type + filename + file data |
| `UPLOAD_AUDIO` | Upload audio file | type + filename + file data |
| `RESPONSE` | Server response | type + response string |

### Available Commands

- `show_video` - Get list of video files
- `show_audio` - Get list of audio files  
- `llm_response` - LLM query (placeholder)

### Example Protocol Flow

**Client sends:**
```cpp
Type: "COMMAND"
Data: "show_video"
```

**Server responds:**
```cpp
Type: "RESPONSE"
Data: "movie1.mp4,movie2.mkv,video.avi"
```

---

## Troubleshooting

### Qt6 Not Found

```bash
# Check Qt installation
qmake6 --version
pkg-config --modversion Qt6Core

# Set CMAKE_PREFIX_PATH manually
export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake ..
```

### SSL Certificate Errors

```bash
# Check certificate validity
openssl x509 -in secret/server.crt -text -noout

# Regenerate if expired
openssl req -new -x509 -key server.key -out server.crt -days 365
```

### Permission Denied

```bash
# Check upload directory permissions
sudo mkdir -p /home/mr_robot/data/upload/videos
sudo mkdir -p /home/mr_robot/data/upload/audio
sudo chown -R $USER:$USER /home/mr_robot/data/upload
```

### Port Already in Use

```bash
# Check what's using the port
sudo lsof -i :12345

# Kill the process or change port in main.cpp
```

---

## Development

### Project Structure

```
rom_socket/
├── CMakeLists.txt          # Build configuration
├── secret/                 # SSL certificates (optional)
│   ├── server.crt
│   └── server.key
├── src/
│   ├── main.cpp           # Server entry point
│   ├── ssl_server.h       # Server header
│   ├── ssl_server.cpp     # Server implementation
│   ├── ssl_client.h       # Client header (for Qt projects)
│   └── ssl_client.cpp     # Client implementation
└── build/                 # Build output
    └── rom_socket         # Executable
```

### Adding New Commands

Edit `src/ssl_server.cpp`:

```cpp
if (command == "your_command") {
    response = yourCustomFunction();
    qDebug() << "Processing your command";
}
```

### Code Style

- C++17 standard
- Qt naming conventions
- 4-space indentation
- Use `qDebug()` for logging

---

## Security Notes

⚠️ **Important:**

- Never commit real SSL private keys to Git
- Use environment variables for production keys
- Rotate certificates regularly
- Use strong passwords for certificate generation
- Enable firewall rules for production deployment

---

## License

[Your License Here]

## Author

[Your Name/Organization]

## Support

For issues and questions:
- GitHub Issues: [Your Repo URL]
- Email: [Your Email]

---

## Version History

- **v1.0.0** - Initial release with SSL server/client
  - Multiple client support
  - File upload functionality
  - Command execution
  - Embedded certificates
# rom_socket
