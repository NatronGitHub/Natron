//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "FileDownloader.h"


FileDownloader::FileDownloader(QUrl imageUrl, QObject *parent)
: QObject(parent)
, m_reply(0)
{
    connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
                SLOT(fileDownloaded(QNetworkReply*)));

    QNetworkRequest request(imageUrl);
    m_reply = m_WebCtrl.get(request);
    QObject::connect(m_reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SIGNAL(error()));
}

FileDownloader::~FileDownloader()
{

}

QNetworkReply* FileDownloader::getReply() const
{
    return m_reply;
}

void FileDownloader::fileDownloaded(QNetworkReply* pReply)
{
    m_DownloadedData = pReply->readAll();
    //emit a signal
    pReply->deleteLater();
    emit downloaded();
}

QByteArray FileDownloader::downloadedData() const
{
    return m_DownloadedData;
}
