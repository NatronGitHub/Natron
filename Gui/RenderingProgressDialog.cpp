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

#include "RenderingProgressDialog.h"

#include <cmath>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include <QTextBrowser>
#include <QApplication>
#include <QThread>
#include <QString>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ProcessHandler.h"

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"

struct RenderingProgressDialogPrivate
{
    Gui* _gui;
    QVBoxLayout* _mainLayout;
    QLabel* _totalLabel;
    QProgressBar* _totalProgress;
    QFrame* _separator;
    QLabel* _perFrameLabel;
    QProgressBar* _perFrameProgress;
    Button* _cancelButton;
    QString _sequenceName;
    int _firstFrame;
    int _lastFrame;
    boost::shared_ptr<ProcessHandler> _process;

    int _nFramesRendered;

    RenderingProgressDialogPrivate(Gui* gui,
                                   const QString & sequenceName,
                                   int firstFrame,
                                   int lastFrame,
                                   const boost::shared_ptr<ProcessHandler> & proc)
        : _gui(gui)
          , _mainLayout(0)
          , _totalLabel(0)
          , _totalProgress(0)
          , _separator(0)
          , _perFrameLabel(0)
          , _perFrameProgress(0)
          , _cancelButton(0)
          , _sequenceName(sequenceName)
          , _firstFrame(firstFrame)
          , _lastFrame(lastFrame)
          , _process(proc)
          , _nFramesRendered(0)
    {
    }
};

void
RenderingProgressDialog::onFrameRendered(int frame)
{


    assert(QThread::currentThread() == qApp->thread());

    ++_imp->_nFramesRendered;

    double percent = _imp->_nFramesRendered / (double)(_imp->_lastFrame - _imp->_firstFrame + 1);
    int progress = std::floor(percent * 100);

    _imp->_totalProgress->setValue(progress);
    _imp->_perFrameLabel->setText(tr("Frame ") + QString::number(frame) + ":");
    QString title = QString::number(progress) + tr("% of ") + _imp->_sequenceName;
    setWindowTitle(title);
    _imp->_perFrameLabel->hide();
    _imp->_perFrameProgress->hide();
    _imp->_separator->hide();
}

void
RenderingProgressDialog::onCurrentFrameProgress(int progress)
{
    if ( !_imp->_perFrameProgress->isVisible() ) {
        _imp->_separator->show();
        _imp->_perFrameProgress->show();
        _imp->_perFrameLabel->show();
    }
    _imp->_perFrameProgress->setValue(progress);
}

void
RenderingProgressDialog::onProcessCanceled()
{
    if ( isVisible() ) {
        hide();
        Natron::informationDialog( tr("Render").toStdString(), tr("Render aborted.").toStdString() );
    }
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
                Natron::StandardButton reply = Natron::questionDialog( tr("Render").toStdString(),tr("The render ended successfully.\n"
                                                                                                     "Would you like to see the log ?").toStdString() );
                if (reply == Natron::Yes) {
                    showLog = true;
                }
            } else {
                Natron::informationDialog( tr("Render").toStdString(), tr("The render ended successfully.").toStdString() );
            }
        } else if (retCode == 1) {
            if (_imp->_process) {
                Natron::StandardButton reply = Natron::questionDialog( tr("Render").toStdString(),
                                                                       tr("The render ended with a return code of 1, a problem occured.\n"
                                                                          "Would you like to see the log ?").toStdString() );
                if (reply == Natron::Yes) {
                    showLog = true;
                }
            } else {
                Natron::errorDialog( tr("Render").toStdString(),tr("The render ended with a return code of 1, a problem occured.").toStdString() );
            }
        } else {
            if (_imp->_process) {
                Natron::StandardButton reply = Natron::questionDialog( tr("Render").toStdString(),tr("The render crashed.\n"
                                                                                                     "Would you like to see the log ?").toStdString() );
                if (reply == Natron::Yes) {
                    showLog = true;
                }
            } else {
                Natron::errorDialog( tr("Render").toStdString(),tr("The render crashed.").toStdString() );
            }
        }
        if (showLog) {
            assert(_imp->_process);
            LogWindow log(_imp->_process->getProcessLog(),this);
            log.exec();
        }
    }
	close();
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

