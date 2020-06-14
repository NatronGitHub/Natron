/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <QtCore/QDebug>

#include "Engine/AppManager.h" // appPTR
#include "Engine/Knob.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/AnimationModuleBase.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/LineEdit.h" // LineEdit
#include "Gui/SpinBox.h" // SpinBox
#include "Gui/Button.h" // Button
#include "Gui/SequenceFileDialog.h" // SequenceFileDialog
#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h" // Label
#include "Gui/AnimationModuleUndoRedo.h"

NATRON_NAMESPACE_ENTER

ImportExportCurveDialog::ImportExportCurveDialog(bool isExportDialog,
                                                 const std::vector<CurveGuiPtr> & curves,
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
    , _startLineEdit(0)
    , _incrContainer(0)
    , _incrLayout(0)
    , _incrLabel(0)
    , _incrLineEdit(0)
    , _endContainer(0)
    , _endLayout(0)
    , _endLabel(0)
    , _endLineEdit(0)
    , _curveColumns()
    , _buttonBox(0)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool integerIncrement = false;
    double xstart = -std::numeric_limits<double>::infinity();
    double xend = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < curves.size(); ++i) {
        integerIncrement |= curves[i]->areKeyFramesTimeClampedToIntegers();
        std::pair<double,double> xrange = curves[i]->getInternalCurve()->getXRange();
        xstart = std::max(xstart, xrange.first);
        xend = std::min(xend, xrange.second);
    }
    if (xstart == -std::numeric_limits<double>::infinity()) {
        xstart = std::min(0., xend - 1.);
    }
    if (xend == std::numeric_limits<double>::infinity()) {
        xend = std::max(xstart + 1., 1.);
    }
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0, TO_DPIX(3), 0, 0);
    _mainLayout->setSpacing(TO_DPIX(2));
    setLayout(_mainLayout);

    //////File
    _fileContainer = new QWidget(this);
    _fileLayout = new QHBoxLayout(_fileContainer);
    _fileLabel = new Label(tr("File:"), _fileContainer);
    _fileLayout->addWidget(_fileLabel);
    _fileLineEdit = new LineEdit(_fileContainer);
    _fileLineEdit->setPlaceholderText( tr("File path...") );
    _fileLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _fileLayout->addWidget(_fileLineEdit);
    _fileBrowseButton = new Button(_fileContainer);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _fileBrowseButton->setIcon( QIcon(pix) );
    QObject::connect( _fileBrowseButton, SIGNAL(clicked()), this, SLOT(open_file()) );
    _fileLayout->addWidget(_fileBrowseButton);
    _mainLayout->addWidget(_fileContainer);

    //////x start value
    _startContainer = new QWidget(this);
    _startLayout = new QHBoxLayout(_startContainer);
    _startLabel = new Label(tr("X start value:"), _startContainer);
    _startLayout->addWidget(_startLabel);
    _startLineEdit = new LineEdit(_startContainer);
    _startLineEdit->setText(QString::number(xstart,'g',10));
    _startLineEdit->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The X of the first value in the ASCII file. This can be a python expression."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _startLayout->addWidget(_startLineEdit);
    _mainLayout->addWidget(_startContainer);

    //////x increment
    _incrContainer = new QWidget(this);
    _incrLayout = new QHBoxLayout(_incrContainer);
    _incrLabel = new Label(tr("X increment:"), _incrContainer);
    _incrLayout->addWidget(_incrLabel);
    _incrLineEdit = new LineEdit(_incrContainer);
    if (xstart == 0. && xend == 1.) {
        _incrLineEdit->setText(QString::fromUtf8("1./255"));
    } else {
        _incrLineEdit->setText(QString::number(1));
    }
    _incrLineEdit->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The X increment between two consecutive values. This can be a python expression."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _incrLayout->addWidget(_incrLineEdit);
    _mainLayout->addWidget(_incrContainer);

    //////x end value
    if (isExportDialog) {
        _endContainer = new QWidget(this);
        _endLayout = new QHBoxLayout(_endContainer);
        _endLabel = new Label(tr("X end value:"), _endContainer);
        _endLabel->setFont( QApplication::font() ); // necessary, or the labels will get the default font size
        _endLayout->addWidget(_endLabel);
        _endLineEdit = new LineEdit(_endContainer);
        _endLineEdit->setText(QString::number(xend,'g',10));
        _incrLineEdit->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The X of the last value in the ASCII file. This can be a python expression."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _endLayout->addWidget(_endLineEdit);
        _mainLayout->addWidget(_endContainer);
    }

    ////curves columns
    for (U32 i = 0; i < curves.size(); ++i) {
        CurveColumn column;
        column._curve = curves[i];
        column._curveContainer = new QWidget(this);
        column._curveLayout = new QHBoxLayout(column._curveContainer);
        column._curveLabel = new Label( tr("%1 column:").arg( curves[i]->getName() ) );
        column._curveLabel->setFont( QApplication::font() ); // necessary, or the labels will get the default font size
        column._curveLayout->addWidget(column._curveLabel);
        column._curveSpinBox = new SpinBox(column._curveContainer, SpinBox::eSpinBoxTypeInt);
        column._curveSpinBox->setValue( (double)i + 1. );
        column._curveLayout->addWidget(column._curveSpinBox);
        _curveColumns.push_back(column);
        _mainLayout->addWidget(column._curveContainer);
    }
    /////buttons
    _buttonBox = new DialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel), Qt::Horizontal, this);
    QObject::connect( _buttonBox, SIGNAL(rejected()), this, SLOT(reject()) );
    QObject::connect( _buttonBox, SIGNAL(accepted()), this, SLOT(accept()) );
    _mainLayout->addWidget(_buttonBox);

    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    QByteArray state;
    if (isExportDialog) {
        state = settings.value( QLatin1String("CurveWidgetExportDialog") ).toByteArray();
    } else {
        state = settings.value( QLatin1String("CurveWidgetImportDialog") ).toByteArray();
    }
    if ( !state.isEmpty() ) {
        restoreState(state);
    }
}

