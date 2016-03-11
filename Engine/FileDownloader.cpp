/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include <cassert>
#include <stdexcept>

#include <QTimer>
#include <QAbstractNetworkCache>

#define NATRON_FILE_DOWNLOAD_HEARBEAT_TIMEOUT_MS 5000

NATRON_NAMESPACE_ENTER;

FileDownloader::FileDownloader(const QUrl& imageUrl,
                               bool useNetworkCache,
                               QObject *parent)
    : QObject(parent)
    , m_reply(0)
    , m_timer(0)
{
    m_timer = new QTimer();
    m_timer->setInterval(NATRON_FILE_DOWNLOAD_HEARBEAT_TIMEOUT_MS);
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));
    
    connect( &m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
             SLOT(fileDownloaded(QNetworkReply*)) );
    
    QNetworkRequest request(imageUrl);
    if (!useNetworkCache) {
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    }
    
    m_reply = m_WebCtrl.get(request);
    m_timer->start();
    QObject::connect( m_reply,SIGNAL(error(QNetworkReply::NetworkError)),this,SIGNAL(error()) );
    QObject::connect( m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    
}

FileDownloader::~FileDownloader()
{
    delete m_timer;
    if (m_reply) {
        m_reply->deleteLater();
    }
}

QNetworkReply*
FileDownloader::getReply() const
{
    return m_reply;
}

void
FileDownloader::onTimerTimeout()
{
    if (m_reply) {
        m_reply->abort();
    }
}

void
FileDownloader::onDownloadProgress(qint64 /*bytesReceived*/, qint64 /*bytesTotal*/)
{
    assert(m_timer);
    m_timer->stop();
    m_timer->start();
}

void
FileDownloader::fileDownloaded(QNetworkReply* pReply)
{
    m_DownloadedData = pReply->readAll();
    Q_EMIT downloaded();
}

QByteArray
FileDownloader::downloadedData() const
{
    return m_DownloadedData;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_FileDownloader.cpp"
