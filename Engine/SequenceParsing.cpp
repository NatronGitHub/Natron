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

#include "SequenceParsing.h"


#include <stdexcept>
#include <QDebug>
#include <QStringList>
#include <QFile>
#include <QDir>

#include "Global/GlobalDefines.h"
#include "Global/QtCompat.h"

///the maximum number of non existing frame before Natron gives up trying to figure out a sequence layout.
#define NATRON_DIALOG_MAX_SEQUENCES_HOLE 1000

namespace  {


/**
 * @brief Given the pattern unpathed without extension (e.g: "filename###") and the file extension (e.g "jpg") ; this
 * functions extracts the common parts and the variables of the pattern ordered from left to right .
 * For example: file%04dname### and the jpg extension would return:
 * 3 common parts: "file","name",".jpg"
 * 2 variables: "%04d", "###"
 * The variables by order vector's second member is an int indicating how many non-variable (chars belonging to common parts) characters
 * were found before this variable.
 **/
static bool extractCommonPartsAndVariablesFromPattern(const QString& patternUnPathedWithoutExt,
                                                      const QString& patternExtension,
                                                      QStringList* commonParts,
                                                      std::vector<std::pair<QString,int> >* variablesByOrder) {
    int i = 0;
    bool inPrintfLikeArg = false;
    int printfLikeArgIndex = 0;
    QString commonPart;
    QString variable;
    int commonCharactersFound = 0;
    bool previousCharIsSharp = false;
    while (i < patternUnPathedWithoutExt.size()) {
        const QChar& c = patternUnPathedWithoutExt.at(i);
        if (c == QChar('#')) {
            if (!commonPart.isEmpty()) {
                commonParts->push_back(commonPart);
                commonCharactersFound += commonPart.size();
                commonPart.clear();
            }
            if (!previousCharIsSharp && !variable.isEmpty()) {
                variablesByOrder->push_back(std::make_pair(variable,commonCharactersFound));
                variable.clear();
            }
            variable.push_back(c);
            previousCharIsSharp = true;
        } else if (c == QChar('%')) {

            QChar next;
            if (i < patternUnPathedWithoutExt.size() - 1) {
                next = patternUnPathedWithoutExt.at(i + 1);
            }
            QChar prev;
            if (i > 0) {
                prev = patternUnPathedWithoutExt.at(i -1);
            }
            
            if (next.isNull()) {
                ///if we're at end, just consider the % character as any other
                commonPart.push_back(c);
            } else if (prev == QChar('%')) {
                ///we escaped the previous %, append this one to the text
                commonPart.push_back(c);
            } else if (next != QChar('%')) {
                ///if next == % then we have escaped the character
                ///we don't support nested  variables
                if (inPrintfLikeArg) {
                    return false;
                }
                printfLikeArgIndex = 0;
                inPrintfLikeArg = true;
                if (!commonPart.isEmpty()) {
                    commonParts->push_back(commonPart);
                    commonCharactersFound += commonPart.size();
                    commonPart.clear();
                }
                if (!variable.isEmpty()) {
                    variablesByOrder->push_back(std::make_pair(variable,commonCharactersFound));
                    variable.clear();
                }
                variable.push_back(c);
            }
        } else if ((c == QChar('d') || c == QChar('v') || c == QChar('V'))  && inPrintfLikeArg) {
            inPrintfLikeArg = false;
            assert(!variable.isEmpty());
            variable.push_back(c);
            variablesByOrder->push_back(std::make_pair(variable,commonCharactersFound));
            variable.clear();
        } else if (inPrintfLikeArg) {
            ++printfLikeArgIndex;
            assert(!variable.isEmpty());
            variable.push_back(c);
            ///if we're after a % character, and c is a letter different than d or v or V
            ///or c is digit different than 0, then we don't support this printf like style.
            if (c.isLetter() ||
                (printfLikeArgIndex == 1 && c != QChar('0'))) {
                commonParts->push_back(variable);
                commonCharactersFound += variable.size();
                variable.clear();
                inPrintfLikeArg = false;
            }
    
        } else {
            commonPart.push_back(c);
            if (!variable.isEmpty()) {
                variablesByOrder->push_back(std::make_pair(variable,commonCharactersFound));
                variable.clear();
            }
        }
        ++i;
    }

    if (!commonPart.isEmpty()) {
        commonParts->append(commonPart);
        commonCharactersFound += commonPart.size();
    }
    if (!variable.isEmpty()) {
        variablesByOrder->push_back(std::make_pair(variable,commonCharactersFound));
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
    } else if (variableToken == "%d") {
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
static bool matchesPattern(const QString& filename,const QStringList& commonPartsOrdered,
                           const std::vector<std::pair<QString,int> >& variablesOrdered,
                           int* frameNumber,int* viewNumber) {
    
    ///initialize the view number
    *viewNumber = -1;
    
    int lastPartPos = -1;
    for (int i = 0; i < commonPartsOrdered.size(); ++i) {
#pragma message WARN("This line will match common parts that could be longer,e.g: marleen would match marleenBG ")
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

    
    if (variablesOrdered.empty()) {
        return true;
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
    int commonCharactersFound = 0;
    bool previousCharIsDigit = false;
    bool wasFrameNumberSet = false;
    bool wasViewNumberSet = false;
    int variableChecked = 0;
 
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
                if (variablesOrdered[nextVariableIndex].second != commonCharactersFound) {
                    return false;
                }
                if (!checkVariable(variablesOrdered[nextVariableIndex].first, variable, 0, &fnumber)) {
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

                if (mid.startsWith("l",Qt::CaseInsensitive) && !startsWithLeft) {

                    ///don't be so harsh with just short views name because the letter
                    /// 'l' or 'r' might be found somewhere else in the filename, so if theres
                    ///no variable expected here just continue
                    
                    if (variablesOrdered[nextVariableIndex].first == QString("%v") &&
                        variablesOrdered[nextVariableIndex].second == commonCharactersFound) {
                        ///the view number doesn't correspond to a previous view variable
                        if (wasViewNumberSet && *viewNumber != 0) {
                            return false;
                        }
                        wasViewNumberSet = true;
                        *viewNumber = 0;
                        ++variableChecked;
                        ++nextVariableIndex;
                    } else {
                        ++commonCharactersFound;
                    }
                    ++i;
                } else if (mid.startsWith("r",Qt::CaseInsensitive) && !startsWithRight) {
                    ///don't be so harsh with just short views name because the letter
                    /// 'l' or 'r' might be found somewhere else in the filename, so if theres
                    ///no variable expected here just continue
                    if (variablesOrdered[nextVariableIndex].first == "%v"  &&
                        variablesOrdered[nextVariableIndex].second == commonCharactersFound) {
                        ///the view number doesn't correspond to a previous view variable
                        if (wasViewNumberSet &&  *viewNumber != 1) {
                            return false;
                        }
                        wasViewNumberSet = true;
                        *viewNumber = 1;
                        ++variableChecked;
                        ++nextVariableIndex;
                    } else {
                        ++commonCharactersFound;
                    }
                    ++i;
                } else if (startsWithLeft) {

                    int viewNo;

                    ///the pattern didn't expect a view name here
                    if (variablesOrdered[nextVariableIndex].second != commonCharactersFound) {
                        return false;
                    }
                    if (!checkVariable(variablesOrdered[nextVariableIndex].first, "left", 2, &viewNo)) {
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
                    if (variablesOrdered[nextVariableIndex].second != commonCharactersFound) {
                        return false;
                    }
                    if (!checkVariable(variablesOrdered[nextVariableIndex].first, "right", 2, &viewNo)) {
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

                        if (variablesOrdered[nextVariableIndex].second != commonCharactersFound) {
                            return false;
                        }
                        const QString& variableToken = variablesOrdered[nextVariableIndex].first;
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
                        commonCharactersFound += 4;
                        i += 4;
                    }
                } else {
                    ++commonCharactersFound;
                    ++i;
                }
                variable.clear();

            } else {
                ++commonCharactersFound;
                ++i;
            }
        }
    }
    if (variableChecked == (int)variablesOrdered.size()) {
        return true;
    } else {
        return false;
    }
}


}


namespace SequenceParsing {
/**
 * @brief A small structure representing an element of a file name.
 * It can be either a text part, or a view part or a frame number part.
 **/
struct FileNameElement {
    
    enum Type { TEXT = 0  , FRAME_NUMBER };
    
    FileNameElement(const QString& data,FileNameElement::Type type)
        : data(data)
        , type(type)
    {}
    
    QString data;
    Type type;
};


////////////////////FileNameContent//////////////////////////

struct FileNameContentPrivate {
    ///Ordered from left to right, these are the elements composing the filename without its path
    std::vector<FileNameElement> orderedElements;
    QString absoluteFileName;
    QString filePath; //< the filepath
    QString filename; //< the filename without path
    QString extension; //< the file extension
    bool hasSingleNumber;
    QString generatedPattern;
    
    FileNameContentPrivate()
        : orderedElements()
        , absoluteFileName()
        , filePath()
        , filename()
        , extension()
        , hasSingleNumber(false)
        , generatedPattern()
    {
    }
    
    void parse(const QString& absoluteFileName);
};


FileNameContent::FileNameContent(const QString& absoluteFilename)
    : _imp(new FileNameContentPrivate())
{
    _imp->parse(absoluteFilename);
}

FileNameContent::FileNameContent(const FileNameContent& other)
    : _imp(new FileNameContentPrivate())
{
    *this = other;
}

FileNameContent::~FileNameContent() {
    
}

void FileNameContent::operator=(const FileNameContent& other) {
    _imp->orderedElements = other._imp->orderedElements;
    _imp->absoluteFileName = other._imp->absoluteFileName;
    _imp->filename = other._imp->filename;
    _imp->filePath = other._imp->filePath;
    _imp->extension = other._imp->extension;
    _imp->hasSingleNumber = other._imp->hasSingleNumber;
    _imp->generatedPattern = other._imp->generatedPattern;
}

void FileNameContentPrivate::parse(const QString& absoluteFileName) {
    this->absoluteFileName = absoluteFileName;
    filename = absoluteFileName;
    filePath = removePath(filename);
    
    int i = 0;
    QString lastNumberStr;
    QString lastTextPart;
    while (i < filename.size()) {
        const QChar& c = filename.at(i);
        if (c.isDigit()) {
            lastNumberStr.push_back(c);
            if (!lastTextPart.isEmpty()) {
                orderedElements.push_back(FileNameElement(lastTextPart,FileNameElement::TEXT));
                lastTextPart.clear();
            }
        } else {
            if (!lastNumberStr.isEmpty()) {
                orderedElements.push_back(FileNameElement(lastNumberStr,FileNameElement::FRAME_NUMBER));
                if (!hasSingleNumber) {
                    hasSingleNumber = true;
                } else {
                    hasSingleNumber = false;
                }
                lastNumberStr.clear();
            }
            
            lastTextPart.push_back(c);
        }
        ++i;
    }
    
    if (!lastNumberStr.isEmpty()) {
        orderedElements.push_back(FileNameElement(lastNumberStr,FileNameElement::FRAME_NUMBER));
        if (!hasSingleNumber) {
            hasSingleNumber = true;
        } else {
            hasSingleNumber = false;
        }
        lastNumberStr.clear();
    }
    if (!lastTextPart.isEmpty()) {
        orderedElements.push_back(FileNameElement(lastTextPart,FileNameElement::TEXT));
        lastTextPart.clear();
    }
    
    
    int lastDotPos = filename.lastIndexOf('.');
    if (lastDotPos != -1) {
        int j = filename.size() - 1;
        while (j > 0 && filename.at(j) != QChar('.')) {
            extension.prepend(filename.at(j));
            --j;
        }
    }
    
    ///now build the generated pattern with the ordered elements.
    int numberIndex = 0;
    for (U32 j = 0; j < orderedElements.size(); ++j) {
        const FileNameElement& e = orderedElements[j];
        switch (e.type) {
        case FileNameElement::TEXT:
            generatedPattern.append(e.data);
            break;
        case FileNameElement::FRAME_NUMBER:
        {
            QString hashStr;
            int c = 0;
            while (c < e.data.size()) {
                hashStr.push_back('#');
                ++c;
            }
            generatedPattern.append(hashStr + QString::number(numberIndex));
            ++numberIndex;
        } break;
        default:
            break;
        }
    }
    
}

QStringList FileNameContent::getAllTextElements() const {
    QStringList ret;
    for (U32 i = 0; i < _imp->orderedElements.size(); ++i) {
        if (_imp->orderedElements[i].type == FileNameElement::TEXT) {
            ret.push_back(_imp->orderedElements[i].data);
        }
    }
    return ret;
}
    
/**
 * @brief Returns the file path, e.g: /Users/Lala/Pictures/ with the trailing separator.
 **/
const QString& FileNameContent::getPath() const {
    return _imp->filePath;
}

/**
 * @brief Returns the filename without its path.
 **/
const QString& FileNameContent::fileName() const {
    return _imp->filename;
}

/**
 * @brief Returns the absolute filename as it was given in the constructor arguments.
 **/
const QString& FileNameContent::absoluteFileName() const {
    return _imp->absoluteFileName;
}

const QString& FileNameContent::getExtension() const {
    return _imp->extension;
}

    
/**
 * @brief Returns true if a single number was found in the filename.
 **/
bool FileNameContent::hasSingleNumber() const {
    return _imp->hasSingleNumber;
}

/**
 * @brief Returns true if the filename is composed only of digits.
 **/
bool FileNameContent::isFileNameComposedOnlyOfDigits() const {
    if ((_imp->orderedElements.size() == 1 || _imp->orderedElements.size() == 2)
            && _imp->orderedElements[0].type == FileNameElement::FRAME_NUMBER) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Returns the file pattern found in the filename with hash characters style for frame number (i.e: ###)
 **/
const QString& FileNameContent::getFilePattern() const {
    return _imp->generatedPattern;
}

/**
 * @brief If the filename is composed of several numbers (e.g: file08_001.png),
 * this functions returns the number at index as a string that will be stored in numberString.
 * If Index is greater than the number of numbers in the filename or if this filename doesn't
 * contain any number, this function returns false.
 **/
bool FileNameContent::getNumberByIndex(int index,QString* numberString) const {
    
    int numbersElementsIndex = 0;
    for (U32 i = 0; i < _imp->orderedElements.size(); ++i) {
        if (_imp->orderedElements[i].type == FileNameElement::FRAME_NUMBER) {
            if (numbersElementsIndex == index) {
                *numberString = _imp->orderedElements[i].data;
                return true;
            }
            ++numbersElementsIndex;
        }
    }
    return false;
}

/**
 * @brief Given the pattern of this file, it tries to match the other file name to this
 * pattern.
 * @param numberIndexToVary [out] In case the pattern contains several numbers (@see getNumberByIndex)
 * this value will be fed the appropriate number index that should be used for frame number.
 * For example, if this filename is myfile001_000.jpg and the other file is myfile001_001.jpg
 * numberIndexToVary would be 1 as the frame number string indentied in that case is the last number.
 * @returns True if it identified 'other' as belonging to the same sequence, false otherwise.
 **/
bool FileNameContent::matchesPattern(const FileNameContent& other,std::vector<int>* numberIndexesToVary) const {
    const std::vector<FileNameElement>& otherElements = other._imp->orderedElements;
    if (otherElements.size() != _imp->orderedElements.size()) {
        return false;
    }
    
    ///potential frame numbers are pairs of strings from this filename and the same
    ///string in the other filename.
    ///Same numbers are not inserted in this vector.
    std::vector< std::pair< int, std::pair<QString,QString> > > potentialFrameNumbers;
    int numbersCount = 0;
    for (U32 i = 0; i < _imp->orderedElements.size(); ++i) {
        if (_imp->orderedElements[i].type != otherElements[i].type) {
            return false;
        }
        if (_imp->orderedElements[i].type == FileNameElement::FRAME_NUMBER) {
            if (_imp->orderedElements[i].data != otherElements[i].data) {
                ///if one frame number string is longer than the other, make sure it is because the represented number
                ///is bigger and not because there's extra padding
                /// For example 10000 couldve been produced with ## only and is valid, and 01 would also produce be ##.
                /// On the other hand 010000 could never have been produced with ## hence it is not valid.
                
                QString longest = _imp->orderedElements[i].data.size() > otherElements[i].data.size() ?
                            _imp->orderedElements[i].data : otherElements[i].data;
                int k = 0;
                bool notValid = false;
                while (k < longest.size() && k < std::abs(_imp->orderedElements[i].data.size() - otherElements[i].data.size())) {
                    if (longest.at(k) == QChar('0')) {
                        notValid = true;
                        break;
                    }
                    ++k;
                }
                
                ///if the number string of this element is shorter than the pattern that we're trying to match
                ///and it starts with 0's then it does not match the pattern.
                if (_imp->orderedElements[i].data.size() < otherElements[i].data.size() && _imp->orderedElements[i].data.at(0) == QChar('0')) {
                    notValid = true;
                }
                
                if (!notValid) {
                    potentialFrameNumbers.push_back(std::make_pair(numbersCount,
                                                                   std::make_pair(_imp->orderedElements[i].data, otherElements[i].data)));
                }
                
            }
            ++numbersCount;
        } else if (_imp->orderedElements[i].type == FileNameElement::TEXT && _imp->orderedElements[i].data != otherElements[i].data) {
            return false;
        }
    }
    ///strings are identical
    if (potentialFrameNumbers.empty()) {
        return false;
    }
    
    ///find out in the potentialFrameNumbers what is the minimum with pairs and pick it up
    /// for example if 1 pair is : < 0001, 802398 > and the other pair is < 01 , 10 > we pick
    /// the second one.
    std::vector<int> minIndexes;
    int minimum = INT_MAX;
    for (U32 i = 0; i < potentialFrameNumbers.size(); ++i) {
        int thisNumber = potentialFrameNumbers[i].second.first.toInt();
        int otherNumber = potentialFrameNumbers[i].second.second.toInt();
        int diff = std::abs(thisNumber - otherNumber);
        if (diff < minimum) {
            minimum = diff;
            minIndexes.clear();
            minIndexes.push_back(i);
        } else if (diff == minimum) {
            minIndexes.push_back(i);
        }
    }
    for (U32 i = 0; i < minIndexes.size(); ++i) {
        numberIndexesToVary->push_back(potentialFrameNumbers[minIndexes[i]].first);
    }
    return true;
    
}
    
bool FileNameContent::generatePatternWithFrameNumberAtIndexes(const std::vector<int>& indexes,QString* pattern) const {
    int numbersCount = 0;
    int lastNumberPos = 0;
    QString indexedPattern = getFilePattern();
    for (U32 i = 0; i < _imp->orderedElements.size(); ++i) {
        if (_imp->orderedElements[i].type == FileNameElement::FRAME_NUMBER) {
            lastNumberPos = indexedPattern.indexOf('#', lastNumberPos);
            assert(lastNumberPos != -1);

            int endTagPos = lastNumberPos;
            while (endTagPos < indexedPattern.size() && indexedPattern.at(endTagPos) == QChar('#')) {
                ++endTagPos;
            }
            
            ///assert that the end of the tag is composed of  a digit
            if (endTagPos < indexedPattern.size()) {
                assert(indexedPattern.at(endTagPos).isDigit());
            }
            
            bool isNumberAFrameNumber = false;
            for (U32 j = 0; j < indexes.size(); ++j) {
                if (indexes[j] == numbersCount) {
                    isNumberAFrameNumber = true;
                    break;
                }
            }
            if (!isNumberAFrameNumber) {
                ///if this is not the number we're interested in to keep the ###, just expand the variable
                ///replace the whole tag with the original data
                indexedPattern.replace(lastNumberPos, endTagPos - lastNumberPos + 1, _imp->orderedElements[i].data);
            } else {
                ///remove the index of the tag and keep the tag.
                if (endTagPos < indexedPattern.size()) {
                    indexedPattern.remove(endTagPos, 1);
                }
            }
            lastNumberPos = endTagPos;

            ++numbersCount;
        }
    }
    
    for (U32 i = 0; i < indexes.size(); ++i) {
        ///check that all index is valid.
        if (indexes[i] >= numbersCount) {
            return false;
        }
    }
    
    *pattern = getPath() + indexedPattern;
    return true;
}


QString removePath(QString& filename) {

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


bool filesListFromPattern(const QString& pattern,SequenceParsing::SequenceFromPattern* sequence) {
    if (pattern.isEmpty()) {
        return false;
    }
    
    QString patternUnPathed = pattern;
    QString patternPath = removePath(patternUnPathed);
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
    std::vector< std::pair<QString,int> > variablesByOrder;
    
    extractCommonPartsAndVariablesFromPattern(patternUnPathed, patternExtension, &commonPartsToFind, &variablesByOrder);


    ///all the interesting files of the pattern directory
    QStringList files = patternDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    for (int i = 0; i < files.size(); ++i) {
        int frameNumber;
        int viewNumber;
        if (matchesPattern(files.at(i), commonPartsToFind, variablesByOrder, &frameNumber, &viewNumber)) {
            SequenceFromPattern::iterator it = sequence->find(frameNumber);
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

QStringList sequenceFromPatternToFilesList(const SequenceParsing::SequenceFromPattern& sequence,int onlyViewIndex ) {
    QStringList ret;
    for (SequenceParsing::SequenceFromPattern::const_iterator it = sequence.begin(); it!=sequence.end(); ++it) {
        const std::map<int,QString>& views = it->second;
       
        for (std::map<int,QString>::const_iterator it2 = views.begin(); it2!=views.end(); ++it2) {
            if (onlyViewIndex != -1 && it2->first != onlyViewIndex && it2->first != -1) {
                continue;
            }
            ret.push_back(it2->second);
        }
    }
    return ret;
}

QString generateFileNameFromPattern(const QString& pattern,int frameNumber,int viewNumber) {
    QString patternUnPathed = pattern;
    QString patternPath = removePath(patternUnPathed);
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
    std::vector<std::pair<QString,int> > variablesByOrder;
    extractCommonPartsAndVariablesFromPattern(patternUnPathed, patternExtension, &commonPartsToFind, &variablesByOrder);

    QString output = pattern;
    int lastVariablePos = -1;
    for (U32 i = 0; i < variablesByOrder.size(); ++i) {
        const QString& variable = variablesByOrder[i].first;
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
        } else if (variable == "%d") {
            output.replace(lastVariablePos, variable.size(), QString::number(frameNumber));
        } else {
            throw std::invalid_argument("Unrecognized pattern: " + pattern.toStdString());
        }
    }
    return output;
}

struct SequenceFromFilesPrivate
{
    /// the parsed files that have matching content with respect to variables.
    std::vector < FileNameContent > sequence;
    
    ///a list with all the files in the sequence, with their absolute file names.
    QStringList filesList;
    
    ///all the files mapped to their index
    std::map<int,QString> filesMap;
    
    /// The index of the frame number string in case there're several numbers in a filename.
    std::vector<int> frameNumberStringIndexes;
    
    quint64 totalSize;
    
    bool sizeEstimationEnabled;
    
    SequenceFromFilesPrivate(bool enableSizeEstimation)
    : sequence()
    , filesList()
    , filesMap()
    , frameNumberStringIndexes()
    , totalSize(0)
    , sizeEstimationEnabled(enableSizeEstimation)
    {
        
    }
    
    bool isInSequence(int index) const {
        return filesMap.find(index) != filesMap.end();
    }
};
  
SequenceFromFiles::SequenceFromFiles(bool enableSizeEstimation)
    : _imp(new SequenceFromFilesPrivate(enableSizeEstimation))
{
        
}
    
SequenceFromFiles::SequenceFromFiles(const FileNameContent& firstFile,  bool enableSizeEstimation)
    : _imp(new SequenceFromFilesPrivate(enableSizeEstimation))
{
    _imp->sequence.push_back(firstFile);
    _imp->filesList << firstFile.absoluteFileName();
    if (enableSizeEstimation) {
        QFile f(firstFile.absoluteFileName());
        _imp->totalSize += f.size();
    }
}
    
SequenceFromFiles::~SequenceFromFiles() {
        
}
    
SequenceFromFiles::SequenceFromFiles(const SequenceFromFiles& other)
    : _imp(new SequenceFromFilesPrivate(false))
{
    *this = other;
}
    
void SequenceFromFiles::operator=(const SequenceFromFiles& other) const {
    _imp->sequence = other._imp->sequence;
    _imp->filesList = other._imp->filesList;
    _imp->filesMap = other._imp->filesMap;
    _imp->frameNumberStringIndexes = other._imp->frameNumberStringIndexes;
    _imp->totalSize = other._imp->totalSize;
    _imp->sizeEstimationEnabled = other._imp->sizeEstimationEnabled;
}
     
bool SequenceFromFiles::tryInsertFile(const FileNameContent& file) {
    
    if (_imp->filesList.isEmpty()) {
        _imp->sequence.push_back(file);
        _imp->filesList << file.absoluteFileName();
        if (_imp->sizeEstimationEnabled) {
            QFile f(file.absoluteFileName());
            _imp->totalSize += f.size();
        }
        return true;
    }
    
    if (file.getPath() != _imp->sequence[0].getPath()) {
        return false;
    }
    
    std::vector<int> frameNumberIndexes;
    bool insert = false;
    if (file.matchesPattern(_imp->sequence[0], &frameNumberIndexes)) {
        
        if (_imp->filesList.contains(file.absoluteFileName())) {
            return false;
        }
        
        if (_imp->frameNumberStringIndexes.empty()) {
            ///this is the second file we add to the sequence, we can now
            ///determine where is the frame number string placed.
            _imp->frameNumberStringIndexes = frameNumberIndexes;
            insert = true;
            
            ///insert the first frame number in the frameIndexes.
            QString firstFrameNumberStr;
            
            for (U32 i = 0; i < frameNumberIndexes.size(); ++i) {
                QString frameNumberStr;
                bool ok = _imp->sequence[0].getNumberByIndex(_imp->frameNumberStringIndexes[i], &frameNumberStr);
                if (ok && firstFrameNumberStr.isEmpty()) {
                    _imp->filesMap.insert(std::make_pair(frameNumberStr.toInt(),file.absoluteFileName()));
                    firstFrameNumberStr = frameNumberStr;
                } else if (!firstFrameNumberStr.isEmpty() && frameNumberStr.toInt() != firstFrameNumberStr.toInt()) {
                    return false;
                }
            }
            
            
        } else if(frameNumberIndexes == _imp->frameNumberStringIndexes) {
            insert = true;
        }
        if (insert) {
            
            QString firstFrameNumberStr;
            
            for (U32 i = 0; i < frameNumberIndexes.size(); ++i) {
                QString frameNumberStr;
                bool ok = file.getNumberByIndex(_imp->frameNumberStringIndexes[i], &frameNumberStr);
                if (ok && firstFrameNumberStr.isEmpty()) {
                    _imp->sequence.push_back(file);
                    _imp->filesList.push_back(file.absoluteFileName());
                    _imp->filesMap.insert(std::make_pair(frameNumberStr.toInt(),file.absoluteFileName()));
                    if (_imp->sizeEstimationEnabled) {
                        QFile f(file.absoluteFileName());
                        _imp->totalSize += f.size();
                    }

                    firstFrameNumberStr = frameNumberStr;
                } else if (!firstFrameNumberStr.isEmpty() && frameNumberStr.toInt() != firstFrameNumberStr.toInt()) {
                    return false;
                }
            }
        }
    }
    return insert;
}

bool SequenceFromFiles::contains(const QString& absoluteFileName) const {
    return _imp->filesList.contains(absoluteFileName);
}
  
bool SequenceFromFiles::empty() const {
    return _imp->filesList.isEmpty();
}
    
int SequenceFromFiles::count() const {
    return _imp->filesList.count();
}
    
bool SequenceFromFiles::isSingleFile() const {
    return _imp->sequence.size() == 1;
}
    
int SequenceFromFiles::getFirstFrame() const {
    if (_imp->filesMap.empty()) {
        return INT_MIN;
    } else {
        return _imp->filesMap.begin()->first;
    }
}
    
int SequenceFromFiles::getLastFrame() const {
    if (_imp->filesMap.empty()) {
        return INT_MAX;
    } else {
        std::map<int,QString>::const_iterator it = _imp->filesMap.end();
        --it;
        return it->first;
    }
}
    
const std::map<int,QString>& SequenceFromFiles::getFrameIndexes() const {
    return _imp->filesMap;
}
    
const QStringList& SequenceFromFiles::getFilesList() const {
    return _imp->filesList;
}
    
quint64 SequenceFromFiles::getEstimatedTotalSize() const {
    return _imp->totalSize;
}

QString SequenceFromFiles::generateValidSequencePattern() const
{
    if (empty()) {
        return "";
    }
    if (isSingleFile()) {
        return _imp->sequence[0].absoluteFileName();
    }
    assert(_imp->filesMap.size() >= 2);
    QString firstFramePattern ;
    _imp->sequence[0].generatePatternWithFrameNumberAtIndexes(_imp->frameNumberStringIndexes, &firstFramePattern);
    return firstFramePattern;
}
    
QString SequenceFromFiles::generateUserFriendlySequencePattern() const {
    if (isSingleFile()) {
        return _imp->sequence[0].absoluteFileName();
    }
    QString pattern = generateValidSequencePattern();
    removePath(pattern);
    
    std::vector< std::pair<int,int> > chunks;
    int first = getFirstFrame();
    while(first <= getLastFrame()){
        
        int breakCounter = 0;
        while (!(_imp->isInSequence(first)) && breakCounter < NATRON_DIALOG_MAX_SEQUENCES_HOLE) {
            ++first;
            ++breakCounter;
        }
        
        if (breakCounter >= NATRON_DIALOG_MAX_SEQUENCES_HOLE) {
            break;
        }
        
        chunks.push_back(std::make_pair(first, getLastFrame()));
        int next = first + 1;
        int prev = first;
        int count = 1;
        while((next <= getLastFrame())
              && _imp->isInSequence(next)
              && (next == prev + 1) ){
            prev = next;
            ++next;
            ++count;
        }
        --next;
        chunks.back().second = next;
        first += count;
    }
    
    if(chunks.size() == 1){
        pattern.append(QString(" %1-%2").arg(QString::number(chunks[0].first)).arg(QString::number(chunks[0].second)));
    }else{
        pattern.append(" ( ");
        for(unsigned int i = 0 ; i < chunks.size() ; ++i) {
            if(chunks[i].first != chunks[i].second){
                pattern.append(QString(" %1-%2").arg(QString::number(chunks[i].first)).arg(QString::number(chunks[i].second)));
            }else{
                pattern.append(QString(" %1").arg(QString::number(chunks[i].first)));
            }
            if(i < chunks.size() -1) pattern.append(" /");
        }
        pattern.append(" ) ");
    }
    return pattern;
}
    
QString SequenceFromFiles::fileExtension() const {
    if (!empty()) {
        return _imp->sequence[0].getExtension();
    } else {
        return "";
    }
}

QString SequenceFromFiles::getPath() const {
    if (!empty()) {
        return _imp->sequence[0].getPath();
    } else {
        return "";
    }
}
    
} // namespace SequenceParsing
