//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
CLANG_DIAG_ON(deprecated-register)

#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

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
    explicit FileDownloader(QUrl imageUrl,
                            QObject *parent = 0);

    virtual ~FileDownloader();

    QByteArray downloadedData() const;

    QNetworkReply* getReply() const;

signals:
    void downloaded();

    void error();

private slots:

    void fileDownloaded(QNetworkReply* pReply);

private:

    QNetworkReply* m_reply;
    QNetworkAccessManager m_WebCtrl;
    QByteArray m_DownloadedData;
};

#endif // FILEDOWNLOADER_H