ImportExportCurveDialog::~ImportExportCurveDialog()
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

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
#if 0 // start and incr depend on the curve and are calculated from it
    stream << _startLineEdit->text();
    stream << _incrLineEdit->text();
    if (_isExportDialog) {
        stream << _endLineEdit->text();
    }
#endif
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
    stream >> file;
    _fileLineEdit->setText(file);
#if 0 // start and incr depend on the curve and are calculated from it
    QString start, incr, end;
    stream >> start;
    stream >> incr;
    _fileLineEdit->setText(file);
    _startLineEdit->setText(start);
    _incrLineEdit->setText(incr);
    if (_isExportDialog) {
        stream >> end;
        _endLineEdit->setText(end);
    }
#endif
}

void
ImportExportCurveDialog::open_file()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    std::vector<std::string> filters;
    filters.push_back("*");
    if (_isExportDialog) {
        SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::eFileDialogModeSave, "", _gui, false);
        if ( dialog.exec() ) {
            std::string file = dialog.filesToSave();
            _fileLineEdit->setText( QString::fromUtf8( file.c_str() ) );
        }
    } else {
        SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::eFileDialogModeOpen, "", _gui, false);
        if ( dialog.exec() ) {
            std::string files = dialog.selectedFiles();
            if ( !files.empty() ) {
                _fileLineEdit->setText( QString::fromUtf8( files.c_str() ) );
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

    double ret = 0.;
    std::string expr = std::string("ret = float(") + _startLineEdit->text().toStdString() + ')';
    std::string error;
    std::string retIsString;
    KnobHelper::ExpressionReturnValueTypeEnum stat = KnobHelper::evaluateExpression(expr, eExpressionLanguagePython, &ret, &retIsString, &error);
    if (stat != KnobHelper::eExpressionReturnValueTypeScalar) {
        return 0;
    }

    //qDebug() << "xstart=" << expr.c_str() << ret;
    return ret;
}

double
ImportExportCurveDialog::getXIncrement() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    double ret = 0.01;
    std::string expr = std::string("ret = float(") + _incrLineEdit->text().toStdString() + ')';
    std::string error;
    std::string retIsString;
    KnobHelper::ExpressionReturnValueTypeEnum stat = KnobHelper::evaluateExpression(expr, eExpressionLanguagePython, &ret, &retIsString, &error);
    if (stat != KnobHelper::eExpressionReturnValueTypeScalar) {
        return 0;
    }

    //qDebug() << "incr=" << expr.c_str() << ret;
    return ret;
}

int
ImportExportCurveDialog::getXCount() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///only valid for export dialogs
    assert(_isExportDialog);

    double ret = 0.;
    std::string expr = std::string("ret = float(1+((")  + _endLineEdit->text().toStdString() + std::string(")-(") + _startLineEdit->text().toStdString() + std::string("))/(") + _incrLineEdit->text().toStdString() + std::string("))");
    std::string error;
    std::string retIsString;
    KnobHelper::ExpressionReturnValueTypeEnum stat = KnobHelper::evaluateExpression(expr, eExpressionLanguagePython, &ret, &retIsString, &error);
    if (stat != KnobHelper::eExpressionReturnValueTypeScalar) {
        return 0;
    }

    //qDebug() << "count=" << expr.c_str() << ret;
    return (int) (std::floor(ret + 0.5));
}

