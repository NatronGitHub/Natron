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

/**
 * @brief Removes from str any path it may have and returns the path.
 * The file doesn't have to necessarily exist on the disk.
 * For example: /Users/Lala/Pictures/mySequence001.jpg would transform 'filename' in
 * mySequence001.jpg and would return /Users/Lala/Pictures/
 **/
QString File_Knob::removePath(QString& filename) {
    
    ///find the last separator
    int pos = filename.lastIndexOf(QChar('/'));
    if (pos == -1) {
        //try out \\ char
        pos = filename.lastIndexOf(QChar('\\'));
    }
    if(pos == -1) {
        ///couldn't find a path
        return "";
    }
    QString path = filename.left(pos + 1); // + 1 to include the trailing separator
    filename = filename.remove(path);
    return path;
}

/**
 * @brief Given the pattern unpathed without extension (e.g: "filename###") and the file extension (e.g "jpg") ; this
 * functions extracts the common parts and the variables of the pattern ordered from left to right .
 * For example: file%04dname### and the jpg extension would return:
 * 3 common parts: "file","name",".jpg"
 * 2 variables: "%04d", "###"
 **/
static bool extractCommonPartsAndVariablesFromPattern(const QString& patternUnPathedWithoutExt,const QString& patternExtension,
                                                      QStringList* commonParts,QStringList* variablesByOrder) {
    int i = 0;
    bool skipNextChars = false;
    QString commonPart;
    QString variable;
    
    bool previousCharIsSharp = false;
    while (i < patternUnPathedWithoutExt.size()) {
        const QChar& c = patternUnPathedWithoutExt.at(i);
        if (c == QChar('#')) {
            if (!commonPart.isEmpty()) {
                commonParts->push_back(commonPart);
                commonPart.clear();
            }
            if (!previousCharIsSharp && !variable.isEmpty()) {
                variablesByOrder->push_back(variable);
                variable.clear();
            }
            variable.push_back(c);
            previousCharIsSharp = true;
        } else if (c == QChar('%')) {
            
            ///we don't support nested %x variables
            if (skipNextChars) {
                return false;
            }
            
            skipNextChars = true;
            if (!commonPart.isEmpty()) {
                commonParts->push_back(commonPart);
                commonPart.clear();
            }
            if (!variable.isEmpty()) {
                variablesByOrder->push_back(variable);
                variable.clear();
            }
            variable.push_back(c);
        } else if ((c == QChar('d') || c == QChar('v') || c == QChar('V'))  && skipNextChars) {
            skipNextChars = false;
            assert(!variable.isEmpty());
            variable.push_back(c);
            variablesByOrder->push_back(variable);
            variable.clear();
        } else if (skipNextChars) {
            assert(!variable.isEmpty());
            variable.push_back(c);
        } else {
            commonPart.push_back(c);
            if (!variable.isEmpty()) {
                variablesByOrder->push_back(variable);
                variable.clear();
            }
        }
        ++i;
    }
    
    if (!commonPart.isEmpty()) {
        commonParts->append(commonPart);
    }
    if (!variable.isEmpty()) {
        variablesByOrder->append(variable);
    }
    
    if (!patternExtension.isEmpty()) {
        commonParts->append(QString("." + patternExtension));
    }
    return true;
}

/**
 * @brief Given the variable token (e.g: %04d or #### or %v or %V, check if the variable and its type
 * coherent with the symbol. If so this function returns true, otherwise false.
 * @param frameNumberOrViewNumber [out] The frame number extracted from the variable, or the view number.
 * The view index is always 0 if there's only a single view.
 * If there'are 2 views, index 0 is considered to be the left view and index 1 is considered to be the right view.
 * If there'are multiple views, the index is corresponding to the view
 * Type is an integer indicating how is the content of the string supposed to be interpreted:
 * 0: a frame number (e.g: 0001)
 * 1: a short view number (e.g: 'l' or 'r')
 * 2: a long view number (e.g: 'left' or 'LEFT' or 'right' or 'RIGHT' or  'view0' or 'VIEW0')
 **/
