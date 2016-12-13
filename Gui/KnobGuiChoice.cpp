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
#include <QPainter>
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
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Plugin.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/GuiDefines.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include "ofxNatron.h"


NATRON_NAMESPACE_ENTER;
using std::make_pair;


KnobComboBox::KnobComboBox(const KnobGuiPtr& knob,
                           DimSpec dimension,
                           ViewIdx view,
                           QWidget* parent)
    : ComboBox(parent)
    , _knob(toKnobChoice(knob->getKnob()))
    , _dnd( KnobWidgetDnD::create(knob, dimension, view, this) )
    , _drawLinkedFrame(false)
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

void
KnobComboBox::setLinkedFrameEnabled(bool enabled)
{
    _drawLinkedFrame = enabled;
    update();
}

void
KnobComboBox::paintEvent(QPaintEvent* event)
{
    ComboBox::paintEvent(event);
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    RGBAColourD color;
    if (_drawLinkedFrame) {
        appPTR->getCurrentSettings()->getExprColor(&color.r, &color.g, &color.b);
        color.a = 1.;
    } else {
        int idx = activeIndex();
        if (!knob->getColorForIndex(idx, &color)) {
            return;
        }
    }

    QPainter p(this);
    QPen pen;
    QColor c;
    c.setRgbF(Image::clamp(color.r,0.,1.),
              Image::clamp(color.g,0.,1.),
              Image::clamp(color.b,0.,1.));
    c.setAlphaF(Image::clamp(color.a,0.,1.));

    pen.setColor(c);
    p.setPen(pen);

    QRectF bRect = rect();
    QRectF roundedRect = bRect.adjusted(1., 1., -2., -2.);
    double roundPixels = 3;
    QPainterPath path;
    path.addRoundedRect(roundedRect, roundPixels, roundPixels);
    p.drawPath(path);
    
} // paintEvent

KnobGuiChoice::KnobGuiChoice(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _comboBox(0)
{
    KnobChoicePtr k = toKnobChoice(knob->getKnob());
    QObject::connect( k.get(), SIGNAL(populated()), this, SLOT(onEntriesPopulated()) );
    QObject::connect( k.get(), SIGNAL(entryAppended(QString,QString)), this, SLOT(onEntryAppended(QString,QString)) );
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
    KnobChoicePtr knob = _knob.lock();
    KnobGuiPtr knobUI = getKnobGui();


    _comboBox = new KnobComboBox( knobUI, DimIdx(0), getView(), layout->parentWidget() );



    _comboBox->setCascading( _knob.lock()->isCascading() );
    onEntriesPopulated();

    std::string textToFitHorizontally = knob->getTextToFitHorizontally();
    if (!textToFitHorizontally.empty()) {
        QFontMetrics fm = _comboBox->fontMetrics();
        _comboBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        _comboBox->setFixedWidth(fm.width(QString::fromUtf8(textToFitHorizontally.c_str())) + 3 * TO_DPIX(DROP_DOWN_ICON_SIZE));
    }

    QObject::connect( _comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)) );
    QObject::connect( _comboBox, SIGNAL(itemNewSelected()), this, SLOT(onItemNewSelected()) );
    ///set the copy/link actions in the right click menu
    KnobGuiWidgets::enableRightClickMenu(knobUI, _comboBox, DimIdx(0), getView());

    layout->addWidget(_comboBox);
}

void
KnobGuiChoice::onCurrentIndexChanged(int i)
{
    KnobGuiPtr knobUI = getKnobGui();
    knobUI->setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, QString() );
    KnobChoicePtr knob = _knob.lock();
    knobUI->pushUndoCommand( new KnobUndoCommand<int>(knob, knob->getValue(DimIdx(0), getView()), i, DimIdx(0), getView()));
}

