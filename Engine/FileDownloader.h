/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef __NATRON_FILEDOWNLOADER_H
#define __NATRON_FILEDOWNLOADER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <boost/scoped_ptr.hpp>

CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated-register)

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
   Usage:
   Load Pixmap from URL

   Declare slot
   private slots:

    void loadImage();
   Connect signal downloaded() to the slot
   QUrl imageUrl("http://qt.digia.com/Documents/1/QtLogo.png");
   m_pImgCtrl = new FileDownloader(imageUrl, this);

   connect(m_pImgCtrl, SIGNAL(downloaded()), SLOT(loadImage()));
   Load QPixmap from the downloaded data
   void MainWindow::loadImage()
   {
    QPixmap buttonImage;
    buttonImage.loadFromData(m_pImgCtrl->downloadedData());
   }

 **/
class FileDownloader
    : public QObject
{
    Q_OBJECT

public:
    explicit FileDownloader(const QUrl &imageUrl,
                            bool useNetworkCache,
                            QObject *parent = 0);

    virtual ~FileDownloader();

    const QByteArray& downloadedData() const;
    QNetworkReply* getReply() const;

Q_SIGNALS:
    void downloaded();

    void error();

public Q_SLOTS:

    void fileDownloaded(QNetworkReply* pReply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onTimerTimeout();

private:

    QNetworkReply* m_reply;
    boost::scoped_ptr<QNetworkAccessManager> m_WebCtrl;
    boost::scoped_ptr<QByteArray> m_DownloadedData;
    boost::scoped_ptr<QTimer> m_timer;
};

NATRON_NAMESPACE_EXIT

#endif // FILEDOWNLOADER_H
