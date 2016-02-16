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

#include "CurveWidgetDialogs.h"

#include <cmath> // std::abs
#include <stdexcept>
//
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
//// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QCoreApplication>
#include <QApplication>
#include <QHBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QVBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QtCore/QThread>
#include <QtCore/QSettings>
#include <QDebug>

#include "Engine/AppManager.h" // appPTR

#include "Gui/LineEdit.h" // LineEdit
#include "Gui/SpinBox.h" // SpinBox
#include "Gui/Button.h" // Button
#include "Gui/CurveWidget.h" // CurveWidget
#include "Gui/SequenceFileDialog.h" // SequenceFileDialog
#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h" // Label

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

NATRON_NAMESPACE_ENTER;

ImportExportCurveDialog::ImportExportCurveDialog(bool isExportDialog,
                                                 const std::vector<boost::shared_ptr<CurveGui> > & curves,
                                                 Gui* gui,
                                                 QWidget* parent)
    : QDialog(parent)
    , _gui(gui)
    , _isExportDialog(isExportDialog)
    , _mainLayout(0)
    , _fileContainer(0)
    , _fileLayout(0)
    , _fileLabel(0)
    , _fileLineEdit(0)
    , _fileBrowseButton(0)
    , _startContainer(0)
    , _startLayout(0)
    , _startLabel(0)
    , _startSpinBox(0)
    , _incrContainer(0)
    , _incrLayout(0)
    , _incrLabel(0)
    , _incrSpinBox(0)
    , _endContainer(0)
    , _endLayout(0)
    , _endLabel(0)
    , _endSpinBox(0)
    , _curveColumns()
    , _buttonsContainer(0)
    , _buttonsLayout(0)
    , _okButton(0)
    , _cancelButton(0)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, 3, 0, 0);
    _mainLayout->setSpacing(2);
    setLayout(_mainLayout);

    //////File
    _fileContainer = new QWidget(this);
    _fileLayout = new QHBoxLayout(_fileContainer);
    _fileLabel = new Label(tr("File:"),_fileContainer);
    _fileLayout->addWidget(_fileLabel);
    _fileLineEdit = new LineEdit(_fileContainer);
    _fileLineEdit->setPlaceholderText( tr("File path...") );
    _fileLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _fileLayout->addWidget(_fileLineEdit);
    _fileBrowseButton = new Button(_fileContainer);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _fileBrowseButton->setIcon( QIcon(pix) );
    QObject::connect( _fileBrowseButton, SIGNAL( clicked() ), this, SLOT( open_file() ) );
    _fileLayout->addWidget(_fileBrowseButton);
    _mainLayout->addWidget(_fileContainer);

    //////x start value
    _startContainer = new QWidget(this);
    _startLayout = new QHBoxLayout(_startContainer);
    _startLabel = new Label(tr("X start value:"),_startContainer);
    _startLayout->addWidget(_startLabel);
    _startSpinBox = new SpinBox(_startContainer,SpinBox::eSpinBoxTypeDouble);
    _startSpinBox->setValue(0);
    _startLayout->addWidget(_startSpinBox);
    _mainLayout->addWidget(_startContainer);

    //////x increment
    _incrContainer = new QWidget(this);
    _incrLayout = new QHBoxLayout(_incrContainer);
    _incrLabel = new Label(tr("X increment:"),_incrContainer);
    _incrLayout->addWidget(_incrLabel);
    _incrSpinBox = new SpinBox(_incrContainer,SpinBox::eSpinBoxTypeDouble);
    _incrSpinBox->setValue(0.01);
    _incrLayout->addWidget(_incrSpinBox);
    _mainLayout->addWidget(_incrContainer);

    //////x end value
    if (isExportDialog) {
        _endContainer = new QWidget(this);
        _endLayout = new QHBoxLayout(_endContainer);
        _endLabel = new Label(tr("X end value:"),_endContainer);
        _endLabel->setFont(QApplication::font()); // necessary, or the labels will get the default font size
        _endLayout->addWidget(_endLabel);
        _endSpinBox = new SpinBox(_endContainer,SpinBox::eSpinBoxTypeDouble);
        _endSpinBox->setValue(1);
        _endLayout->addWidget(_endSpinBox);
        _mainLayout->addWidget(_endContainer);
    }

    ////curves columns
    double min = 0,max = 0;
    bool curveIsClampedToIntegers = false;
    for (U32 i = 0; i < curves.size(); ++i) {
        CurveColumn column;
        double curvemin = curves[i]->getInternalCurve()->getMinimumTimeCovered();
        double curvemax = curves[i]->getInternalCurve()->getMaximumTimeCovered();
        if (curvemin < min) {
            min = curvemin;
        }
        if (curvemax > max) {
            max = curvemax;
        }
        if ( curves[i]->areKeyFramesTimeClampedToIntegers() ) {
            curveIsClampedToIntegers = true;
        }
        column._curve = curves[i];
        column._curveContainer = new QWidget(this);
        column._curveLayout = new QHBoxLayout(column._curveContainer);
        column._curveLabel = new Label( curves[i]->getName() + tr(" column:") );
        column._curveLabel->setFont(QApplication::font()); // necessary, or the labels will get the default font size
        column._curveLayout->addWidget(column._curveLabel);
        column._curveSpinBox = new SpinBox(column._curveContainer,SpinBox::eSpinBoxTypeInt);
        column._curveSpinBox->setValue( (double)i + 1. );
        column._curveLayout->addWidget(column._curveSpinBox);
        _curveColumns.push_back(column);
        _mainLayout->addWidget(column._curveContainer);
    }
    if (isExportDialog) {
        _startSpinBox->setValue(min);
        _endSpinBox->setValue(max);
    }
    if (curveIsClampedToIntegers) {
        _incrSpinBox->setValue(1);
    }
    /////buttons
    _buttonsContainer = new QWidget(this);
    _buttonsLayout = new QHBoxLayout(_buttonsContainer);
    _okButton = new Button(tr("Ok"),_buttonsContainer);
    QObject::connect( _okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    _buttonsLayout->addWidget(_okButton);
    _cancelButton = new Button(tr("Cancel"),_buttonsContainer);
    QObject::connect( _cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
    _buttonsLayout->addWidget(_cancelButton);
    _mainLayout->addWidget(_buttonsContainer);
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    
    QByteArray state;
    if (isExportDialog) {
        state = settings.value(QLatin1String("CurveWidgetExportDialog") ).toByteArray();
    } else {
        state = settings.value(QLatin1String("CurveWidgetImportDialog") ).toByteArray();
    }
    if (!state.isEmpty()) {
        restoreState(state);
    }
}

ImportExportCurveDialog::~ImportExportCurveDialog()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    if (_isExportDialog) {
        settings.setValue( QLatin1String("CurveWidgetExportDialog"), saveState() );
    } else {
        settings.setValue( QLatin1String("CurveWidgetImportDialog"), saveState() );
    }
}