void
KnobGuiChoice::onEntryAppended(const QString& entry,
                               const QString& help)
{
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string activeEntry = knob->getActiveEntryText();

    if ( knob->getNewOptionCallback()) {
        _comboBox->insertItem(_comboBox->count() - 1, entry, QIcon(), QKeySequence(), help);
    } else {
        _comboBox->addItem(entry, QIcon(), QKeySequence(), help);
    }
    int activeIndex = knob->getValue();
    if (activeIndex >= 0) {
        _comboBox->setCurrentIndex_no_emit(activeIndex);
    } else {
        _comboBox->setCurrentText_no_emit( QString::fromUtf8( activeEntry.c_str() ) );
    }
    if ( !activeEntry.empty() ) {
        bool activeIndexPresent = knob->isActiveEntryPresentInEntries(getView());
        if (!activeIndexPresent) {
            QString error = tr("The value %1 no longer exist in the menu.").arg(QString::fromUtf8(activeEntry.c_str()));
            getKnobGui()->setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, NATRON_NAMESPACE::convertFromPlainText(error, NATRON_NAMESPACE::WhiteSpaceNormal) );
        } else {
            getKnobGui()->setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, QString() );
        }
    }
}

void
KnobGuiChoice::onEntriesReset()
{
    onEntriesPopulated();
}



QString
KnobGuiChoice::getPixmapPathFromFilePath(const KnobHolderPtr& holder, const QString& filePath)
{
    if ( QFile::exists(filePath) ) {
        return filePath;
    }

    QString customFilePath = filePath;

    EffectInstancePtr instance = toEffectInstance(holder);
    if (instance) {
        QString resourcesPath = QString::fromUtf8( instance->getNode()->getPluginResourcesPath().c_str() );
        if ( !resourcesPath.endsWith( QLatin1Char('/') ) ) {
            resourcesPath += QLatin1Char('/');
        }
        customFilePath.prepend(resourcesPath);
    }

    return customFilePath;
}

ComboBox*
KnobGuiChoice::getCombobox() const
{
    return _comboBox;
}


QString
KnobGuiChoice::getPixmapPathFromFilePath(const QString &filePath) const
{
    return getPixmapPathFromFilePath(_knob.lock()->getHolder(),filePath);
}

void
KnobGuiChoice::onEntriesPopulated()
{
    KnobChoicePtr knob = _knob.lock();

    _comboBox->clear();
    std::vector<std::string> entries = knob->getEntries();
    const std::vector<std::string> help =  knob->getEntriesHelp();
    std::string activeEntry = knob->getActiveEntryText();

    std::string pluginShortcutGroup;
    EffectInstancePtr isEffect = toEffectInstance(knob->getHolder());
    if (isEffect) {
        PluginPtr plugin = isEffect->getNode()->getOriginalPlugin();
        if (plugin) {
            pluginShortcutGroup = plugin->getPluginShortcutGroup();
        }
    }


    const std::map<int, std::string>& shortcutsMap = knob->getShortcuts();
    const std::map<int, std::string>& iconsMap = knob->getIcons();

    for (U32 i = 0; i < entries.size(); ++i) {
        std::string helpStr;
        if ( !help.empty() && !help[i].empty() ) {
            helpStr = help[i];
        }

        std::string shortcutID;
        std::string iconFilePath;
        if (!pluginShortcutGroup.empty()) {
            std::map<int, std::string>::const_iterator foundShortcut = shortcutsMap.find(i);
            if (foundShortcut != shortcutsMap.end()) {
                shortcutID = foundShortcut->second;
            }
        }

        std::map<int, std::string>::const_iterator foundIcon = iconsMap.find(i);
        if (foundIcon != iconsMap.end()) {
            iconFilePath = foundIcon->second;
        }

        
        QIcon icon;
        if (!iconFilePath.empty()) {
            QPixmap pix( getPixmapPathFromFilePath( QString::fromUtf8( iconFilePath.c_str() ) ) );
            if (!pix.isNull()) {
                pix = pix.scaled(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));
                icon.addPixmap(pix);
            }
        }

        if (!shortcutID.empty() && !pluginShortcutGroup.empty() && !_comboBox->isCascading()) {
            QAction* action = new ActionWithShortcut(pluginShortcutGroup,
                                                     shortcutID,
                                                     entries[i],
                                                     _comboBox);
            if (!icon.isNull()) {
                action->setIcon(icon);
            }
            _comboBox->addAction(action);

        } else {
            _comboBox->addItem( QString::fromUtf8( entries[i].c_str() ), icon, QKeySequence(), QString::fromUtf8( helpStr.c_str() ) );
            
        }
        
        
    }

    const std::vector<int>& separators = knob->getSeparators();
    for (std::size_t i = 0; i < separators.size(); ++i) {
        _comboBox->insertSeparator(separators[i]);
    }

    // the "New" menu is only added to known parameters (e.g. the choice of output channels)
    if (knob->getNewOptionCallback()) {
        _comboBox->addItemNew();
    }
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///we don't want that to happen because the index actually didn't change.
    if ( _comboBox->isCascading() || activeEntry.empty() ) {
        _comboBox->setCurrentIndex_no_emit( knob->getValue() );
    } else {
        _comboBox->setCurrentText_no_emit( QString::fromUtf8( activeEntry.c_str() ) );
    }


    if ( !activeEntry.empty() ) {
        bool activeIndexPresent = knob->isActiveEntryPresentInEntries(getView());
        if (!activeIndexPresent) {
            QString error = tr("The value %1 no longer exist in the menu.").arg(QString::fromUtf8(activeEntry.c_str()));
            getKnobGui()->setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, NATRON_NAMESPACE::convertFromPlainText(error, NATRON_NAMESPACE::WhiteSpaceNormal) );
        } else {
            getKnobGui()->setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, QString() );
        }
    }
} // onEntriesPopulated

