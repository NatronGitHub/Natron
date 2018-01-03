/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/AppInstance.h"

#include <SequenceParsing.h> // for SequenceParsing::generateFileNameFromPattern

#include "Global/QtCompat.h"

#include "Serialization/KnobSerialization.h"


NATRON_NAMESPACE_ENTER


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
: KnobStringBase(holder, name, dimension)
, _imp(new KnobFilePrivate())
{
}

KnobFile::KnobFile(const KnobHolderPtr& holder,const KnobIPtr& mainInstance)
: KnobStringBase(holder, mainInstance)
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


static void
projectEnvVar_getProxy(const KnobHolderPtr& holder, std::string& str)
{
    if (!holder) {
        return;
    }
    AppInstancePtr app = holder->getApp();
    if (!app) {
        return;
    }
    ProjectPtr proj = app->getProject();
    if (!proj) {
        return;
    }
    proj->canonicalizePath(str);
}

static void
projectEnvVar_setProxy(const KnobHolderPtr& holder, std::string& str)
{
    if (!holder) {
        return;
    }
    AppInstancePtr app = holder->getApp();
    if (!app) {
        return;
    }
    ProjectPtr proj = app->getProject();
    if (!proj) {
        return;
    }
    proj->simplifyPath(str);
}

bool
FileKnobDimView::setValueAndCheckIfChanged(const std::string& value)
{
    std::string str = value;
    KnobIPtr knob;
    {
        QMutexLocker k(&valueMutex);
        knob = sharedKnobs.begin()->knob.lock();
    }

    KnobHolderPtr holder;
    if (knob) {
        holder = knob->getHolder();
    }
    if (holder && _expansionEnabled) {
        projectEnvVar_setProxy(holder,str);
    }
    return ValueKnobDimView<std::string>::setValueAndCheckIfChanged(str);
    
} // setValueAndCheckIfChanged

ValueChangedReturnCodeEnum
FileKnobDimView::setKeyFrame(const KeyFrame& key, SetKeyFrameFlags flags)
{
    KnobIPtr knob;
    {
        QMutexLocker k(&valueMutex);
        knob = sharedKnobs.begin()->knob.lock();
    }

    KnobHolderPtr holder;
    if (knob) {
        holder = knob->getHolder();
    }
    KeyFrame k = key;
    if (holder && _expansionEnabled) {
        std::string str;
        k.getPropertySafe<std::string>(kKeyFramePropString, 0, &str);
        projectEnvVar_setProxy(holder, str);
        k.setProperty(kKeyFramePropString, str, 0, false /*failIfnotExist*/);
    }

    return ValueKnobDimView<std::string>::setKeyFrame(k, flags);

} // setKeyFrame

KnobDimViewBasePtr
KnobFile::createDimViewData() const
{
    boost::shared_ptr<FileKnobDimView> ret(new FileKnobDimView);
    return ret;
}

std::string
KnobFile::getFileNameWithoutVariablesExpension(DimIdx dimension, ViewIdx view)
{
    return Knob<std::string>::getValue(dimension, view);
}

