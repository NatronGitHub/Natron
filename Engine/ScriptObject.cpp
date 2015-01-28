//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


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