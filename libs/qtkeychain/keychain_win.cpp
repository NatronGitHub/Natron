/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#include "keychain_p.h"

#include <QSettings>

#include <windows.h>
#include <wincrypt.h>

#include <memory>

using namespace QKeychain;

#if defined(USE_CREDENTIAL_STORE)
#include <wincred.h>

void ReadPasswordJobPrivate::scheduledStart() {
    LPCWSTR name = (LPCWSTR)key.utf16();
    //Use settings member if there, create local settings object if not
    std::auto_ptr<QSettings> local( !q->settings() ? new QSettings( q->service() ) : 0 );
    PCREDENTIALW cred;

    if (!CredReadW(name, CRED_TYPE_GENERIC, 0, &cred)) {
        Error error;
        QString msg;
        switch(GetLastError()) {
        case ERROR_NOT_FOUND:
            error = EntryNotFound;
            msg = tr("Password entry not found");
            break;
        default:
            error = OtherError;
            msg = tr("Could not decrypt data");
            break;
        }

        q->emitFinishedWithError( error, msg );
        return;
    }

    data = QByteArray((char*)cred->CredentialBlob, cred->CredentialBlobSize);
    CredFree(cred);

    q->emitFinished();
}

void WritePasswordJobPrivate::scheduledStart() {
    CREDENTIALW cred;
    char *pwd = data.data();
    LPWSTR name = (LPWSTR)key.utf16();

    memset(&cred, 0, sizeof(cred));
    cred.Comment = L"QtKeychain";
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = name;
    cred.CredentialBlobSize = data.size();
    cred.CredentialBlob = (LPBYTE)pwd;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

    if (!CredWriteW(&cred, 0)) {
        q->emitFinishedWithError( OtherError, tr("Encryption failed") ); //TODO more details available?
    } else {
        q->emitFinished();
    }
}

void DeletePasswordJobPrivate::scheduledStart() {
    LPCWSTR name = (LPCWSTR)key.utf16();

    if (!CredDeleteW(name, CRED_TYPE_GENERIC, 0)) {
        Error error;
        QString msg;
        switch(GetLastError()) {
        case ERROR_NOT_FOUND:
            error = EntryNotFound;
            msg = tr("Password entry not found");
            break;
        default:
            error = OtherError;
            msg = tr("Could not decrypt data");
            break;
        }

        q->emitFinishedWithError( error, msg );
    } else {
        q->emitFinished();
    }
}
#else
void ReadPasswordJobPrivate::scheduledStart() {
    //Use settings member if there, create local settings object if not
    std::auto_ptr<QSettings> local( !q->settings() ? new QSettings( q->service() ) : 0 );
    QSettings* actual = q->settings() ? q->settings() : local.get();

    QByteArray encrypted = actual->value( key ).toByteArray();
    if ( encrypted.isNull() ) {
        q->emitFinishedWithError( EntryNotFound, tr("Entry not found") );
        return;
    }

    DATA_BLOB blob_in, blob_out;

    blob_in.pbData = reinterpret_cast<BYTE*>( encrypted.data() );
    blob_in.cbData = encrypted.size();

    const BOOL ret = CryptUnprotectData( &blob_in,
                                        NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         0,
                                         &blob_out );
    if ( !ret ) {
        q->emitFinishedWithError( OtherError, tr("Could not decrypt data") );
        return;
    }

    data = QByteArray( reinterpret_cast<char*>( blob_out.pbData ), blob_out.cbData );
    SecureZeroMemory( blob_out.pbData, blob_out.cbData );
    LocalFree( blob_out.pbData );

    q->emitFinished();
}

void WritePasswordJobPrivate::scheduledStart() {
    DATA_BLOB blob_in, blob_out;
    blob_in.pbData = reinterpret_cast<BYTE*>( data.data() );
    blob_in.cbData = data.size();
    const BOOL res = CryptProtectData( &blob_in,
                                       L"QKeychain-encrypted data",
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       &blob_out );
    if ( !res ) {
        q->emitFinishedWithError( OtherError, tr("Encryption failed") ); //TODO more details available?
        return;
    }

    const QByteArray encrypted( reinterpret_cast<char*>( blob_out.pbData ), blob_out.cbData );
    LocalFree( blob_out.pbData );

    //Use settings member if there, create local settings object if not
    std::auto_ptr<QSettings> local( !q->settings() ? new QSettings( q->service() ) : 0 );
    QSettings* actual = q->settings() ? q->settings() : local.get();
    actual->setValue( key, encrypted );
    actual->sync();
    if ( actual->status() != QSettings::NoError ) {

        const QString errorString = actual->status() == QSettings::AccessError
                ? tr("Could not store encrypted data in settings: access error")
                : tr("Could not store encrypted data in settings: format error");
        q->emitFinishedWithError( OtherError, errorString );
        return;
    }

    q->emitFinished();
}

void DeletePasswordJobPrivate::scheduledStart() {
    //Use settings member if there, create local settings object if not
    std::auto_ptr<QSettings> local( !q->settings() ? new QSettings( q->service() ) : 0 );
    QSettings* actual = q->settings() ? q->settings() : local.get();
    actual->remove( key );
    actual->sync();
    if ( actual->status() != QSettings::NoError ) {
        const QString err = actual->status() == QSettings::AccessError
                ? tr("Could not delete encrypted data from settings: access error")
                : tr("Could not delete encrypted data from settings: format error");
        q->emitFinishedWithError( OtherError, err );
    } else {
        q->emitFinished();
    }
}
#endif
