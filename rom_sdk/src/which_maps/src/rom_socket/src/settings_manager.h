#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QString>
#include <QMap>
#include <QVariant>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

/**
 * @brief Generic YAML settings reader/writer
 * 
 * Handles flat key-value YAML files in data/app/app_settings/.
 * Thread-safe: all read/write operations are mutex-protected.
 * 
 * Supported YAML value types: string, int, float, bool
 * Does NOT use yaml-cpp — uses lightweight custom parser for flat key:value files.
 * This avoids adding a heavy dependency for simple flat YAML files.
 */
class SettingsManager
{
public:
    explicit SettingsManager(const QString &basePath = "/home/mr_robot/data/app/app_settings");
    
    // ===== Read Operations =====
    
    /// Read all key-value pairs from a settings file, returns JSON-like string
    /// @param settingsName YAML filename without extension (e.g., "basic_settings")
    /// @return Formatted string: "key1:value1\nkey2:value2\n..." or error message
    QString readAll(const QString &settingsName) const;
    
    /// Read a single value from a settings file
    /// @return Value as string, or empty string if key not found
    QString readValue(const QString &settingsName, const QString &key) const;
    
    /// List all available settings files
    /// @return Comma-separated list of settings file names (without .yaml)
    QString listSettings() const;
    
    // ===== Write Operations =====
    
    /// Write/update a single key-value pair in a settings file
    /// @return true if successful
    bool writeValue(const QString &settingsName, const QString &key, const QString &value);
    
    /// Write multiple key-value pairs at once
    /// @param keyValues Format: "key1:value1\nkey2:value2\n..."
    /// @return true if all writes successful
    bool writeMultiple(const QString &settingsName, const QString &keyValues);

private:
    /// Resolve settings name to full file path
    /// Checks both root and distribution_modes/ subdirectory
    QString resolveFilePath(const QString &settingsName) const;
    
    /// Parse a YAML file into key-value map (preserves comments for rewrite)
    QMap<QString, QString> parseYaml(const QString &filePath) const;
    
    /// Rewrite YAML file preserving comments and order, updating changed values
    bool rewriteYaml(const QString &filePath, const QString &key, const QString &newValue);
    
    QString basePath_;
    mutable QMutex mutex_; // Thread safety for file operations
};

#endif // SETTINGS_MANAGER_H
