//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_RenderingProgressDialog_h_
#define _Gui_RenderingProgressDialog_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
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
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    RenderingProgressDialog(Gui* gui,
                            const QString & sequenceName,
                            int firstFrame,
                            int lastFrame,
                            const boost::shared_ptr<ProcessHandler> & process,
                            QWidget* parent = 0);

    virtual ~RenderingProgressDialog();

public Q_SLOTS:

    void onProcessDeleted();

    void onFrameRendered(int);
    
    void onFrameRenderedWithTimer(int frame, double timeElapsedForFrame, double remainingTime);

    void onProcessCanceled();

    void onProcessFinished(int);

    void onVideoEngineStopped(int);
    
    void onCancelButtonClicked();

Q_SIGNALS:

    void canceled();

private:
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    virtual void closeEvent(QCloseEvent* e) OVERRIDE FINAL;

    boost::scoped_ptr<RenderingProgressDialogPrivate> _imp;
};

#endif // _Gui_RenderingProgressDialog_h_