RenderingProgressDialog::RenderingProgressDialog(Gui* gui,
                                                 const QString & sequenceName,
                                                 int firstFrame,
                                                 int lastFrame,
                                                 const boost::shared_ptr<ProcessHandler> & process,
                                                 QWidget* parent)
    : QDialog(parent)
      , _imp( new RenderingProgressDialogPrivate(gui,sequenceName,firstFrame,lastFrame,process) )

{
    QString title = QString::number(0) + tr("% of ") + _imp->_sequenceName;

    setMinimumWidth(fontMetrics().width(title) + 100);
    setWindowTitle(QString::number(0) + tr("% of ") + _imp->_sequenceName);
    //setWindowFlags(Qt::WindowStaysOnTopHint);
    _imp->_mainLayout = new QVBoxLayout(this);
    setLayout(_imp->_mainLayout);
    _imp->_mainLayout->setContentsMargins(5, 5, 0, 0);
    _imp->_mainLayout->setSpacing(5);

    _imp->_totalLabel = new QLabel(tr("Total progress:"),this);
    _imp->_mainLayout->addWidget(_imp->_totalLabel);
    _imp->_totalProgress = new QProgressBar(this);
    _imp->_totalProgress->setRange(0, 100);
    _imp->_totalProgress->setMinimumWidth(150);

    _imp->_mainLayout->addWidget(_imp->_totalProgress);

    _imp->_separator = new QFrame(this);
    _imp->_separator->setFrameShadow(QFrame::Raised);
    _imp->_separator->setMinimumWidth(100);
    _imp->_separator->setMaximumHeight(2);
    _imp->_separator->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

    _imp->_mainLayout->addWidget(_imp->_separator);

    QString txt("Frame ");
    txt.append( QString::number(firstFrame) );
    txt.append(":");
    _imp->_perFrameLabel = new QLabel(txt,this);
    _imp->_mainLayout->addWidget(_imp->_perFrameLabel);

    _imp->_perFrameProgress = new QProgressBar(this);
    _imp->_perFrameProgress->setMinimumWidth(150);
    _imp->_perFrameProgress->setRange(0, 100);
    _imp->_mainLayout->addWidget(_imp->_perFrameProgress);

    _imp->_cancelButton = new Button(tr("Cancel"),this);
    _imp->_cancelButton->setMaximumWidth(50);
    _imp->_mainLayout->addWidget(_imp->_cancelButton);

    QObject::connect( _imp->_cancelButton, SIGNAL( clicked() ), this, SIGNAL( canceled() ) );


    if (process) {
        QObject::connect( this,SIGNAL( canceled() ),process.get(),SLOT( onProcessCanceled() ) );
        QObject::connect( process.get(),SIGNAL( processCanceled() ),this,SLOT( onProcessCanceled() ) );
        QObject::connect( process.get(),SIGNAL( frameRendered(int) ),this,SLOT( onFrameRendered(int) ) );
        QObject::connect( process.get(),SIGNAL( frameProgress(int) ),this,SLOT( onCurrentFrameProgress(int) ) );
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
    QObject::disconnect( _imp->_process.get(),SIGNAL( frameProgress(int) ),this,SLOT( onCurrentFrameProgress(int) ) );
    QObject::disconnect( _imp->_process.get(),SIGNAL( processFinished(int) ),this,SLOT( onProcessFinished(int) ) );
    QObject::disconnect( _imp->_process.get(),SIGNAL( deleted() ),this,SLOT( onProcessDeleted() ) );
}

LogWindow::LogWindow(const QString & log,
                     QWidget* parent)
    : QDialog(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    textBrowser->setText(log);

    mainLayout->addWidget(textBrowser);

    QWidget* buttonsContainer = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    
    clearButton = new Button(tr("Clear"),buttonsContainer);
    buttonsLayout->addWidget(clearButton);
    QObject::connect(clearButton, SIGNAL(clicked()), this, SLOT(onClearButtonClicked()));
    buttonsLayout->addStretch();
    okButton = new Button(tr("Ok"),buttonsContainer);
    buttonsLayout->addWidget(okButton);
    QObject::connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    mainLayout->addWidget(buttonsContainer);
}

void
LogWindow::onClearButtonClicked()
{
    appPTR->clearOfxLog_mt_safe();
    textBrowser->clear();
}
