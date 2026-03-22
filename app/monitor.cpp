#include "monitor.h"

#include <QFile>
#include <QStringList>

namespace {
quint64 readFirstNumberFromLine(const QString &line)
{
    const auto parts = line.simplified().split(' ');
    for (int i = 1; i < parts.size(); ++i) {
        bool ok = false;
        const quint64 value = parts.at(i).toULongLong(&ok);
        if (ok) {
            return value;
        }
    }
    return 0;
}
}

CpuSnapshot Monitor::readCpuSnapshot(const QString &statContent)
{
    CpuSnapshot snapshot;
    const QString firstLine = statContent.section('\n', 0, 0).simplified();
    const QStringList parts = firstLine.split(' ');
    if (parts.size() < 5 || parts.first() != "cpu") {
        return snapshot;
    }

    QList<quint64> values;
    for (int i = 1; i < parts.size(); ++i) {
        bool ok = false;
        const quint64 value = parts.at(i).toULongLong(&ok);
        if (ok) {
            values.append(value);
        }
    }

    for (quint64 value : values) {
        snapshot.total += value;
    }

    if (values.size() >= 4) {
        snapshot.idle = values.at(3);
    }
    if (values.size() >= 5) {
        snapshot.idle += values.at(4);
    }

    return snapshot;
}

double Monitor::calculateCpuUsage(const CpuSnapshot &previous, const CpuSnapshot &current)
{
    if (current.total <= previous.total || current.idle < previous.idle) {
        return 0.0;
    }

    const quint64 totalDelta = current.total - previous.total;
    const quint64 idleDelta = current.idle - previous.idle;
    if (totalDelta == 0) {
        return 0.0;
    }

    const double busy = static_cast<double>(totalDelta - idleDelta);
    return (busy / static_cast<double>(totalDelta)) * 100.0;
}

bool Monitor::parseMemInfo(const QString &memInfoContent, quint64 &totalKb, quint64 &availableKb)
{
    totalKb = 0;
    availableKb = 0;

    const QStringList lines = memInfoContent.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith("MemTotal:")) {
            totalKb = readFirstNumberFromLine(line);
        } else if (line.startsWith("MemAvailable:")) {
            availableKb = readFirstNumberFromLine(line);
        }
    }

    return totalKb > 0 && availableKb > 0 && availableKb <= totalKb;
}

bool Monitor::parseProcStatus(const QString &statusContent, quint64 &vmRssKb, quint64 &vmSizeKb)
{
    vmRssKb = 0;
    vmSizeKb = 0;

    const QStringList lines = statusContent.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith("VmRSS:")) {
            vmRssKb = readFirstNumberFromLine(line);
        } else if (line.startsWith("VmSize:")) {
            vmSizeKb = readFirstNumberFromLine(line);
        }
    }

    return vmRssKb > 0 || vmSizeKb > 0;
}

ResourceStats Monitor::collect()
{
    static CpuSnapshot previousSnapshot;
    static bool hasPreviousSnapshot = false;

    ResourceStats stats;

    QFile statFile("/proc/stat");
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const CpuSnapshot currentSnapshot = readCpuSnapshot(QString::fromUtf8(statFile.readAll()));
        if (hasPreviousSnapshot) {
            stats.cpuUsagePercent = calculateCpuUsage(previousSnapshot, currentSnapshot);
        }
        previousSnapshot = currentSnapshot;
        hasPreviousSnapshot = true;
    }

    QFile memInfoFile("/proc/meminfo");
    if (memInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        quint64 availableKb = 0;
        if (parseMemInfo(QString::fromUtf8(memInfoFile.readAll()), stats.totalMemoryKb, availableKb)) {
            stats.usedMemoryKb = stats.totalMemoryKb - availableKb;
            if (stats.totalMemoryKb > 0) {
                stats.memoryUsagePercent =
                    (static_cast<double>(stats.usedMemoryKb) / static_cast<double>(stats.totalMemoryKb)) * 100.0;
            }
        }
    }

    QFile procStatusFile("/proc/self/status");
    if (procStatusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        parseProcStatus(QString::fromUtf8(procStatusFile.readAll()),
                        stats.processResidentKb,
                        stats.processVirtualKb);
    }

    return stats;
}
