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

#ifndef SCRIPTOBJECT_H
#define SCRIPTOBJECT_H

#include <string>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"


struct ScriptObjectPrivate;
class ScriptObject
{
public:
    
    ScriptObject();
    
    virtual ~ScriptObject();
    
    void setLabel(const std::string& label);
    
    std::string getLabel() const;
    
    void setScriptName(const std::string& name);
    
    std::string getScriptName() const;
    
private:
    
    boost::scoped_ptr<ScriptObjectPrivate> _imp;
};

#endif // SCRIPTOBJECT_H
