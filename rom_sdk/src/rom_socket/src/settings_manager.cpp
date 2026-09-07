#include "settings_manager.h"

SettingsManager::SettingsManager(const QString &basePath)
    : basePath_(basePath)
{
    qDebug() << "SettingsManager initialized with base path:" << basePath_;
}

QString SettingsManager::resolveFilePath(const QString &settingsName) const
{
    // Sanitize: prevent path traversal attacks
    if (settingsName.contains("..") || settingsName.contains("/") || settingsName.contains("\\")) {
        qWarning() << "Invalid settings name (path traversal attempt):" << settingsName;
        return QString();
    }
    
    // Check root directory first
    QString rootPath = basePath_ + "/" + settingsName + ".yaml";
    if (QFile::exists(rootPath)) {
        return rootPath;
    }
    
    // Check distribution_modes/ subdirectory
    QString modePath = basePath_ + "/distribution_modes/" + settingsName + ".yaml";
    if (QFile::exists(modePath)) {
        return modePath;
    }
    
    qWarning() << "Settings file not found:" << settingsName;
    return QString();
}

QMap<QString, QString> SettingsManager::parseYaml(const QString &filePath) const
{
    QMap<QString, QString> result;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open YAML file:" << filePath;
        return result;
    }
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QString trimmed = line.trimmed();
        
        // Skip empty lines and comments
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }
        
        // Parse "key: value" format
        int colonIdx = trimmed.indexOf(':');
        if (colonIdx > 0) {
            QString key = trimmed.left(colonIdx).trimmed();
            QString value = trimmed.mid(colonIdx + 1).trimmed();
            
            // Strip inline comments (but not inside quoted strings)
            if (!value.startsWith('"') && !value.startsWith('\'')) {
                int commentIdx = value.indexOf('#');
                if (commentIdx > 0) {
                    value = value.left(commentIdx).trimmed();
                }
            }
            
            // Remove surrounding quotes
            if ((value.startsWith('"') && value.endsWith('"')) ||
                (value.startsWith('\'') && value.endsWith('\''))) {
                value = value.mid(1, value.length() - 2);
            }
            
            result.insert(key, value);
        }
    }
    
    file.close();
    return result;
}

bool SettingsManager::rewriteYaml(const QString &filePath, const QString &key, const QString &newValue)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open YAML for rewrite:" << filePath;
        return false;
    }
    
    QStringList lines;
    QTextStream in(&file);
    bool keyFound = false;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        QString trimmed = line.trimmed();
        
        // Check if this line contains the target key
        if (!trimmed.startsWith('#') && !trimmed.isEmpty()) {
            int colonIdx = trimmed.indexOf(':');
            if (colonIdx > 0) {
                QString lineKey = trimmed.left(colonIdx).trimmed();
                if (lineKey == key) {
                    // Determine if value needs quoting
                    bool needsQuotes = false;
                    // Quote if: contains special chars, starts with special, or was originally quoted
                    bool isNumeric = false;
                    bool isBool = (newValue == "true" || newValue == "false");
                    newValue.toDouble(&isNumeric);
                    
                    if (!isNumeric && !isBool && 
                        (newValue.contains(':') || newValue.contains('#') || 
                         newValue.contains('"') || newValue.contains(' ') ||
                         newValue.isEmpty())) {
                        needsQuotes = true;
                    }
                    // Also quote if original was quoted
                    QString origValue = trimmed.mid(colonIdx + 1).trimmed();
                    if (origValue.startsWith('"')) {
                        needsQuotes = true;
                    }
                    
                    // Preserve original indentation
                    QString indent = line.left(line.indexOf(lineKey));
                    
                    // Preserve inline comment if any
                    QString inlineComment;
                    if (!origValue.startsWith('"')) {
                        int commentIdx = origValue.indexOf('#');
                        if (commentIdx > 0) {
                            inlineComment = " " + origValue.mid(commentIdx);
                        }
                    }
                    
                    if (needsQuotes) {
                        line = indent + key + ": \"" + newValue + "\"" + inlineComment;
                    } else {
                        line = indent + key + ": " + newValue + inlineComment;
                    }
                    keyFound = true;
                }
            }
        }
        lines.append(line);
    }
    file.close();
    
    if (!keyFound) {
        qWarning() << "Key not found in YAML:" << key;
        return false;
    }
    
    // Write back atomically: write to temp, then rename
    QString tempPath = filePath + ".tmp";
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot create temp file:" << tempPath;
        return false;
    }
    
    QTextStream out(&tempFile);
    for (const QString &line : lines) {
        out << line << "\n";
    }
    tempFile.close();
    
    // Atomic rename
    QFile::remove(filePath);
    if (!QFile::rename(tempPath, filePath)) {
        qCritical() << "Failed to rename temp file to:" << filePath;
        return false;
    }
    
    qDebug() << "Settings updated:" << key << "=" << newValue << "in" << filePath;
    return true;
}

