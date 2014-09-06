//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef RENDERINGPROGRESSDIALOG_H
#define RENDERINGPROGRESSDIALOG_H

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

class QVBoxLayout;
class QTextBrowser;
class Button;
class QString;
class ProcessHandler;
struct RenderingProgressDialogPrivate;
class Gui;
class RenderingProgressDialog
    : public QDialog
{
    Q_OBJECT

public:

    RenderingProgressDialog(Gui* gui,
                            const QString & sequenceName,
                            int firstFrame,
                            int lastFrame,
                            const boost::shared_ptr<ProcessHandler> & process,
                            QWidget* parent = 0);

    virtual ~RenderingProgressDialog();

public slots:

    void onProcessDeleted();

    void onFrameRendered(int);

    void onCurrentFrameProgress(int);

    void onProcessCanceled();

    void onProcessFinished(int);

    void onVideoEngineStopped(int);

signals:

    void canceled();

private:

    boost::scoped_ptr<RenderingProgressDialogPrivate> _imp;
};


class LogWindow
    : public QDialog
{
    Q_OBJECT

    QVBoxLayout* mainLayout;
    QTextBrowser* textBrowser;
    Button* okButton;
    Button* clearButton;
public:

    LogWindow(const QString & log,
              QWidget* parent = 0);
    
public slots:
    
    void onClearButtonClicked();
};

#endif // RENDERINGPROGRESSDIALOG_H
