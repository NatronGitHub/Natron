//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#ifndef DISKCACHENODE_H
#define DISKCACHENODE_H

#include "Engine/EffectInstance.h"

struct DiskCacheNodePrivate;
class DiskCacheNode : public Natron::OutputEffectInstance
{
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new DiskCacheNode(n);
    }
    
    DiskCacheNode(boost::shared_ptr<Natron::Node> node);
    
    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }
    
    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }
    
    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }
    
    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }
    
    virtual std::string getPluginID() const WARN_UNUSED_RETURN
    {
        return "DiskCache";
    }

    virtual std::string getPluginLabel() const WARN_UNUSED_RETURN
    {
        return "DiskCache";
    }

    virtual std::string getDescription() const WARN_UNUSED_RETURN
    {
        return "This node caches all images of the connected input node onto the disk with full 32bit floating point raw data. "
    "When an image is found in the cache, " NATRON_APPLICATION_NAME " will then not request the input branch to render out that image. "
    "The DiskCache node only caches full images and does not split up the images in chunks.  "
    "The DiskCache node is useful if you're working with a large and complex node tree: this allows to break the tree into smaller "
    "branches and cache any branch that you're no longer working on. The cached images are saved by default in the same directory that is used "
    "for the viewer cache but you can set its location and size in the preferences. A solid state drive disk is recommended for efficiency of this node. "
    "By default all images that pass into the node are cached but they depend on the zoom-level of the viewer. For convenience you can cache "
    "a specific frame range at scale 100% much like a writer node would do. \n"
    "WARNING: The DiskCache node must be part of the tree when you want to read cached data from it. ";
    }

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_OTHER);
    }

    virtual std::string getInputLabel (int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Source";
    }
    
    virtual bool isInputOptional(int /*inputNb*/) const
    {
        return false;
    }
    
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponentsEnum>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    
    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual EffectInstance::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return EffectInstance::eRenderSafetyFullySafeFrame;
    }

    virtual bool supportsTiles() const
    {
        return false;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;

    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE FINAL;

    virtual void getPreferredDepthAndComponents(int inputNb,Natron::ImageComponentsEnum* comp,Natron::ImageBitDepthEnum* depth) const OVERRIDE FINAL;

    virtual Natron::ImagePremultiplicationEnum getOutputPremultiplication() const OVERRIDE FINAL;

    virtual double getPreferredAspectRatio() const OVERRIDE FINAL;

private:

    virtual void knobChanged(KnobI* k, Natron::ValueChangedReasonEnum reason, int view, SequenceTime time) OVERRIDE FINAL;

    virtual Natron::StatusEnum render(SequenceTime time,
                                      const RenderScale& originalScale,
                                      const RenderScale & mappedScale,
                                      const RectI & roi, //!< renderWindow in pixel coordinates
                                      int view,
                                      bool isSequentialRender,
                                      bool isRenderResponseToUserInteraction,
                                      boost::shared_ptr<Natron::Image> output) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool shouldCacheOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    boost::scoped_ptr<DiskCacheNodePrivate> _imp;
};

#endif // DISKCACHENODE_H
