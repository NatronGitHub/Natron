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

#ifndef BACKDROP_H
#define BACKDROP_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/NoOpBase.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct BackDropPrivate;

class BackDrop : public NoOpBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Node> n)
    {
        return new BackDrop(n);
    }
    
    BackDrop(boost::shared_ptr<Node> node);
    
    virtual ~BackDrop();
  
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_BACKDROP;
    }
    
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "BackDrop";
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
    
Q_SIGNALS:
    
    void labelChanged(QString);
    
private:
    
    virtual void knobChanged(KnobI* k,
                             ValueChangedReasonEnum /*reason*/,
                             int /*view*/,
                             double /*time*/,
                             bool /*originatedFromMainThread*/) OVERRIDE FINAL;

    virtual void initializeKnobs() OVERRIDE FINAL;
    
    boost::scoped_ptr<BackDropPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // BACKDROP_H
