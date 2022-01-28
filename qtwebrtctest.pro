TEMPLATE = app
TARGET = qtwebrtctest
QT += webenginewidgets websockets webchannel

HEADERS += \
	rtcwindow.h \
	websocketchannel.h

SOURCES += \
    main.cpp \
    rtcwindow.cpp \
    websocketchannel.cpp

RESOURCES += qtwebrtctest.qrc