static bool checkVariable(const QString& variableToken,const QString& variable,int type,int* frameNumberOrViewNumber) {
    if (variableToken == "%v") {
        if (type != 1) {
            return false;
        }
        if (variable == "r") {
            *frameNumberOrViewNumber = 1;
            return true;
        } else if (variable == "l") {
            *frameNumberOrViewNumber = 0;
            return true;
        } else if (variable.startsWith("view")) {
            QString viewNo = variable;
            viewNo = viewNo.remove("view",Qt::CaseInsensitive);
            *frameNumberOrViewNumber = viewNo.toInt();
            return true;
        } else {
            return false;
        }
        
    } else if (variableToken == "%V") {
        if (type != 2) {
            return false;
        }
        if (variable == "right") {
            *frameNumberOrViewNumber = 1;
            return true;
        } else if (variable == "left") {
            *frameNumberOrViewNumber = 0;
            return true;
        } else if (variable.startsWith("view")) {
            QString viewNo = variable;
            viewNo = viewNo.remove("view",Qt::CaseInsensitive);
            *frameNumberOrViewNumber = viewNo.toInt();
            return true;
        } else {
            return false;
        }
    } else if (variableToken.contains(QChar('#'))) {
        if (type != 0) {
            return false;
        } else if(variable.size() < variableToken.size()) {
            return false;
        }
        int prepending0s = 0;
        int i = 0;
        while (i < variable.size()) {
            if (variable.at(i) != QChar('0')) {
                break;
            } else {
                ++prepending0s;
            }
            ++i;
        }
        
        ///extra padding on numbers bigger than the hash chars count are not allowed.
        if (variable.size() > variableToken.size() && prepending0s > 0) {
            return false;
        }
        
        *frameNumberOrViewNumber = variable.toInt();
        return true;
    } else if (variableToken.startsWith("%0") && variableToken.endsWith(QChar('d'))) {
        if (type != 0) {
            return false;
        }
        QString extraPaddingCountStr = variableToken;
        extraPaddingCountStr = extraPaddingCountStr.remove("%0");
        extraPaddingCountStr = extraPaddingCountStr.remove(QChar('d'));
        int extraPaddingCount = extraPaddingCountStr.toInt();
        if (variable.size() < extraPaddingCount) {
            return false;
        }
        
        int prepending0s = 0;
        int i = 0;
        while (i < variable.size()) {
            if (variable.at(i) != QChar('0')) {
                break;
            } else {
                ++prepending0s;
            }
            ++i;
        }
        
        ///extra padding on numbers bigger than the hash chars count are not allowed.
        if (variable.size() > extraPaddingCount && prepending0s > 0) {
            return false;
        }
        
        *frameNumberOrViewNumber = variable.toInt();
        return true;
    } else {
        throw std::invalid_argument("Variable token unrecognized: " + variableToken.toStdString());
    }
}

/**
 * @brief Tries to match a given filename with the common parts and the variables of a pattern.
 * Note that if 2 variables have the exact same meaning (e.g: ### and %04d) and they do not correspond to the
 * same frame number it will reject the filename against the pattern.
 * @see extractCommonPartsAndVariablesFromPattern
 **/
