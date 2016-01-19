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

#include "RenderingProgressDialog.h"

#include <cmath>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QProgressBar>
#include <QFrame>
#include <QTextBrowser>
#include <QApplication>
#include <QThread>
#include <QKeyEvent>
#include <QString>
#include <QtCore/QTextStream>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ProcessHandler.h"
#include "Engine/Timer.h"

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"
#include "Gui/LogWindow.h"

NATRON_NAMESPACE_USING

struct NATRON_NAMESPACE::RenderingProgressDialogPrivate
{
    Gui* _gui;
    QVBoxLayout* _mainLayout;
    Natron::Label* _totalProgressLabel;
    Natron::Label* _totalProgressInfo;
    QProgressBar* _totalProgressBar;
    Natron::Label* _estimatedWaitTimeLabel;
    Natron::Label* _estimatedWaitTimeInfo;
    Button* _cancelButton;
    QString _sequenceName;
    int _firstFrame;
    int _lastFrame;
    int _frameStep;
    boost::shared_ptr<ProcessHandler> _process;

    int _nFramesRendered;

    RenderingProgressDialogPrivate(Gui* gui,
                                   const QString & sequenceName,
                                   int firstFrame,
                                   int lastFrame,
                                   int frameStep,
                                   const boost::shared_ptr<ProcessHandler> & proc)
    : _gui(gui)
    , _mainLayout(0)
    , _totalProgressLabel(0)
    , _totalProgressInfo(0)
    , _totalProgressBar(0)
    , _estimatedWaitTimeLabel(0)
    , _estimatedWaitTimeInfo(0)
    , _cancelButton(0)
    , _sequenceName(sequenceName)
    , _firstFrame(firstFrame)
    , _lastFrame(lastFrame)
    , _frameStep(frameStep)
    , _process(proc)
    , _nFramesRendered(0)
    {
    }
};




void
RenderingProgressDialog::onFrameRenderedWithTimer(int frame, double /*timeElapsedForFrame*/, double remainingTime)
{
    assert(QThread::currentThread() == qApp->thread());
    
    ++_imp->_nFramesRendered;
    U64 totalFrames = (double)(_imp->_lastFrame - _imp->_firstFrame + 1) / _imp->_frameStep;
    double percent = 0;
    if (totalFrames > 0) {
        percent = _imp->_nFramesRendered / (double)totalFrames;
    }
    double progress = percent * 100;
    
    _imp->_totalProgressBar->setValue(progress);
    
    QString infoStr;
    QTextStream ts(&infoStr);
    ts << "Frame " << frame << " (" << QString::number(progress,'f',1) << "%)";
    _imp->_totalProgressInfo->setText(infoStr);
    
    if (remainingTime != -1) {
        QString timeStr = Timer::printAsTime(remainingTime, true);
        _imp->_estimatedWaitTimeInfo->setText(timeStr);
    } else {
        _imp->_estimatedWaitTimeInfo->setText(tr("Unknown"));
    }
}

void
RenderingProgressDialog::onFrameRendered(int frame)
{


    assert(QThread::currentThread() == qApp->thread());

    ++_imp->_nFramesRendered;
    U64 totalFrames = (double)(_imp->_lastFrame - _imp->_firstFrame + 1) / _imp->_frameStep;
    double percent = 0;
    if (totalFrames > 0) {
        percent = _imp->_nFramesRendered / (double)totalFrames;
    }
    double progress = percent * 100;

    _imp->_totalProgressBar->setValue(progress);
    
    QString infoStr;
    QTextStream ts(&infoStr);
    ts << "Frame " << frame << " (" << QString::number(progress,'f',1) << "%)";
    _imp->_totalProgressInfo->setText(infoStr);
    _imp->_estimatedWaitTimeInfo->setText("...");
}


void
RenderingProgressDialog::onProcessCanceled()
{
	close();
}

void
RenderingProgressDialog::onProcessFinished(int retCode)
{
    if ( isVisible() ) {
        hide();

        bool showLog = false;
        if (retCode == 0) {
            if (_imp->_process) {
                StandardButtonEnum reply = natronQuestionDialog( tr("Render").toStdString(),tr("The render ended successfully.\n"
                                                                                                     "Would you like to see the log ?").toStdString(), false );
                if (reply == eStandardButtonYes) {
                    showLog = true;
                }
            } else {
                natronInformationDialog( tr("Render").toStdString(), tr("The render ended successfully.").toStdString() );
            }
        } else if (retCode == 1) {
            if (_imp->_process) {
                StandardButtonEnum reply = natronQuestionDialog( tr("Render").toStdString(),
                                                                       tr("The render ended with a return code of 1, a problem occured.\n"
                                                                          "Would you like to see the log ?").toStdString(), false );
                if (reply == eStandardButtonYes) {
                    showLog = true;
                }
            } else {
                natronErrorDialog( tr("Render").toStdString(),tr("The render ended with a return code of 1, a problem occured.").toStdString() );
            }
        } else {
            if (_imp->_process) {
                StandardButtonEnum reply = natronQuestionDialog( tr("Render").toStdString(),tr("The render crashed.\n"
                                                                                                         "Would you like to see the log ?").toStdString() , false);
                if (reply == eStandardButtonYes) {
                    showLog = true;
                }
            } else {
                natronErrorDialog( tr("Render").toStdString(),tr("The render crashed.").toStdString() );
            }
        }
        if (showLog) {
            assert(_imp->_process);
            LogWindow log(_imp->_process->getProcessLog(),this);
            ignore_result(log.exec());
        }
    }
    accept();
}

