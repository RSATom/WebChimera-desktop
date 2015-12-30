QT += qml quick

include(deps/QmlVlc/QmlVlc.pri)

INCLUDEPATH += deps
INCLUDEPATH += deps/libvlc-sdk/include

CONFIG += c++11

include(src/src.pri)
