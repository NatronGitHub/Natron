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
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QFrame>
#include <QTextBrowser>
#include <QApplication>
#include <QThread>
#include <QKeyEvent>
#include <QMutex>
#include <QString>
#include <QTimer>
#include <QProgressBar>
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

#define NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS 4000
#define NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS 1000

NATRON_NAMESPACE_ENTER;

struct RenderingProgressDialogPrivate
{
    Gui* _gui;
    QVBoxLayout* _mainLayout;
    Label* _totalProgressLabel;
    Label* _totalProgressInfo;
    QProgressBar* _totalProgressBar;
    Label* _estimatedWaitTimeLabel;
    Label* _estimatedWaitTimeInfo;
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
                StandardButtonEnum reply = Dialogs::questionDialog( tr("Render").toStdString(),tr("The render ended successfully.\n"
                                                                                                     "Would you like to see the log ?").toStdString(), false );
                if (reply == eStandardButtonYes) {
                    showLog = true;
                }
            } else {
                Dialogs::informationDialog( tr("Render").toStdString(), tr("The render ended successfully.").toStdString() );
            }
        } else if (retCode == 1) {
            if (_imp->_process) {
                StandardButtonEnum reply = Dialogs::questionDialog( tr("Render").toStdString(),
                                                                       tr("The render ended with a return code of 1, a problem occured.\n"
                                                                          "Would you like to see the log ?").toStdString(), false );
                if (reply == eStandardButtonYes) {
                    showLog = true;
                }
            } else {
                Dialogs::errorDialog( tr("Render").toStdString(),tr("The render ended with a return code of 1, a problem occured.").toStdString() );
            }
        } else {
            if (_imp->_process) {
                StandardButtonEnum reply = Dialogs::questionDialog( tr("Render").toStdString(),tr("The render crashed.\n"
                                                                                                         "Would you like to see the log ?").toStdString() , false);
                if (reply == eStandardButtonYes) {
                    showLog = true;
                }
            } else {
                Dialogs::errorDialog( tr("Render").toStdString(),tr("The render crashed.").toStdString() );
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
        Dialogs::informationDialog( tr("Render").toStdString(), tr("Render aborted.").toStdString() );
        
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
    : QDialog(parent,Qt::WindowStaysOnTopHint)
      , _imp( new RenderingProgressDialogPrivate(gui,sequenceName,firstFrame,lastFrame,frameStep,process) )

{
    setMinimumWidth(fontMetrics().width(_imp->_sequenceName) + 100);
    setWindowTitle(_imp->_sequenceName);
    _imp->_mainLayout = new QVBoxLayout(this);
    setLayout(_imp->_mainLayout);
    _imp->_mainLayout->setContentsMargins(5, 5, 0, 0);
    _imp->_mainLayout->setSpacing(5);

    QWidget* totalProgressContainer = new QWidget(this);
    QHBoxLayout* totalProgressLayout = new QHBoxLayout(totalProgressContainer);
    _imp->_mainLayout->addWidget(totalProgressContainer);

    
    _imp->_totalProgressLabel = new Label(tr("Total progress:"),totalProgressContainer);
    totalProgressLayout->addWidget(_imp->_totalProgressLabel);
    
    _imp->_totalProgressInfo = new Label("0%",totalProgressContainer);
    totalProgressLayout->addWidget(_imp->_totalProgressInfo);
    
    QWidget* waitTimeContainer = new QWidget(this);
    QHBoxLayout* waitTimeLayout = new QHBoxLayout(waitTimeContainer);
    _imp->_mainLayout->addWidget(waitTimeContainer);

    _imp->_estimatedWaitTimeLabel = new Label(tr("Time remaining:"),waitTimeContainer);
    waitTimeLayout->addWidget(_imp->_estimatedWaitTimeLabel);
    
    _imp->_estimatedWaitTimeInfo = new Label("...",waitTimeContainer);
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


struct GeneralProgressDialogPrivate
{
    
    bool canCancel;
    mutable QMutex canceledMutex;
    bool canceled;
    
    QVBoxLayout* mainLayout;
    Label* descLabel;
    
    QWidget* progressContainer;
    QHBoxLayout* progressLayout;
    QProgressBar* progressBar;
    Label* timeRemainingLabel;
    bool timerLabelSet;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    Button* cancelButton;
    
    TimeLapse timer;
    
    QTimer refreshLabelTimer;
    
    double timeRemaining;
    
    GeneralProgressDialogPrivate(bool canCancel)
    : canCancel(canCancel)
    , canceledMutex()
    , canceled(false)
    , mainLayout(0)
    , descLabel(0)
    , progressContainer(0)
    , progressLayout(0)
    , progressBar(0)
    , timeRemainingLabel(0)
    , timerLabelSet(false)
    , buttonsContainer(0)
    , buttonsLayout(0)
    , cancelButton(0)
    , timer()
    , refreshLabelTimer()
    , timeRemaining(-1)
    {
        
    }
};

GeneralProgressDialog::GeneralProgressDialog(const QString& title, bool canCancel, QWidget* parent)
: QDialog(parent, Qt::WindowStaysOnTopHint)
, _imp(new GeneralProgressDialogPrivate(canCancel))
{
    setModal(false);
    setWindowTitle(tr("Progress"));
    
    assert(QThread::currentThread() == qApp->thread());
    
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->descLabel = new Label(title,this);
    
    _imp->mainLayout->addWidget(_imp->descLabel);
    
    _imp->progressContainer = new QWidget(this);
    _imp->progressLayout = new QHBoxLayout(_imp->progressContainer);
    _imp->progressLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->addWidget(_imp->progressContainer);

    _imp->timeRemainingLabel = new Label(tr("Time to go:") + ' ' + tr("Estimating..."),_imp->progressContainer);
    _imp->progressLayout->addWidget(_imp->timeRemainingLabel);
    
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->mainLayout->addWidget(_imp->buttonsContainer);

    
    _imp->progressBar = new QProgressBar(_imp->buttonsContainer);
    _imp->progressBar->setRange(0, 100);
    _imp->progressBar->setValue(0);
    _imp->buttonsLayout->addWidget(_imp->progressBar);
    
    if (canCancel) {
        
        _imp->cancelButton = new Button(QIcon(), tr("Cancel"), this);
        QObject::connect(_imp->cancelButton, SIGNAL(clicked(bool)), this, SLOT(onCancelRequested()));
        _imp->buttonsLayout->addWidget(_imp->cancelButton);
    }
    
    _imp->mainLayout->addStretch();
    
    onRefreshLabelTimeout();
    _imp->refreshLabelTimer.start(NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS);
    QObject::connect(&_imp->refreshLabelTimer, SIGNAL(timeout()), this, SLOT(onRefreshLabelTimeout()));
    
    setMinimumWidth(TO_DPIX(400));

}

GeneralProgressDialog::~GeneralProgressDialog()
{
    
}

bool
GeneralProgressDialog::wasCanceled() const
{
    if (!_imp->canCancel) {
        return false;
    }
    QMutexLocker k (&_imp->canceledMutex);
    return _imp->canceled;
}

void
GeneralProgressDialog::onRefreshLabelTimeout()
{
    QString timeStr = tr("Time to go:") + ' ';
    if (_imp->timeRemaining == -1) {
        timeStr += tr("Estimating...");
    } else {
        timeStr += Timer::printAsTime(_imp->timeRemaining, true) + '.';
        _imp->timerLabelSet = true;
    }
    _imp->timeRemainingLabel->setText(timeStr);
}

void
GeneralProgressDialog::updateProgress(double p)
{
    assert(QThread::currentThread() == qApp->thread());
    p = std::max(std::min(p, 1.), 0.);
    _imp->progressBar->setValue(p * 100.);
    
    double timeElapsedSecs = _imp->timer.getTimeSinceCreation();
    _imp->timeRemaining = p == 0 ? 0 : timeElapsedSecs * (1. - p) / p;
    
    if (!isVisible() && !wasCanceled()) {
        ///Show the dialog if the total estimated time is gt NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS
        double totalTime = p == 0 ? 0 : timeElapsedSecs * 1. / p;
        //also,  don't show if it was not shown yet but there are less than NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS remaining
        if (_imp->timerLabelSet && std::min(_imp->timeRemaining, totalTime) * 1000 > NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS) {
            printf("timeRemaining=%g\n", _imp->timeRemaining);
            show();
        }
    }

}

void
GeneralProgressDialog::onCancelRequested()
{
    {
        QMutexLocker k (&_imp->canceledMutex);
        _imp->canceled = true;
    }
    close();
}

void
GeneralProgressDialog::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        onCancelRequested();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
GeneralProgressDialog::closeEvent(QCloseEvent* /*e*/)
{
    QDialog::DialogCode ret = (QDialog::DialogCode)result();
    if (ret != QDialog::Accepted) {
        {
            QMutexLocker k (&_imp->canceledMutex);
            _imp->canceled = true;
        }
    }
    
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RenderingProgressDialog.cpp"
