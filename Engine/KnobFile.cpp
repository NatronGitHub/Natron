/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include "Engine/Project.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/AppInstance.h"

#include <SequenceParsing.h>
#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER

//using std::make_pair;
//using std::pair;

/***********************************KnobFile*****************************************/

KnobFile::KnobFile(KnobHolder* holder,
                   const std::string &description,
                   int dimension,
                   bool declaredByPlugin)
    : AnimatingKnobStringHelper(holder, description, dimension, declaredByPlugin)
    , _isInputImage(false)
{
}

KnobFile::~KnobFile()
{
}

void
KnobFile::reloadFile()
{
    assert( getHolder() );
    EffectInstance* effect = dynamic_cast<EffectInstance*>( getHolder() );
    if (effect) {
        effect->purgeCaches();
        effect->clearPersistentMessage(false);
    }
    evaluateValueChange(0, getCurrentTime(), ViewIdx(0), eValueChangedReasonNatronInternalEdited);
}

bool
KnobFile::canAnimate() const
{
    return true;
}

const std::string KnobFile::_typeNameStr("InputFile");
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
KnobFile::getFileName(int time,
                      ViewSpec view)
{
    if (!_isInputImage) {
        return getValue(0, view);
    } else {
        ///try to interpret the pattern and generate a filename if indexes are found
        std::vector<std::string> views;
        if ( getHolder() && getHolder()->getApp() ) {
            views = getHolder()->getApp()->getProject()->getProjectViewNames();
        }
        if ( !view.isViewIdx() ) {
            view = ViewIdx(0);
        }

        return SequenceParsing::generateFileNameFromPattern(getValue( 0, ViewIdx(0) ), views, time, view);
    }
}

/***********************************KnobOutputFile*****************************************/

KnobOutputFile::KnobOutputFile(KnobHolder* holder,
                               const std::string &description,
                               int dimension,
                               bool declaredByPlugin)
    : KnobStringBase(holder, description, dimension, declaredByPlugin)
    , _isOutputImage(false)
    , _sequenceDialog(true)
{
}


void
KnobOutputFile::rewriteFile()
{
    assert( getHolder() );
    EffectInstance* effect = dynamic_cast<EffectInstance*>( getHolder() );
    if (effect) {
        effect->purgeCaches();
        effect->clearPersistentMessage(false);
    }
    evaluateValueChange(0, getCurrentTime(), ViewIdx(0), eValueChangedReasonNatronInternalEdited);
}

bool
KnobOutputFile::canAnimate() const
{
    return false;
}

const std::string KnobOutputFile::_typeNameStr("OutputFile");
const std::string &
KnobOutputFile::typeNameStatic()
{
    return _typeNameStr;
}

const std::string &
KnobOutputFile::typeName() const
{
    return typeNameStatic();
}

QString
KnobOutputFile::generateFileNameAtTime(SequenceTime time,
                                       ViewSpec view)
{
    std::vector<std::string> views;

    if ( getHolder() && getHolder()->getApp() ) {
        views = getHolder()->getApp()->getProject()->getProjectViewNames();
    }
    if ( !view.isViewIdx() ) {
        view = ViewIdx(0);
    }

    return QString::fromUtf8( SequenceParsing::generateFileNameFromPattern(getValue( 0, ViewIdx(0) ), views, time, view).c_str() );
}

/***********************************KnobPath*****************************************/

KnobPath::KnobPath(KnobHolder* holder,
                   const std::string &description,
                   int dimension,
                   bool declaredByPlugin)
    : KnobTable(holder, description, dimension, declaredByPlugin)
    , _isMultiPath(false)
    , _isStringList(false)
{
}

const std::string KnobPath::_typeNameStr("Path");
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
    _isMultiPath = b;
}

bool
KnobPath::isMultiPath() const
{
    return _isMultiPath;
}

void
KnobPath::setAsStringList(bool b)
{
    setMultiPath(b);
    _isStringList = b;
}

bool
KnobPath::getIsStringList() const
{
    return _isStringList;
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
    if ( (col == 0) && _isMultiPath && !_isStringList ) {
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
        assert(it->size() == 2);
        paths->push_back( (*it)[1] );
    }
}

void
KnobPath::prependPath(const std::string& path)
{
    if (!_isMultiPath) {
        setValue(path);
    } else {
        std::list<std::vector<std::string> > paths;
        getTable(&paths);
        std::string name = generateUniquePathID(paths);
        std::vector<std::string> row(2);
        row[0] = name;
        row[1] = path;
        paths.push_front(row);
        setTable(paths);
    }
}

void
KnobPath::appendPath(const std::string& path)
{
    if (!_isMultiPath) {
        setValue(path);
    } else {
        std::list<std::vector<std::string> > paths;
        getTable(&paths);
        for (std::list<std::vector<std::string> >::iterator it = paths.begin(); it != paths.end(); ++it) {
            if ( (*it)[1] == path ) {
                return;
            }
        }
        std::string name = generateUniquePathID(paths);
        std::vector<std::string> row(2);
        row[0] = name;
        row[1] = path;
        paths.push_back(row);
        setTable(paths);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobFile.cpp"