static bool matchesPattern(const QString& filename,const QStringList& commonPartsOrdered,const QStringList& variablesOrdered,
                           int* frameNumber,int* viewNumber) {
    int lastPartPos = -1;
    for (int i = 0; i < commonPartsOrdered.size(); ++i) {
        int pos = filename.indexOf(commonPartsOrdered.at(i),lastPartPos == - 1 ? 0 : lastPartPos);
        ///couldn't find a common part
        if (pos == -1) {
            return false;
        }
        
        //the common parts order is not the same.
        if (pos <= lastPartPos) {
            return false;
        }
    }
    
    ///if we reach here that means the filename contains all the common parts. We can now expand the variables and start
    ///looking for a matching pattern
    
    ///we first extract all interesting informations from the filename by order
    ///i.e: we extract the digits and the view names.
    
    ///for each variable, a string representing the number (e.g: 0001 or LEFT/RIGHT or even VIEW0)
    ///corresponding either to a view number or a frame number
 
    
    int i = 0;
    QString variable;
    ///extracting digits
    bool previousCharIsDigit = false;
    bool wasFrameNumberSet = false;
    bool wasViewNumberSet = false;
    int variableChecked = 0;
    ///initialize the view number
    *viewNumber = -1;
    ///the index in variablesOrdered to check
    int nextVariableIndex = 0;
    
    while (i < filename.size()) {
        const QChar& c = filename.at(i);
        if (c.isDigit()) {
            
            assert((!previousCharIsDigit && variable.isEmpty()) || previousCharIsDigit);
            previousCharIsDigit = true;
            variable.push_back(c);
            ++i;
        } else {
            previousCharIsDigit = false;
            
            if (!variable.isEmpty()) {
                int fnumber;
                
                ///the pattern wasn't expecting a frame number or the digits count doesn't respect the
                ///hashes character count or the printf-like padding count is wrong.
                if (!checkVariable(variablesOrdered[nextVariableIndex], variable, 0, &fnumber)) {
                    return false;
                }
                
                ///a previous frame number variable had a different frame number
                if (wasFrameNumberSet && fnumber != *frameNumber) {
                    return false;
                }
                ++variableChecked;
                ++nextVariableIndex;
                wasFrameNumberSet = true;
                *frameNumber = fnumber;
                variable.clear();
            }
            
            ///these are characters that trigger a view name, start looking for a view name
            if(c.toLower() == QChar('l') || c.toLower() == QChar('r')  || c.toLower() == QChar('v')) {
                QString mid = filename.mid(i);
                bool startsWithLeft = mid.startsWith("left",Qt::CaseInsensitive);
                bool startsWithRight = mid.startsWith("right",Qt::CaseInsensitive);
                bool startsWithView = mid.startsWith("view",Qt::CaseInsensitive);
                
                QChar previousChar;
                if (i > 0) {
                    previousChar = filename.at(i-1);
                }
                QChar nextChar;
                if (i < filename.size() -1) {
                    nextChar = filename.at(i+1);
                }
                
                if (mid.startsWith("l",Qt::CaseInsensitive) && !startsWithLeft
                    && !previousChar.isNull() && !previousChar.isLetter() //< 'l' letter in the middle of filename is never going to be parsed
                    && !nextChar.isNull() && !nextChar.isLetter()) {    // correctly. Restrict it to things like ".l." or "_l."
                    
                    ///don't be so harsh with just short views name because the letter
                    /// 'l' or 'r' might be found somewhere else in the filename, so if theres
                    ///no variable expected here just continue
                    if (variablesOrdered[nextVariableIndex] == "%v") {
                        ///the view number doesn't correspond to a previous view variable
                        if (wasViewNumberSet && *viewNumber != 0) {
                            return false;
                        }
                        wasViewNumberSet = true;
                        *viewNumber = 0;
                        ++variableChecked;
                        ++nextVariableIndex;
                    }
                    ++i;
                } else if (mid.startsWith("r",Qt::CaseInsensitive) && !startsWithRight
                           && !previousChar.isNull() && !previousChar.isLetter()//< same as for 'l' letter
                           && !nextChar.isNull() && !nextChar.isLetter()) {
                    ///don't be so harsh with just short views name because the letter
                    /// 'l' or 'r' might be found somewhere else in the filename, so if theres
                    ///no variable expected here just continue
                    if (variablesOrdered[nextVariableIndex] == "%v") {
                        ///the view number doesn't correspond to a previous view variable
                        if (wasViewNumberSet &&  *viewNumber != 1) {
                            return false;
                        }
                        wasViewNumberSet = true;
                        *viewNumber = 1;
                        ++variableChecked;
                        ++nextVariableIndex;
                    }
                    ++i;
                } else if (startsWithLeft) {

                    int viewNo;
                    
                    ///the pattern didn't expect a view name here
                    if (!checkVariable(variablesOrdered[nextVariableIndex], "left", 2, &viewNo)) {
                        return false;
                    }
                    ///the view number doesn't correspond to a previous view variable
                    if (wasViewNumberSet && viewNo != *viewNumber) {
                        return false;
                    }
                    ++nextVariableIndex;
                    ++variableChecked;
                    wasViewNumberSet = true;
                    *viewNumber = viewNo;

                    i += 4;
                } else if (startsWithRight) {
                    
                    int viewNo;
                    
                    ///the pattern didn't expect a view name here
                    if (!checkVariable(variablesOrdered[nextVariableIndex], "right", 2, &viewNo)) {
                        return false;
                    }
                    ///the view number doesn't correspond to a previous view variable
                    if (wasViewNumberSet && viewNo != *viewNumber) {
                        return false;
                    }
                    ++variableChecked;
                    ++nextVariableIndex;
                    wasViewNumberSet = true;
                    *viewNumber = viewNo;
                    i += 5;
                } else if(startsWithView) {
                    ///extract the view number
                    variable.push_back("view");
                    QString viewNumberStr;
                    int j = 4;
                    while (j < mid.size() && mid.at(j).isDigit()) {
                        viewNumberStr.push_back(mid.at(j));
                        ++j;
                    }
                    if (!viewNumberStr.isEmpty()) {
                        variable.push_back(viewNumberStr);

                        int viewNo;
                        
                        const QString& variableToken = variablesOrdered[nextVariableIndex];
                        ///if the variableToken is %v just put a type of 1
                        ///otherwise a type of 2, this is because for either %v or %V we write view<N>
                        int type = variableToken == "%v" ? 1 : 2;
                        if (!checkVariable(variableToken, variable, type, &viewNo)) {
                            ///the pattern didn't expect a view name here
                            return false;
                        }
                        ///the view number doesn't correspond to a previous view variable
                        if (wasViewNumberSet && viewNo != *viewNumber) {
                            return false;
                        }
                        ++variableChecked;
                        ++nextVariableIndex;
                        wasViewNumberSet = true;
                        *viewNumber = viewNo;
                        
                        i += variable.size();
                    } else {
                        i += 4;
                    }
                } else {
                    ++i;
                }
                variable.clear();
                
            } else {
                ++i;
            }
        }
    }
    if (variableChecked == variablesOrdered.size()) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Given a pattern string, returns a string list with all the file names
 * that matches the mattern.
 * The rules of the pattern are:
 *
 * 1) Hashes character ('#') are counted as one digit. One digit account for extra
 * 0 padding that may exist prepending the actual number. Therefore
 * /Users/Lala/Pictures/mySequence###.jpg will accept the following filenames:
 * /Users/Lala/Pictures/mySequence001.jpg
 * /Users/Lala/Pictures/mySequence100.jpg
 * /Users/Lala/Pictures/mySequence1000.jpg
 * But not /Users/Lala/Pictures/mySequence01.jpg because it has less digits than the hashes characters specified.
 * Neither /Users/Lala/Pictures/mySequence01000.jpg because it has extra padding and more digits than hashes characters specified.
 *
 * 2) The same as hashes characters can be achieved by specifying the option %0<paddingCharactersCount>d
 * Ex: /Users/Lala/Pictures/mySequence###.jpg can be achieved the same way with /Users/Lala/Pictures/mySequence%03d.jpg
 *
 * 3) %v will be replaced automatically by 'l' or 'r' to search for a matching filename.
 *  For example /Users/Lala/Pictures/mySequence###_%v.jpg would accept:
 * /Users/Lala/Pictures/mySequence001_l.jpg and /Users/Lala/Pictures/mySequence001_r.jpg
 *
 * %V would achieve exactly the same but would be replaced by 'left' or 'right' instead.
 *
 **/