QByteArray
ImportExportCurveDialog::saveState()
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << _fileLineEdit->text();
    stream << _startSpinBox->value();
    stream << _incrSpinBox->value();
    if (_isExportDialog) {
        stream << _endSpinBox->value();
    }
    return data;
}

void
ImportExportCurveDialog::restoreState(const QByteArray& state)
{
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    
    if ( stream.atEnd() ) {
        return;
    }
    
    QString file;
    double start,incr,end;
    stream >> file;
    stream >> start;
    stream >> incr;
    _fileLineEdit->setText(file);
    _startSpinBox->setValue(start);
    _incrSpinBox->setValue(incr);
    if (_isExportDialog) {
        stream >> end;
        _endSpinBox->setValue(end);
    }

}

void
ImportExportCurveDialog::open_file()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<std::string> filters;
    filters.push_back("*");
    if (_isExportDialog) {
        SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::eFileDialogModeSave,"",_gui,false);
        if ( dialog.exec() ) {
            std::string file = dialog.filesToSave();
            _fileLineEdit->setText( file.c_str() );
        }
    } else {
        SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::eFileDialogModeOpen,"",_gui,false);
        if ( dialog.exec() ) {
            std::string files = dialog.selectedFiles();
            if ( !files.empty() ) {
                _fileLineEdit->setText( files.c_str() );
            }
        }
    }
}

QString
ImportExportCurveDialog::getFilePath()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _fileLineEdit->text();
}

double
ImportExportCurveDialog::getXStart() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _startSpinBox->value();
}

double
ImportExportCurveDialog::getXIncrement() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _incrSpinBox->value();
}

