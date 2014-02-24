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

#include "Engine/StringAnimationManager.h"
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
    const File_Knob& o = dynamic_cast<const File_Knob&>(other);
    _animation->clone(*(o._animation));
}


void File_Knob::loadExtraData(const QString& str) {
    _animation->load(str);
}

QString File_Knob::saveExtraData() const {
    return _animation->save();
}

void File_Knob::setFiles(const QStringList& files) {
    std::map<int, QString> seq;
    filesListToSequence(files, &seq);
    setFiles(seq);
}

void File_Knob::setFiles(const std::map<int, QString>& fileSequence) {
    if (fileSequence.empty()) {
        return;
    }
    beginValueChange(Natron::PLUGIN_EDITED);
    if (isAnimationEnabled()) {
        for (std::map<int, QString>::const_iterator it = fileSequence.begin(); it!=fileSequence.end(); ++it) {
            setValueAtTime<QString>(it->first, it->second);
        }
      
    }
    ///necessary for the changedParam call!
    setValue(Variant(fileSequence.begin()->second),0,true);
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
        return getValueAtTime(f, 0).toString();
    }
}

void File_Knob::filesListToSequence(const QStringList& files,std::map<int, QString>* sequence) {
    for (int i = 0; i < files.size(); ++i) {
        const QString& file = files.at(i);
        QString fileWithoutExtension = file;
        Natron::removeFileExtension(fileWithoutExtension);
        int time;
        int pos = fileWithoutExtension.size() -1;
        QString timeStr;
        while (pos >= 0 && fileWithoutExtension.at(pos).isDigit()) {
            timeStr.prepend(fileWithoutExtension.at(pos));
            --pos;
        }
        if (!timeStr.isEmpty()) {
            time = timeStr.toInt();
        } else {
            time = -1;
        }
        sequence->insert(std::make_pair(time, file));
    }
}


void File_Knob::getFiles(QStringList* files) {
    int kfCount = getKeyFramesCount(0);
    for (int i = 0; i < kfCount; ++i) {
        Variant v;
        bool success = getKeyFrameValueByIndex(0, i, &v);
        assert(success);
        files->push_back(v.toString());
    }
}

void File_Knob::onKeyframesRemoved(int /*dimension*/) {
    _animation->clearKeyFrames();
}

void File_Knob::onKeyFrameRemoved(int /*dimension*/, double time) {
    _animation->removeKeyFrame(time);
}

/***********************************OUTPUT_FILE_KNOB*****************************************/

OutputFile_Knob::OutputFile_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
, _isOutputImage(false)
, _sequenceDialog(true)
{
}

std::string OutputFile_Knob::getFileName() const
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

