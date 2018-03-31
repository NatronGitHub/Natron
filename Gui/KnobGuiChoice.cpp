/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "KnobGuiChoice.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QToolTip>
#include <QTreeWidget>
#include <QAction>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER
using std::make_pair;


//=============================CHOICE_KNOB_GUI===================================
KnobComboBox::KnobComboBox(const KnobGuiPtr& knob,
                           int dimension,
                           QWidget* parent)
    : ComboBox(parent)
    , _dnd( KnobWidgetDnD::create(knob, dimension, this) )
{
}

KnobComboBox::~KnobComboBox()
{
}

void
KnobComboBox::wheelEvent(QWheelEvent *e)
{
    bool mustIgnore = false;

    if ( !_dnd->mouseWheel(e) ) {
        mustIgnore = true;
        ignoreWheelEvent = true;
    }
    ComboBox::wheelEvent(e);
    if (mustIgnore) {
        ignoreWheelEvent = false;
    }
}

void
KnobComboBox::enterEvent(QEvent* e)
{
    _dnd->mouseEnter(e);
    ComboBox::enterEvent(e);
}

void
KnobComboBox::leaveEvent(QEvent* e)
{
    _dnd->mouseLeave(e);
    ComboBox::leaveEvent(e);
}

void
KnobComboBox::keyPressEvent(QKeyEvent* e)
{
    _dnd->keyPress(e);
    ComboBox::keyPressEvent(e);
}

void
KnobComboBox::keyReleaseEvent(QKeyEvent* e)
{
    _dnd->keyRelease(e);
    ComboBox::keyReleaseEvent(e);
}

void
KnobComboBox::mousePressEvent(QMouseEvent* e)
{
    if ( !_dnd->mousePress(e) ) {
        ComboBox::mousePressEvent(e);
    }
}

void
KnobComboBox::mouseMoveEvent(QMouseEvent* e)
{
    if ( !_dnd->mouseMove(e) ) {
        ComboBox::mouseMoveEvent(e);
    }
}

void
KnobComboBox::mouseReleaseEvent(QMouseEvent* e)
{
    _dnd->mouseRelease(e);
    ComboBox::mouseReleaseEvent(e);
}

void
KnobComboBox::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !_dnd->dragEnter(e) ) {
        ComboBox::dragEnterEvent(e);
    }
}

void
KnobComboBox::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !_dnd->dragMove(e) ) {
        ComboBox::dragMoveEvent(e);
    }
}

void
KnobComboBox::dropEvent(QDropEvent* e)
{
    if ( !_dnd->drop(e) ) {
        ComboBox::dropEvent(e);
    }
}

void
KnobComboBox::focusInEvent(QFocusEvent* e)
{
    _dnd->focusIn();
    ComboBox::focusInEvent(e);
}

void
KnobComboBox::focusOutEvent(QFocusEvent* e)
{
    _dnd->focusOut();
    ComboBox::focusOutEvent(e);
}

KnobGuiChoice::KnobGuiChoice(KnobIPtr knob,
                             KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _comboBox(0)
{
    boost::shared_ptr<KnobChoice> k = boost::dynamic_pointer_cast<KnobChoice>(knob);
    QObject::connect( k.get(), SIGNAL(populated()), this, SLOT(onEntriesPopulated()) );
    QObject::connect( k.get(), SIGNAL(entryAppended()), this, SLOT(onEntryAppended()) );
    QObject::connect( k.get(), SIGNAL(entriesReset()), this, SLOT(onEntriesReset()) );

    _knob = k;
}

KnobGuiChoice::~KnobGuiChoice()
{
}

void
KnobGuiChoice::removeSpecificGui()
{
    _comboBox->deleteLater();
}

void
KnobGuiChoice::createWidget(QHBoxLayout* layout)
{
    _comboBox = new KnobComboBox( shared_from_this(), 0, layout->parentWidget() );
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (!knob) {
        return;
    }
    _comboBox->setCascading( knob->isCascading() );
    onEntriesPopulated();

    QObject::connect( _comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)) );
    QObject::connect( _comboBox, SIGNAL(itemNewSelected()), this, SLOT(onItemNewSelected()) );
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_comboBox, 0);

    layout->addWidget(_comboBox);
}

