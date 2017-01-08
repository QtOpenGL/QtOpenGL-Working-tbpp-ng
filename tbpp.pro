QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

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
    space.cpp \
    utils.cpp \

HEADERS += \
    backend.h \
    civil.h \
    globaltime.h \
    mainwindow.h \
    myopenglwidget.h \
    space.h \
    utils.h \

FORMS += mainwindow.ui

LIBS += -lopengl32

QMAKE_CXXFLAGS += -O3 -frename-registers
