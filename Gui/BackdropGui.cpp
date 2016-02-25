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

#include "BackdropGui.h"

#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGraphicsTextItem>
#include <QCoreApplication>
#include <QThread>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Backdrop.h"
#include "Engine/Settings.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiString.h"
#include "Gui/NodeGraphTextItem.h"


#define NATRON_BACKDROP_DEFAULT_WIDTH 80
#define NATRON_BACKDROP_DEFAULT_HEIGHT 80

NATRON_NAMESPACE_ENTER;

struct BackdropGuiPrivate
{
    BackdropGui* _publicInterface;
    
    NodeGraphTextItem* label;

    BackdropGuiPrivate(BackdropGui* publicInterface)
    : _publicInterface(publicInterface)
    , label(0)
    {
    }

    void refreshLabelText(int nameHeight,const QString & text);
    
    std::string getLabelValue() const;
    
};

BackdropGui::BackdropGui(QGraphicsItem* parent)
    : NodeGui(parent)
    , _imp( new BackdropGuiPrivate(this) )
{
}


BackdropGui::~BackdropGui()
{
    
}



std::string
BackdropGuiPrivate::getLabelValue() const
{
    KnobPtr k = _publicInterface->getNode()->getKnobByName("Label");
    assert(k);
    KnobString* isStr = dynamic_cast<KnobString*>(k.get());
    assert(isStr);
    return isStr ? isStr->getValue() : "";
}

void
BackdropGui::getInitialSize(int *w, int *h) const
{
    *w = TO_DPIX(NATRON_BACKDROP_DEFAULT_WIDTH);
    *h = TO_DPIY(NATRON_BACKDROP_DEFAULT_HEIGHT);
}

void
BackdropGui::createGui()
{
    NodeGui::createGui();
    
    _imp->label = new NodeGraphTextItem(getDagGui(), this, false);
    _imp->label->setDefaultTextColor( QColor(0,0,0,255) );
    _imp->label->setZValue(getBaseDepth() + 1);
    
    EffectInstPtr effect = getNode()->getEffectInstance();
    assert(effect);
    Backdrop* isBd = dynamic_cast<Backdrop*>(effect.get());
    assert(isBd);
    
    QObject::connect(isBd,SIGNAL(labelChanged(QString)),this, SLOT(onLabelChanged(QString)));
    
    refreshTextLabelFromKnob();
}

void
BackdropGui::onLabelChanged(const QString& label)
{
    int nameHeight = getFrameNameHeight();
    _imp->refreshLabelText(nameHeight, label);
}


void
BackdropGui::adjustSizeToContent(int *w,int *h,bool /*adjustToTextSize*/)
{
    NodeGui::adjustSizeToContent(w, h,false);
    QRectF labelBbox = _imp->label->boundingRect();
    
    *h = std::max((double)*h,labelBbox.height() * 1.5);
    *w = std::max((double)*w, _imp->label->textWidth());
    
}

void
BackdropGui::resizeExtraContent(int /*w*/,int /*h*/,bool forceResize)
{
    QPointF p = pos();
    QPointF thisItemPos = mapFromParent(p);
    
    int nameHeight = getFrameNameHeight();
    
    _imp->label->setPos(thisItemPos.x(), thisItemPos.y() + nameHeight + TO_DPIY(10));
    if (!forceResize) {
        _imp->label->adjustSize();
    }
}

void
BackdropGui::refreshTextLabelFromKnob()
{
    int nameHeight = getFrameNameHeight();
    _imp->refreshLabelText( nameHeight, QString( _imp->getLabelValue().c_str() ) );
}

void
BackdropGuiPrivate::refreshLabelText(int nameHeight,const QString &text)
{
    QString textLabel = text;

    textLabel.replace("\n", "<br>");
    textLabel.prepend("<div align=\"left\">");
    textLabel.append("</div>");
    QFont f;
    QColor color;
    if (!text.isEmpty()) {
        KnobGuiString::parseFont(textLabel, &f, &color);
        
        bool antialias = appPTR->getCurrentSettings()->isNodeGraphAntiAliasingEnabled();
        if (!antialias) {
            f.setStyleStrategy(QFont::NoAntialias);
        }
        label->setFont(f);
    }
    
    label->setHtml(textLabel);

    
    QRectF bbox = _publicInterface->boundingRect();
    
    //label->adjustSize();
    int w = std::max( bbox.width(), label->textWidth() * 1.2 );
    QRectF labelBbox = label->boundingRect();
    int h = std::max( labelBbox.height() + nameHeight + TO_DPIX(10), bbox.height() );
    _publicInterface->resize(w, h);
    _publicInterface->update();
    
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_BackdropGui.cpp"
