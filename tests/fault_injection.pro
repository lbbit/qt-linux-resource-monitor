QT += testlib core
CONFIG += testcase c++17 console warn_on
TEMPLATE = app
TARGET = tst_fault_injection

INCLUDEPATH += ../app
DEPENDPATH += ../app

SOURCES += \
    test_fault_injection.cpp
