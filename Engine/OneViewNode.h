/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef ENGINE_ONEVIEWNODE_H
#define ENGINE_ONEVIEWNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EffectInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct OneViewNodePrivate;
class OneViewNode
    : public EffectInstance
{

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    OneViewNode(const NodePtr& n);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new OneViewNode(node) );
    }

    static PluginPtr createPlugin();

    virtual ~OneViewNode();

    virtual int getMaxInputCount() const OVERRIDE WARN_UNUSED_RETURN
    {
        return 1;
    }
    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputMask(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void addAcceptedComponents(int inputNb, std::bitset<4>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;


    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual bool isHostChannelSelectorSupported(bool* /*defaultR*/,
                                                bool* /*defaultG*/,
                                                bool* /*defaultB*/,
                                                bool* /*defaultA*/) const OVERRIDE WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isViewAware() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual ViewInvarianceLevel isViewInvariant() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eViewInvarianceAllViewsVariant;
    }


private:

    virtual void onMetadataChanged(const NodeMetadata& metadata) OVERRIDE FINAL;

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual ActionRetCodeEnum getFramesNeeded(TimeValue time, ViewIdx view, const TreeRenderNodeArgsPtr& render, FramesNeededMap* results) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                            const RenderScale & scale,
                            const RectI & roi,
                            ViewIdx view,
                            const TreeRenderNodeArgsPtr& render,
                            TimeValue* inputTime,
                            ViewIdx* inputView,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
    boost::scoped_ptr<OneViewNodePrivate> _imp;
};

inline OneViewNodePtr
toOneViewNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<OneViewNode>(effect);
}

NATRON_NAMESPACE_EXIT;

#endif // ENGINE_ONEVIEWNODE_H
