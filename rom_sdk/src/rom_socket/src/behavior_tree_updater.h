#ifndef BEHAVIOR_TREE_UPDATER_H
#define BEHAVIOR_TREE_UPDATER_H

#include <QString>
#include <QMutex>

/**
 * @brief Updates BehaviorTree XML files when YAML settings change
 *
 * Maps YAML setting keys to XML attributes in BT files under /home/buc_robot/data/trees/.
 * Thread-safe: file read/write is mutex-protected.
 *
 * Mapping:
 *   waypoints_mode_settings → waypoints_mode.xml
 *   service_mode_settings   → service_mode.xml
 *   patrol_mode_settings    → patrol_mode.xml
 */
class BehaviorTreeUpdater
{
public:
    explicit BehaviorTreeUpdater(const QString &treesDir = "/home/buc_robot/data/trees");

    /**
     * @brief Update the corresponding BT XML when a YAML setting changes
     * @param settingsName YAML file name (without .yaml), e.g. "waypoints_mode_settings"
     * @param key YAML key that was changed
     * @param value New value
     * @return true if update succeeded or was not needed (unrelated settings)
     */
    bool updateBehaviorTree(const QString &settingsName, const QString &key, const QString &value);

private:
    /// Map settings name to BT XML filename
    QString resolveTreeFile(const QString &settingsName) const;

    /// Replace a simple global attribute: attrName="old" → attrName="new"
    static int replaceGlobalAttribute(QString &xml, const QString &attrName, const QString &newValue);

    /// Replace attribute only in elements containing a context identifier
    /// e.g. replace number_of_retries only in tags with name="NavigateRecovery"
    static int replaceContextAttribute(QString &xml, const QString &contextId,
                                       const QString &attrName, const QString &newValue);

    QString treesDir_;
    mutable QMutex mutex_;
};

#endif // BEHAVIOR_TREE_UPDATER_H
