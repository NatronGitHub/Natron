//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef ROTOPAINT_H
#define ROTOPAINT_H

#include "Engine/EffectInstance.h"

struct RotoPaintPrivate;
class RotoPaint : public Natron::EffectInstance
{
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new RotoPaint(n);
    }
    
    RotoPaint(boost::shared_ptr<Natron::Node> node);
    
    virtual ~RotoPaint();
    
    virtual bool isRotoPaintNode() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return true; }
    
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
        return 4;
    }
    
    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }
    
    virtual std::string getPluginID() const WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_ROTOPAINT;
    }

    virtual std::string getPluginLabel() const WARN_UNUSED_RETURN
    {
        return "RotoPaint";
    }

    virtual std::string getDescription() const WARN_UNUSED_RETURN;

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_PAINT);
    }

    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isInputOptional(int /*inputNb*/) const
    {
        return true;
    }

    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual EffectInstance::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return EffectInstance::eRenderSafetyFullySafeFrame;
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

    virtual void getPreferredDepthAndComponents(int inputNb,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const OVERRIDE FINAL;

    virtual Natron::ImagePremultiplicationEnum getOutputPremultiplication() const OVERRIDE FINAL;

    virtual double getPreferredAspectRatio() const OVERRIDE FINAL;

    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;
private:

    virtual Natron::StatusEnum
    getRegionOfDefinition(U64 hash,SequenceTime time, const RenderScale & scale, int view, RectD* rod) OVERRIDE WARN_UNUSED_RETURN;


    virtual Natron::StatusEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;

    boost::scoped_ptr<RotoPaintPrivate> _imp;

};

#endif // ROTOPAINT_H