double
ImportExportCurveDialog::getXEnd() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///only valid for export dialogs
    assert(_isExportDialog);

    return _endSpinBox->value();
}

void
ImportExportCurveDialog::getCurveColumns(std::map<int,boost::shared_ptr<CurveGui> >* columns) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (U32 i = 0; i < _curveColumns.size(); ++i) {
        columns->insert( std::make_pair( (int)(_curveColumns[i]._curveSpinBox->value() - 1),_curveColumns[i]._curve ) );
    }
}

struct EditKeyFrameDialogPrivate
{
    
    CurveWidget* curveWidget;
    KeyPtr key;
    double originalX,originalY;
    
    QVBoxLayout* mainLayout;
    
    QWidget* boxContainer;
    QHBoxLayout* boxLayout;
    Label* xLabel;
    SpinBox* xSpinbox;
    Label* yLabel;
    SpinBox* ySpinbox;
    
    bool wasAccepted;
    
    EditKeyFrameDialog::EditModeEnum mode;
    
    EditKeyFrameDialogPrivate(EditKeyFrameDialog::EditModeEnum mode,CurveWidget* curveWidget,const KeyPtr& key)
        : curveWidget(curveWidget)
        , key(key)
        , originalX(key->key.getTime())
        , originalY(key->key.getValue())
        , mainLayout(0)
        , boxContainer(0)
        , boxLayout(0)
        , xLabel(0)
        , xSpinbox(0)
        , yLabel(0)
        , ySpinbox(0)
        , wasAccepted(false)
        , mode(mode)
    {
        if (mode == EditKeyFrameDialog::eEditModeLeftDerivative) {
            originalX = key->key.getLeftDerivative();
        } else if (mode == EditKeyFrameDialog::eEditModeRightDerivative) {
            originalX = key->key.getRightDerivative();
        }
        

    }
};

