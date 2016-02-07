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

#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QImage>
#include <QWidget>
#include <QApplication>

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
    KnobGui* knob;
    int dimension;
    QPoint dragPos;
    bool dragging;
    QWidget* widget;
    
    KnobWidgetDnDPrivate(KnobGui* knob,int dimension, QWidget* widget)
    : knob(knob)
    , dimension(dimension)
    , dragPos()
    , dragging(false)
    , widget(widget)
    {
        assert(widget);
    }
    
    bool canDrop(bool warn,bool setCursor) const;
};

KnobWidgetDnD::KnobWidgetDnD(KnobGui* knob,int dimension, QWidget* widget)
: _imp(new KnobWidgetDnDPrivate(knob, dimension, widget))
{
    widget->setMouseTracking(true);
    widget->setAcceptDrops(true);
}

KnobWidgetDnD::~KnobWidgetDnD()
{
    
}

bool
KnobWidgetDnD::mousePress(QMouseEvent* e)
{
    if ( buttonDownIsLeft(e) && (modCASIsControl(e) || modCASIsControlShift(e))) {
        _imp->dragPos = e->pos();
        _imp->dragging = true;
        return true;
    }
    return false;
}

void
KnobWidgetDnD::keyPress(QKeyEvent* e)
{
    if (modCASIsControl(e)) {
        _imp->widget->setCursor(appPTR->getLinkToCursor());
    } else if (modCASIsControlShift(e)) {
        _imp->widget->setCursor(appPTR->getLinkToMultCursor());
    } else {
        _imp->widget->unsetCursor();
    }

}

void
KnobWidgetDnD::keyRelease(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control || e->key() == Qt::Key_Shift) {
        _imp->widget->unsetCursor();
    }
    
}

