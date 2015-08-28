/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef ROTOSMEAR_H
#define ROTOSMEAR_H

#include "Engine/EffectInstance.h"

struct RotoSmearPrivate;
class RotoSmear : public Natron::EffectInstance
{
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new RotoSmear(n);
    }
    
    RotoSmear(boost::shared_ptr<Natron::Node> node);
    
    virtual ~RotoSmear();
        
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
    
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_ROTOSMEAR;
    }
    
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Smear";
    }
    
    virtual std::string getDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN { return std::string(); }

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_PAINT);
    }
    
    virtual std::string getInputLabel (int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN {
        return "Source";
    }
    
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }
    
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    
    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual Natron::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return Natron::eRenderSafetyFullySafeFrame;
    }

    // We cannot support tiles with our algorithm
    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }
    
    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isPaintingOverItselfEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual void getPreferredDepthAndComponents(int inputNb,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const OVERRIDE FINAL;
    
    virtual Natron::ImagePremultiplicationEnum getOutputPremultiplication() const OVERRIDE FINAL;
    
    virtual double getPreferredAspectRatio() const OVERRIDE FINAL;

private:

    virtual Natron::StatusEnum
    getRegionOfDefinition(U64 hash,double time, const RenderScale & scale, int view, RectD* rod) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool isIdentity(double time,
                        const RenderScale & scale,
                        const RectI & roi,
                        int view,
                        double* inputTime,
                        int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual Natron::StatusEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;

    boost::scoped_ptr<RotoSmearPrivate> _imp;
};

#endif // ROTOSMEAR_H
