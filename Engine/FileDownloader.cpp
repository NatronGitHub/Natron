/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "FileDownloader.h"


FileDownloader::FileDownloader(QUrl imageUrl,
                               QObject *parent)
    : QObject(parent)
      , m_reply(0)
{
    connect( &m_WebCtrl, SIGNAL( finished(QNetworkReply*) ),
             SLOT( fileDownloaded(QNetworkReply*) ) );

    QNetworkRequest request(imageUrl);
    m_reply = m_WebCtrl.get(request);
    QObject::connect( m_reply,SIGNAL( error(QNetworkReply::NetworkError) ),this,SIGNAL( error() ) );
}

FileDownloader::~FileDownloader()
{
}

QNetworkReply*
FileDownloader::getReply() const
{
    return m_reply;
}

void
FileDownloader::fileDownloaded(QNetworkReply* pReply)
{
    m_DownloadedData = pReply->readAll();
    //Q_EMIT a signal
    pReply->deleteLater();
    Q_EMIT downloaded();
}

QByteArray
FileDownloader::downloadedData() const
{
    return m_DownloadedData;
}

