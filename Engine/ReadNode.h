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

#ifndef READNODE_H
#define READNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EffectInstance.h"

#define NATRON_READER_INPUT_NAME "Sync"

#define kNatronReadNodeParamDecodingPluginChoice "decodingPluginChoice"
#define kNatronReadNodeParamDecodingPluginID "decodingPluginID"

#define kNatronReadNodeOCIOParamInputSpace "ocioInputSpace"

NATRON_NAMESPACE_ENTER;

/**
 * @brief A wrapper around all OpenFX Readers nodes so that to the user they all appear under a single Read node that has a dynamic
 * settings panel.
 * Every method forwards to the corresponding OpenFX plug-in.
 **/
struct ReadNodePrivate;
class ReadNode
    : public EffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    /**
     * @brief Returns if the given plug-in is compatible with this ReadNode container.
     * by default all nodes which inherits GenericReader in OpenFX are.
     **/
    static bool isBundledReader(const std::string& pluginID);

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    ReadNode(const NodePtr& n);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new ReadNode(node) );
    }

    static PluginPtr createPlugin();

    virtual ~ReadNode();

    NodePtr getEmbeddedReader() const;

    void setEmbeddedReader(const NodePtr& node);

    virtual bool isReader() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isGenerator() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isMultiPlanar() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isViewAware() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool supportsMultipleClipDepths() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual SequentialPreferenceEnum getSequentialPreference() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual EffectInstance::ViewInvarianceLevel isViewInvariant() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual EffectInstance::PassThroughEnum isPassThroughForNonRenderedPlanes() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputOptional(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputMask(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void addAcceptedComponents(int inputNb, std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    virtual void onInputChanged(int inputNo, const NodePtr& oldNode, const NodePtr& newNode) OVERRIDE FINAL;
    virtual void purgeCaches() OVERRIDE FINAL;
    virtual void onEffectCreated(const CreateNodeArgs& defaultParamValues) OVERRIDE FINAL;

private:

    virtual ActionRetCodeEnum getTimeInvariantMetaDatas(NodeMetadata& metadata) OVERRIDE FINAL;
    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual void onKnobsAboutToBeLoaded(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization) OVERRIDE FINAL;
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE FINAL;
    virtual ActionRetCodeEnum getRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view, const TreeRenderNodeArgsPtr& render, RectD* rod) OVERRIDE WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum getFrameRange(const TreeRenderNodeArgsPtr& render, double *first, double *last) OVERRIDE FINAL;
    virtual ActionRetCodeEnum getComponentsAction(TimeValue time,
                                                ViewIdx view,
                                                const TreeRenderNodeArgsPtr& render,
                                                std::map<int, std::list<ImageComponents> >* inputLayersNeeded,
                                                std::list<ImageComponents>* layersProduced,
                                                TimeValue* passThroughTime,
                                                ViewIdx* passThroughView,
                                                int* passThroughInputNb) OVERRIDE;
    virtual ActionRetCodeEnum beginSequenceRender(double first,
                                           double last,
                                           double step,
                                           bool interactive,
                                           const RenderScale & scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           bool draftMode,
                                           ViewIdx view,
                                           RenderBackendTypeEnum backend,
                                           const EffectOpenGLContextDataPtr& glContextData,
                                           const TreeRenderNodeArgsPtr& render) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum endSequenceRender(double first,
                                         double last,
                                         double step,
                                         bool interactive,
                                         const RenderScale & scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         bool draftMode,
                                         ViewIdx view,
                                         RenderBackendTypeEnum backend,
                                         const EffectOpenGLContextDataPtr& glContextData,
                                         const TreeRenderNodeArgsPtr& render) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum getRegionsOfInterest(TimeValue time,
                                      const RenderScale & scale,
                                      const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                      ViewIdx view,
                                      const TreeRenderNodeArgsPtr& render,
                                      RoIMap* ret) OVERRIDE FINAL;
    virtual ActionRetCodeEnum getFramesNeeded(TimeValue time, ViewIdx view, const TreeRenderNodeArgsPtr& render, FramesNeededMap* framesNeeded) OVERRIDE WARN_UNUSED_RETURN;
    boost::scoped_ptr<ReadNodePrivate> _imp;
};

inline ReadNodePtr
toReadNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<ReadNode>(effect);
}

NATRON_NAMESPACE_EXIT;

#endif // READNODE_H
