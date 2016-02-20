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

#ifndef Engine_JoinViewsNode_h
#define Engine_JoinViewsNode_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#include "Engine/EffectInstance.h"

NATRON_NAMESPACE_ENTER;

struct JoinViewsNodePrivate;
class JoinViewsNode : public EffectInstance
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new JoinViewsNode(n);
    }
    

    
    JoinViewsNode(NodePtr node);
    
    virtual ~JoinViewsNode();
    
    
    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }
    
    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }
    
    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_JOINVIEWS;
    }
    
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "JoinViews";
    }
    
    virtual std::string getPluginDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Take in input separate views to make a multiple view stream output. "
        "The first view from each input is copied to one of the view of the output.";
    }
    
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_MULTIVIEW);
    }
    
    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual void addAcceptedComponents(int inputNb,std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    
    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eRenderSafetyFullySafeFrame;
    }
    
    virtual bool isViewAware() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual void initializeKnobs() OVERRIDE FINAL;
    
    virtual bool isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
  
public Q_SLOTS:
    
    void onProjectViewsChanged();
    
private:
    
    
    virtual bool isIdentity(double time,
                            const RenderScale & scale,
                            const RectI & roi,
                            ViewIdx view,
                            double* inputTime,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
    boost::scoped_ptr<JoinViewsNodePrivate> _imp;
};

NATRON_NAMESPACE_EXIT;
#endif // Engine_JoinViewsNode_h
