#include "AppConfig.h"

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

enum {
    MAX_CONFIG_SIZE = 4096,
};

AppConfig::AppConfig( QObject *parent ) :
    QObject( parent ), m_networkManager( nullptr ), m_networkReply( nullptr )
{
}

AppConfig::~AppConfig()
{
}

bool AppConfig::parseConfigData( const QByteArray& configData )
{
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson( configData, &parseError );
    if( QJsonParseError::NoError != parseError.error || jsonDoc.isNull() ) {
        qDebug() << parseError.errorString();
        return false;
    }

    if( !jsonDoc.isObject() )
        return false;

    // FIXME! convert keys to lower case
    m_options = jsonDoc.object().toVariantMap();

    return true;
}

void AppConfig::networkDataReady()
{
    if( !m_networkManager || !m_networkReply || !m_configData ) {
        Q_ASSERT( false );
        return;
    }

    if( m_configData->size() + m_networkReply->bytesAvailable() < MAX_CONFIG_SIZE )
        m_configData->append( m_networkReply->readAll() );
    else
        m_networkReply->abort();
}

void AppConfig::downloadFinished()
{
    if( !m_networkManager || !m_networkReply || !m_configData ) {
        Q_ASSERT( false );
        return;
    }

    if( QNetworkReply::NoError == m_networkReply->error() &&
        parseConfigData( *m_configData ) )
    {
        Q_EMIT loadFinished( m_networkReply->url(), m_options );
    } else {
        qDebug() << m_networkReply->errorString();
        Q_EMIT loadError();
    }

    m_networkReply->deleteLater();
    m_networkReply = nullptr;
    m_networkManager->deleteLater();
    m_networkManager = nullptr;
    m_configData.reset();
}

void AppConfig::loadConfig( const QUrl& configFile )
{
    Q_ASSERT( !m_networkManager && !m_networkReply && !m_configData );

    if( m_networkManager || m_networkReply || m_configData )
        return;

    if( configFile.isLocalFile() ) {
        QFile jsonFile( configFile.toLocalFile() );
        if( jsonFile.open( QFile::ReadOnly ) &&
            parseConfigData( jsonFile.readAll() ) )
        {
            Q_EMIT loadFinished( configFile, m_options );
        } else {
            qDebug() << configFile.toLocalFile();
            qDebug() << jsonFile.errorString();
            Q_EMIT loadError();
        }
    } else {
        m_configData.reset( new QByteArray );
        m_configData->reserve( MAX_CONFIG_SIZE );

        m_networkManager = new QNetworkAccessManager( this );
        m_networkReply = m_networkManager->get( QNetworkRequest( configFile ) );
        m_networkReply->setReadBufferSize( 4096 );

        connect( m_networkReply, &QNetworkReply::readyRead,
                 this, &AppConfig::networkDataReady );
        connect( m_networkReply, &QNetworkReply::finished,
                 this, &AppConfig::downloadFinished );
    }
}

const QVariantMap& AppConfig::configOptions() const
{
    return m_options;
}
