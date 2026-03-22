#include <QtTest>
#include <cstddef>

namespace {
volatile int g_sink = 0;

__attribute__((noinline)) void triggerLeak()
{
    char *ptr = new char[128];
    ptr[0] = 'L';
    g_sink += ptr[0];
    (void)ptr;
}

__attribute__((noinline)) void triggerUseAfterFree()
{
    char *ptr = new char[16];
    ptr[0] = 'U';
    delete[] ptr;
    g_sink += ptr[0];
}

__attribute__((noinline)) void triggerHeapOverflow()
{
    char *ptr = new char[8];
    for (std::size_t i = 0; i <= 8; ++i) {
        ptr[i] = static_cast<char>('A' + i);
    }
    g_sink += ptr[8];
    delete[] ptr;
}
}

class FaultInjectionTest : public QObject
{
    Q_OBJECT

private slots:
    void noFault();
    void injectLeak();
    void injectUseAfterFree();
    void injectHeapOverflow();
};

void FaultInjectionTest::noFault()
{
    QVERIFY(true);
}

void FaultInjectionTest::injectLeak()
{
    if (qEnvironmentVariableIsSet("INJECT_LEAK")) {
        triggerLeak();
    }
    QVERIFY(true);
}

void FaultInjectionTest::injectUseAfterFree()
{
    if (qEnvironmentVariableIsSet("INJECT_USE_AFTER_FREE")) {
        triggerUseAfterFree();
    }
    QVERIFY(true);
}

void FaultInjectionTest::injectHeapOverflow()
{
    if (qEnvironmentVariableIsSet("INJECT_HEAP_OVERFLOW")) {
        triggerHeapOverflow();
    }
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(FaultInjectionTest)
#include "test_fault_injection.moc"
