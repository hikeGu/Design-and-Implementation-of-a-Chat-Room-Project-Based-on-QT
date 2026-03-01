QT       += core gui sql network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aichatclient.cpp \
    chatmanager.cpp \
    databasemanager.cpp \
    filetransfermanager.cpp \
    filetransferserver.cpp \
    main.cpp \
    mainwindow.cpp \
    server.cpp \
    usermanager.cpp \
    voicechatserver.cpp

HEADERS += \
    aichatclient.h \
    chatmanager.h \
    databasemanager.h \
    filetransfermanager.h \
    filetransferserver.h \
    mainwindow.h \
    server.h \
    usermanager.h \
    voicechatserver.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
