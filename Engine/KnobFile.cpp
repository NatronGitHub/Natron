/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "KnobFile.h"

#include <utility>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include <QtCore/QStringList>
#include <QtCore/QMutexLocker>
#include <QtCore/QDebug>

#include "Engine/EffectInstance.h"
#include "Engine/Transform.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/AppInstance.h"

#include <SequenceParsing.h> // for SequenceParsing::generateFileNameFromPattern

#include "Global/QtCompat.h"

#include "Serialization/KnobSerialization.h"

NATRON_NAMESPACE_ENTER;

struct KnobFilePrivate
{

    KnobFile::KnobFileDialogTypeEnum dialogType;
    std::vector<std::string> dialogFilters;

    KnobFilePrivate()
    : dialogType(KnobFile::eKnobFileDialogTypeOpenFile)
    , dialogFilters()
    {

    }
};

KnobFile::KnobFile(const KnobHolderPtr& holder,
                   const std::string &name,
                   int dimension)
: AnimatingKnobStringHelper(holder, name, dimension)
, _imp(new KnobFilePrivate())
{
}

KnobFile::KnobFile(const KnobHolderPtr& holder,const KnobIPtr& mainInstance)
: AnimatingKnobStringHelper(holder, mainInstance)
, _imp(toKnobFile(mainInstance)->_imp)
{

}

KnobFile::~KnobFile()
{
}

void
KnobFile::reloadFile()
{
    assert( getHolder() );
    EffectInstancePtr effect = toEffectInstance( getHolder() );
    if (effect) {
        effect->purgeCaches_public();
        effect->getNode()->clearAllPersistentMessages(false);
    }
    evaluateValueChange(DimSpec(0), getHolder()->getTimelineCurrentTime(), ViewSetSpec(0), eValueChangedReasonUserEdited);
}

bool
KnobFile::canAnimate() const
{
    return true;
}

const std::string KnobFile::_typeNameStr(kKnobFileTypeName);
const std::string &
KnobFile::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobFile::typeName() const
{
    return typeNameStatic();
}


std::string
KnobFile::getRawFileName(DimIdx dimension, ViewIdx view)
{
    return Knob<std::string>::getValue(dimension, view);
}

std::string
KnobFile::getValueForHash(DimIdx dim, ViewIdx view)
{
    return getRawFileName(dim, view);
}

std::string
KnobFile::getValue(DimIdx dimension, ViewIdx view, bool clampToMinMax)
{
    return getValueAtTime(getCurrentRenderTime(), dimension, view, clampToMinMax);
}

void
KnobFile::setDialogType(KnobFileDialogTypeEnum type)
{
    _imp->dialogType = type;
}

KnobFile::KnobFileDialogTypeEnum
KnobFile::getDialogType() const
{
    return _imp->dialogType;
}

void
KnobFile::setDialogFilters(const std::vector<std::string>& filters)
{
    _imp->dialogFilters = filters;
}

const std::vector<std::string>&
KnobFile::getDialogFilters() const
{
    return _imp->dialogFilters;
}


std::string
KnobFile::getValueAtTime(TimeValue time, DimIdx dimension, ViewIdx view, bool /*clampToMinMax*/)
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    if (_imp->dialogType == eKnobFileDialogTypeOpenFileSequences ||
        _imp->dialogType == eKnobFileDialogTypeSaveFileSequences) {
        ///try to interpret the pattern and generate a filename if indexes are found
        std::vector<std::string> views;
        if ( getHolder() && getHolder()->getApp() ) {
            views = getHolder()->getApp()->getProject()->getProjectViewNames();
        }
        std::string pattern = Knob<std::string>::getValue(dimension, view);
        return SequenceParsing::generateFileNameFromPattern(pattern, views, time, view_i);
    }
    return Knob<std::string>::getValue(dimension, view);
}

/***********************************KnobPath*****************************************/
struct KnobPathPrivate
{
    bool isMultiPath;
    bool isStringList;

    KnobPathPrivate()
    : isMultiPath(false)
    , isStringList(false)
    {
        
    }
};

