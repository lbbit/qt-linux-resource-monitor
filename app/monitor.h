#ifndef MONITOR_H
#define MONITOR_H

#include <QString>

struct CpuSnapshot {
    quint64 total = 0;
    quint64 idle = 0;
};

struct ResourceStats {
    double cpuUsagePercent = 0.0;
    double memoryUsagePercent = 0.0;
    quint64 totalMemoryKb = 0;
    quint64 usedMemoryKb = 0;
    quint64 processResidentKb = 0;
    quint64 processVirtualKb = 0;
};

class Monitor
{
public:
    static CpuSnapshot readCpuSnapshot(const QString &statContent);
    static double calculateCpuUsage(const CpuSnapshot &previous, const CpuSnapshot &current);
    static bool parseMemInfo(const QString &memInfoContent, quint64 &totalKb, quint64 &availableKb);
    static bool parseProcStatus(const QString &statusContent, quint64 &vmRssKb, quint64 &vmSizeKb);
    static ResourceStats collect();
};

#endif // MONITOR_H
