//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef NOOP_H
#define NOOP_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"

class Bool_Knob;
/**
 * @brief A NoOp is an effect that doesn't do anything. It is useful for scripting (adding custom parameters)
 * and it is also used to implement the "Dot" node.
 **/
class NoOpBase
    : public Natron::OutputEffectInstance
{
public:

    NoOpBase(boost::shared_ptr<Natron::Node> n);

    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }

    virtual int getMaxInputCount() const OVERRIDE WARN_UNUSED_RETURN
    {
        return 1;
    }
    
    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }

    virtual std::string getPluginID() const WARN_UNUSED_RETURN = 0;
    virtual std::string getPluginLabel() const WARN_UNUSED_RETURN = 0;
    virtual std::string getDescription() const WARN_UNUSED_RETURN = 0;
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_OTHER);
    }

    virtual bool isInputOptional(int /*inputNb*/) const
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

    virtual Natron::StatusEnum getTransform(SequenceTime time,
                                            const RenderScale& renderScale,
                                            int view,
                                            Natron::EffectInstance** inputToTransform,
                                            Transform::Matrix3x3* transform) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool getInputsHoldingTransform(std::list<int>* inputs) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isOutput() const OVERRIDE WARN_UNUSED_RETURN
    {
        return false;
    }
    
    
    virtual bool isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE FINAL;
    
private:

    /**
     * @brief A NoOp is always an identity on its input.
     **/
    virtual bool isIdentity(SequenceTime time,
                            const RenderScale & scale,
                            const RectI & renderWindow,
                            int view,
                            SequenceTime* inputTime,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

class Dot
    : public NoOpBase
{
public:

    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new Dot(n);
    }

    Dot(boost::shared_ptr<Natron::Node> n)
        : NoOpBase(n)
    {
    }

    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_DOT;
    }

    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Dot";
    }

    virtual std::string getDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getInputLabel(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "";
    }
};


class GroupInput
: public NoOpBase
{
    boost::weak_ptr<Bool_Knob> optional;
    boost::weak_ptr<Bool_Knob> mask;
    
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new GroupInput(n);
    }
    
    GroupInput(boost::shared_ptr<Natron::Node> n)
    : NoOpBase(n)
    {
    }
    
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_INPUT;
    }
    
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Input";
    }
    
    virtual std::string getDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual std::string getInputLabel(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "";
    }
    
    virtual int getMaxInputCount() const OVERRIDE FINAL  WARN_UNUSED_RETURN
    {
        return 0;
    }
    
    virtual bool isGenerator() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;
    
    virtual void knobChanged(KnobI* k,
                             Natron::ValueChangedReasonEnum /*reason*/,
                             int /*view*/,
                             SequenceTime /*time*/,
                             bool /*originatedFromMainThread*/) OVERRIDE FINAL;
    
    
};


class GroupOutput
: public NoOpBase
{
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new GroupOutput(n);
    }
    
    GroupOutput(boost::shared_ptr<Natron::Node> n)
    : NoOpBase(n)
    {
    }
    
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_OUTPUT;
    }
    
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Output";
    }
    
    virtual std::string getDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual std::string getInputLabel(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Source";
    }
    
    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }
    
    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    
};



#endif // NOOP_H
