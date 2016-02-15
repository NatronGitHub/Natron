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

#ifndef Engine_GroupInput_h
#define Engine_GroupInput_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/NoOpBase.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

class GroupInput
: public NoOpBase
{
    boost::weak_ptr<KnobBool> optional;
    boost::weak_ptr<KnobBool> mask;
    
public:
    
    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new GroupInput(n);
    }
    
    GroupInput(NodePtr n)
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
    
    virtual std::string getPluginDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
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
                             ValueChangedReasonEnum /*reason*/,
                             ViewSpec /*view*/,
                             double /*time*/,
                             bool /*originatedFromMainThread*/) OVERRIDE FINAL;
    
    virtual ImagePremultiplicationEnum getOutputPremultiplication() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
};

NATRON_NAMESPACE_EXIT;

#endif // NOOP_H
