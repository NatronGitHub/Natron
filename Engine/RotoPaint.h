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

#ifndef ROTOPAINT_H
#define ROTOPAINT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/NodeGroup.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

#define ROTOPAINT_MAX_INPUTS_COUNT 11
#define ROTOPAINT_MASK_INPUT_INDEX 10


NATRON_NAMESPACE_ENTER;

struct RotoPaintPrivate;
class RotoPaint
    : public NodeGroup
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected: // derives from EffectInstance, parent of RotoNode
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    RotoPaint(const NodePtr& node,
              bool isPaintByDefault);

public:
    static EffectInstancePtr create(const NodePtr& node,
                                    bool isPaintByDefault) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoPaint(node, isPaintByDefault) );
    }

    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoPaint(node, true) );
    }

    virtual ~RotoPaint();

    bool isDefaultBehaviourPaintContext() const;

    virtual bool isRotoPaintNode() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return true; }

    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }


    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual std::string getPluginID() const OVERRIDE WARN_UNUSED_RETURN;
    virtual std::string getPluginLabel() const OVERRIDE WARN_UNUSED_RETURN;
    virtual std::string getPluginDescription() const OVERRIDE WARN_UNUSED_RETURN;
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_PAINT);
    }


    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eRenderSafetyFullySafeFrame;
    }

    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual StatusEnum getPreferredMetaDatas(NodeMetadata& metadata) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;
    virtual bool isHostMaskingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }

    virtual bool isHostMixingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return true; }

    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
    virtual void onKnobsLoaded() OVERRIDE FINAL;
    virtual void onEnableOpenGLKnobValueChanged(bool activated) OVERRIDE FINAL;

    bool mustDoNeatRender() const;

    void setIsDoingNeatRender(bool doing);

    bool isDoingNeatRender() const;

    virtual void onEffectCreated(bool mayCreateFileDialog, const CreateNodeArgs& args) OVERRIDE FINAL;

    NodePtr getPremultNode() const;

    NodePtr getInternalInputNode(int index) const;

    NodePtr getMetadataFixerNode() const;

    void getEnabledChannelKnobs(KnobBoolPtr* r,KnobBoolPtr* g, KnobBoolPtr* b, KnobBoolPtr *a) const;

    virtual bool isSubGraphUserVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN;

public Q_SLOTS:


    void onRefreshAsked();

    void onCurveLockedChanged(int);

    void onSelectionChanged(int reason);

    void onBreakMultiStrokeTriggered();

private:

    virtual bool shouldPreferPluginOverlayOverHostOverlay() const OVERRIDE FINAL;

    virtual bool shouldDrawHostOverlay() const OVERRIDE FINAL;

    virtual bool hasOverlay() const OVERRIDE FINAL
    {
        return true;
    }


    virtual void getPluginShortcuts(std::list<PluginActionShortcut>* shortcuts) const OVERRIDE FINAL;
    virtual void drawOverlay(double time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayPenDown(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenMotion(double time, const RenderScale & renderScale, ViewIdx view,
                                    const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenUp(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenDoubleClicked(double time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           const QPointF & viewportPos,
                                           const QPointF & pos) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyDown(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyUp(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyRepeat(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayFocusGained(double time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayFocusLost(double time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual void onInteractViewportSelectionCleared() OVERRIDE FINAL;
    virtual void onInteractViewportSelectionUpdated(const RectD& rectangle, bool onRelease) OVERRIDE FINAL;
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSpec view,
                             double time,
                             bool originatedFromMainThread) OVERRIDE FINAL;

    virtual void refreshExtraStateAfterTimeChanged(bool isPlayback, double time)  OVERRIDE FINAL;


    /**
     * @brief When finishing a stroke with the paint brush, we need to re-render it because the interpolation of the curve
     * will be much smoother with more points than what it was during painting.
     * We explicitly freeze the UI while waiting for the image to be drawn, otherwise the user might attempt to do a
     * multiple stroke on top of it which could make some artifacts.
     **/
    void evaluateNeatStrokeRender();

    
    boost::scoped_ptr<RotoPaintPrivate> _imp;
};

/**
 * @brief Same as RotoPaint except that by default RGB checkboxes are unchecked and the default selected tool is not the same
 **/
class RotoNode
    : public RotoPaint
{
private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    RotoNode(const NodePtr& node)
        : RotoPaint(node, false) {}
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new RotoNode(node) );
    }

private:

    virtual std::string getPluginID() const OVERRIDE WARN_UNUSED_RETURN;
    virtual std::string getPluginLabel() const OVERRIDE WARN_UNUSED_RETURN;
    virtual std::string getPluginDescription() const OVERRIDE WARN_UNUSED_RETURN;
    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
};

inline RotoPaintPtr
toRotoPaint(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<RotoPaint>(effect);
}

inline RotoNodePtr
toRotoNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<RotoNode>(effect);
}

NATRON_NAMESPACE_EXIT;

#endif // ROTOPAINT_H
