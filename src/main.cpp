#include <QDebug>

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QCommandLineParser>
#include <QSettings>
#include <QDir>

#include <QmlVlc.h>
#include <QmlVlc/QmlVlcConfig.h>

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif

#include "AppConfig.h"

#ifndef Q_OS_ANDROID
#include <QtWebEngine>
#endif

#define PROTOCOL "webchimera"
#define PROTOCOL_DESC "WebChimera URI"

#ifdef Q_OS_WIN

bool registerProtocol()
{
    QSettings protocolSettings( QStringLiteral( "HKEY_CLASSES_ROOT\\" QT_UNICODE_LITERAL( PROTOCOL ) ),
                                QSettings::NativeFormat );
    protocolSettings.setValue( QStringLiteral( "." ), QStringLiteral( PROTOCOL_DESC ) );
    protocolSettings.setValue( QStringLiteral( "URL Protocol" ), QStringLiteral( "" ) );
    protocolSettings.setValue( QStringLiteral( "shell/." ), QStringLiteral( "open" ) );
    protocolSettings.setValue( QStringLiteral( "shell/open/command/." ),
                               QStringLiteral( "\"%1\" \"%2\"" ).arg(
                                   QDir::toNativeSeparators( qApp->applicationFilePath() ),
                                   QStringLiteral( "%1" ) ) );

    return protocolSettings.status() == QSettings::NoError;
}

#else

bool registerProtocol()
{
    return false;
}

#endif

#ifdef Q_OS_ANDROID

bool ParseStartupAgruments(QString* configFile, QString* error)
{
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if(!activity.isValid()) {
        *error = QStringLiteral("Fail get activity");
        return false;
    }

    QAndroidJniObject intent = activity.callObjectMethod("getIntent", "()Landroid/content/Intent;");
    if(!intent.isValid()) {
        *error = QStringLiteral("Fail get intent");
        return false;
    }

    QAndroidJniObject data = intent.callObjectMethod("getData", "()Landroid/net/Uri;");
    if(!data.isValid()) {
        *error = QStringLiteral("Fail get intent data");
        return false;
    }

    *configFile = data.toString();
    return true;
}

#else

bool ParseStartupAgruments(QString* configFile, QString* error)
{
    Q_ASSERT(configFile);
    if(!configFile)
        return false;

    QCommandLineParser parser;
    parser.addPositionalArgument(
        "config",
        QCoreApplication::translate("Application", "config file"));

#ifdef Q_OS_WIN
    QCommandLineOption registerOption(QStringList() << "r" << "register");
    parser.addOption(registerOption);
#endif

    parser.process(*qApp);

#ifdef Q_OS_WIN
    if(parser.isSet(registerOption)) {
        registerProtocol();
        return true;
    }
#endif

    const QStringList args = parser.positionalArguments();
    switch(args.size()) {
    case 1:
        *configFile = args[0];
        break;
    default:
        if(error)
            *error = QObject::tr("Invalid command line arguments count");
        return false;
    }

    if(configFile->isEmpty()) {
        if(error)
            *error = QObject::tr("Missing configuration file");
        return false;
    }

    return true;
}

#endif

void showError(QQmlApplicationEngine* engine, const QString& error)
{
    engine->rootContext()->setContextProperty(QStringLiteral("error"), error);
    engine->load(QUrl(QStringLiteral("qrc:/error.qml")));
}

void applyConfig( QQmlApplicationEngine* engine, const QUrl& configUrl, const QVariantMap& options )
{
    auto it = options.find( QStringLiteral( "qmlsrc" ) );
    if( it != options.end() && it.value().type() == QVariant::String ) {
        QUrl qmlsrc = configUrl.resolved( QUrl( it.value().toString() ) );
        qDebug() << qmlsrc;
        engine->load( qmlsrc );
    }
}

int main( int argc, char *argv[] )
{
    QString error;

#ifdef Q_OS_ANDROID
    QAndroidJniObject::callStaticMethod<void>(
        "java/lang/System",
        "load",
        "(Ljava/lang/String;)V",
        QAndroidJniObject::fromString( "libvlcjni.so" ).object<jstring>() );

    QAndroidJniEnvironment env;
    if( env->ExceptionCheck() ) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return -1; // FIXME! show error message
    }
#endif

    RegisterQmlVlc();

    QGuiApplication app( argc, argv );

#ifndef Q_OS_ANDROID
    QtWebEngine::initialize();
#endif

    QQmlApplicationEngine engine;

    AppConfig config;
    QObject::connect(&config, &AppConfig::loadFinished,
        [&engine] (const QUrl& configUrl, const QVariantMap& options) {
            applyConfig(&engine, configUrl, options);
        }
    );
    QObject::connect(&config, &AppConfig::loadError,
         [&engine] (const QString& error) {
            showError(&engine, QStringLiteral("Fail load config file:\n%1").arg(error));
         }
    );

    QString configFileArg;
    if(ParseStartupAgruments(&configFileArg, &error)) {
        if(configFileArg.startsWith(QStringLiteral(PROTOCOL QT_UNICODE_LITERAL(":")), Qt::CaseInsensitive))
            configFileArg = configFileArg.right(configFileArg.size() - sizeof(PROTOCOL));

        const QUrl configFileUrl =
            QUrl::fromUserInput(configFileArg, app.applicationDirPath(), QUrl::AssumeLocalFile);

        if(configFileUrl.isValid())
            config.loadConfig(configFileUrl);
        else
            error = QObject::tr("Invalid config file URL");
    }

    if(!error.isEmpty())
        showError(&engine, error);

    return app.exec();
}