bool
KnobWidgetDnD::mouseMove(QMouseEvent* e)
{
    if (modCASIsControl(e)) {
        _imp->widget->setCursor(appPTR->getLinkToCursor());
    } else if (modCASIsControlShift(e)) {
        _imp->widget->setCursor(appPTR->getLinkToMultCursor());
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
    mimeData->setData(KNOB_DND_MIME_DATA_KEY, QByteArray());
    drag->setMimeData(mimeData);
    
    
    KnobPtr internalKnob = _imp->knob->getKnob();
    if (!internalKnob) {
        return;
    }
    _imp->knob->getGui()->getApp()->setKnobDnDData(drag, internalKnob, _imp->dimension);
    
    QFont font = _imp->widget->font();
    font.setBold(true);
    QFontMetrics fmetrics(font,0);
    QString textFirstLine( QObject::tr("Linking value from:") );
    
    QString knobLine;
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(internalKnob->getHolder());
    if (isEffect) {
        knobLine.append(isEffect->getNode()->getFullyQualifiedName().c_str());
        knobLine += '.';
    }
    knobLine += (internalKnob->getName().c_str());
    
    
    if (internalKnob->getDimension() > 1) {
        if (_imp->dimension != -1) {
            knobLine += '.';
            knobLine += internalKnob->getDimensionName(_imp->dimension).c_str();
        } else {
            knobLine += ' ';
            knobLine += QObject::tr("(all dimensions)");
        }
    }
    
    QString textThirdLine;
    if (_imp->dimension == -1) {
        textThirdLine = QObject::tr("Drag it to another parameter's label of the same type");
    } else {
        textThirdLine = QObject::tr("Drag it to another parameter's dimension of the same type");
    }
    
    int textWidth = std::max( std::max( fmetrics.width(textFirstLine), fmetrics.width(knobLine) ),fmetrics.width(textThirdLine) );
    QImage dragImg(textWidth,(fmetrics.height() + 5) * 3,QImage::Format_ARGB32);
    dragImg.fill(QColor(0, 0, 0, 200));
    QPainter p(&dragImg);
    
    
    double tR,tG,tB;
    appPTR->getCurrentSettings()->getTextColor(&tR, &tG, &tB);
    QColor textColor;
    textColor.setRgbF(Image::clamp(tR, 0., 1.), Image::clamp(tG, 0., 1.), Image::clamp(tB, 0., 1.));
    p.setPen(textColor);
    p.setFont(font);
    p.drawText(QPointF(0,dragImg.height() - 2.5), textThirdLine);
    p.drawText(QPointF(0,dragImg.height() - fmetrics.height() - 5), knobLine);
    p.drawText(QPointF(0,dragImg.height() - fmetrics.height() * 2 - 10), textFirstLine);
    
    drag->setPixmap( QPixmap::fromImage(dragImg) );
    drag->exec();

}

bool
KnobWidgetDnD::dragEnter(QDragEnterEvent* e)
{
    if (e->source() == _imp->widget) {
        return false;
    }
    QStringList formats = e->mimeData()->formats();
    if (formats.contains(KNOB_DND_MIME_DATA_KEY) && _imp->canDrop(false, true) ) {
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
    if (formats.contains(KNOB_DND_MIME_DATA_KEY) && _imp->canDrop(false, true) ) {
        e->acceptProposedAction();
        return true;
    }
    return false;
}

bool
KnobWidgetDnDPrivate::canDrop(bool warn, bool setCursor) const
{
    KnobPtr source;
    KnobPtr thisKnob = knob->getKnob();
    int srcDim;
    QDrag* drag;
    knob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim);
    
    bool ret = true;
    if (source) {
        
        if (source->typeName() != thisKnob->typeName()) {
            if (warn) {
                Dialogs::errorDialog(QObject::tr("Link").toStdString(), QObject::tr("You can only link parameters of the same type. To overcome this, use an expression instead.").toStdString());
            }
            ret = false;
        }
        
        if (ret && srcDim != -1 && dimension == -1) {
            if (warn) {
                Dialogs::errorDialog(QObject::tr("Link").toStdString(), QObject::tr("When linking on all dimensions, original and target parameters must have the same dimension.").toStdString());
            }
            ret = false;
        }
        
        if (ret && (dimension == -1 || srcDim == -1) && source->getDimension() != thisKnob->getDimension()) {
            if (warn) {
                Dialogs::errorDialog(QObject::tr("Link").toStdString(), QObject::tr("When linking on all dimensions, original and target parameters must have the same dimension.").toStdString());
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
}

bool
KnobWidgetDnD::drop(QDropEvent* e)
{
    e->accept();
    QStringList formats = e->mimeData()->formats();
    
    if (formats.contains(KNOB_DND_MIME_DATA_KEY) && _imp->canDrop(true, false)) {
        
        KnobPtr source;
        KnobPtr thisKnob = _imp->knob->getKnob();
        int srcDim;
        QDrag* drag;
        _imp->knob->getGui()->getApp()->getKnobDnDData(&drag, &source, &srcDim);
        _imp->knob->getGui()->getApp()->setKnobDnDData(0, KnobPtr(), -1);
        if (source) {
            
            Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
            if ((mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier)) == (Qt::ControlModifier | Qt::ShiftModifier)) {
                
                EffectInstance* effect = dynamic_cast<EffectInstance*>(source->getHolder());
                if (!effect) {
                    return false;
                }
                boost::shared_ptr<NodeCollection> group = effect->getNode()->getGroup();
                NodeGroup* isGroup = dynamic_cast<NodeGroup*>(group.get());

                
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
                    ss << "dimension";
                }
                ss << ")";
                ss << " * curve(frame,dimension)";
                

                
                _imp->knob->pushUndoCommand(new SetExpressionCommand(_imp->knob->getKnob(), false, _imp->dimension,expr));
            } else if ((mods & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier)) == (Qt::ControlModifier)) {
                _imp->knob->pushUndoCommand(new PasteUndoCommand(_imp->knob, eKnobClipBoardTypeCopyLink, srcDim, _imp->dimension, source));
            }
            return true;
        }
        
        e->acceptProposedAction();
        
    }
    return false;
}

void
KnobWidgetDnD::mouseEnter(QEvent* /*e*/)
{
    if (Gui::isFocusStealingPossible()) {
        _imp->widget->setFocus();
    }
}

void
KnobWidgetDnD::mouseLeave(QEvent* /*e*/)
{
    if (_imp->widget->hasFocus()) {
        _imp->widget->clearFocus();
    }
}

NATRON_NAMESPACE_EXIT;