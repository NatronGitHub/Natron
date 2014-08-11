//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#ifndef NOOP_H
#define NOOP_H

#include <boost/shared_ptr.hpp>
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"

/**
 * @brief A NoOp is an effect that doesn't do anything. It is useful for scripting (adding custom parameters)
 * and it is also used to implement the "Dot" node.
 **/
class NoOpBase : public Natron::EffectInstance
{
public:
    
    NoOpBase(boost::shared_ptr<Natron::Node> n);
    
    virtual int majorVersion() const  OVERRIDE FINAL WARN_UNUSED_RETURN { return 1; }
    
    virtual int minorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return 0; }
    
    virtual int maximumInputs() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return 1; }
    
    virtual std::string pluginID() const  WARN_UNUSED_RETURN = 0;
    
    virtual std::string pluginLabel() const  WARN_UNUSED_RETURN = 0;
    
    virtual std::string description() const  WARN_UNUSED_RETURN = 0;
    
    virtual void pluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL { grouping->push_back(PLUGIN_GROUP_OTHER); }
    
    virtual bool isInputOptional(int /*inputNb*/) const { return false; }
    
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;

    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const OVERRIDE FINAL;
    
    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    { return EffectInstance::FULLY_SAFE_FRAME; }
private:
    
    /**
     * @brief A NoOp is always an identity on its input.
     **/
    virtual bool isIdentity(SequenceTime time,
                            const RenderScale& scale,
                            const RectD& rod, //!< image rod in canonical coordinates
                            int view,
                            SequenceTime* inputTime,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

class Dot: public NoOpBase
{
    
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new Dot(n);
    }
    
    Dot(boost::shared_ptr<Natron::Node> n) : NoOpBase(n) {}
    
    virtual std::string pluginID() const  OVERRIDE FINAL WARN_UNUSED_RETURN { return "Dot"; }
    
    virtual std::string pluginLabel() const  OVERRIDE FINAL WARN_UNUSED_RETURN { return "Dot"; }
    
    virtual std::string description() const  OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual std::string inputLabel(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN { return ""; }
};

#endif // NOOP_H
