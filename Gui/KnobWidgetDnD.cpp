/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "KnobWidgetDnD.h"

#include <sstream> // stringstream

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
#include "Engine/KnobUndoCommand.h"
#include "Engine/Image.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/KnobGui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"


NATRON_NAMESPACE_ENTER


struct KnobWidgetDnDPrivate
{
    Q_DECLARE_TR_FUNCTIONS(KnobWidgetDnd)

public:
    KnobGuiWPtr knob;
    DimSpec dimension;
    ViewSetSpec view;
    QPoint dragPos;
    bool dragging;
    QWidget* widget;
    bool userInputSinceFocusIn;


    KnobWidgetDnDPrivate(const KnobGuiPtr& knob,
                         DimSpec dimension,
                         ViewSetSpec view,
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


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct KnobWidgetDnD::MakeSharedEnabler: public KnobWidgetDnD
{
    MakeSharedEnabler(const KnobGuiPtr& knob,
                      DimSpec dimension,
                      ViewSetSpec view,
                      QWidget* widget) : KnobWidgetDnD(knob, dimension, view, widget) {
    }
};


boost::shared_ptr<KnobWidgetDnD>
KnobWidgetDnD::create(const KnobGuiPtr& knob,
                      DimSpec dimension,
                      ViewSetSpec view,
                      QWidget* widget)
{
    return boost::make_shared<KnobWidgetDnD::MakeSharedEnabler>(knob, dimension, view, widget);
}


KnobWidgetDnD::KnobWidgetDnD(const KnobGuiPtr& knob,
                             DimSpec dimension,
                             ViewSetSpec view,
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
    DimSpec dragDim;
    ViewSetSpec dragView;
    internalKnob->convertDimViewArgAccordingToKnobState(_imp->dimension, _imp->view, &dragDim, &dragView);


    if (dragDim.isAll()) {
        dragDim = DimSpec(0);
    }

    if (dragView.isAll()) {
        dragView = ViewSetSpec(0);
    }

    if ( buttonDownIsLeft(e) && ( modCASIsControl(e) || modCASIsControlShift(e) ) && internalKnob->isEnabled() ) {

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

    DimSpec dragDim;
    ViewSetSpec dragView;
    internalKnob->convertDimViewArgAccordingToKnobState(_imp->dimension, _imp->view, &dragDim, &dragView);

    guiKnob->getGui()->getApp()->setKnobDnDData(drag, internalKnob, dragDim, dragView);

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

    if (dragView.isAll()) {
        knobLine += tr(" (all views)");
    } else if (dragDim.isAll()) {
        knobLine += tr(" (all dimensions)");
    } else {
        knobLine += QLatin1Char('.');
        knobLine += QString::fromUtf8( internalKnob->getDimensionName(DimIdx(dragDim)).c_str() );
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
    if (!guiKnob) {
        return false;
    }
    KnobIPtr thisKnob = guiKnob->getKnob();
    if (!thisKnob) {
        return false;
    }
    DimSpec targetDim;
    ViewSetSpec targetView;
    thisKnob->convertDimViewArgAccordingToKnobState(dimension, view, &targetDim, &targetView);

    bool isEnabled = thisKnob->isEnabled();
    if (!isEnabled) {
        return false;
    }
    DimSpec srcDim;
    ViewSetSpec srcView;
    QDrag* drag;
    guiKnob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim, &srcView);


    bool ret = true;
    if (source) {
        ret = guiKnob->canPasteKnob(source, eKnobClipBoardTypeCopyLink, srcDim, srcView, targetDim, view, warn);
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
        ViewSetSpec srcView;
        QDrag* drag;
        guiKnob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim, &srcView);

        // Clear clipboard
        guiKnob->getGui()->getApp()->setKnobDnDData(0, KnobIPtr(), DimSpec::all(), ViewIdx(0));
        if ( source && (source != thisKnob) ) {

            DimSpec targetDim;
            ViewSetSpec targetView;
            thisKnob->convertDimViewArgAccordingToKnobState(_imp->dimension, _imp->view, &targetDim, &targetView);

            Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
            KnobClipBoardType type = eKnobClipBoardTypeCopyLink;
            if ( ( mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) ) == (Qt::ControlModifier | Qt::ShiftModifier) ) {
                type = eKnobClipBoardTypeCopyExpressionMultCurveLink;
            } else if ((mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) ) == (Qt::ControlModifier)) {
                type = eKnobClipBoardTypeCopyLink;
            } else {
                return false;
            }
            return guiKnob->pasteKnob(source, type, srcDim, srcView, targetDim, targetView);
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
    KnobIPtr internalKnob = knob->getKnob();
    if (!internalKnob) {
        return;
    }
    DimSpec targetDim;
    ViewSetSpec targetView;
    internalKnob->convertDimViewArgAccordingToKnobState(_imp->dimension, _imp->view, &targetDim, &targetView);

    bool isEnabled = internalKnob->isEnabled();

    if (Gui::isFocusStealingPossible() && _imp->widget->isEnabled() && isEnabled) {
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

NATRON_NAMESPACE_EXIT
