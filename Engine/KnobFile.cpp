/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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
#include <string>
#include <vector>

#include <QStringList>
#include <QMutexLocker>
#include <QDebug>

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

// Replaces the view variables of a file name: %V is the view name, %v its first letter in upper case, as
// SequenceParsing::generateFileNameFromPattern() does. Without a valid view the name is returned as is.
static std::string
replaceViewVariables(const std::string& name,
                     const std::vector<std::string>& views,
                     ViewIdx view)
{
    const int viewIndex = view.value();
    if ( (viewIndex < 0) || ( viewIndex >= (int)views.size() ) ) {
        return name;
    }
    const QString viewName = QString::fromUtf8( views[viewIndex].c_str() );
    QString result = QString::fromUtf8( name.c_str() );
    result.replace( QString::fromUtf8("%V"), viewName );
    result.replace( QString::fromUtf8("%v"), viewName.left(1).toUpper() );

    return result.toStdString();
}

// The file name of a file parameter at a time and view. The name of an image sequence is a pattern, whose
// frame number ('#', "%04d") and view ("%v", "%V") variables are replaced. The frame number variables are only
// those of the last component, the file name, as when a sequence is listed (FileSystemModel::filesListFromPattern()
// looks for the files in the directory as written): a '#' or "%02d" in a directory name is part of the name
// (e.g. "Frog #01/plate.exr" was read as "Frog 101/plate.exr", #848). The view variables are replaced in the
// directories too. A video file name is not a sequence pattern: only its view variables are replaced, and a '#'
// in it is part of the name (#693). The callers say whether the holder is a video reader or writer (for a Read or
// Write node, the ReadNode or WriteNode knows its embedded plug-in); the holder gives the project views.
static std::string
fileNameFromPattern(const std::string& pattern,
                    const KnobHolder* holder,
                    bool isVideo,
                    int time,
                    ViewSpec view)
{
    std::vector<std::string> views;

    if ( holder && holder->getApp() ) {
        views = holder->getApp()->getProject()->getProjectViewNames();
    }
    if ( !view.isViewIdx() ) {
        view = ViewIdx(0);
    }
    const ViewIdx viewIdx( view.value() );
    if (isVideo) {
        return replaceViewVariables(pattern, views, viewIdx);
    }
    // split as the sequence listing and OutputEffectInstance::createWriterPath() do
    std::string name = pattern;
    const std::string directory = SequenceParsing::removePath(name);

    return replaceViewVariables(directory, views, viewIdx) +
           SequenceParsing::generateFileNameFromPattern(name, views, time, viewIdx);
}

std::string
KnobFile::getFileName(int time,
                      ViewSpec view)
{
    if (!_isInputImage) {
        return getValue(0, view);
    } else {
        ///try to interpret the pattern and generate a filename if indexes are found
        const EffectInstance* effect = dynamic_cast<const EffectInstance*>( getHolder() );

        return fileNameFromPattern(getValue( 0, ViewIdx(0) ), getHolder(), effect && effect->isVideoReader(), time, view);
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
    const EffectInstance* effect = dynamic_cast<const EffectInstance*>( getHolder() );

    return QString::fromUtf8( fileNameFromPattern(getValue( 0, ViewIdx(0) ), getHolder(), effect && effect->isVideoWriter(), time, view).c_str() );
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