double
ImportExportCurveDialog::getXEnd() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    ///only valid for export dialogs
    assert(_isExportDialog);

    double ret = 1.;
    std::string expr = std::string("ret = float(") + _endLineEdit->text().toStdString() + ')';
    std::string retIsString, error;
    KnobHelper::ExpressionReturnValueTypeEnum stat = KnobHelper::evaluateExpression(expr, eExpressionLanguagePython, &ret, &retIsString, &error);
    if (stat != KnobHelper::eExpressionReturnValueTypeScalar) {
        return 0;
    }

    //qDebug() << "xend=" << expr.c_str() << ret;
    return ret;
}

void
ImportExportCurveDialog::getCurveColumns(std::map<int, CurveGuiPtr>* columns) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    for (U32 i = 0; i < _curveColumns.size(); ++i) {
        columns->insert( std::make_pair( (int)(_curveColumns[i]._curveSpinBox->value() - 1), _curveColumns[i]._curve ) );
    }
}

struct EditKeyFrameDialogPrivate
{
    AnimationModuleView* curveWidget;
    AnimItemDimViewKeyFrame key;
    double originalX, originalY;
    QVBoxLayout* mainLayout;
    QWidget* boxContainer;
    QHBoxLayout* boxLayout;
    Label* xLabel;
    SpinBox* xSpinbox;
    Label* yLabel;
    SpinBox* ySpinbox;
    bool wasAccepted;
    EditKeyFrameDialog::EditModeEnum mode;

    EditKeyFrameDialogPrivate(EditKeyFrameDialog::EditModeEnum mode,
                              AnimationModuleView* curveWidget,
                              const AnimItemDimViewKeyFrame& key)
        : curveWidget(curveWidget)
        , key(key)
        , originalX( key.key.getTime() )
        , originalY( key.key.getValue() )
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
            originalX = key.key.getLeftDerivative();
        } else if (mode == EditKeyFrameDialog::eEditModeRightDerivative) {
            originalX = key.key.getRightDerivative();
        }
    }
};

EditKeyFrameDialog::EditKeyFrameDialog(EditModeEnum mode,
                                       AnimationModuleView* curveWidget,
                                       const AnimItemDimViewKeyFrame& key,
                                       QWidget* parent)
    : QDialog(parent)
    , _imp( new EditKeyFrameDialogPrivate(mode, curveWidget, key) )
{
    setWindowFlags(Qt::Popup);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);

    _imp->boxContainer = new QWidget(this);
    _imp->boxLayout = new QHBoxLayout(_imp->boxContainer);
    _imp->boxLayout->setContentsMargins(0, 0, 0, 0);

    QString xLabel;
    switch (mode) {
    case eEditModeKeyframePosition:
        xLabel = QString::fromUtf8("x: ");
        break;
    case eEditModeLeftDerivative:
        xLabel = QString( tr("Left slope: ") );
        break;
    case eEditModeRightDerivative:
        xLabel = QString( tr("Right slope: ") );
        break;
    }
    _imp->xLabel = new Label(xLabel, _imp->boxContainer);
    _imp->xLabel->setFont( QApplication::font() ); // necessary, or the labels will get the default font size
    _imp->boxLayout->addWidget(_imp->xLabel);

    SpinBox::SpinBoxTypeEnum xType;

//    if (mode == eEditModeKeyframePosition) {
//        xType = key->curve->areKeyFramesTimeClampedToIntegers() ? SpinBox::eSpinBoxTypeInt : SpinBox::eSpinBoxTypeDouble;
//    } else {
    xType = SpinBox::eSpinBoxTypeDouble;
