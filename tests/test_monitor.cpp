#include <QtTest>

#include "../app/monitor.h"

class MonitorTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesCpuSnapshot();
    void calculatesCpuUsage();
    void parsesMemInfo();
    void parsesProcStatus();
};

void MonitorTest::parsesCpuSnapshot()
{
    const QString content = "cpu  100 20 30 400 50 0 0 0 0 0\n";
    const CpuSnapshot snapshot = Monitor::readCpuSnapshot(content);
    QCOMPARE(snapshot.total, static_cast<quint64>(600));
    QCOMPARE(snapshot.idle, static_cast<quint64>(450));
}

void MonitorTest::calculatesCpuUsage()
{
    CpuSnapshot previous;
    previous.total = 100;
    previous.idle = 30;

    CpuSnapshot current;
    current.total = 180;
    current.idle = 50;

    const double usage = Monitor::calculateCpuUsage(previous, current);
    QVERIFY(qAbs(usage - 75.0) < 0.001);
}

void MonitorTest::parsesMemInfo()
{
    quint64 total = 0;
    quint64 available = 0;
    const QString content = "MemTotal:       16000000 kB\nMemFree:        1000000 kB\nMemAvailable:   6000000 kB\n";
    QVERIFY(Monitor::parseMemInfo(content, total, available));
    QCOMPARE(total, static_cast<quint64>(16000000));
    QCOMPARE(available, static_cast<quint64>(6000000));
}

void MonitorTest::parsesProcStatus()
{
    quint64 rss = 0;
    quint64 vsize = 0;
    const QString content = "Name:\ttest\nVmSize:\t   20480 kB\nVmRSS:\t   10240 kB\n";
    QVERIFY(Monitor::parseProcStatus(content, rss, vsize));
    QCOMPARE(rss, static_cast<quint64>(10240));
    QCOMPARE(vsize, static_cast<quint64>(20480));
}

QTEST_APPLESS_MAIN(MonitorTest)
#include "test_monitor.moc"
