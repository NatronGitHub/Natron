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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ScriptObject.h"

#include <string>
#include <QMutex>

struct ScriptObjectPrivate
{
    mutable QMutex nameMutex;
    std::string name,label;
    
    ScriptObjectPrivate()
    : nameMutex()
    , name()
    , label()
    {
        
    }
};

ScriptObject::ScriptObject()
: _imp(new ScriptObjectPrivate())
{
}

ScriptObject::~ScriptObject()
{
    
}

void
ScriptObject::setLabel(const std::string& label)
{
    QMutexLocker k(&_imp->nameMutex);
    _imp->label = label;
}

std::string
ScriptObject::getLabel() const
{
    QMutexLocker k(&_imp->nameMutex);
    return _imp->label;
}

void
ScriptObject::setScriptName(const std::string& name)
{
    QMutexLocker k(&_imp->nameMutex);
    _imp->name = name;
}

std::string
ScriptObject::getScriptName() const
{
    QMutexLocker k(&_imp->nameMutex);
    return _imp->name;
}