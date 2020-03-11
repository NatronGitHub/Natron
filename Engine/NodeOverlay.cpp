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

#include "NodePrivate.h"
#include "Engine/CornerPinOverlayInteract.h"
#include "Engine/PointOverlayInteract.h"
#include "Engine/TransformOverlayInteract.h"

NATRON_NAMESPACE_ENTER


bool
Node::canHandleRenderScaleForOverlays() const
{
    return _imp->effect->canHandleRenderScaleForOverlays();
}

bool
Node::isDoingInteractAction() const
{
    if (QThread::currentThread() != qApp->thread()) {
        return false;
    }

    return _imp->duringInteractAction;
}


bool
Node::shouldDrawOverlay(TimeValue time, ViewIdx view, OverlayViewportTypeEnum type) const
{

    if (!_imp->effect->hasOverlayInteract(type)) {
        return false;
    }
    
    if (!_imp->isPersistent) {
        return false;
    }

    if ( _imp->effect->isNodeDisabledForFrame(time, view) ) {
        return false;
    }

    if ( !isEffectViewerNode() && !isSettingsPanelVisible() ) {
        return false;
    }

    if ( isSettingsPanelMinimized() ) {
        return false;
    }


    return true;
}


void
Node::setCurrentCursor(CursorEnum defaultCursor)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->setCurrentCursor(defaultCursor);
    }
}

bool
Node::setCurrentCursor(const QString& customCursorFilePath)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->setCurrentCursor(customCursorFilePath);
    }

    return false;
}


void
Node::onNodeUIOverlayColorChanged(double r,
                                  double g,
                                  double b)
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    _imp->overlayColor[0] = r;
    _imp->overlayColor[1] = g;
    _imp->overlayColor[2] = b;

}

void
Node::setOverlayColor(double r, double g, double b)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setOverlayColor(r, g, b);
    } else {
        onNodeUIOverlayColorChanged(r,g,b);
    }
}

bool
Node::getOverlayColor(double* r,
                      double *g,
                      double* b) const
{
    NodeGuiIPtr node_ui = getNodeGui();
    if (node_ui && node_ui->isOverlayLocked()) {
        *r = 0.5;
        *g = 0.5;
        *b = 0.5;
        return true;
    }

    double tmpColor[3];
    {
        QMutexLocker k(&_imp->nodeUIDataMutex);
        tmpColor[0] = _imp->overlayColor[0];
        tmpColor[1] = _imp->overlayColor[1];
        tmpColor[2] = _imp->overlayColor[2];
    }
    if (tmpColor[0] == -1 && tmpColor[1] == -1 && tmpColor[2] == -1) {
        // No overlay color
        return false;
    }
    *r = tmpColor[0];
    *g = tmpColor[1];
    *b = tmpColor[2];

    return true;
}
NATRON_NAMESPACE_EXIT