void
KnobGuiChoice::onItemNewSelected()
{
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    ChoiceKnobDimView::KnobChoiceNewItemCallback callback = knob->getNewOptionCallback();
    if (!callback) {
        return;
    }
    callback(knob);
  
}

void
KnobGuiChoice::updateToolTip()
{
    getKnobGui()->toolTip(_comboBox, getView());

}

void
KnobGuiChoice::updateGUI()
{
    ///we don't use setCurrentIndex because the signal emitted by combobox will call onCurrentIndexChanged and
    ///change the internal value of the knob again...
    ///The slot connected to onCurrentIndexChanged is reserved to catch user interaction with the combobox.
    ///This function is called in response to an internal change.
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string activeEntry = knob->getActiveEntryText();

    if ( !activeEntry.empty() ) {
        bool activeIndexPresent = knob->isActiveEntryPresentInEntries(getView());
        if (!activeIndexPresent) {
            QString error = tr("The value %1 no longer exist in the menu.").arg(QString::fromUtf8(activeEntry.c_str()));
            getKnobGui()->setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, NATRON_NAMESPACE::convertFromPlainText(error, NATRON_NAMESPACE::WhiteSpaceNormal) );
        } else {
            getKnobGui()->setWarningValue( KnobGui::eKnobWarningChoiceMenuOutOfDate, QString() );
        }
    }
    if ( _comboBox->isCascading() || activeEntry.empty() ) {
        _comboBox->setCurrentIndex_no_emit( knob->getValue() );
    } else {
        _comboBox->setCurrentText_no_emit( QString::fromUtf8( activeEntry.c_str() ) );
    }
}

void
KnobGuiChoice::reflectAnimationLevel(DimIdx /*dimension*/,
                                     AnimationLevelEnum level)
{

    bool isEnabled = _knob.lock()->isEnabled();
    _comboBox->setEnabled_natron(level != eAnimationLevelExpression && isEnabled);

    if ( level != (AnimationLevelEnum)_comboBox->getAnimation() ) {
        _comboBox->setAnimation((int)level);
    }
}

void
KnobGuiChoice::setWidgetsVisible(bool visible)
{
    _comboBox->setVisible(visible);
}

void
KnobGuiChoice::setEnabled(const std::vector<bool>& perDimEnabled)
{
    KnobChoicePtr knob = _knob.lock();

    _comboBox->setEnabled_natron(perDimEnabled[0]);
}

void
KnobGuiChoice::reflectMultipleSelection(bool dirty)
{
    _comboBox->setIsSelectedMultipleTimes(dirty);
}


void
KnobGuiChoice::reflectSelectionState(bool selected)
{
    _comboBox->setIsSelected(selected);
}

void
KnobGuiChoice::reflectLinkedState(DimIdx /*dimension*/, bool linked)
{
    _comboBox->setLinkedFrameEnabled(linked);
}

void
KnobGuiChoice::reflectModificationsState()
{
    bool hasModif = _knob.lock()->hasModifications();

    _comboBox->setIsModified(hasModif);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiChoice.cpp"