EditKeyFrameDialog::EditKeyFrameDialog(EditModeEnum mode,CurveWidget* curveWidget,const KeyPtr& key,QWidget* parent)
    : QDialog(parent)
    , _imp(new EditKeyFrameDialogPrivate(mode,curveWidget,key))
{
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->boxContainer = new QWidget(this);
    _imp->boxLayout = new QHBoxLayout(_imp->boxContainer);
    _imp->boxLayout->setContentsMargins(0, 0, 0, 0);
    
    QString xLabel;
    switch (mode) {
    case eEditModeKeyframePosition:
        xLabel = QString("x: ");
        break;
    case eEditModeLeftDerivative:
        xLabel = QString(tr("Left slope: "));
        break;
    case eEditModeRightDerivative:
        xLabel = QString(tr("Right slope: "));
        break;
    }
    _imp->xLabel = new Label(xLabel,_imp->boxContainer);
    _imp->xLabel->setFont(QApplication::font()); // necessary, or the labels will get the default font size
    _imp->boxLayout->addWidget(_imp->xLabel);
    
    SpinBox::SpinBoxTypeEnum xType;
    
//    if (mode == eEditModeKeyframePosition) {
//        xType = key->curve->areKeyFramesTimeClampedToIntegers() ? SpinBox::eSpinBoxTypeInt : SpinBox::eSpinBoxTypeDouble;
//    } else {
        xType = SpinBox::eSpinBoxTypeDouble;
//    }
    
    _imp->xSpinbox = new SpinBox(_imp->boxContainer,xType);
    _imp->xSpinbox->setValue(_imp->originalX);
    QObject::connect(_imp->xSpinbox, SIGNAL(valueChanged(double)), this, SLOT(onXSpinBoxValueChanged(double)));
    QObject::connect(_imp->xSpinbox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
    _imp->boxLayout->addWidget(_imp->xSpinbox);
    
    if (mode == eEditModeKeyframePosition) {
        

        _imp->yLabel = new Label("y: ",_imp->boxContainer);
        _imp->yLabel->setFont(QApplication::font()); // necessary, or the labels will get the default font size
        _imp->boxLayout->addWidget(_imp->yLabel);
        
        bool clampedToInt = key->curve->areKeyFramesValuesClampedToIntegers() ;
        bool clampedToBool = key->curve->areKeyFramesValuesClampedToBooleans();
        SpinBox::SpinBoxTypeEnum yType = (clampedToBool || clampedToInt) ? SpinBox::eSpinBoxTypeInt : SpinBox::eSpinBoxTypeDouble;
        
        _imp->ySpinbox = new SpinBox(_imp->boxContainer,yType);
        
        if (clampedToBool) {
            _imp->ySpinbox->setMinimum(0);
            _imp->ySpinbox->setMaximum(1);
        } else {
            std::pair<double,double> range = key->curve->getCurveYRange();
            _imp->ySpinbox->setMinimum(range.first);
            _imp->ySpinbox->setMaximum(range.second);
        }
        
        _imp->ySpinbox->setValue(_imp->originalY);
        QObject::connect(_imp->ySpinbox, SIGNAL(valueChanged(double)), this, SLOT(onYSpinBoxValueChanged(double)));
        QObject::connect(_imp->ySpinbox, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
        _imp->boxLayout->addWidget(_imp->ySpinbox);
    }
    
    _imp->mainLayout->addWidget(_imp->boxContainer);

}

EditKeyFrameDialog::~EditKeyFrameDialog()
{
    
}

void
EditKeyFrameDialog::moveKeyTo(double newX,double newY)
{
    std::vector<MoveKeysCommand::KeyToMove> keys(1);
    keys[0].key = _imp->key;
    keys[0].prevIsSelected = false;
    keys[0].nextIsSelected = false;
    double curY = _imp->key->key.getValue();
    double curX = _imp->key->key.getTime();
    
    if (_imp->mode == eEditModeKeyframePosition) {
        ///Check that another keyframe doesn't have this time

        int expectedEqualKeys = 0;
        if (newX == curX) {
            expectedEqualKeys = 1;
        }
        
        int curEqualKeys = 0;
        KeyFrameSet set = _imp->key->curve->getKeyFrames();
        for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
            
            if (std::abs(it->getTime() - newX) <= NATRON_CURVE_X_SPACING_EPSILON) {
                _imp->xSpinbox->setValue(curX);
                if (curEqualKeys >= expectedEqualKeys) {
                    return;
                }
                ++curEqualKeys;
            }
        }
    }
    
    _imp->curveWidget->pushUndoCommand(new MoveKeysCommand(_imp->curveWidget,keys,newX - curX, newY - curY,true));

}

void
EditKeyFrameDialog::moveDerivativeTo(double d)
{
    MoveTangentCommand::SelectedTangentEnum deriv;
    if (_imp->mode == eEditModeLeftDerivative) {
        deriv = MoveTangentCommand::eSelectedTangentLeft;
    } else {
        deriv = MoveTangentCommand::eSelectedTangentRight;
    }
    _imp->curveWidget->pushUndoCommand(new MoveTangentCommand(_imp->curveWidget,deriv,_imp->key,d));

}

void
EditKeyFrameDialog::onXSpinBoxValueChanged(double d)
{
    if (_imp->mode == eEditModeKeyframePosition) {
        moveKeyTo(d, _imp->key->key.getValue());
    } else {
        moveDerivativeTo(d);
    }
}

void
EditKeyFrameDialog::onYSpinBoxValueChanged(double d)
{
    moveKeyTo(_imp->key->key.getTime(), d);

}
                     
void
EditKeyFrameDialog::onEditingFinished()
{
    _imp->wasAccepted = true;
    accept();
}

void
EditKeyFrameDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        _imp->wasAccepted = true;
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        if (_imp->mode == eEditModeKeyframePosition) {
            moveKeyTo(_imp->originalX, _imp->originalY);
        } else {
            moveDerivativeTo(_imp->originalX);
        }
        _imp->xSpinbox->blockSignals(true);
        if (_imp->ySpinbox) {
            _imp->ySpinbox->blockSignals(true);
        }
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
EditKeyFrameDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange && !_imp->wasAccepted) {
        if ( !isActiveWindow() ) {
            if (_imp->mode == eEditModeKeyframePosition) {
                moveKeyTo(_imp->originalX, _imp->originalY);
            } else {
                moveDerivativeTo(_imp->originalX);
            }
            _imp->xSpinbox->blockSignals(true);
            if (_imp->ySpinbox) {
                _imp->ySpinbox->blockSignals(true);
            }
            reject();
            
            return;
        }
    }
    QDialog::changeEvent(e);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_CurveWidgetDialogs.cpp"
