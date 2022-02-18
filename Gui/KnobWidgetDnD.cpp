/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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
#include "Engine/Image.h"
#include "Engine/Settings.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobUndoCommand.h"
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
    int dimension;
    QPoint dragPos;
    bool dragging;
    QWidget* widget;
    bool userInputSinceFocusIn;


    KnobWidgetDnDPrivate(const KnobGuiPtr& knob,
                         int dimension,
                         QWidget* widget)
        : knob(knob)
        , dimension(dimension)
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
                      int dimension,
                      QWidget* widget) : KnobWidgetDnD(knob, dimension, widget) {
    }
};

KnobWidgetDnDPtr KnobWidgetDnD::create(const KnobGuiPtr& knob,
                                                       int dimension,
                                                       QWidget* widget)
{
    return boost::make_shared<KnobWidgetDnD::MakeSharedEnabler>(knob, dimension, widget);
}

KnobWidgetDnD::KnobWidgetDnD(const KnobGuiPtr& knob,
                             int dimension,
                             QWidget* widget)
    : _imp( new KnobWidgetDnDPrivate(knob, dimension, widget) )
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
    int dragDim = _imp->dimension;
    if ( (dragDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
        dragDim = -1;
    }

    dragDim = dragDim == -1 ? 0 : dragDim;
    // Note: prior to fixing https://github.com/NatronGitHub/Natron/issues/167 only enabled nodes could be linked.
    if ( buttonDownIsLeft(e) && ( modCASIsControl(e) || modCASIsControlShift(e) ) ) {
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

    int dragDim = _imp->dimension;
    if ( (dragDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
        dragDim = -1;
    }

    guiKnob->getGui()->getApp()->setKnobDnDData(drag, internalKnob, dragDim);

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
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>( internalKnob->getHolder() );
    if (isEffect) {
        knobLine.append( QString::fromUtf8( isEffect->getNode()->getFullyQualifiedName().c_str() ) );
        knobLine += QLatin1Char('.');
    }
    knobLine += QString::fromUtf8( internalKnob->getName().c_str() );


    if (internalKnob->getDimension() > 1) {
        if (dragDim != -1) {
            knobLine += QLatin1Char('.');
            knobLine += QString::fromUtf8( internalKnob->getDimensionName(dragDim).c_str() );
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
    if (dragDim == -1) {
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

    // Note: prior to fixing https://github.com/NatronGitHub/Natron/issues/167 only enabled nodes could be linked.

    int srcDim;
    QDrag* drag;
    guiKnob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim);


    bool ret = true;
    if (source) {
        int targetDim = dimension;
        if ( (targetDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
            targetDim = -1;
        }

        if ( !KnobI::areTypesCompatibleForSlave( source.get(), thisKnob.get() ) ) {
            if (warn) {
                Dialogs::errorDialog( tr("Link").toStdString(), tr("You can only link parameters of the same type. To overcome this, use an expression instead.").toStdString() );
            }
            ret = false;
        }

        if ( ret && (srcDim != -1) && (targetDim == -1) ) {
            if (warn) {
                Dialogs::errorDialog( tr("Link").toStdString(), tr("When linking on all dimensions, original and target parameters must have the same dimension.").toStdString() );
            }
            ret = false;
        }

        if ( ret && ( (targetDim == -1) || (srcDim == -1) ) && ( source->getDimension() != thisKnob->getDimension() ) ) {
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
        int srcDim;
        QDrag* drag;
        guiKnob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim);
        guiKnob->getGui()->getApp()->setKnobDnDData(0, KnobIPtr(), -1);
        if ( source && (source != thisKnob) ) {
            int targetDim = _imp->dimension;
            if ( (targetDim == 0) && !guiKnob->getAllDimensionsVisible() ) {
                targetDim = -1;
            }

            Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
            if ( ( mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) ) == (Qt::ControlModifier | Qt::ShiftModifier) ) {
                EffectInstance* effect = dynamic_cast<EffectInstance*>( source->getHolder() );
                if (!effect) {
                    return false;
                }
                NodeCollectionPtr group = effect->getNode()->getGroup();
                NodeGroup* isGroup = dynamic_cast<NodeGroup*>( group.get() );
                std::string expr;
                std::stringstream ss;
                if (isGroup) {
                    ss << "thisGroup.";
                } else {
                    ss << effect->getApp()->getAppIDString() << ".";
                }
                ss << effect->getNode()->getScriptName_mt_safe() << "." << source->getName();
                ss << ".getValue(";
                if (source->getDimension() > 1) {
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
                guiKnob->pushUndoCommand( new PasteUndoCommand(guiKnob, eKnobClipBoardTypeCopyLink, srcDim, targetDim, source) );
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
    // Note: prior to fixing https://github.com/NatronGitHub/Natron/issues/167 only enabled nodes could be linked.
    if (Gui::isFocusStealingPossible()) {
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
