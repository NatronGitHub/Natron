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

File_Knob::File_Knob(KnobHolder* holder, const std::string &description, int dimension)
: AnimatingString_KnobHelper(holder, description, dimension)
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


void File_Knob::setFiles(const std::vector<std::string>& files) {

    SequenceParsing::SequenceFromFiles sequence(false);
    for (U32 i = 0; i < files.size(); ++i) {
        sequence.tryInsertFile(SequenceParsing::FileNameContent(files.at(i)));
    }
    setFiles(sequence);
}

void File_Knob::setFilesInternal(const SequenceParsing::SequenceFromFiles& fileSequence) {
    KnobI::removeAnimation(0);
    if (!fileSequence.empty()) {
        if (isAnimationEnabled()) {
            const std::map<int, std::string>& filesMap  = fileSequence.getFrameIndexes();
            if (!filesMap.empty()) {
                for (std::map<int, std::string>::const_iterator it = filesMap.begin(); it!=filesMap.end(); ++it) {
                    setValueAtTime(it->first, it->second,0);
                }
            } else {
                ///the sequence has no indexes,if it has one file set a keyframe at time 0 for the single file
                if (fileSequence.isSingleFile()) {
                    setValueAtTime(0, fileSequence.getFilesList().at(0),0);
                }
            }
            
        }
    }

}

void File_Knob::setFiles(const SequenceParsing::SequenceFromFiles& fileSequence) {
    
    
    beginValueChange(Natron::PLUGIN_EDITED);
    setFilesInternal(fileSequence);
    _pattern = fileSequence.generateValidSequencePattern().c_str();
    
    ///necessary for the changedParam call!
    setValue(_pattern.toStdString(),0,true);
    endValueChange();
}

///called when a value changes, just update the pattern
void File_Knob::processNewValue(Natron::ValueChangedReason reason) {
    if (reason != Natron::TIME_CHANGED) {
        if (isSlave(0)) {
            std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(0);
            assert(master.second);
            boost::shared_ptr< Knob<std::string> > isString = boost::dynamic_pointer_cast<Knob<std::string > >(master.second);
            assert(isString);
            
            _pattern = isString->getValueForEachDimension()[master.first].c_str();
        } else {
            _pattern = dynamic_cast< Knob<std::string>* >(this)->getValueForEachDimension()[0].c_str();
        }
 
    }    
}

void File_Knob::cloneExtraData(const boost::shared_ptr<KnobI>& other)
{
    File_Knob* isFile = dynamic_cast<File_Knob*>(other.get());
    if (isFile) {
        _pattern = isFile->getPattern();
    }
    AnimatingString_KnobHelper::cloneExtraData(other);
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

std::string File_Knob::getValueAtTimeConditionally(int f, bool loadNearestIfNotFound) const
{
    if (!isAnimationEnabled()) {
        return getValue();
    } else {
        
        if (!loadNearestIfNotFound) {
            int ksIndex = getKeyFrameIndex(0, f);
            if (ksIndex == -1) {
                return "";
            }
        }
        if (getKeyFramesCount(0) == 0) {
            return "";
        } else {
            return getValueAtTime(f, 0);
        }
    }
}



void File_Knob::getFiles(SequenceParsing::SequenceFromFiles* files) {
    int kfCount = getKeyFramesCount(0);
    for (int i = 0; i < kfCount; ++i) {
        bool success;
        std::string v = getKeyFrameValueByIndex(0, i, &success);
        assert(success);
        files->tryInsertFile(SequenceParsing::FileNameContent(v.c_str()));
    }
}

void File_Knob::animationRemoved_virtual(int dimension) {
    AnimatingString_KnobHelper::animationRemoved_virtual(dimension);
    _pattern.clear();
}

/***********************************OUTPUT_FILE_KNOB*****************************************/

OutputFile_Knob::OutputFile_Knob(KnobHolder* holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
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

Path_Knob::Path_Knob(KnobHolder* holder, const std::string &description, int dimension)
: Knob(holder,description,dimension)
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

