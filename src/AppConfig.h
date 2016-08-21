#pragma once

#include <QObject>
#include <QVariantMap>

QT_FORWARD_DECLARE_CLASS( QNetworkAccessManager )
QT_FORWARD_DECLARE_CLASS( QNetworkReply )

class AppConfig : public QObject
{
    Q_OBJECT
public:
    explicit AppConfig( QObject *parent = 0 );
    ~AppConfig();

    void loadConfig( const QUrl& configFile );

    const QVariantMap& configOptions() const;

Q_SIGNALS:
    void loadFinished( const QUrl& configUrl, const QVariantMap& options );
    void loadError(const QString& error);

private Q_SLOTS:
    void networkDataReady();
    void downloadFinished();

private:
    bool parseConfigData( const QByteArray& configData );

private:
    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_networkReply;
    QScopedPointer<QByteArray> m_configData;

    QVariantMap m_options;
};
