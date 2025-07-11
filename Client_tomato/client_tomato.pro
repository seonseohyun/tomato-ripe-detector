QT       += core gui
QT += core gui widgets network
QT += core gui widgets charts
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    login.cpp \
    server_connector.cpp \
    tomato_history.cpp \
    chart.cpp

HEADERS += \
    common_struct.h \
    mainwindow.h \
    login.h \
    server_connector.h \
    tomato_history.h \
    chart.h

FORMS += \
    mainwindow.ui \
    login.ui \
    tomato_history.ui \
    chart.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc
