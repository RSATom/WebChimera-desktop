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

#include <QtWebEngine>

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

QString ParseStartupAgruments()
{
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if( !activity.isValid() )
        return QString();

    QAndroidJniObject intent = activity.callObjectMethod( "getIntent", "()Landroid/content/Intent;" );
    if( !intent.isValid() )
        return QString();

    QAndroidJniObject data = intent.callObjectMethod( "getData", "()Landroid/net/Uri;" );
    if( !data.isValid() )
        return QString();

    return data.toString();
}

#else

QString ParseStartupAgruments()
{
    QCommandLineParser parser;
    parser.addPositionalArgument(
        "config",
        QCoreApplication::translate( "Application", "config file" ) );

#ifdef Q_OS_WIN
    QCommandLineOption registerOption( QStringList() << "r" << "register" );
    parser.addOption( registerOption );
#endif

    parser.process( *qApp );

#ifdef Q_OS_WIN
    if( parser.isSet( registerOption ) ) {
        registerProtocol();
        return QString();
    }
#endif

    const QStringList args = parser.positionalArguments();
    if( args.size() != 1 )
        return QString(); // FIXME! show error message

    return args[0];
}

#endif

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

    QtWebEngine::initialize();

    QString arg = ParseStartupAgruments();
    if( arg.isEmpty() )
        return 0;

    if( arg.startsWith( QStringLiteral( PROTOCOL QT_UNICODE_LITERAL( ":" ) ), Qt::CaseInsensitive ) ) {
        arg = arg.right( arg.size() - sizeof( PROTOCOL ) );
    }

    const QUrl configFileUrl =
        QUrl::fromUserInput( arg, app.applicationDirPath(), QUrl::AssumeLocalFile );

    if( !configFileUrl.isValid() )
        return -1; // FIXME! show error message

    QQmlApplicationEngine engine;

    AppConfig config;

    QObject::connect( &config, &AppConfig::loadFinished,
        [&engine] ( const QUrl& configUrl, const QVariantMap& options ) {
            applyConfig( &engine, configUrl, options );
        }
    );

    QObject::connect( &config, &AppConfig::loadError,
                      &app, QGuiApplication::quit );

    config.loadConfig( configFileUrl );

    return app.exec();
}