//    }

    _imp->xSpinbox = new SpinBox(_imp->boxContainer, xType);
    _imp->xSpinbox->setValue(_imp->originalX);
    QObject::connect( _imp->xSpinbox, SIGNAL(valueChanged(double)), this, SLOT(onXSpinBoxValueChanged(double)) );
    QObject::connect( _imp->xSpinbox, SIGNAL(returnPressed()), this, SLOT(onEditingFinished()) );
    _imp->boxLayout->addWidget(_imp->xSpinbox);

    if (mode == eEditModeKeyframePosition) {
        _imp->yLabel = new Label(QString::fromUtf8("y: "), _imp->boxContainer);
        _imp->yLabel->setFont( QApplication::font() ); // necessary, or the labels will get the default font size
        _imp->boxLayout->addWidget(_imp->yLabel);

        CurveGuiPtr curve = key.id.item->getCurveGui(key.id.dim, key.id.view);
        assert(curve);
        bool clampedToInt = curve->areKeyFramesValuesClampedToIntegers();
        bool clampedToBool = curve->areKeyFramesValuesClampedToBooleans();
        SpinBox::SpinBoxTypeEnum yType = (clampedToBool || clampedToInt) ? SpinBox::eSpinBoxTypeInt : SpinBox::eSpinBoxTypeDouble;

        _imp->ySpinbox = new SpinBox(_imp->boxContainer, yType);

        if (clampedToBool) {
            _imp->ySpinbox->setMinimum(0);
            _imp->ySpinbox->setMaximum(1);
        } else {
            Curve::YRange range = curve->getCurveYRange();
            _imp->ySpinbox->setMinimum(range.min);
            _imp->ySpinbox->setMaximum(range.max);
        }

        _imp->ySpinbox->setValue(_imp->originalY);
        QObject::connect( _imp->ySpinbox, SIGNAL(valueChanged(double)), this, SLOT(onYSpinBoxValueChanged(double)) );
        QObject::connect( _imp->ySpinbox, SIGNAL(returnPressed()), this, SLOT(onEditingFinished()) );
        _imp->boxLayout->addWidget(_imp->ySpinbox);
    }

    _imp->mainLayout->addWidget(_imp->boxContainer);
}

EditKeyFrameDialog::~EditKeyFrameDialog()
{
}

void
EditKeyFrameDialog::moveKeyTo(double newX, double newY)
{
    AnimationModuleBasePtr model = _imp->key.id.item->getModel();

    CurvePtr curve = _imp->key.id.item->getCurve(_imp->key.id.dim, _imp->key.id.view);
    if (!curve) {
        return;
    }

    AnimItemDimViewKeyFramesMap keysToMove;
    KeyFrameSet& keys = keysToMove[_imp->key.id];
    keys.insert(_imp->key.key);

    double curY = _imp->key.key.getValue();
    double curX = _imp->key.key.getTime();

    if (_imp->mode == eEditModeKeyframePosition) {
        // Check that another keyframe doesn't have this time

        int expectedEqualKeys = 0;
        if (newX == curX) {
            expectedEqualKeys = 1;
        }

        int curEqualKeys = 0;
        KeyFrameSet set = curve->getKeyFrames_mt_safe();
        for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
            if (std::abs(it->getTime() - newX) <= NATRON_CURVE_X_SPACING_EPSILON) {
                _imp->xSpinbox->setValue(curX);
                if (curEqualKeys >= expectedEqualKeys) {
                    return;
                }
                ++curEqualKeys;
            }
        }

        std::vector<NodeAnimPtr> nodesToMove;
        std::vector<TableItemAnimPtr> tableItemsToMove;
        model->pushUndoCommand( new WarpKeysCommand(keysToMove, model, nodesToMove, tableItemsToMove, newX - curX, newY - curY) );
        refreshSelectedKey();
    }

}

void
EditKeyFrameDialog::refreshSelectedKey()
{
    AnimationModuleBasePtr model = _imp->key.id.item->getModel();
    const AnimItemDimViewKeyFramesMap& selectedKeys = model->getSelectionModel()->getCurrentKeyFramesSelection();
    assert(!selectedKeys.empty() && selectedKeys.size() == 1);
    AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin();
    _imp->key.id = it->first;
    assert(!it->second.empty() && it->second.size() == 1);
    _imp->key.key = *it->second.begin();

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
    AnimationModuleBasePtr model = _imp->key.id.item->getModel();
    model->pushUndoCommand( new MoveTangentCommand(_imp->key.id.item->getModel(), deriv, _imp->key, d) );
    refreshSelectedKey();
}

void
EditKeyFrameDialog::onXSpinBoxValueChanged(double d)
{
    if (_imp->mode == eEditModeKeyframePosition) {
        moveKeyTo( d, _imp->key.key.getValue() );
    } else {
        moveDerivativeTo(d);
    }
}

void
EditKeyFrameDialog::onYSpinBoxValueChanged(double d)
{
    moveKeyTo(_imp->key.key.getTime(), d);
}

void
EditKeyFrameDialog::onEditingFinished()
{
    _imp->wasAccepted = true;
    Q_EMIT dialogFinished(true);
}

void
EditKeyFrameDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        _imp->wasAccepted = true;
        Q_EMIT dialogFinished(true);
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
        Q_EMIT dialogFinished(false);
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
EditKeyFrameDialog::changeEvent(QEvent* e)
{
    if ( (e->type() == QEvent::ActivationChange) && !_imp->wasAccepted ) {
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
            Q_EMIT dialogFinished(false);

            return;
        }
    }
    QDialog::changeEvent(e);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_CurveWidgetDialogs.cpp"
