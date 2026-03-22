#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLabel;
class QProgressBar;
class QTextEdit;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshStats();

private:
    QLabel *cpuLabel;
    QLabel *memoryLabel;
    QLabel *processLabel;
    QProgressBar *cpuBar;
    QProgressBar *memoryBar;
    QTextEdit *detailText;
    QTimer refreshTimer;
};

#endif // MAINWINDOW_H
