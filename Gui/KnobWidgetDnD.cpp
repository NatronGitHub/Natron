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

#include "KnobWidgetDnD.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDrag>
#include <QtCore/QMimeData>
#include <QPainter>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QImage>
#include <QWidget>
#include <QApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Knob.h"
#include "Engine/NodeGroup.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/Settings.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"


NATRON_NAMESPACE_ENTER;


struct KnobWidgetDnDPrivate
{
    Q_DECLARE_TR_FUNCTIONS(KnobWidgetDnd)

public:
    KnobGuiWPtr knob;
    DimSpec dimension;
    ViewIdx view;
    QPoint dragPos;
    bool dragging;
    QWidget* widget;
    bool userInputSinceFocusIn;


    KnobWidgetDnDPrivate(const KnobGuiPtr& knob,
                         DimSpec dimension,
                         ViewIdx view,
                         QWidget* widget)
    : knob(knob)
    , dimension(dimension)
    , view(view)
    , dragPos()
    , dragging(false)
    , widget(widget)
    , userInputSinceFocusIn(false)
    {
        assert(widget);
    }

    bool canDrop(bool warn, bool setCursor) const;

    KnobGuiPtr getKnob() const
    {
        return knob.lock();
    }
};

KnobWidgetDnD::KnobWidgetDnD(const KnobGuiPtr& knob,
                             DimSpec dimension,
                             ViewIdx view,
                             QWidget* widget)
    : _imp( new KnobWidgetDnDPrivate(knob, dimension, view, widget) )
{
    widget->setMouseTracking(true);
    widget->setAcceptDrops(true);
}

KnobWidgetDnD::~KnobWidgetDnD()
{
}

QWidget*
KnobWidgetDnD::getWidget() const
{
    return _imp->widget;
}

bool
KnobWidgetDnD::mousePress(QMouseEvent* e)
{
    _imp->userInputSinceFocusIn = true;
    KnobGuiPtr guiKnob = _imp->getKnob();
    if (!guiKnob) {
        return false;
    }
    KnobIPtr internalKnob = guiKnob->getKnob();
    if (!internalKnob) {
        return false;
    }
    DimSpec dragDim = _imp->dimension;
    if ( (dragDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
        dragDim = DimSpec::all();
    }

    if (guiKnob->getGui()) {
        guiKnob->getGui()->setCurrentKnobWidgetFocus(shared_from_this());
    }

    if (dragDim.isAll()) {
        dragDim = DimSpec(0);
    }
    if ( buttonDownIsLeft(e) && ( modCASIsControl(e) || modCASIsControlShift(e) ) && ( internalKnob->isEnabled(DimIdx(dragDim), _imp->view) || internalKnob->isSlave(DimIdx(dragDim), _imp->view) ) ) {
        _imp->dragPos = e->pos();
        _imp->dragging = true;

        return true;
    }

    return false;
}

void
KnobWidgetDnD::keyPress(QKeyEvent* e)
{

    KnobGuiPtr guiKnob = _imp->getKnob();
    if (!guiKnob) {
        return;
    }
    if (guiKnob->getGui()) {
        guiKnob->getGui()->setCurrentKnobWidgetFocus(shared_from_this());
    }
    //_imp->userInputSinceFocusIn = true;
    if ( modCASIsControl(e) ) {
        _imp->widget->setCursor( appPTR->getLinkToCursor() );
    } else if ( modCASIsControlShift(e) ) {
        _imp->widget->setCursor( appPTR->getLinkToMultCursor() );
    } else {
        _imp->widget->unsetCursor();
    }
}

void
KnobWidgetDnD::keyRelease(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Shift) ) {
        _imp->widget->unsetCursor();
    }
}

bool
KnobWidgetDnD::mouseMove(QMouseEvent* e)
{
    if ( modCASIsControl(e) ) {
        _imp->widget->setCursor( appPTR->getLinkToCursor() );
    } else if ( modCASIsControlShift(e) ) {
        _imp->widget->setCursor( appPTR->getLinkToMultCursor() );
    } else {
        _imp->widget->unsetCursor();
    }
    if (_imp->dragging) {
        // If the left button isn't pressed anymore then return
        if ( !buttonDownIsLeft(e) ) {
            return false;
        }
        // If the distance is too small then return
        if ( (e->pos() - _imp->dragPos).manhattanLength() < QApplication::startDragDistance() ) {
            return false;
        }

        startDrag();
    }

    return false;
}

void
KnobWidgetDnD::mouseRelease(QMouseEvent* /*e*/)
{
    _imp->dragging = false;
    _imp->widget->unsetCursor();
}

