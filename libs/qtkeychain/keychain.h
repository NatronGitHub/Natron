/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#ifndef KEYCHAIN_H
#define KEYCHAIN_H

#include "qkeychain_export.h"

#include <QtCore/QObject>
#include <QtCore/QString>

class QSettings;

#define QTKEYCHAIN_VERSION 0x000100

namespace QKeychain {

/**
 * Error codes
 */
enum Error {
    NoError=0, /**< No error occurred, operation was successful */
    EntryNotFound, /**< For the given key no data was found */
    CouldNotDeleteEntry, /**< Could not delete existing secret data */
    AccessDeniedByUser, /**< User denied access to keychain */
    AccessDenied, /**< Access denied for other reasons */
    NoBackendAvailable, /**< No platform-specific keychain service available */
    NotImplemented, /**< Not implemented on platform */
    OtherError /**< Something else went wrong (errorString() might provide details) */
};

class JobExecutor;
class JobPrivate;

class QKEYCHAIN_EXPORT Job : public QObject {
    Q_OBJECT
public:    
    ~Job();

    QSettings* settings() const;
    void setSettings( QSettings* settings );

    void start();

    QString service() const;

    Error error() const;
    QString errorString() const;

    bool autoDelete() const;
    void setAutoDelete( bool autoDelete );

    bool insecureFallback() const;
    void setInsecureFallback( bool insecureFallback );

    QString key() const;
    void setKey( const QString& key );

Q_SIGNALS:
    void finished( QKeychain::Job* );

protected:
    explicit Job( JobPrivate *q, QObject* parent=0 );
    Q_INVOKABLE void doStart();

private:
    void setError( Error error );
    void setErrorString( const QString& errorString );
    void emitFinished();
    void emitFinishedWithError(Error, const QString& errorString);

    void scheduledStart();

protected:
    JobPrivate* const d;

friend class JobExecutor;
friend class JobPrivate;
friend class ReadPasswordJobPrivate;
friend class WritePasswordJobPrivate;
friend class DeletePasswordJobPrivate;
};

class ReadPasswordJobPrivate;

class QKEYCHAIN_EXPORT ReadPasswordJob : public Job {
    Q_OBJECT
public:
    explicit ReadPasswordJob( const QString& service, QObject* parent=0 );
    ~ReadPasswordJob();

    QByteArray binaryData() const;
    QString textData() const;

private:
    friend class QKeychain::ReadPasswordJobPrivate;
};

class WritePasswordJobPrivate;

class QKEYCHAIN_EXPORT WritePasswordJob : public Job {
    Q_OBJECT
public:
    explicit WritePasswordJob( const QString& service, QObject* parent=0 );
    ~WritePasswordJob();

    void setBinaryData( const QByteArray& data );
    void setTextData( const QString& data );

private:

    friend class QKeychain::WritePasswordJobPrivate;
};

class DeletePasswordJobPrivate;

class QKEYCHAIN_EXPORT DeletePasswordJob : public Job {
    Q_OBJECT
public:
    explicit DeletePasswordJob( const QString& service, QObject* parent=0 );
    ~DeletePasswordJob();

private:
    friend class QKeychain::DeletePasswordJobPrivate;
};

} // namespace QtKeychain

#endif