KnobPath::KnobPath(const KnobHolderPtr& holder,
                   const std::string &name,
                   int dimension)
: KnobTable(holder, name, dimension)
, _imp(new KnobPathPrivate())
{
}

KnobPath::KnobPath(const KnobHolderPtr& holder,const KnobIPtr& mainInstance)
: KnobTable(holder, mainInstance)
, _imp(toKnobPath(mainInstance)->_imp)
{

}

KnobPath::~KnobPath()
{
    
}

const std::string KnobPath::_typeNameStr(kKnobPathTypeName);
const std::string &
KnobPath::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobPath::typeName() const
{
    return typeNameStatic();
}

void
KnobPath::setMultiPath(bool b)
{
    _imp->isMultiPath = b;
}

bool
KnobPath::isMultiPath() const
{
    return _imp->isMultiPath;
}

void
KnobPath::setAsStringList(bool b)
{
    setMultiPath(b);
    _imp->isStringList = b;
}

bool
KnobPath::getIsStringList() const
{
    return _imp->isStringList;
}

std::string
KnobPath::generateUniquePathID(const std::list<std::vector<std::string> >& paths)
{
    std::string baseName("Path");
    int idx = 0;
    bool found;
    std::string name;

    do {
        std::stringstream ss;
        ss << baseName;
        ss << idx;
        name = ss.str();
        found = false;
        for (std::list<std::vector<std::string> >::const_iterator it = paths.begin(); it != paths.end(); ++it) {
            if ( (*it)[0] == name ) {
                found = true;
                break;
            }
        }
        ++idx;
    } while (found);

    return name;
}

bool
KnobPath::isCellEnabled(int /*row*/,
                        int /*col*/,
                        const QStringList& values) const
{
    if ( ( values[0] == QString::fromUtf8(NATRON_PROJECT_ENV_VAR_NAME) ) ||
         ( values[0] == QString::fromUtf8(NATRON_OCIO_ENV_VAR_NAME) ) ) {
        return false;
    }

    return true;
}

bool
KnobPath::isCellBracketDecorated(int /*row*/,
                                 int col) const
{
    if ( (col == 0) && _imp->isMultiPath && !_imp->isStringList ) {
        return true;
    }

    return false;
}

void
KnobPath::getPaths(std::list<std::string>* paths)
{
    std::list<std::vector<std::string> > table;

    getTable(&table);
    for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it) {
        if (_imp->isStringList) {
            assert(it->size() == 1);
            paths->push_back( (*it)[0] );
        } else {
            assert(it->size() == 2);
            paths->push_back( (*it)[1] );
        }
    }
}

void
KnobPath::prependPath(const std::string& path)
{
    if (!_imp->isMultiPath) {
        setValue(path);
    } else {
        std::list<std::vector<std::string> > paths;
        getTable(&paths);

        std::vector<std::string> row;
        if (_imp->isStringList) {
            row.push_back(path);
        } else {
            std::string name = generateUniquePathID(paths);
            row.push_back(name);
            row.push_back(path);
        }
        paths.push_front(row);
        setTable(paths);
    }
}

void
KnobPath::appendPath(const std::string& path)
{
    if (!_imp->isMultiPath) {
        setValue(path);
    } else {
        std::list<std::vector<std::string> > paths;
        getTable(&paths);
        for (std::list<std::vector<std::string> >::iterator it = paths.begin(); it != paths.end(); ++it) {
            if ( (_imp->isStringList && (*it)[0] == path) || (!_imp->isStringList && (*it)[1] == path) ) {
                return;
            }
        }
        std::vector<std::string> row;
        if (_imp->isStringList) {
            row.push_back(path);
        } else {
            std::string name = generateUniquePathID(paths);
            row.push_back(name);
            row.push_back(path);
        }
        paths.push_back(row);
        setTable(paths);
    }
}

int
KnobPath::getColumnsCount() const
{
    return _imp->isStringList ? 1 : 2;
}

bool
KnobPath::useEditButton() const
{
    return _imp->isMultiPath && !_imp->isStringList;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobFile.cpp"
