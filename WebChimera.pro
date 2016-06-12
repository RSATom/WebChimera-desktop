QT += qml quick

include(deps/QmlVlc/QmlVlc.pri)

INCLUDEPATH += deps
INCLUDEPATH += deps/libvlc-sdk/include

CONFIG += c++11

include(src/src.pri)

android {
    QT += androidextras

    OTHER_FILES += android/dependencies.qml

    LIBS += -L$$PWD/android/libs/armeabi-v7a -lvlcjni

    DISTFILES += android/AndroidManifest.xml

    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

linux-rasp-pi2-g++ {
    QMAKE_RPATHDIR += lib
}