void
KnobWidgetDnD::startDrag()
{
    QDrag* drag = new QDrag(_imp->widget);
    QMimeData* mimeData = new QMimeData;

    mimeData->setData( QString::fromUtf8(KNOB_DND_MIME_DATA_KEY), QByteArray() );
    drag->setMimeData(mimeData);

    KnobGuiPtr guiKnob = _imp->getKnob();
    KnobIPtr internalKnob = guiKnob->getKnob();
    if (!internalKnob) {
        return;
    }

    DimSpec dragDim = _imp->dimension;
    if ( (dragDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
        dragDim = DimSpec::all();
    }

    guiKnob->getGui()->getApp()->setKnobDnDData(drag, internalKnob, dragDim, _imp->view);

    QFont font = _imp->widget->font();
    font.setBold(true);
    QFontMetrics fmetrics(font, 0);
    Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
    bool isExprMult = ( mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) ) == (Qt::ControlModifier | Qt::ShiftModifier);
    QString textFirstLine;
    if (isExprMult) {
        textFirstLine = tr("Linking (expression):");
    } else {
        textFirstLine = tr("Linking (hard-link) from:");
    }


    QString knobLine;
    EffectInstancePtr isEffect = toEffectInstance( internalKnob->getHolder() );
    if (isEffect) {
        knobLine.append( QString::fromUtf8( isEffect->getNode()->getFullyQualifiedName().c_str() ) );
        knobLine += QLatin1Char('.');
    }
    knobLine += QString::fromUtf8( internalKnob->getName().c_str() );


    if (internalKnob->getNDimensions() > 1) {
        if (!dragDim.isAll()) {
            knobLine += QLatin1Char('.');
            knobLine += QString::fromUtf8( internalKnob->getDimensionName(DimIdx(dragDim)).c_str() );
        } else {
            if (!isExprMult) {
                knobLine += QLatin1Char(' ');
                knobLine += tr("(all dimensions)");
            }
        }
    }

    if (isExprMult) {
        knobLine += QString::fromUtf8(" * curve(frame, dimension)");
    }

    QString textThirdLine;
    if (dragDim.isAll()) {
        textThirdLine = tr("Drag it to the label of another parameter of the same type");
    } else {
        textThirdLine = tr("Drag it to a dimension of another parameter of the same type");
    }

    int textWidth = std::max( std::max( fmetrics.width(textFirstLine), fmetrics.width(knobLine) ), fmetrics.width(textThirdLine) );
    QImage dragImg(textWidth, (fmetrics.height() + 5) * 3, QImage::Format_ARGB32);
    dragImg.fill( QColor(0, 0, 0, 200) );
    QPainter p(&dragImg);
    double tR, tG, tB;
    appPTR->getCurrentSettings()->getTextColor(&tR, &tG, &tB);
    QColor textColor;
    textColor.setRgbF( Image::clamp(tR, 0., 1.), Image::clamp(tG, 0., 1.), Image::clamp(tB, 0., 1.) );
    p.setPen(textColor);
    p.setFont(font);
    p.drawText(QPointF(0, dragImg.height() - 2.5), textThirdLine);
    p.drawText(QPointF(0, dragImg.height() - fmetrics.height() - 5), knobLine);
    p.drawText(QPointF(0, dragImg.height() - fmetrics.height() * 2 - 10), textFirstLine);

    drag->setPixmap( QPixmap::fromImage(dragImg) );
    drag->exec();
} // KnobWidgetDnD::startDrag

bool
KnobWidgetDnD::dragEnter(QDragEnterEvent* e)
{
    if (e->source() == _imp->widget) {
        return false;
    }
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains( QString::fromUtf8(KNOB_DND_MIME_DATA_KEY) ) && _imp->canDrop(false, true) ) {
        e->acceptProposedAction();

        return true;
    }

    return false;
}

bool
KnobWidgetDnD::dragMove(QDragMoveEvent* e)
{
    if (e->source() == _imp->widget) {
        return false;
    }
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains( QString::fromUtf8(KNOB_DND_MIME_DATA_KEY) ) && _imp->canDrop(false, true) ) {
        e->acceptProposedAction();

        return true;
    }

    return false;
}

bool
KnobWidgetDnDPrivate::canDrop(bool warn,
                              bool setCursor) const
{
    KnobIPtr source;
    KnobGuiPtr guiKnob = knob.lock();
    KnobIPtr thisKnob = guiKnob->getKnob();

    bool isEnabled = dimension.isAll() ? thisKnob->isEnabled(DimIdx(0), view) : thisKnob->isEnabled(DimIdx(dimension), view);
    if (!isEnabled) {
        return false;
    }
    DimSpec srcDim;
    ViewIdx srcView;
    QDrag* drag;
    guiKnob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim, &srcView);


    bool ret = true;
    if (source) {
        DimSpec targetDim = dimension;
        if ( (targetDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
            targetDim = DimSpec::all();
        }

        if ( !KnobI::areTypesCompatibleForSlave( source, thisKnob ) ) {
            if (warn) {
                Dialogs::errorDialog( tr("Link").toStdString(), tr("You can only link parameters of the same type. To overcome this, use an expression instead.").toStdString() );
            }
            ret = false;
        }

        if ( ret && !srcDim.isAll() && targetDim.isAll() ) {
            if (warn) {
                Dialogs::errorDialog( tr("Link").toStdString(), tr("When linking on all dimensions, original and target parameters must have the same dimension.").toStdString() );
            }
            ret = false;
        }

        if ( ret && ( targetDim.isAll() || srcDim.isAll() ) && ( source->getNDimensions() != thisKnob->getNDimensions() ) ) {
            if (warn) {
                Dialogs::errorDialog( tr("Link").toStdString(), tr("When linking on all dimensions, original and target parameters must have the same dimension.").toStdString() );
            }
            ret = false;
        }
    } else {
        ret = false;
    }
    if (setCursor) {
        if (ret) {
            drag->setDragCursor(QPixmap(), Qt::LinkAction);
        } else {
        }
    }

    return ret;
} // KnobWidgetDnDPrivate::canDrop

