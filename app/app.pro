QT += widgets core
CONFIG += c++17 warn_on
TARGET = qt_linux_resource_monitor
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    monitor.cpp

HEADERS += \
    mainwindow.h \
    monitor.h