bool File_Knob::filesListFromPattern(const QString& pattern,File_Knob::FileSequence* sequence) {
    QString patternUnPathed = pattern;
    QString patternPath = File_Knob::removePath(patternUnPathed);
    QString patternExtension = Natron::removeFileExtension(patternUnPathed);
    
    ///the pattern has no extension, switch the extension and the unpathed part
    if (patternUnPathed.isEmpty()) {
        patternUnPathed = patternExtension;
        patternExtension.clear();
    }
    
    QDir patternDir(patternPath);
    if (!patternDir.exists()) {
        return false;
    }
    
    ///this list represents the common parts of the filename to find in a file in order for it to match the pattern.
    QStringList commonPartsToFind;
    ///this list represents the variables ( ###  %04d %v etc...) found in the pattern ordered from left to right in the
    ///original string.
    QStringList variablesByOrder;
    extractCommonPartsAndVariablesFromPattern(patternUnPathed, patternExtension, &commonPartsToFind, &variablesByOrder);
    
    
    ///all the interesting files of the pattern directory
    QStringList files = patternDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    
    for (int i = 0; i < files.size(); ++i) {
        int frameNumber;
        int viewNumber;
        if (matchesPattern(files.at(i), commonPartsToFind, variablesByOrder, &frameNumber, &viewNumber)) {
            FileSequence::iterator it = sequence->find(frameNumber);
            QString absoluteFileName = patternDir.absoluteFilePath(files.at(i));
            if (it != sequence->end()) {
                std::pair<std::map<int,QString>::iterator,bool> ret =
                it->second.insert(std::make_pair(viewNumber,absoluteFileName));
                if (!ret.second) {
                    qDebug() << "There was an issue populating the file sequence. Several files with the same frame number"
                    " have the same view index.";
                }
            } else {
                std::map<int, QString> viewsMap;
                viewsMap.insert(std::make_pair(viewNumber, absoluteFileName));
                sequence->insert(std::make_pair(frameNumber, viewsMap));
            }
        }
    }
    return true;
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

QString OutputFile_Knob::generateFileNameFromPattern(const QString& pattern,int frameNumber,int viewNumber) {
    QString patternUnPathed = pattern;
    QString patternPath = File_Knob::removePath(patternUnPathed);
    QString patternExtension = Natron::removeFileExtension(patternUnPathed);
    
    ///the pattern has no extension, switch the extension and the unpathed part
    if (patternUnPathed.isEmpty()) {
        patternUnPathed = patternExtension;
        patternExtension.clear();
    }
    ///this list represents the common parts of the filename to find in a file in order for it to match the pattern.
    QStringList commonPartsToFind;
    ///this list represents the variables ( ###  %04d %v etc...) found in the pattern ordered from left to right in the
    ///original string.
    QStringList variablesByOrder;
    extractCommonPartsAndVariablesFromPattern(patternUnPathed, patternExtension, &commonPartsToFind, &variablesByOrder);
    
    QString output = pattern;
    int lastVariablePos = -1;
    for (int i = 0; i < variablesByOrder.size(); ++i) {
        const QString& variable = variablesByOrder.at(i);
        lastVariablePos = output.indexOf(variable,lastVariablePos != -1 ? lastVariablePos : 0,Qt::CaseInsensitive);
        
        ///if we can't find the variable that means extractCommonPartsAndVariablesFromPattern is bugged.
        assert(lastVariablePos != -1);
        
        if (variable.contains(QChar('#'))) {
            QString frameNoStr = QString::number(frameNumber);
            ///prepend with extra 0's
            while (frameNoStr.size() < variable.size()) {
                frameNoStr.prepend(QChar('0'));
            }
            output.replace(lastVariablePos, variable.size(), frameNoStr);
        } else if (variable.contains("%v")) {
            QString viewNumberStr;
            if (viewNumber == 0) {
                viewNumberStr = "l";
            } else if (viewNumber == 1) {
                viewNumberStr = "r";
            } else {
                viewNumberStr = QString("view") + QString::number(viewNumber);
            }
            QChar previousChar;
            if (lastVariablePos > 0) {
                previousChar = output.at(lastVariablePos - 1);
            }
            QChar nextChar;
            if (lastVariablePos < output.size() - 2) {
                nextChar = output.at(lastVariablePos + 2);
            }
            if (previousChar.isLetterOrNumber()) {
                viewNumberStr.prepend('.');
            }
            if (nextChar.isLetterOrNumber()) {
                viewNumberStr.append('.');
            }
            
            output.replace(lastVariablePos,variable.size(), viewNumberStr);
        } else if (variable.contains("%V")) {
            QString viewNumberStr;
            if (viewNumber == 0) {
                viewNumberStr = "left";
            } else if (viewNumber == 1) {
                viewNumberStr = "right";
            } else {
                viewNumberStr = QString("view") + QString::number(viewNumber);
            }
            QChar previousChar;
            if (lastVariablePos > 0) {
                previousChar = output.at(lastVariablePos - 1);
            }
            QChar nextChar;
            if (lastVariablePos < output.size() - 2) {
                nextChar = output.at(lastVariablePos + 2);
            }
            if (previousChar.isLetterOrNumber()) {
                viewNumberStr.prepend('.');
            }
            if (nextChar.isLetterOrNumber()) {
                viewNumberStr.append('.');
            }

            output.replace(lastVariablePos, variable.size(), viewNumberStr);
        } else if(variable.startsWith("%0") && variable.endsWith('d')) {
            QString digitsCountStr = variable;
            digitsCountStr = digitsCountStr.remove("%0");
            digitsCountStr = digitsCountStr.remove('d');
            int digitsCount = digitsCountStr.toInt();
            QString frameNoStr = QString::number(frameNumber);
            //prepend with extra 0's
            while (frameNoStr.size() < digitsCount) {
                frameNoStr.prepend(QChar('0'));
            }
            output.replace(lastVariablePos, variable.size(), frameNoStr);
        } else {
            throw std::invalid_argument("Unrecognized pattern: " + pattern.toStdString());
        }
    }
    return output;
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
