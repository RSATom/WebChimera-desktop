#include <QDebug>

#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QCommandLineParser>
#include <QSettings>
#include <QDir>

#include <QmlVlc.h>
#include <QmlVlc/QmlVlcConfig.h>

#include "AppConfig.h"

#define PROTOCOL "webchimera"
#define PROTOCOL_DESC "WebChimera URI"

bool registerProtocol()
{
#ifdef Q_OS_WIN
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
#else
    return false;
#endif
}

void applyConfig( QQmlApplicationEngine* engine, const QUrl& configUrl, const QVariantMap& options )
{
    auto it = options.find( QStringLiteral( "qmlsrc" ) );
    if( it != options.end() && it.value().type() == QMetaType::QString ) {
        QUrl qmlsrc = configUrl.resolved( QUrl( it.value().toString() ) );
        qDebug() << qmlsrc;
        engine->load( qmlsrc );
    }
}

int main( int argc, char *argv[] )
{
    RegisterQmlVlc();

    QGuiApplication app( argc, argv );

    QCommandLineParser parser;
    parser.addPositionalArgument(
        "config",
        QCoreApplication::translate( "Application", "config file" ) );

    QCommandLineOption registerOption( QStringList() << "r" << "register" );
    parser.addOption( registerOption );

    parser.process( app );

    if( parser.isSet( registerOption ) )
        return registerProtocol() ? 0 : -1;

    const QStringList args = parser.positionalArguments();
    if( args.size() != 1 )
        return -1; // FIXME! show error message

    const QUrl configFileUrl =
        QUrl::fromUserInput( args[0], app.applicationDirPath(), QUrl::AssumeLocalFile );
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
