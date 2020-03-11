/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/Backdrop.h"
#include "Engine/Settings.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiString.h"
#include "Gui/NodeGraphTextItem.h"
#include "Gui/NodeGraph.h"


#define NATRON_BACKDROP_DEFAULT_WIDTH 80
#define NATRON_BACKDROP_DEFAULT_HEIGHT 80

NATRON_NAMESPACE_ENTER

struct BackdropGuiPrivate
{
    BackdropGui* _publicInterface; // can not be a smart ptr
    NodeGraphTextItem* label;
    boost::weak_ptr<Backdrop> backdropEffect;

    BackdropGuiPrivate(BackdropGui* publicInterface)
        : _publicInterface(publicInterface)
        , label(0)
        , backdropEffect()
    {
    }

    void refreshLabelText();

};

BackdropGui::BackdropGui(QGraphicsItem* parent)
    : NodeGui(parent)
    , _imp( new BackdropGuiPrivate(this) )
{

}

BackdropGui::~BackdropGui()
{
}


void
BackdropGui::getInitialSize(int *w,
                            int *h) const
{
    *w = TO_DPIX(NATRON_BACKDROP_DEFAULT_WIDTH);
    *h = TO_DPIY(NATRON_BACKDROP_DEFAULT_HEIGHT);
}

void
BackdropGui::createGui()
{
    NodeGui::createGui();

    _imp->backdropEffect = boost::dynamic_pointer_cast<Backdrop>(getNode()->getEffectInstance());
    assert(_imp->backdropEffect.lock());

    _imp->label = new NodeGraphTextItem(getDagGui(), this, false);
    _imp->label->setDefaultTextColor( QColor(0, 0, 0, 255) );
    _imp->label->setZValue(getBaseDepth() + 1);

    QObject::connect( _imp->backdropEffect.lock().get(), SIGNAL(labelChanged(QString)), this, SLOT(onLabelChanged(QString)) );

    _imp->refreshLabelText();

    // Make the backdrop large enough to contain the selected nodes and position it correctly
    NodesGuiList selectedNodes =  getDagGui()->getSelectedNodes();
    QRectF bbox;
    for (NodesGuiList::const_iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        QRectF nodeBbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        bbox = bbox.united(nodeBbox);
    }

    double border50 = mapToScene( QPoint(50, 0) ).x();
    double border0 = mapToScene( QPoint(0, 0) ).x();
    double border = border50 - border0;
    double headerHeight = getFrameNameHeight();
    QPointF scenePos(bbox.x() - border, bbox.y() - border);

    setPos( mapToParent( mapFromScene(scenePos) ) );
    resize(bbox.width() + 2 * border, bbox.height() + 2 * border - headerHeight);

}

void
BackdropGui::onLabelChanged(const QString& /*label*/)
{
    _imp->refreshLabelText();
}

void
BackdropGui::adjustSizeToContent(int *w,
                                 int *h,
                                 bool /*adjustToTextSize*/)
{
    NodeGui::adjustSizeToContent(w, h, false);
    QRectF labelBbox = _imp->label->boundingRect();

    *h = std::max( (double)*h, labelBbox.height() * 1.5 );
    *w = std::max( (double)*w, _imp->label->textWidth() );
}

void
BackdropGui::resizeExtraContent(int /*w*/,
                                int /*h*/,
                                bool forceResize)
{
    QPointF p = pos();
    QPointF thisItemPos = mapFromParent(p);
    int nameHeight = getFrameNameHeight();

    _imp->label->setPos( thisItemPos.x(), thisItemPos.y() + nameHeight + TO_DPIY(10) );
    if (!forceResize) {
        _imp->label->adjustSize();
    }
}

void
BackdropGuiPrivate::refreshLabelText()
{
    KnobStringPtr labelKnob = backdropEffect.lock()->getTextAreaKnob();
    int nameHeight = _publicInterface->getFrameNameHeight();

    QString textLabel = QString::fromUtf8(labelKnob->getValue().c_str());

    double fontColorD[3];
    labelKnob->getFontColor(&fontColorD[0], &fontColorD[1], &fontColorD[2]);

    QColor color;
    color.setRgbF(Image::clamp(fontColorD[0], 0., 1.), Image::clamp(fontColorD[1], 0., 1.), Image::clamp(fontColorD[2], 0., 1.));

    textLabel.replace( QString::fromUtf8("\n"), QString::fromUtf8("<br>") );
    textLabel.prepend( QString::fromUtf8("<div align=\"left\">") );
    textLabel.append( QString::fromUtf8("</div>") );
    QFont f;
    f.setFamily(QString::fromUtf8(labelKnob->getFontFamily().c_str()));
    f.setItalic(labelKnob->getItalicActivated());
    f.setBold(labelKnob->getBoldActivated());
    f.setPointSize(labelKnob->getFontSize());
    bool antialias = appPTR->getCurrentSettings()->isNodeGraphAntiAliasingEnabled();
    if (!antialias) {
        f.setStyleStrategy(QFont::NoAntialias);
    }

    label->setDefaultTextColor(color);
    label->setFont(f);

    label->setHtml(textLabel);

    QRectF bbox = _publicInterface->boundingRect();

    //label->adjustSize();
    int w = std::max( bbox.width(), label->textWidth() * 1.2 );
    QRectF labelBbox = label->boundingRect();
    int h = std::max( labelBbox.height() + nameHeight + TO_DPIX(10), bbox.height() );
    _publicInterface->resize(w, h);
    _publicInterface->update();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_BackdropGui.cpp"