void
KnobGuiChoice::onCurrentIndexChanged(int i)
{
    setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, QString() );
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (!knob) {
        return;
    }
    pushUndoCommand( new KnobUndoCommand<int>(shared_from_this(), knob->getValue(0), i, 0, false, 0) );
}

void
KnobGuiChoice::onEntryAppended()
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (!knob) {
        return;
    }

    std::vector<ChoiceOption> options = knob->getEntries_mt_safe();

    for (int i = _comboBox->count(); i < (int)options.size(); ++i) {
        if ( knob->getHostCanAddOptions()) {
            _comboBox->insertItem(_comboBox->count() - 1, QString::fromUtf8(options[i].label.c_str()), QIcon(), QKeySequence(), QString::fromUtf8(options[i].tooltip.c_str()));
        } else {
            _comboBox->addItem(QString::fromUtf8(options[i].label.c_str()), QIcon(), QKeySequence(), QString::fromUtf8(options[i].tooltip.c_str()));
        }

    }

    updateGUI(0);
}

void
KnobGuiChoice::onEntriesReset()
{
    onEntriesPopulated();
}


void
KnobGuiChoice::onEntriesPopulated()
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();

    _comboBox->clear();
    std::vector<ChoiceOption> entries = knob->getEntries_mt_safe();



    for (U32 i = 0; i < entries.size(); ++i) {
        _comboBox->addItem( QString::fromUtf8( entries[i].label.c_str() ), QIcon(), QKeySequence(), QString::fromUtf8( entries[i].tooltip.c_str() ) );
    } // for all entries

    // the "New" menu is only added to known parameters (e.g. the choice of output channels)
    if (knob->getHostCanAddOptions()) {
        _comboBox->addItemNew();
    }
    
    updateGUI(0);
}

void
KnobGuiChoice::onItemNewSelected()
{
    NewLayerDialog dialog( ImagePlaneDesc::getNoneComponents(), getGui() );

    if ( dialog.exec() ) {
        ImagePlaneDesc comps = dialog.getComponents();
        if ( comps == ImagePlaneDesc::getNoneComponents() ) {
            Dialogs::errorDialog( tr("Layer").toStdString(), tr("A layer must contain at least 1 channel and channel names must be "
                                                                "Python compliant.").toStdString() );

            return;
        }
        boost::shared_ptr<KnobChoice> knob = _knob.lock();
        if (!knob) {
            return;
        }
        KnobHolder* holder = knob->getHolder();
        assert(holder);
        EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
        assert(effect);
        if (effect) {
            assert( effect->getNode() );
            if ( !effect->getNode()->addUserComponents(comps) ) {
                Dialogs::errorDialog( tr("Layer").toStdString(), tr("A Layer with the same name already exists").toStdString() );
            }
        }
    }
}

void
KnobGuiChoice::reflectExpressionState(int /*dimension*/,
                                      bool hasExpr)
{
    _comboBox->setAnimation(3);
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool isEnabled = knob->isEnabled(0);
    _comboBox->setEnabled_natron(!hasExpr && isEnabled);
}

void
KnobGuiChoice::updateToolTip()
{
    QString tt = toolTip();

    _comboBox->setToolTip( tt );
}

