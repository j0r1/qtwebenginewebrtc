TEMPLATE = app
TARGET = qtwebrtctest
QT += webenginewidgets websockets webchannel

HEADERS += \
	rtcwindow.h

SOURCES += \
    main.cpp \
    rtcwindow.cpp

RESOURCES += qtwebrtctest.qrc
