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
#include "Engine/SequenceParsing.h"
#include "Global/QtCompat.h"

using namespace Natron;
using std::make_pair;
using std::pair;

/***********************************FILE_KNOB*****************************************/

File_Knob::File_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
, _animation(new StringAnimationManager(this))
{
}

File_Knob::~File_Knob() {
    delete _animation;
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


Natron::Status File_Knob::variantToKeyFrameValue(int time,const Variant& v,double* returnValue) {
    _animation->insertKeyFrame(time, v.toString(), returnValue);
    return StatOK;
}

void File_Knob::variantFromInterpolatedValue(double interpolated,Variant* returnValue) const {
    QString str;
    _animation->stringFromInterpolatedIndex(interpolated, &str);
    returnValue->setValue<QString>(str);
}

void File_Knob::cloneExtraData(const Knob& other) {
    if (other.typeName() == String_Knob::typeNameStatic()) {
        const String_Knob& o = dynamic_cast<const String_Knob&>(other);
        _animation->clone(o.getAnimation());
    } else if(other.typeName() == File_Knob::typeNameStatic()) {
        const File_Knob& o = dynamic_cast<const File_Knob&>(other);
        _animation->clone(o.getAnimation());
    }
}


void File_Knob::loadExtraData(const QString& str) {
    _animation->load(str);
    SequenceParsing::SequenceFromFiles sequence(false);
    getFiles(&sequence);
    _pattern = sequence.generateValidSequencePattern();
}

QString File_Knob::saveExtraData() const {
    return _animation->save();
}


void File_Knob::setFiles(const QStringList& files) {

    SequenceParsing::SequenceFromFiles sequence(false);
    for (int i = 0; i < files.size(); ++i) {
        sequence.tryInsertFile(SequenceParsing::FileNameContent(files.at(i)));
    }
    setFiles(sequence);
}

void File_Knob::setFilesInternal(const SequenceParsing::SequenceFromFiles& fileSequence) {
    removeAnimation(0);
    if (!fileSequence.empty()) {
        if (isAnimationEnabled()) {
            const std::map<int, QString>& filesMap  = fileSequence.getFrameIndexes();
            if (!filesMap.empty()) {
                for (std::map<int, QString>::const_iterator it = filesMap.begin(); it!=filesMap.end(); ++it) {
                    setValueAtTime<QString>(it->first, it->second);
                }
            } else {
                ///the sequence has no indexes,if it has one file set a keyframe at time 0 for the single file
                if (fileSequence.isSingleFile()) {
                    setValueAtTime<QString>(0, fileSequence.getFilesList().at(0));
                }
            }
            
        }
    }

}

void File_Knob::setFiles(const SequenceParsing::SequenceFromFiles& fileSequence) {
    
    
    beginValueChange(Natron::PLUGIN_EDITED);
    setFilesInternal(fileSequence);
    _pattern = fileSequence.generateValidSequencePattern();
    
    ///necessary for the changedParam call!
    setValue(Variant(_pattern),0,true);
    endValueChange();
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

int File_Knob::nearestFrame(int f) const
{
    double nearest;
    getNearestKeyFrameTime(0, f, &nearest);
    return (int)nearest;
}

QString File_Knob::getRandomFrameName(int f, bool loadNearestIfNotFound) const
{
    if (!isAnimationEnabled()) {
        return getValue().toString();
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
            return getValueAtTime(f, 0).toString();
        }
    }
}



void File_Knob::getFiles(SequenceParsing::SequenceFromFiles* files) {
    int kfCount = getKeyFramesCount(0);
    for (int i = 0; i < kfCount; ++i) {
        Variant v;
        bool success = getKeyFrameValueByIndex(0, i, &v);
        assert(success);
        files->tryInsertFile(SequenceParsing::FileNameContent(v.toString()));
    }
}

void File_Knob::animationRemoved_virtual(int /*dimension*/) {
    _animation->clearKeyFrames();
}

void File_Knob::keyframeRemoved_virtual(int /*dimension*/, double time) {
    _animation->removeKeyFrame(time);
}

bool File_Knob::isTypeCompatible(const Knob& other) const {
    if (other.typeName() == String_Knob::typeNameStatic() ||
        other.typeName() == OutputFile_Knob::typeNameStatic() ||
        other.typeName() == Path_Knob::typeNameStatic() ||
        other.typeName() == File_Knob::typeNameStatic()) {
        return true;
    } else {
        return false;
    }
}

/***********************************OUTPUT_FILE_KNOB*****************************************/

OutputFile_Knob::OutputFile_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
, _isOutputImage(false)
, _sequenceDialog(true)
{
}

std::string OutputFile_Knob::getPattern() const
{
    return  getValue<QString>().toStdString();
}

void OutputFile_Knob::setFile(const QString& file){
    setValue(Variant(file), 0);
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

bool OutputFile_Knob::isTypeCompatible(const Knob& other) const {
    if (other.typeName() == String_Knob::typeNameStatic() ||
        other.typeName() == OutputFile_Knob::typeNameStatic() ||
        other.typeName() == Path_Knob::typeNameStatic() ||
        other.typeName() == File_Knob::typeNameStatic()) {
        return true;
    } else {
        return false;
    }
}

QString OutputFile_Knob::generateFileNameAtTime(SequenceTime time,int view) const {
    return SequenceParsing::generateFileNameFromPattern(getValue<QString>(), time, view);
}

/***********************************PATH_KNOB*****************************************/

Path_Knob::Path_Knob(KnobHolder *holder, const std::string &description, int dimension)
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

bool Path_Knob::isTypeCompatible(const Knob& other) const {
    if (other.typeName() == String_Knob::typeNameStatic() ||
        other.typeName() == OutputFile_Knob::typeNameStatic() ||
        other.typeName() == Path_Knob::typeNameStatic() ||
        other.typeName() == File_Knob::typeNameStatic()) {
        return true;
    } else {
        return false;
    }
}



