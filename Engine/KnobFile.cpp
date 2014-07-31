//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#include "KnobFile.h"

#include <utility>
#include <QtCore/QStringList>
#include <QtCore/QMutexLocker>
#include <QDebug>

#include "Engine/StringAnimationManager.h"
#include "Engine/KnobTypes.h"
#include <SequenceParsing.h>
#include "Global/QtCompat.h"

using namespace Natron;
using std::make_pair;
using std::pair;

/***********************************FILE_KNOB*****************************************/

File_Knob::File_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin)
: AnimatingString_KnobHelper(holder, description, dimension,declaredByPlugin)
, _isInputImage(false)
{
}

File_Knob::~File_Knob() {
}

bool File_Knob::canAnimate() const {
    return true;
}

const std::string File_Knob::_typeNameStr("InputFile");

const std::string& File_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& File_Knob::typeName() const
{
    return typeNameStatic();
}



int File_Knob::firstFrame() const
{
    double time;
    bool foundKF = getFirstKeyFrameTime(0, &time);
    return foundKF ? (int)time : INT_MIN;
}

int File_Knob::lastFrame() const
{
    double time;
    bool foundKF = getLastKeyFrameTime(0, &time);
    return foundKF ? (int)time : INT_MAX;
  
}

int File_Knob::frameCount() const {
    return getKeyFramesCount(0);
}

std::string File_Knob::getFileName(int time,int view) const
{
    if (!_isInputImage) {
        return getValue();
    } else {
        ///try to interpret the pattern and generate a filename if indexes are found
        return SequenceParsing::generateFileNameFromPattern(getValue(), time, view);
    }
}


/***********************************OUTPUT_FILE_KNOB*****************************************/

OutputFile_Knob::OutputFile_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin)
: Knob<std::string>(holder, description, dimension,declaredByPlugin)
, _isOutputImage(false)
, _sequenceDialog(true)
{
}


bool OutputFile_Knob::canAnimate() const {
    return false;
}

const std::string OutputFile_Knob::_typeNameStr("OutputFile");

const std::string& OutputFile_Knob::typeNameStatic()
{
    return _typeNameStr;
}

const std::string& OutputFile_Knob::typeName() const
{
    return typeNameStatic();
}

QString OutputFile_Knob::generateFileNameAtTime(SequenceTime time,int view) const {
    return SequenceParsing::generateFileNameFromPattern(getValue(0), time, view).c_str();
}

/***********************************PATH_KNOB*****************************************/

Path_Knob::Path_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin)
: Knob<std::string>(holder,description,dimension,declaredByPlugin)
, _isMultiPath(false)
{
    
}

const std::string Path_Knob::_typeNameStr("Path");

const std::string& Path_Knob::typeNameStatic() {
    return _typeNameStr;
}

bool Path_Knob::canAnimate() const {
    return false;
}

const std::string& Path_Knob::typeName() const {
    return typeNameStatic();
}

void Path_Knob::setMultiPath(bool b) {
    _isMultiPath = b;
}

bool Path_Knob::isMultiPath() const {
    return _isMultiPath;
}