std::string
KnobFile::getRawFileName(DimIdx dimension, ViewIdx view)
{
    std::string ret = Knob<std::string>::getValue(dimension, view);
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
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

std::string
KnobFile::getDefaultValue(DimIdx dimension) const
{
    std::string ret = KnobStringBase::getDefaultValue(dimension);
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
}

std::string
KnobFile::getInitialDefaultValue(DimIdx dimension) const
{
    std::string ret = KnobStringBase::getInitialDefaultValue(dimension);
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
}

void
KnobFile::setDefaultValue(const std::string & v, DimSpec dimension)
{
    std::string ret = v;
    projectEnvVar_setProxy(getHolder(), ret);
    KnobStringBase::setDefaultValue(ret, dimension);
}

void
KnobFile::setDefaultValues(const std::vector<std::string>& values, DimIdx dimensionStartOffset)
{
    std::vector<std::string> ret = values;
    for (std::size_t i = 0; i < ret.size(); ++i) {
        projectEnvVar_setProxy(getHolder(), ret[i]);
    }
    KnobStringBase::setDefaultValues(ret, dimensionStartOffset);

}

void
KnobFile::setDefaultValueWithoutApplying(const std::string& v, DimSpec dimension)
{
    std::string ret = v;
    projectEnvVar_setProxy(getHolder(), ret);
    KnobStringBase::setDefaultValueWithoutApplying(ret, dimension);
}

void
KnobFile::setDefaultValuesWithoutApplying(const std::vector<std::string>& values, DimIdx dimensionStartOffset)
{
    std::vector<std::string> ret = values;
    for (std::size_t i = 0; i < ret.size(); ++i) {
        projectEnvVar_setProxy(getHolder(), ret[i]);
    }
    KnobStringBase::setDefaultValuesWithoutApplying(ret, dimensionStartOffset);
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
        projectEnvVar_getProxy(getHolder(), pattern);
        return SequenceParsing::generateFileNameFromPattern(pattern, views, time, view_i);
    }
    std::string ret = Knob<std::string>::getValue(dimension, view);
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
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

std::string
KnobPath::getFileNameWithoutVariablesExpension(DimIdx dimension, ViewIdx view)
{
    return KnobTable::getValue(dimension, view);
}

std::string
KnobPath::getValueAtTime(TimeValue time, DimIdx dimension, ViewIdx view, bool /*clampToMinMax*/)
{
    std::string ret = KnobTable::getValueAtTime(time, dimension, view);
    if (isMultiPath()) {
        return ret;
    }
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
}

std::string
KnobPath::getValue(DimIdx dimension, ViewIdx view, bool clampToMinMax)
{
    std::string ret = KnobTable::getValue(dimension, view, clampToMinMax);
    if (isMultiPath()) {
        return ret;
    }
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
}

std::string
KnobPath::getDefaultValue(DimIdx dimension) const
{
    std::string ret = KnobTable::getDefaultValue(dimension);
    if (isMultiPath()) {
        return ret;
    }
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
}

std::string
KnobPath::getInitialDefaultValue(DimIdx dimension) const
{
    std::string ret = KnobTable::getInitialDefaultValue(dimension);
    if (isMultiPath()) {
        return ret;
    }
    projectEnvVar_getProxy(getHolder(), ret);
    return ret;
}

void
KnobPath::setDefaultValue(const std::string & v, DimSpec dimension)
{
    std::string ret = v;
    if (!isMultiPath()) {
        projectEnvVar_setProxy(getHolder(), ret);
    }
    KnobTable::setDefaultValue(ret, dimension);
}

void
KnobPath::setDefaultValues(const std::vector<std::string>& values, DimIdx dimensionStartOffset)
{
    std::vector<std::string> ret = values;
    if (!isMultiPath()) {
        for (std::size_t i = 0; i < ret.size(); ++i) {
            projectEnvVar_setProxy(getHolder(), ret[i]);
        }
    }
    KnobTable::setDefaultValues(ret, dimensionStartOffset);

}

void
KnobPath::setDefaultValueWithoutApplying(const std::string& v, DimSpec dimension)
{
    std::string ret = v;
    if (!isMultiPath()) {
        projectEnvVar_setProxy(getHolder(), ret);
    }
    KnobTable::setDefaultValueWithoutApplying(ret, dimension);
}

void
KnobPath::setDefaultValuesWithoutApplying(const std::vector<std::string>& values, DimIdx dimensionStartOffset)
{
    std::vector<std::string> ret = values;
    if (!isMultiPath()) {
        for (std::size_t i = 0; i < ret.size(); ++i) {
            projectEnvVar_setProxy(getHolder(), ret[i]);
        }
    }
    KnobTable::setDefaultValuesWithoutApplying(ret, dimensionStartOffset);
}

KnobDimViewBasePtr
KnobPath::createDimViewData() const
{
    boost::shared_ptr<FileKnobDimView> ret(new FileKnobDimView);
    return ret;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobFile.cpp"
