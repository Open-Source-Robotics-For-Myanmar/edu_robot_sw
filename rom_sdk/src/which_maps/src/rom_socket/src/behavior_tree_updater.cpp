#include "behavior_tree_updater.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QMutexLocker>

BehaviorTreeUpdater::BehaviorTreeUpdater(const QString &treesDir)
    : treesDir_(treesDir)
{
}

QString BehaviorTreeUpdater::resolveTreeFile(const QString &settingsName) const
{
    // Map: YAML settings name → BT XML filename
    if (settingsName == "waypoints_mode_settings")
        return treesDir_ + "/waypoints_mode.xml";
    if (settingsName == "service_mode_settings")
        return treesDir_ + "/service_mode.xml";
    if (settingsName == "patrol_mode_settings")
        return treesDir_ + "/patrol_mode.xml";
    return {};
}

bool BehaviorTreeUpdater::updateBehaviorTree(const QString &settingsName,
                                             const QString &key,
                                             const QString &value)
{
    QString treeFile = resolveTreeFile(settingsName);
    if (treeFile.isEmpty()) {
        // Not one of the 3 modes that have BT XMLs — silently succeed
        return true;
    }

    QMutexLocker locker(&mutex_);

    // Read the XML file
    QFile file(treeFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "BehaviorTreeUpdater: Cannot open" << treeFile;
        return false;
    }
    QString xml = QTextStream(&file).readAll();
    file.close();

    int replacements = 0;

    // ===== Simple global attributes (unique attribute names) =====
    if (key == "planner_id") {
        replacements = replaceGlobalAttribute(xml, "planner_id", value);

    } else if (key == "controller_id") {
        replacements = replaceGlobalAttribute(xml, "controller_id", value);

    } else if (key == "server_timeout") {
        replacements = replaceGlobalAttribute(xml, "server_timeout", value);

    } else if (key == "spin_dist") {
        replacements = replaceGlobalAttribute(xml, "spin_dist", value);

    } else if (key == "wait_duration") {
        replacements = replaceGlobalAttribute(xml, "wait_duration", value);

    } else if (key == "backup_dist") {
        replacements = replaceGlobalAttribute(xml, "backup_dist", value);

    } else if (key == "backup_speed") {
        replacements = replaceGlobalAttribute(xml, "backup_speed", value);

    } else if (key == "delay_between_waypoints_ms") {
        // Maps to <Delay delay_msec="...">
        replacements = replaceGlobalAttribute(xml, "delay_msec", value);

    } else if (key == "num_cycles") {
        // Maps to <Repeat num_cycles="...">
        replacements = replaceGlobalAttribute(xml, "num_cycles", value);

    // ===== Context-dependent attributes =====
    } else if (key == "replanning_rate_hz") {
        // Maps to hz="..." in RateController elements
        replacements = replaceContextAttribute(xml, "RateController", "hz", value);

    } else if (key == "navigate_recovery_retries") {
        // Maps to number_of_retries in elements with name="NavigateRecovery"
        replacements = replaceContextAttribute(xml, "NavigateRecovery", "number_of_retries", value);

    } else if (key == "compute_path_retries") {
        // Maps to number_of_retries in elements with name="ComputePathToPose"
        replacements = replaceContextAttribute(xml, "ComputePathToPose", "number_of_retries", value);

    } else if (key == "follow_path_retries") {
        // Maps to number_of_retries in elements with name="FollowPath"
        replacements = replaceContextAttribute(xml, "FollowPath", "number_of_retries", value);

    } else if (key == "spin_time_allowance") {
        // Maps to time_allowance in Spin elements
        // Matches tags containing "Spin" (either <Spin or ID="Spin")
        replacements = replaceContextAttribute(xml, "Spin", "time_allowance", value);

    } else if (key == "backup_time_allowance") {
        // Maps to time_allowance in BackUp elements
        replacements = replaceContextAttribute(xml, "BackUp", "time_allowance", value);

    } else {
        // Unknown key — no BT update needed
        return true;
    }

    if (replacements == 0) {
        qWarning() << "BehaviorTreeUpdater: No matches for key" << key << "in" << treeFile;
        // Not necessarily an error — the attribute might not exist in this tree
        return true;
    }

    // Write back the modified XML
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "BehaviorTreeUpdater: Cannot write" << treeFile;
        return false;
    }
    QTextStream out(&file);
    out << xml;
    file.close();

    qDebug() << "BehaviorTreeUpdater: Updated" << replacements
             << "occurrence(s) of" << key << "in" << treeFile;
    return true;
}

// Replace all occurrences of attrName="old" → attrName="new" globally
int BehaviorTreeUpdater::replaceGlobalAttribute(QString &xml, const QString &attrName,
                                                const QString &newValue)
{
    // Pattern: attrName="any_value"
    // Word boundary \b ensures we don't match partial attribute names
    QRegularExpression rx(
        "\\b" + QRegularExpression::escape(attrName) + "=\"[^\"]*\"");

    QString replacement = attrName + "=\"" + newValue + "\"";

    int count = 0;
    auto it = rx.globalMatch(xml);
    // Count matches first
    while (it.hasNext()) {
        it.next();
        count++;
    }

    if (count > 0) {
        xml.replace(rx, replacement);
    }
    return count;
}

// Replace attrName="old" → attrName="new" only within opening tags that contain contextId
// contextId can be a tag name (e.g., "RateController") or a name attribute value (e.g., "NavigateRecovery")
int BehaviorTreeUpdater::replaceContextAttribute(QString &xml, const QString &contextId,
                                                 const QString &attrName, const QString &newValue)
{
    // Match opening tags: everything from '<' to '>' or '/>'
    // [^>] also matches newlines in PCRE2 (Qt), so multiline tags are handled
    QRegularExpression tagRx("<[^>]+>");
    auto it = tagRx.globalMatch(xml);

    // Collect replacement offset/length pairs (process in reverse to preserve offsets)
    struct Replacement {
        int offset;
        int length;
        QString newText;
    };
    QVector<Replacement> replacements;

    // Pattern to find the attribute within a tag
    QRegularExpression attrRx(
        "\\b" + QRegularExpression::escape(attrName) + "=\"[^\"]*\"");
    QString attrReplacement = attrName + "=\"" + newValue + "\"";

    while (it.hasNext()) {
        auto match = it.next();
        QString tag = match.captured(0);

        // Check if this tag contains our context identifier
        // (either as tag name like <RateController or as attribute value like name="NavigateRecovery")
        if (!tag.contains(contextId)) {
            continue;
        }

        // Find and replace the attribute within this tag
        auto attrMatch = attrRx.match(tag);
        if (attrMatch.hasMatch()) {
            // Calculate absolute offset in the full XML string
            int absOffset = match.capturedStart(0) + attrMatch.capturedStart(0);
            Replacement r;
            r.offset = absOffset;
            r.length = attrMatch.capturedLength(0);
            r.newText = attrReplacement;
            replacements.append(r);
        }
    }

    // Apply replacements in reverse order to maintain correct offsets
    for (int i = replacements.size() - 1; i >= 0; --i) {
        xml.replace(replacements[i].offset, replacements[i].length, replacements[i].newText);
    }

    return replacements.size();
}
