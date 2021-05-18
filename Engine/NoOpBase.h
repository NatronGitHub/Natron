/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef Engine_NoOpBase_h
#define Engine_NoOpBase_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/OutputEffectInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief A NoOp is an effect that doesn't do anything. It is useful for scripting (adding custom parameters)
 * and it is also used to implement the "Dot" node.
 **/
class NoOpBase
    : public OutputEffectInstance
{
public:

    NoOpBase(NodePtr n);

    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }

    virtual int getNInputs() const OVERRIDE WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }

    virtual std::string getPluginID() const OVERRIDE WARN_UNUSED_RETURN = 0;
    virtual std::string getPluginLabel() const OVERRIDE WARN_UNUSED_RETURN = 0;
    virtual std::string getPluginDescription() const OVERRIDE WARN_UNUSED_RETURN = 0;
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_OTHER);
    }

    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE
    {
        return false;
    }

    virtual void addAcceptedComponents(int inputNb, std::list<ImagePlaneDesc>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eRenderSafetyFullySafeFrame;
    }

    virtual StatusEnum getTransform(double time,
                                    const RenderScale & renderScale,
                                    bool draftRender,
                                    ViewIdx view,
                                    EffectInstancePtr* inputToTransform,
                                    Transform::Matrix3x3* transform) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getInputsHoldingTransform(std::list<int>* inputs) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isOutput() const OVERRIDE WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE FINAL;

private:

    /**
     * @brief A NoOp is always an identity on its input.
     **/
    virtual bool isIdentity(double time,
                            const RenderScale & scale,
                            const RectI & renderWindow,
                            ViewIdx view,
                            double* inputTime,
                            ViewIdx* inputView,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_NoOpBase_h
