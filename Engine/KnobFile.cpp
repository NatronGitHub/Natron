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

#include "KnobFile.h"

#include <utility>
#include <stdexcept>

#include <QtCore/QStringList>
#include <QtCore/QMutexLocker>
#include <QDebug>

#include "Engine/Transform.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include <SequenceParsing.h>
#include "Global/QtCompat.h"

using namespace Natron;
using std::make_pair;
using std::pair;

/***********************************KnobFile*****************************************/

KnobFile::KnobFile(KnobHolder* holder,
                     const std::string &description,
                     int dimension,
                     bool declaredByPlugin)
    : AnimatingKnobStringHelper(holder, description, dimension,declaredByPlugin)
      , _isInputImage(false)
{
}

KnobFile::~KnobFile()
{
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

int
KnobFile::firstFrame() const
{
    double time;
    bool foundKF = getFirstKeyFrameTime(0, &time);

    return foundKF ? (int)time : INT_MIN;
}

int
KnobFile::lastFrame() const
{
    double time;
    bool foundKF = getLastKeyFrameTime(0, &time);

    return foundKF ? (int)time : INT_MAX;
}

int
KnobFile::frameCount() const
{
    return getKeyFramesCount(0);
}

std::string
KnobFile::getFileName(int time) const
{
    int view = getHolder() ? getHolder()->getCurrentView() : 0;
    
    if (!_isInputImage) {
        return getValue();
    } else {
        ///try to interpret the pattern and generate a filename if indexes are found
        return SequenceParsing::generateFileNameFromPattern(getValue(), time, view);
    }
}

/***********************************KnobOutputFile*****************************************/

KnobOutputFile::KnobOutputFile(KnobHolder* holder,
                                 const std::string &description,
                                 int dimension,
                                 bool declaredByPlugin)
    : Knob<std::string>(holder, description, dimension,declaredByPlugin)
      , _isOutputImage(false)
      , _sequenceDialog(true)
{
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
KnobOutputFile::generateFileNameAtTime(SequenceTime time) const
{
    int view = getHolder() ? getHolder()->getCurrentView() : 0;
    return SequenceParsing::generateFileNameFromPattern(getValue(0), time, view).c_str();
}

/***********************************KnobPath*****************************************/

KnobPath::KnobPath(KnobHolder* holder,
                     const std::string &description,
                     int dimension,
                     bool declaredByPlugin)
    : Knob<std::string>(holder,description,dimension,declaredByPlugin)
      , _isMultiPath(false)
{
}

const std::string KnobPath::_typeNameStr("Path");
const std::string &
KnobPath::typeNameStatic()
{
    return _typeNameStr;
}

bool
KnobPath::canAnimate() const
{
    return false;
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
KnobPath::getVariables(std::list<std::pair<std::string,std::string> >* paths) const
{
    if (!_isMultiPath) {
        return;
    }
    
    std::string startNameTag(NATRON_ENV_VAR_NAME_START_TAG);
    std::string endNameTag(NATRON_ENV_VAR_NAME_END_TAG);
    std::string startValueTag(NATRON_ENV_VAR_VALUE_START_TAG);
    std::string endValueTag(NATRON_ENV_VAR_VALUE_END_TAG);
    
    std::string raw = getValue().c_str();
    size_t i = raw.find(startNameTag);
    while (i != std::string::npos) {
        i += startNameTag.size();
        assert(i < raw.size());
        size_t endNamePos = raw.find(endNameTag,i);
        assert(endNamePos != std::string::npos && endNamePos < raw.size());
        if (endNamePos == std::string::npos || endNamePos >= raw.size()) {
            throw std::logic_error("KnobPath::getVariables()");
        }
        std::string name,value;
        while (i < endNamePos) {
            name.push_back(raw[i]);
            ++i;
        }
        
        i = raw.find(startValueTag,i);
        i += startValueTag.size();
        assert(i != std::string::npos && i < raw.size());
        
        size_t endValuePos = raw.find(endValueTag,i);
        assert(endValuePos != std::string::npos && endValuePos < raw.size());

        while (endValuePos != std::string::npos && endValuePos < raw.size() && i < endValuePos) {
            value.push_back(raw.at(i));
            ++i;
        }
        
        // In order to use XML tags, the text inside the tags has to be unescaped.
        paths->push_back(std::make_pair(name,Project::unescapeXML(value).c_str()));
        
        i = raw.find(startNameTag,i);
    }
}


void
KnobPath::getPaths(std::list<std::string> *paths) const
{
    std::string raw = getValue().c_str();
    
    if (_isMultiPath) {
        std::list<std::pair<std::string,std::string> > ret;
        getVariables(&ret);
        for (std::list<std::pair<std::string,std::string> >::iterator it = ret.begin(); it != ret.end(); ++it) {
            paths->push_back(it->second);
        }
    } else {
        paths->push_back(raw);
    }
    
}

void
KnobPath::setPaths(const std::list<std::pair<std::string,std::string> >& paths)
{
    if (!_isMultiPath) {
        return;
    }
    
    std::string path;
    

    for (std::list<std::pair<std::string,std::string> >::const_iterator it = paths.begin(); it != paths.end(); ++it) {
        // In order to use XML tags, the text inside the tags has to be escaped.
        path += NATRON_ENV_VAR_NAME_START_TAG;
        path += Project::escapeXML(it->first);
        path += NATRON_ENV_VAR_NAME_END_TAG;
        path += NATRON_ENV_VAR_VALUE_START_TAG;
        path += Project::escapeXML(it->second);
        path += NATRON_ENV_VAR_VALUE_END_TAG;
    }
    setValue(path, 0);
}

std::string
KnobPath::generateUniquePathID(const std::list<std::pair<std::string,std::string> >& paths)
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
        for (std::list<std::pair<std::string,std::string> >::const_iterator it = paths.begin(); it != paths.end(); ++it) {
            if (it->first == name) {
                found = true;
                break;
            }
        }
        ++idx;
    } while (found);
    return name;
}

void
KnobPath::prependPath(const std::string& path)
{
    if (!_isMultiPath) {
        setValue(path, 0);
    } else {
        std::list<std::pair<std::string,std::string> > paths;
        getVariables(&paths);
        std::string name = generateUniquePathID(paths);
        paths.push_front(std::make_pair(name, path));
        setPaths(paths);
    }
}

void
KnobPath::appendPath(const std::string& path)
{
    if (!_isMultiPath) {
        setValue(path, 0);
    } else {
        std::list<std::pair<std::string,std::string> > paths;
        getVariables(&paths);
        for (std::list<std::pair<std::string,std::string> >::iterator it = paths.begin(); it!=paths.end(); ++it) {
            if (it->second == path) {
                return;
            }
        }
        std::string name = generateUniquePathID(paths);
        paths.push_back(std::make_pair(name, path));
        setPaths(paths);
    }
}
