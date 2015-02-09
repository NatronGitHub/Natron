//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NodeGuiSerialization.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QColor>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/NodeGui.h"

#include "Engine/Node.h"


void
NodeGuiSerialization::initialize(const NodeGui*  n)
{
    ////All this code is MT-safe
    _nodeName = n->getNode()->getFullyQualifiedName();
    QPointF pos = n->getPos_mt_safe();
    _posX = pos.x();
    _posY = pos.y();
    n->getSize(&_width, &_height);
    _previewEnabled = n->getNode()->isPreviewEnabled();
    QColor color = n->getCurrentColor();
    _r = color.redF();
    _g = color.greenF();
    _b = color.blueF();
    _selected = n->isSelected();
    
    _hasOverlayColor = n->getOverlayColor(&_overlayR, &_overlayG, &_overlayB);
    
}

