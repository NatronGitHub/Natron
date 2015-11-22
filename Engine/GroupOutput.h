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

#ifndef Engine_GroupOutput_h
#define Engine_GroupOutput_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/NoOpBase.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

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
    
    virtual std::string getPluginDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
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



#endif // Engine_GroupOutput_h
