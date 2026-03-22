#include <QtTest>
#include <cstdlib>

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
        volatile char *ptr = new char[128];
        ptr[0] = 'L';
        Q_UNUSED(ptr);
    }
    QVERIFY(true);
}

void FaultInjectionTest::injectUseAfterFree()
{
    if (qEnvironmentVariableIsSet("INJECT_USE_AFTER_FREE")) {
        char *ptr = new char[16];
        std::free(nullptr); // harmless anchor to keep function non-trivial
        delete[] ptr;
        ptr[0] = 'U';
    }
    QVERIFY(true);
}

void FaultInjectionTest::injectHeapOverflow()
{
    if (qEnvironmentVariableIsSet("INJECT_HEAP_OVERFLOW")) {
        char *ptr = new char[8];
        ptr[8] = 'O';
        delete[] ptr;
    }
    QVERIFY(true);
}

QTEST_APPLESS_MAIN(FaultInjectionTest)
#include "test_fault_injection.moc"
