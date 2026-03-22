#include "mainwindow.h"
#include "monitor.h"

#include <QFormLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      cpuLabel(new QLabel(this)),
      memoryLabel(new QLabel(this)),
      processLabel(new QLabel(this)),
      cpuBar(new QProgressBar(this)),
      memoryBar(new QProgressBar(this)),
      detailText(new QTextEdit(this))
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    auto *formLayout = new QFormLayout();

    cpuBar->setRange(0, 100);
    memoryBar->setRange(0, 100);
    detailText->setReadOnly(true);

    formLayout->addRow("CPU 使用率", cpuLabel);
    formLayout->addRow("内存使用率", memoryLabel);
    formLayout->addRow("进程内存", processLabel);

    layout->addLayout(formLayout);
    layout->addWidget(cpuBar);
    layout->addWidget(memoryBar);
    layout->addWidget(detailText);

    setCentralWidget(central);
    setWindowTitle("Qt Linux 资源监控器");
    resize(640, 360);

    connect(&refreshTimer, &QTimer::timeout, this, &MainWindow::refreshStats);
    refreshTimer.start(1000);
    refreshStats();
}

void MainWindow::refreshStats()
{
    const ResourceStats stats = Monitor::collect();

    cpuLabel->setText(QString::number(stats.cpuUsagePercent, 'f', 2) + "%");
    memoryLabel->setText(QString("%1% (%2 MB / %3 MB)")
                             .arg(QString::number(stats.memoryUsagePercent, 'f', 2))
                             .arg(QString::number(stats.usedMemoryKb / 1024.0, 'f', 2))
                             .arg(QString::number(stats.totalMemoryKb / 1024.0, 'f', 2)));
    processLabel->setText(QString("RSS %1 MB, VSZ %2 MB")
                              .arg(QString::number(stats.processResidentKb / 1024.0, 'f', 2))
                              .arg(QString::number(stats.processVirtualKb / 1024.0, 'f', 2)));

    cpuBar->setValue(static_cast<int>(stats.cpuUsagePercent));
    memoryBar->setValue(static_cast<int>(stats.memoryUsagePercent));

    detailText->setPlainText(
        QString("Linux /proc 采样详情\n"
                "- CPU: 读取 /proc/stat，两次采样做差分计算 busy/total\n"
                "- 系统内存: 读取 /proc/meminfo，使用 MemTotal 与 MemAvailable\n"
                "- 进程内存: 读取 /proc/self/status，显示 VmRSS / VmSize\n\n"
                "当前快照\n"
                "cpuUsagePercent=%1\n"
                "memoryUsagePercent=%2\n"
                "totalMemoryKb=%3\n"
                "usedMemoryKb=%4\n"
                "processResidentKb=%5\n"
                "processVirtualKb=%6")
            .arg(QString::number(stats.cpuUsagePercent, 'f', 2))
            .arg(QString::number(stats.memoryUsagePercent, 'f', 2))
            .arg(stats.totalMemoryKb)
            .arg(stats.usedMemoryKb)
            .arg(stats.processResidentKb)
            .arg(stats.processVirtualKb));
}