// ===== Public API =====

QString SettingsManager::readAll(const QString &settingsName) const
{
    QMutexLocker locker(&mutex_);
    
    QString filePath = resolveFilePath(settingsName);
    if (filePath.isEmpty()) {
        return "ERROR:Settings not found: " + settingsName;
    }
    
    QMap<QString, QString> settings = parseYaml(filePath);
    if (settings.isEmpty()) {
        return "ERROR:Empty or invalid settings file: " + settingsName;
    }
    
    // Format as "key:value" pairs separated by newlines
    QStringList pairs;
    for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
        pairs.append(it.key() + ":" + it.value());
    }
    
    return pairs.join("\n");
}

QString SettingsManager::readValue(const QString &settingsName, const QString &key) const
{
    QMutexLocker locker(&mutex_);
    
    QString filePath = resolveFilePath(settingsName);
    if (filePath.isEmpty()) {
        return "ERROR:Settings not found: " + settingsName;
    }
    
    QMap<QString, QString> settings = parseYaml(filePath);
    if (!settings.contains(key)) {
        return "ERROR:Key not found: " + key;
    }
    
    return settings.value(key);
}

QString SettingsManager::listSettings() const
{
    QMutexLocker locker(&mutex_);
    
    QStringList allSettings;
    
    // Root directory YAML files
    QDir rootDir(basePath_);
    rootDir.setNameFilters(QStringList() << "*.yaml");
    for (const QString &file : rootDir.entryList(QDir::Files)) {
        allSettings.append(file.chopped(5)); // Remove ".yaml"
    }
    
    // distribution_modes/ subdirectory
    QDir modesDir(basePath_ + "/distribution_modes");
    if (modesDir.exists()) {
        modesDir.setNameFilters(QStringList() << "*.yaml");
        for (const QString &file : modesDir.entryList(QDir::Files)) {
            allSettings.append(file.chopped(5)); // Remove ".yaml"
        }
    }
    
    return allSettings.join(",");
}

bool SettingsManager::writeValue(const QString &settingsName, const QString &key, const QString &value)
{
    QMutexLocker locker(&mutex_);
    
    QString filePath = resolveFilePath(settingsName);
    if (filePath.isEmpty()) {
        return false;
    }
    
    return rewriteYaml(filePath, key, value);
}

bool SettingsManager::writeMultiple(const QString &settingsName, const QString &keyValues)
{
    QMutexLocker locker(&mutex_);
    
    QString filePath = resolveFilePath(settingsName);
    if (filePath.isEmpty()) {
        return false;
    }
    
    // Parse "key1:value1\nkey2:value2" format
    QStringList pairs = keyValues.split("\n", Qt::SkipEmptyParts);
    
    for (const QString &pair : pairs) {
        int colonIdx = pair.indexOf(':');
        if (colonIdx <= 0) {
            qWarning() << "Invalid key:value pair:" << pair;
            continue;
        }
        
        QString key = pair.left(colonIdx).trimmed();
        QString value = pair.mid(colonIdx + 1).trimmed();
        
        if (!rewriteYaml(filePath, key, value)) {
            qWarning() << "Failed to write:" << key << "=" << value;
            return false;
        }
    }
    
    return true;
}
