//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#ifndef RENDERINGPROGRESSDIALOG_H
#define RENDERINGPROGRESSDIALOG_H

#include <QDialog>

#include <boost/scoped_ptr.hpp>

class QString;

struct RenderingProgressDialogPrivate;
class RenderingProgressDialog : public QDialog {

    Q_OBJECT

public:

    RenderingProgressDialog(const QString& sequenceName,int firstFrame,int lastFrame,QWidget* parent = 0);

    virtual ~RenderingProgressDialog();


public slots:

    void onFrameRendered(int);

    void onCurrentFrameProgress(int);

    void onProcessCanceled();

signals:

    void canceled();

private:
    
    boost::scoped_ptr<RenderingProgressDialogPrivate> _imp;
    
};

#endif // RENDERINGPROGRESSDIALOG_H
