/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef Engine_WriteNode_h
#define Engine_WriteNode_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/NodeGroup.h"


#define kNatronWriteNodeParamEncodingPluginChoice "encodingPluginChoice"
#define kNatronWriteNodeParamEncodingPluginID "encodingPluginID"

#define kNatronWriteParamFrameStep "frameIncr"
#define kNatronWriteParamFrameStepLabel "Frame Increment"
#define kNatronWriteParamFrameStepHint "The number of frames the timeline should step before rendering the new frame. " \
    "If 1, all frames will be rendered, if 2 only 1 frame out of 2, " \
    "etc. This number cannot be less than 1."

#define kNatronWriteParamReadBack "readBack"
#define kNatronWriteParamReadBackLabel "Read back file"
#define kNatronWriteParamReadBackHint "When checked, the output of this node comes from reading the written file instead of the input node"

#define kNatronWriteParamStartRender "startRender"

NATRON_NAMESPACE_ENTER

/**
 * @brief A wrapper around all OpenFX Writers nodes so that to the user they all appear under a single Write node that has a dynamic
 * settings panel.
 * Every method forwards to the corresponding OpenFX plug-in.
 **/
struct WriteNodePrivate;
class WriteNode
    : public NodeGroup
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new WriteNode(n);
    }

    /**
     * @brief Returns if the given plug-in is compatible with this WriteNode container.
     * by default all nodes which inherits GenericWriter in OpenFX are.
     **/
    static bool isBundledWriter(const std::string& pluginID, bool wasProjectCreatedWithLowerCaseIDs);
    bool isBundledWriter(const std::string& pluginID);

    WriteNode(NodePtr n);

    virtual ~WriteNode();

    NodePtr getEmbeddedWriter() const;

    void setEmbeddedWriter(const NodePtr& node);

    static bool isVideoWriter(const std::string& pluginID);
    
    virtual bool isWriter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isVideoWriter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isGenerator() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getPluginDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL;
    virtual void onEffectCreated(bool mayCreateFileDialog, const CreateNodeArgs& defaultParamValues) OVERRIDE FINAL;
    virtual bool isSubGraphUserVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    void renderSequenceStarted();
    void renderSequenceEnd();

    virtual SequentialPreferenceEnum getSequentialPreference() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isViewAware() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasOverlay() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getOverlayCursor(CursorEnum* cursor,
                                  std::string* customImage,
                                  int* hotSpotX,
                                  int* hotSpotY) const OVERRIDE FINAL;
    virtual void onInteractViewportSelectionCleared(KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual void onInteractViewportSelectionUpdated(const RectD& rectangle, bool onRelease, KeyboardModifiers modifiers) OVERRIDE FINAL;

public Q_SLOTS:

    void onSequenceRenderStarted();
    void onSequenceRenderFinished();

private:

    virtual void getFrameRange(double *first, double *last) OVERRIDE FINAL;
    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual void onKnobsAboutToBeLoaded(const NodeSerializationPtr& serialization) OVERRIDE FINAL;
    virtual bool knobChanged(KnobI* k,
                             ValueChangedReasonEnum reason,
                             ViewSpec view,
                             double time,
                             bool originatedFromMainThread) OVERRIDE FINAL;

    // Overlay interacts: forwarded to the embedded writer plug-in, which owns the OFX overlay interact
    EffectInstancePtr getEmbeddedWriterEffect() const;
    virtual void setCurrentViewportForOverlays(OverlaySupport* viewport) OVERRIDE FINAL;
    virtual void setInteractColourPicker(const OfxRGBAColourD& color, bool setColor, bool hasColor) OVERRIDE FINAL;
    virtual void drawOverlay(double time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayPenDown(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, PenType pen, KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenDoubleClicked(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenMotion(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenUp(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyDown(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyUp(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyRepeat(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayFocusGained(double time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayFocusLost(double time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL WARN_UNUSED_RETURN;
    std::unique_ptr<WriteNodePrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_WriteNode_h
