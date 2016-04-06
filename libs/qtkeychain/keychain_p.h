/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#ifndef KEYCHAIN_P_H
#define KEYCHAIN_P_H

#include <QCoreApplication>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QQueue>

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)

#include <QDBusPendingCallWatcher>

#include "kwallet_interface.h"
#else

class QDBusPendingCallWatcher;

#endif

#include "keychain.h"

namespace QKeychain {

class JobExecutor;

class JobPrivate : public QObject {
    Q_OBJECT
public:
    enum Mode {
	Text,
	Binary
    };

    virtual void scheduledStart() = 0;

    static QString modeToString(Mode m);
    static Mode stringToMode(const QString& s);

    Mode mode;

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    org::kde::KWallet* iface;
    int walletHandle;

    static void gnomeKeyring_readCb( int result, const char* string, JobPrivate* data );
    static void gnomeKeyring_writeCb( int result, JobPrivate* self );

    virtual void fallbackOnError(const QDBusError& err) = 0;

protected Q_SLOTS:
    void kwalletWalletFound( QDBusPendingCallWatcher* watcher );
    virtual void kwalletFinished( QDBusPendingCallWatcher* watcher );
    virtual void kwalletOpenFinished( QDBusPendingCallWatcher* watcher );
#else
    void kwalletWalletFound(QDBusPendingCallWatcher *watcher) {}
    virtual void kwalletFinished( QDBusPendingCallWatcher* watcher ) {}
    virtual void kwalletOpenFinished( QDBusPendingCallWatcher* watcher ) {}
#endif

protected:
    JobPrivate( const QString& service_, Job *q );

protected:
    QKeychain::Error error;
    QString errorString;
    QString service;
    bool autoDelete;
    bool insecureFallback;
    QPointer<QSettings> settings;
    QString key;
    Job* const q;
    QByteArray data;

friend class Job;
friend class JobExecutor;
friend class ReadPasswordJob;
friend class WritePasswordJob;
};

class ReadPasswordJobPrivate : public JobPrivate {
    Q_OBJECT
public:
    explicit ReadPasswordJobPrivate( const QString &service_, ReadPasswordJob* qq );
    void scheduledStart();

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    void fallbackOnError(const QDBusError& err);

private Q_SLOTS:
    void kwalletOpenFinished( QDBusPendingCallWatcher* watcher );
    void kwalletEntryTypeFinished( QDBusPendingCallWatcher* watcher );
    void kwalletFinished( QDBusPendingCallWatcher* watcher );
#else //moc's too dumb to respect above macros, so just define empty slot implementations
private Q_SLOTS:
    void kwalletOpenFinished( QDBusPendingCallWatcher* ) {}
    void kwalletEntryTypeFinished( QDBusPendingCallWatcher* ) {}
    void kwalletFinished( QDBusPendingCallWatcher* ) {}
#endif

    friend class ReadPasswordJob;
};

class WritePasswordJobPrivate : public JobPrivate {
    Q_OBJECT
public:
    explicit WritePasswordJobPrivate( const QString &service_, WritePasswordJob* qq );
    void scheduledStart();

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    void fallbackOnError(const QDBusError& err);
#endif

    friend class WritePasswordJob;
};

class DeletePasswordJobPrivate : public JobPrivate {
    Q_OBJECT
public:
    explicit DeletePasswordJobPrivate( const QString &service_, DeletePasswordJob* qq );

    void scheduledStart();

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    void fallbackOnError(const QDBusError& err);
#endif

protected:
    void doStart();

    friend class DeletePasswordJob;
};

class JobExecutor : public QObject {
    Q_OBJECT
public:

    static JobExecutor* instance();

    void enqueue( Job* job );

private:
    explicit JobExecutor();
    void startNextIfNoneRunning();

private Q_SLOTS:
    void jobFinished( QKeychain::Job* );
    void jobDestroyed( QObject* object );

private:
    static JobExecutor* s_instance;
    QQueue<QPointer<Job> > m_queue;
    bool m_jobRunning;
};

}

#endif // KEYCHAIN_P_H
