/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include "keychain_p.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <QDebug>

using namespace QKeychain;

template <typename T>
struct Releaser {
    explicit Releaser( const T& v ) : value( v ) {}
    ~Releaser() {
        CFRelease( value );
    }

    const T value;
};

static QString strForStatus( OSStatus os ) {
    const Releaser<CFStringRef> str( SecCopyErrorMessageString( os, 0 ) );
    const char * const buf = CFStringGetCStringPtr( str.value,  kCFStringEncodingUTF8 );
    if ( !buf )
        return QObject::tr( "%1 (OSStatus %2)" )
                .arg( "OSX Keychain Error" ).arg( os );
    return QObject::tr( "%1 (OSStatus %2)" )
            .arg( QString::fromUtf8( buf, strlen( buf ) ) ).arg( os );
}

static OSStatus readPw( QByteArray* pw,
                        const QString& service,
                        const QString& account,
                        SecKeychainItemRef* ref ) {
    Q_ASSERT( pw );
    pw->clear();
    const QByteArray serviceData = service.toUtf8();
    const QByteArray accountData = account.toUtf8();

    void* data = 0;
    UInt32 len = 0;

    const OSStatus ret = SecKeychainFindGenericPassword( NULL, // default keychain
                                                         serviceData.size(),
                                                         serviceData.constData(),
                                                         accountData.size(),
                                                         accountData.constData(),
                                                         &len,
                                                         &data,
                                                         ref );
    if ( ret == noErr ) {
        *pw = QByteArray( reinterpret_cast<const char*>( data ), len );
        const OSStatus ret2 = SecKeychainItemFreeContent ( 0, data );
        if ( ret2 != noErr )
            qWarning() << "Could not free item content: " << strForStatus( ret2 );
    }
    return ret;
}

void ReadPasswordJobPrivate::scheduledStart()
{
    QString errorString;
    Error error = NoError;
    const OSStatus ret = readPw( &data, q->service(), q->key(), 0 );

    switch ( ret ) {
    case noErr:
        break;
    case errSecItemNotFound:
        errorString = tr("Password not found");
        error = EntryNotFound;
        break;
    default:
        errorString = strForStatus( ret );
        error = OtherError;
        break;
    }
    q->emitFinishedWithError( error, errorString );
}


static QKeychain::Error deleteEntryImpl( const QString& service, const QString& account, QString* err ) {
    SecKeychainItemRef ref;
    QByteArray pw;
    const OSStatus ret1 = readPw( &pw, service, account, &ref );
    if ( ret1 == errSecItemNotFound )
        return NoError; // No item stored, we're done
    if ( ret1 != noErr ) {
        *err = strForStatus( ret1 );
        //TODO map error code, set errstr
        return OtherError;
    }
    const Releaser<SecKeychainItemRef> releaser( ref );

    const OSStatus ret2 = SecKeychainItemDelete( ref );

    if ( ret2 == noErr )
        return NoError;
    //TODO map error code
    *err = strForStatus( ret2 );
    return CouldNotDeleteEntry;
}

static QKeychain::Error writeEntryImpl( const QString& service,
                                        const QString& account,
                                        const QByteArray& data,
                                        QString* err ) {
    Q_ASSERT( err );
    err->clear();
    const QByteArray serviceData = service.toUtf8();
    const QByteArray accountData = account.toUtf8();
    const OSStatus ret = SecKeychainAddGenericPassword( NULL, //default keychain
                                                        serviceData.size(),
                                                        serviceData.constData(),
                                                        accountData.size(),
                                                        accountData.constData(),
                                                        data.size(),
                                                        data.constData(),
                                                        NULL //item reference
                                                        );
    if ( ret != noErr ) {
        switch ( ret ) {
        case errSecDuplicateItem:
        {
            Error derr = deleteEntryImpl( service, account, err );
            if ( derr != NoError )
                return CouldNotDeleteEntry;
            else
                return writeEntryImpl( service, account, data, err );
        }
        default:
            *err = strForStatus( ret );
            return OtherError;
        }
    }

    return NoError;
}

void WritePasswordJobPrivate::scheduledStart()
{
    QString errorString;
    Error error = NoError;

    error = writeEntryImpl( q->service(), key, data, &errorString );
    q->emitFinishedWithError( error, errorString );
}

void DeletePasswordJobPrivate::scheduledStart()
{
    QString errorString;
    Error error = NoError;

    const Error derr = deleteEntryImpl( q->service(), key, &errorString );
    if ( derr != NoError )
        error = CouldNotDeleteEntry;
    q->emitFinishedWithError( error, errorString );
}
