//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/


#include "NodeGuiSerialization.h"

#include <QColor>
#include "Gui/NodeGui.h"

#include "Engine/Node.h"


void NodeGuiSerialization::initialize(const boost::shared_ptr<NodeGui>& n)
{
    
    ////All this code is MT-safe
    _nodeName = n->getNode()->getName_mt_safe();
    QPointF pos = n->getPos_mt_safe();
    _posX = pos.x();
    _posY = pos.y();
    _previewEnabled = n->getNode()->isPreviewEnabled();
    QColor color = n->getCurrentColor();
    _r = color.redF();
    _g = color.greenF();
    _b = color.blueF();
    _selected = n->isSelected();
}
