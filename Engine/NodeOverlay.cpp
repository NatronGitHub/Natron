/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "NodePrivate.h"

#include <QtCore/QThread>

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/NodeGuiI.h"

NATRON_NAMESPACE_ENTER


bool
Node::canHandleRenderScaleForOverlays() const
{
    return _imp->effect->canHandleRenderScaleForOverlays();
}


bool
Node::shouldDrawOverlay() const
{
    if ( !hasOverlay() && !getRotoContext() ) {
        return false;
    }

    if (!_imp->isPartOfProject) {
        return false;
    }

    if ( !isActivated() ) {
        return false;
    }

    if ( isNodeDisabled() ) {
        return false;
    }

    if ( getParentMultiInstance() ) {
        return false;
    }

    if ( !isSettingsPanelVisible() ) {
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


bool
Node::getOverlayColor(double* r,
                      double* g,
                      double* b) const
{
    NodeGuiIPtr gui_i = getNodeGui();

    if (!gui_i) {
        return false;
    }

    return gui_i->getOverlayColor(r, g, b);
}


void
Node::drawHostOverlay(double time,
                      const RenderScale& renderScale,
                      ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->drawHostOverlay(time, renderScale, view);
    }
}


bool
Node::onOverlayPenDownDefault(double time,
                              const RenderScale& renderScale,
                              ViewIdx view,
                              const QPointF & viewportPos,
                              const QPointF & pos,
                              double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenDownDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}


bool
Node::onOverlayPenDoubleClickedDefault(double time,
                                       const RenderScale& renderScale,
                                       ViewIdx view,
                                       const QPointF & viewportPos,
                                       const QPointF & pos)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenDoubleClickedDefault(time, renderScale, view, viewportPos, pos);
    }

    return false;
}


bool
Node::onOverlayPenMotionDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenMotionDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}


bool
Node::onOverlayPenUpDefault(double time,
                            const RenderScale& renderScale,
                            ViewIdx view,
                            const QPointF & viewportPos,
                            const QPointF & pos,
                            double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenUpDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}


bool
Node::onOverlayKeyDownDefault(double time,
                              const RenderScale& renderScale,
                              ViewIdx view,
                              Key key,
                              KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyDownDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}


bool
Node::onOverlayKeyUpDefault(double time,
                            const RenderScale& renderScale,
                            ViewIdx view,
                            Key key,
                            KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyUpDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}


bool
Node::onOverlayKeyRepeatDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view,
                                Key key,
                                KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyRepeatDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}


bool
Node::onOverlayFocusGainedDefault(double time,
                                  const RenderScale& renderScale,
                                  ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayFocusGainedDefault(time, renderScale, view);
    }

    return false;
}


bool
Node::onOverlayFocusLostDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayFocusLostDefault(time, renderScale, view);
    }

    return false;
}

void
Node::removePositionHostOverlay(KnobI* knob)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->removePositionHostOverlay(knob);
    }
}


void
Node::addPositionInteract(const KnobDoublePtr& position,
                          const KnobBoolPtr& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }

    HostOverlayKnobsPositionPtr knobs = boost::make_shared<HostOverlayKnobsPosition>();
    knobs->addKnob(position, HostOverlayKnobsPosition::eKnobsEnumerationPosition);
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsPosition::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}


