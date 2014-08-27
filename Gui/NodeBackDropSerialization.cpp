//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "NodeBackDropSerialization.h"

#include "Gui/NodeBackDrop.h"

NodeBackDropSerialization::NodeBackDropSerialization()
    : posX(0),posY(0),width(0),height(0),name(),label(),r(0),g(0),b(0),selected(false), _isNull(true)
{
}

void
NodeBackDropSerialization::initialize(const NodeBackDrop* n)
{
    QPointF pos = n->getPos_mt_safe();

    posX = pos.x();
    posY = pos.y();
    n->getSize(width,height);

    label.reset(new KnobSerialization);
    label->initialize( n->getLabelKnob() );
    QColor color = n->getCurrentColor();
    r = color.redF();
    g = color.greenF();
    b = color.blueF();
    name = n->getName_mt_safe();
    NodeBackDrop* master = n->getMaster();
    if (master) {
        masterBackdropName = master->getName_mt_safe();
    }
    selected = n->getIsSelected();
    _isNull = false;
}