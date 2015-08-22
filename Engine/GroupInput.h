//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef _Engine_GroupInput_h_
#define _Engine_GroupInput_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/NoOpBase.h"

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

#endif // NOOP_H
