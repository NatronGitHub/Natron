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

using namespace Natron;
using std::make_pair;
using std::pair;

/***********************************FILE_KNOB*****************************************/

File_Knob::File_Knob(KnobHolder *holder, const std::string &description, int dimension)
: Knob(holder, description, dimension)
, _isInputImage(false)
, _sequenceDialog(true)
{
}

bool File_Knob::canAnimate() const {
    return false;
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

void File_Knob::setFiles(const QStringList& files) {
    setValue(Variant(files), 0);
}

int File_Knob::firstFrame() const
{
    std::map<int, QString>::const_iterator it = _filesSequence.begin();
    if (it == _filesSequence.end()) {
        return INT_MIN;
    }
    return it->first;
}

int File_Knob::lastFrame() const
{
    if (_filesSequence.empty()) {
        return INT_MAX;
    }
    
    if (_filesSequence.size() == 1) {
        return _filesSequence.begin()->first;
    }
    
    std::map<int, QString>::const_iterator it = _filesSequence.end();
    --it;
    return it->first;
  
}

int File_Knob::frameCount() const {
    return _filesSequence.size();
}

int File_Knob::nearestFrame(int f) const
{
    int first = firstFrame();
    int last = lastFrame();
    if (f < first) {
        return first;
    }
    if (f > last) {
        return last;
    }

    std::map<int, int> distanceMap;
    for (std::map<int, QString>::const_iterator it = _filesSequence.begin(); it != _filesSequence.end(); ++it) {
        distanceMap.insert(make_pair(std::abs(f - it->first), it->first));
    }
    if (!distanceMap.empty()) {
        return distanceMap.begin()->second;
    } else {
        return 0;
    }
}

QString File_Knob::getRandomFrameName(int f, bool loadNearestIfNotFound) const
{
    ///when the sequence has juste 1 file (i.e: this is a video or maybe a single image) just return it.
    if(_filesSequence.size() == 1) {
        return _filesSequence.begin()->second;
    }
    
    if (loadNearestIfNotFound) {
        f = nearestFrame(f);
    }
    std::map<int, QString>::const_iterator it = _filesSequence.find(f);
    if (it != _filesSequence.end()) {
        return it->second;
    } else {
        return "";
    }
}

void File_Knob::cloneExtraData(const Knob &other)
{
    _filesSequence = dynamic_cast<const File_Knob &>(other)._filesSequence;
}

void File_Knob::processNewValue()
{
    _filesSequence.clear();
    QStringList fileNameList = getValue<QStringList>();
    bool first_time = true;
    QString originalName;
    for (int i = 0 ; i < fileNameList.size(); ++i) {
        QString fileName = fileNameList.at(i);
        QString unModifiedName = fileName;
        if (first_time) {
            int extensionPos = fileName.lastIndexOf('.');
            if (extensionPos != -1) {
                fileName = fileName.left(extensionPos);
            } else {
                continue;
            }
            int j = fileName.size() - 1;
            QString frameIndexStr;
            while (j > 0 && fileName.at(j).isDigit()) {
                frameIndexStr.push_front(fileName.at(j));
                --j;
            }
            if (j > 0) {
                int frameNumber = 0;
                if (fileNameList.size() > 1) {
                    frameNumber = frameIndexStr.toInt();
                }
                _filesSequence.insert(make_pair(frameNumber, unModifiedName));
                originalName = fileName.left(fileName.indexOf(frameIndexStr));

            } else {
                _filesSequence.insert(make_pair(0, unModifiedName));
            }
            first_time = false;
        } else {
            if (fileName.contains(originalName)) {
                int extensionPos = fileName.lastIndexOf('.');
                if (extensionPos != -1) {
                    fileName = fileName.left(extensionPos);
                } else {
                    continue;
                }
                int j = fileName.size() - 1;
                QString frameIndexStr;
                while (j > 0 && fileName.at(j).isDigit()) {
                    frameIndexStr.push_front(fileName.at(j));
                    --j;
                }
                if (j > 0) {
                    int number = frameIndexStr.toInt();
                    _filesSequence.insert(make_pair(number, unModifiedName));
                } else {
                    std::cout << " File_Knob : WARNING !! several frames in sequence but no frame count found in their name " << std::endl;
                }
            }
        }
    }
    emit frameRangeChanged(firstFrame(), lastFrame());

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