void
RenderingProgressDialog::onVideoEngineStopped(int retCode)
{
    if (retCode == 1) {
        onProcessCanceled();
    } else {
        onProcessFinished(0);
    }
}

void
RenderingProgressDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        onCancelButtonClicked();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
RenderingProgressDialog::closeEvent(QCloseEvent* /*e*/)
{
    QDialog::DialogCode ret = (QDialog::DialogCode)result();
    if (ret != QDialog::Accepted) {
        Q_EMIT canceled();
        reject();
        natronInformationDialog( tr("Render").toStdString(), tr("Render aborted.").toStdString() );
        
    }
    
}

void
RenderingProgressDialog::onCancelButtonClicked()
{
    Q_EMIT canceled();
    close();
}

RenderingProgressDialog::RenderingProgressDialog(Gui* gui,
                                                 const QString & sequenceName,
                                                 int firstFrame,
                                                 int lastFrame,
                                                 int frameStep,
                                                 const boost::shared_ptr<ProcessHandler> & process,
                                                 QWidget* parent)
    : QDialog(parent)
      , _imp( new RenderingProgressDialogPrivate(gui,sequenceName,firstFrame,lastFrame,frameStep,process) )

{
    setMinimumWidth(fontMetrics().width(_imp->_sequenceName) + 100);
    setWindowTitle(_imp->_sequenceName);
    //setWindowFlags(Qt::WindowStaysOnTopHint);
    _imp->_mainLayout = new QVBoxLayout(this);
    setLayout(_imp->_mainLayout);
    _imp->_mainLayout->setContentsMargins(5, 5, 0, 0);
    _imp->_mainLayout->setSpacing(5);

    QWidget* totalProgressContainer = new QWidget(this);
    QHBoxLayout* totalProgressLayout = new QHBoxLayout(totalProgressContainer);
    _imp->_mainLayout->addWidget(totalProgressContainer);

    
    _imp->_totalProgressLabel = new Natron::Label(tr("Total progress:"),totalProgressContainer);
    totalProgressLayout->addWidget(_imp->_totalProgressLabel);
    
    _imp->_totalProgressInfo = new Natron::Label("0%",totalProgressContainer);
    totalProgressLayout->addWidget(_imp->_totalProgressInfo);
    
    QWidget* waitTimeContainer = new QWidget(this);
    QHBoxLayout* waitTimeLayout = new QHBoxLayout(waitTimeContainer);
    _imp->_mainLayout->addWidget(waitTimeContainer);

    _imp->_estimatedWaitTimeLabel = new Natron::Label(tr("Time remaining:"),waitTimeContainer);
    waitTimeLayout->addWidget(_imp->_estimatedWaitTimeLabel);
    
    _imp->_estimatedWaitTimeInfo = new Natron::Label("...",waitTimeContainer);
    waitTimeLayout->addWidget(_imp->_estimatedWaitTimeInfo);

    _imp->_totalProgressBar = new QProgressBar(this);
    _imp->_totalProgressBar->setRange(0, 100);
    _imp->_totalProgressBar->setMinimumWidth(150);
    
    _imp->_mainLayout->addWidget(_imp->_totalProgressBar);
    
    
    _imp->_cancelButton = new Button(tr("Cancel"),this);
    _imp->_cancelButton->setMaximumWidth(50);
    _imp->_mainLayout->addWidget(_imp->_cancelButton);

    QObject::connect( _imp->_cancelButton, SIGNAL( clicked() ), this, SLOT( onCancelButtonClicked() ) );


    if (process) {
        QObject::connect( this,SIGNAL( canceled() ),process.get(),SLOT( onProcessCanceled() ) );
        QObject::connect( process.get(),SIGNAL( processCanceled() ),this,SLOT( onProcessCanceled() ) );
        QObject::connect( process.get(),SIGNAL( frameRendered(int) ),this,SLOT( onFrameRendered(int) ) );
        QObject::connect( process.get(),SIGNAL( frameRenderedWithTimer(int,double,double)),this,SLOT(onFrameRenderedWithTimer(int,double,double)));
        QObject::connect( process.get(),SIGNAL( processFinished(int) ),this,SLOT( onProcessFinished(int) ) );
        QObject::connect( process.get(),SIGNAL( deleted() ),this,SLOT( onProcessDeleted() ) );
    }
}

RenderingProgressDialog::~RenderingProgressDialog()
{
}

void
RenderingProgressDialog::onProcessDeleted()
{
    assert(_imp->_process);
    QObject::disconnect( this,SIGNAL( canceled() ),_imp->_process.get(),SLOT( onProcessCanceled() ) );
    QObject::disconnect( _imp->_process.get(),SIGNAL( processCanceled() ),this,SLOT( onProcessCanceled() ) );
    QObject::disconnect( _imp->_process.get(),SIGNAL( frameRendered(int) ),this,SLOT( onFrameRendered(int) ) );
    QObject::disconnect( _imp->_process.get(),SIGNAL( frameRenderedWithTimer(int,double,double) ),this,
                        SLOT( onFrameRenderedWithTimer(int,double,double) ) );
    QObject::disconnect( _imp->_process.get(),SIGNAL( processFinished(int) ),this,SLOT( onProcessFinished(int) ) );
    QObject::disconnect( _imp->_process.get(),SIGNAL( deleted() ),this,SLOT( onProcessDeleted() ) );
}

#include "moc_RenderingProgressDialog.cpp"
