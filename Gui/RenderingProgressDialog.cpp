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

#include "RenderingProgressDialog.h"

#include <cmath>

#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>

#include <QString>

#include "Gui/Button.h"

struct RenderingProgressDialogPrivate {
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
    
    RenderingProgressDialogPrivate(const QString& sequenceName,int firstFrame,int lastFrame)
    : _mainLayout(0)
    , _totalLabel(0)
    , _totalProgress(0)
    , _separator(0)
    , _perFrameLabel(0)
    , _perFrameProgress(0)
    , _cancelButton(0)
    , _sequenceName(sequenceName)
    , _firstFrame(firstFrame)
    , _lastFrame(lastFrame)
    {
        
    }
};

void RenderingProgressDialog::onFrameRendered(int frame){
    double percent = (double)(frame - _imp->_firstFrame+1)/(double)(_imp->_lastFrame - _imp->_firstFrame+1);
    int progress = std::floor(percent*100);
    _imp->_totalProgress->setValue(progress);
    _imp->_perFrameLabel->setText("Frame " + QString::number(frame) + ":");
    QString title = QString::number(progress) + "% of " + _imp->_sequenceName;
    setWindowTitle(title);
    _imp->_perFrameLabel->hide();
    _imp->_perFrameProgress->hide();
    _imp->_separator->hide();
}

void RenderingProgressDialog::onCurrentFrameProgress(int progress){
    if(!_imp->_perFrameProgress->isVisible()){
        _imp->_separator->show();
        _imp->_perFrameProgress->show();
        _imp->_perFrameLabel->show();
    }
    _imp->_perFrameProgress->setValue(progress);
}

void RenderingProgressDialog::onProcessCanceled() {
    hide();
}

RenderingProgressDialog::RenderingProgressDialog(const QString& sequenceName,int firstFrame,int lastFrame,QWidget* parent)
: QDialog(parent)
, _imp(new RenderingProgressDialogPrivate(sequenceName,firstFrame,lastFrame))

{
    
    QString title = QString::number(0) + "% of " + _imp->_sequenceName;
    setMinimumWidth(fontMetrics().width(title)+100);
    setWindowTitle(QString::number(0) + "% of " + _imp->_sequenceName);
    //setWindowFlags(Qt::WindowStaysOnTopHint);
    _imp->_mainLayout = new QVBoxLayout(this);
    setLayout(_imp->_mainLayout);
    _imp->_mainLayout->setContentsMargins(5, 5, 0, 0);
    _imp->_mainLayout->setSpacing(5);
    
    _imp->_totalLabel = new QLabel("Total progress:",this);
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
    txt.append(QString::number(firstFrame));
    txt.append(":");
    _imp->_perFrameLabel = new QLabel(txt,this);
    _imp->_mainLayout->addWidget(_imp->_perFrameLabel);
    
    _imp->_perFrameProgress = new QProgressBar(this);
    _imp->_perFrameProgress->setMinimumWidth(150);
    _imp->_perFrameProgress->setRange(0, 100);
    _imp->_mainLayout->addWidget(_imp->_perFrameProgress);
    
    _imp->_cancelButton = new Button("Cancel",this);
    _imp->_cancelButton->setMaximumWidth(50);
    _imp->_mainLayout->addWidget(_imp->_cancelButton);
    
    QObject::connect(_imp->_cancelButton, SIGNAL(clicked()), this, SIGNAL(canceled()));
    
    
}

RenderingProgressDialog::~RenderingProgressDialog() {}
