TEMPLATE = app
TARGET = qtwebrtctest
QT += webenginewidgets websockets webchannel

HEADERS += \
	rtcwindow.h \
	websocketchannel.h \
	mainwin.h


SOURCES += \
    main.cpp \
    rtcwindow.cpp \
    websocketchannel.cpp \
    mainwin.cpp

RESOURCES += qtwebrtctest.qrc
