TEMPLATE = app

QT += qml quick core gui dbus opengl
CONFIG += c++11

SOURCES += qmlapp.cpp \
    object.cpp \
    parser.cpp \
    appsettings.cpp

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    qmlapp.h \
    appsettings.h

RESOURCES +=