void
Node::addTransformInteract(const KnobDoublePtr& translate,
                           const KnobDoublePtr& scale,
                           const KnobBoolPtr& scaleUniform,
                           const KnobDoublePtr& rotate,
                           const KnobDoublePtr& skewX,
                           const KnobDoublePtr& skewY,
                           const KnobChoicePtr& skewOrder,
                           const KnobDoublePtr& center,
                           const KnobBoolPtr& invert,
                           const KnobBoolPtr& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }

    HostOverlayKnobsTransformPtr knobs = boost::make_shared<HostOverlayKnobsTransform>();
    knobs->addKnob(translate, HostOverlayKnobsTransform::eKnobsEnumerationTranslate);
    knobs->addKnob(scale, HostOverlayKnobsTransform::eKnobsEnumerationScale);
    knobs->addKnob(scaleUniform, HostOverlayKnobsTransform::eKnobsEnumerationUniform);
    knobs->addKnob(rotate, HostOverlayKnobsTransform::eKnobsEnumerationRotate);
    knobs->addKnob(center, HostOverlayKnobsTransform::eKnobsEnumerationCenter);
    knobs->addKnob(skewX, HostOverlayKnobsTransform::eKnobsEnumerationSkewx);
    knobs->addKnob(skewY, HostOverlayKnobsTransform::eKnobsEnumerationSkewy);
    knobs->addKnob(skewOrder, HostOverlayKnobsTransform::eKnobsEnumerationSkewOrder);
    if (invert) {
        knobs->addKnob(invert, HostOverlayKnobsTransform::eKnobsEnumerationInvert);
    }
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsTransform::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}


void
Node::addCornerPinInteract(const KnobDoublePtr& from1,
                           const KnobDoublePtr& from2,
                           const KnobDoublePtr& from3,
                           const KnobDoublePtr& from4,
                           const KnobDoublePtr& to1,
                           const KnobDoublePtr& to2,
                           const KnobDoublePtr& to3,
                           const KnobDoublePtr& to4,
                           const KnobBoolPtr& enable1,
                           const KnobBoolPtr& enable2,
                           const KnobBoolPtr& enable3,
                           const KnobBoolPtr& enable4,
                           const KnobChoicePtr& overlayPoints,
                           const KnobBoolPtr& invert,
                           const KnobBoolPtr& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }
    HostOverlayKnobsCornerPinPtr knobs = boost::make_shared<HostOverlayKnobsCornerPin>();
    knobs->addKnob(from1, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom1);
    knobs->addKnob(from2, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom2);
    knobs->addKnob(from3, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom3);
    knobs->addKnob(from4, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom4);

    knobs->addKnob(to1, HostOverlayKnobsCornerPin::eKnobsEnumerationTo1);
    knobs->addKnob(to2, HostOverlayKnobsCornerPin::eKnobsEnumerationTo2);
    knobs->addKnob(to3, HostOverlayKnobsCornerPin::eKnobsEnumerationTo3);
    knobs->addKnob(to4, HostOverlayKnobsCornerPin::eKnobsEnumerationTo4);

    knobs->addKnob(enable1, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable1);
    knobs->addKnob(enable2, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable2);
    knobs->addKnob(enable3, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable3);
    knobs->addKnob(enable4, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable4);

    knobs->addKnob(overlayPoints, HostOverlayKnobsCornerPin::eKnobsEnumerationOverlayPoints);

    if (invert) {
        knobs->addKnob(invert, HostOverlayKnobsCornerPin::eKnobsEnumerationInvert);
    }
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsCornerPin::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}


void
Node::initializeHostOverlays()
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (!nodeGui) {
        return;
    }
    for (std::list<HostOverlayKnobsPtr> ::iterator it = _imp->nativeOverlays.begin(); it != _imp->nativeOverlays.end(); ++it) {
        nodeGui->addDefaultInteract(*it);
    }
    _imp->nativeOverlays.clear();
}


bool
Node::hasOverlay() const
{
    if (!_imp->effect) {
        return false;
    }

    NodeGuiIPtr nodeGui = getNodeGui();
    if (nodeGui) {
        if ( nodeGui->hasHostOverlay() ) {
            return true;
        }
    }

    return _imp->effect->hasOverlay();
}


bool
Node::hasHostOverlayForParam(const KnobI* knob) const
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlayForParam(knob) ) {
        return true;
    }

    return false;
}


bool
Node::hasHostOverlay() const
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlay() ) {
        return true;
    }

    return false;
}


void
Node::setCurrentViewportForHostOverlays(OverlaySupport* viewPort)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlay() ) {
        nodeGui->setCurrentViewportForHostOverlays(viewPort);
    }
}

NATRON_NAMESPACE_EXIT

