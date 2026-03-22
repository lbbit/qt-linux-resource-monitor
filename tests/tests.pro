QT += testlib core
CONFIG += testcase c++17 console warn_on
TEMPLATE = app
TARGET = tst_monitor

INCLUDEPATH += ../app
DEPENDPATH += ../app

SOURCES += \
    test_monitor.cpp \
    ../app/monitor.cpp

HEADERS += \
    ../app/monitor.h
