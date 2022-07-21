#-------------------------------------------------
#
# Project created by QtCreator 2022-06-08T15:05:21
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = SummerProject_old
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    dialogcoredir.cpp \
    qcustomplot.cpp

HEADERS += \
        mainwindow.h \
    dialogcoredir.h \
    ../qcustomplot/qcustomplot.h \
    qcustomplot.h

FORMS += \
        mainwindow.ui \
    dialogcoredir.ui

RESOURCES += \
    resources.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../papi/src/release/ -lpapi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../papi/src/debug/ -lpapi
else:unix: LIBS += -L$$PWD/../../../papi/src/ -lpapi

INCLUDEPATH += $$PWD/../../../papi/src
DEPENDPATH += $$PWD/../../../papi/src

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../papi/src/release/libpapi.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../papi/src/debug/libpapi.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../papi/src/release/papi.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../papi/src/debug/papi.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../../papi/src/libpapi.a