bool
KnobWidgetDnD::drop(QDropEvent* e)
{
    e->accept();
    QStringList formats = e->mimeData()->formats();

    if ( formats.contains( QString::fromUtf8(KNOB_DND_MIME_DATA_KEY) ) && _imp->canDrop(true, false) ) {
        KnobGuiPtr guiKnob = _imp->getKnob();
        KnobIPtr source;
        KnobIPtr thisKnob = guiKnob->getKnob();
        DimSpec srcDim;
        ViewIdx srcView;
        QDrag* drag;
        guiKnob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim, &srcView);
        guiKnob->getGui()->getApp()->setKnobDnDData(0, KnobIPtr(), DimSpec::all(), ViewIdx(0));
        if ( source && (source != thisKnob) ) {
            DimSpec targetDim = _imp->dimension;
            if ( (targetDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
                targetDim = DimSpec::all();
            }

            Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
            if ( ( mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) ) == (Qt::ControlModifier | Qt::ShiftModifier) ) {
                EffectInstancePtr effect = toEffectInstance( source->getHolder() );
                if (!effect) {
                    return false;
                }
                NodeCollectionPtr group = effect->getNode()->getGroup();
                NodeGroupPtr isGroup = toNodeGroup( group );
                std::string expr;
                std::stringstream ss;
                if (isGroup) {
                    ss << "thisGroup.";
                } else {
                    ss << effect->getApp()->getAppIDString() << ".";
                }
                ss << effect->getNode()->getScriptName_mt_safe() << "." << source->getName();
                ss << ".getValue(";
                if (source->getNDimensions() > 1) {
                    if (srcDim == -1) {
                        ss << "dimension";
                    } else {
                        ss << srcDim;
                    }
                }
                ss << ")";
                ss << " * curve(frame,";
                if (targetDim == -1) {
                    ss << "dimension";
                } else {
                    ss << targetDim;
                }
                ss << ")";


                expr = ss.str();
                guiKnob->pushUndoCommand( new SetExpressionCommand(guiKnob->getKnob(), false, targetDim, expr) );
            } else if ( ( mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) ) == (Qt::ControlModifier) ) {
                guiKnob->pushUndoCommand( new PasteKnobClipBoardUndoCommand(guiKnob, eKnobClipBoardTypeCopyLink, srcDim, targetDim, source) );
            }

            return true;
        }

        e->acceptProposedAction();
    }

    return false;
} // KnobWidgetDnD::drop

bool
KnobWidgetDnD::mouseWheel(QWheelEvent* /*e*/)
{
    if (_imp->userInputSinceFocusIn) {
        return true;
    }

    return false;
}

void
KnobWidgetDnD::mouseEnter(QEvent* /*e*/)
{
    KnobGuiPtr knob = _imp->knob.lock();

    if (!knob) {
        return;
    }
    bool enabled = _imp->dimension.isAll() ? knob->getKnob()->isEnabled(DimIdx(0), _imp->view) : knob->getKnob()->isEnabled(DimIdx(_imp->dimension), _imp->view);

    if (Gui::isFocusStealingPossible() && _imp->widget->isEnabled() && enabled) {
        _imp->widget->setFocus();
    }
}

void
KnobWidgetDnD::mouseLeave(QEvent* /*e*/)
{
    if (_imp->widget->hasFocus() && !_imp->userInputSinceFocusIn) {
        KnobGuiPtr knob = _imp->knob.lock();

        if (!knob) {
            return;
        }
        if (knob->getGui()) {
            boost::shared_ptr<KnobWidgetDnD> currentKnobWidget = knob->getGui()->getCurrentKnobWidgetFocus();
            if (currentKnobWidget) {
                QWidget* internalWidget = currentKnobWidget->getWidget();
                if (internalWidget) {
                    internalWidget->setFocus();
                    return;
                }
            }
        }
        _imp->widget->clearFocus();
    }
}

void
KnobWidgetDnD::focusIn()
{
    _imp->userInputSinceFocusIn = false;
}

void
KnobWidgetDnD::focusOut()
{
    _imp->userInputSinceFocusIn = false;
}

NATRON_NAMESPACE_EXIT;
