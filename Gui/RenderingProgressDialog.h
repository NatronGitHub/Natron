/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef _Gui_RenderingProgressDialog_h_
#define _Gui_RenderingProgressDialog_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