// The project only saves the choice ID that was selected and not the associated label.
// If the ID cannot be found in the menu upon loading the project, then the ID of the choice
// will be displayed in the menu.
// For a plane selector, if the plane is not present in the menu when loading the project it will display
// the raw ID of the plane which contains stuff that should not be displayed to the user.
static void ensureUnknownChocieIsNotInternalPlaneID(QString& label)
{
    if (label.contains(QLatin1String(kNatronColorPlaneID))) {
        label.replace(QLatin1String(kNatronColorPlaneID), QLatin1String(kNatronColorPlaneLabel)); ;
    } else if (label.contains(QLatin1String(kNatronBackwardMotionVectorsPlaneID))) {
        label.replace(QLatin1String(kNatronBackwardMotionVectorsPlaneID), QLatin1String(kNatronBackwardMotionVectorsPlaneLabel)); ;
    } else if (label.contains(QLatin1String(kNatronForwardMotionVectorsPlaneID))) {
        label.replace(QLatin1String(kNatronForwardMotionVectorsPlaneID), QLatin1String(kNatronForwardMotionVectorsPlaneLabel)); ;
    } else if (label.contains(QLatin1String(kNatronDisparityLeftPlaneID))) {
        label.replace(QLatin1String(kNatronDisparityLeftPlaneID), QLatin1String(kNatronDisparityLeftPlaneLabel)); ;
    } else if (label.contains(QLatin1String(kNatronDisparityRightPlaneID))) {
        label.replace(QLatin1String(kNatronDisparityRightPlaneID), QLatin1String(kNatronDisparityRightPlaneLabel)); ;
    }

}

void
KnobGuiChoice::updateGUI(int /*dimension*/)
{
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///change the internal value of the knob again...
    ///The slot connected to onCurrentIndexChanged is reserved to catch user interaction with the combobox.
    ///This function is called in response to an internal change.
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    ChoiceOption activeEntry = knob->getActiveEntry();

    QString activeEntryLabel;
    if (!activeEntry.label.empty()) {
        activeEntryLabel = QString::fromUtf8(activeEntry.label.c_str());
    } else {
        activeEntryLabel = QString::fromUtf8(activeEntry.id.c_str());
    }
    if ( !activeEntry.id.empty() ) {
        bool activeIndexPresent = knob->isActiveEntryPresentInEntries();
        if (!activeIndexPresent) {
            QString error = tr("The value %1 no longer exist in the menu.").arg(activeEntryLabel);
            setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, NATRON_NAMESPACE::convertFromPlainText(error, NATRON_NAMESPACE::WhiteSpaceNormal) );
        } else {
            setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, QString() );
        }
    }
    if ( _comboBox->isCascading() || activeEntry.id.empty() ) {
        _comboBox->setCurrentIndex_no_emit( knob->getValue() );
    } else {
        ensureUnknownChocieIsNotInternalPlaneID(activeEntryLabel);
        _comboBox->setCurrentText_no_emit( activeEntryLabel );
    }
}

void
KnobGuiChoice::reflectAnimationLevel(int /*dimension*/,
                                     AnimationLevelEnum level)
{
    int value;

    switch (level) {
    case eAnimationLevelNone:
        value = 0;
        break;
    case eAnimationLevelInterpolatedValue:
        value = 1;
        break;
    case eAnimationLevelOnKeyframe:
        value = 2;
        break;
    default:
        value = 0;
        break;
    }
    if ( value != _comboBox->getAnimation() ) {
        _comboBox->setAnimation(value);
    }
}

void
KnobGuiChoice::_hide()
{
    _comboBox->hide();
}

void
KnobGuiChoice::_show()
{
    _comboBox->show();
}

void
KnobGuiChoice::setEnabled()
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    bool b = knob->isEnabled(0) && knob->getExpression(0).empty();

    _comboBox->setEnabled_natron(b);
}

void
KnobGuiChoice::setReadOnly(bool readOnly,
                           int /*dimension*/)
{
    _comboBox->setEnabled_natron(!readOnly);
}

void
KnobGuiChoice::setDirty(bool dirty)
{
    _comboBox->setDirty(dirty);
}

KnobIPtr
KnobGuiChoice::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiChoice::reflectModificationsState()
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool hasModif = knob->hasModifications();

    _comboBox->setAltered(!hasModif);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiChoice.cpp"
