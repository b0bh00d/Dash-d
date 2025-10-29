QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

mac {
    DEFINES += QT_OSX
}

unix:!mac {
    DEFINES += QT_LINUX
    #INCLUDEPATH += ../miniaudio
}

win32 {
    DEFINES += QT_WIN

    # for Registry API
    #LIBS += -ladvapi32

    # CONFIG(static) {
    #     # make sure we match the linkage for Crypto++
    #     CONFIG(debug, debug|release) {
    #         QMAKE_CFLAGS += /MTd
    #         QMAKE_CXXFLAGS += /MTd
    #     } else {
    #         QMAKE_CFLAGS += /MT
    #         QMAKE_CXXFLAGS += /MT
    #     }
    # }
}

#DEFINES += TEST

SOURCES += \
    ../SharedTypes.cpp \
    Dashboard.cpp \
    Domain.cpp \
    Receiver.cpp \
    Sensor.cpp \
    main.cpp \
    Dialog.cpp

HEADERS += \
    ../SharedTypes.h \
    Dashboard.h \
    Domain.h \
    Packet.h \
    Receiver.h \
    Sensor.h \
    Types.h \
    Dialog.h

FORMS += \
    Dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    dashboard.qrc
