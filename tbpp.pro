QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport widgets

TARGET = tbpp
TEMPLATE = app

SOURCES += \
    backend.cpp \
    civil.cpp \
    color.cpp \
    globaltime.cpp \
    maingui.cpp \
    mainwindow.cpp \
    mesh.cpp \
    myopenglwidget.cpp \
    qcustomplot.cpp \
    serial.cpp \
    space.cpp \
    statwindow.cpp \
    utils.cpp \

HEADERS += \
    backend.h \
    civil.h \
    globaltime.h \
    mainwindow.h \
    myopenglwidget.h \
    qcustomplot.h \
    space.h \
    statwindow.h \
    utils.h \

FORMS += \
    mainwindow.ui \
    statwindow.ui \

LIBS += -lopengl32

QMAKE_CXXFLAGS += -O3 -frename-registers
