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

#ifndef NATRON_ENGINE_KNOBFILE_H_
#define NATRON_ENGINE_KNOBFILE_H_

#include <vector>
#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QString>

#include "Engine/KnobTypes.h"

#include "Global/Macros.h"

namespace SequenceParsing {
    class SequenceFromFiles;
}
/******************************FILE_KNOB**************************************/

class File_Knob : public QObject, public AnimatingString_KnobHelper
{
    
    Q_OBJECT
    
public:

    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new File_Knob(holder, description, dimension,declaredByPlugin);
    }

    File_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    virtual ~File_Knob();

    static const std::string& typeNameStatic();
    
    void setAsInputImage() { _isInputImage = true; }
    
    bool isInputImageFile() const { return _isInputImage; }
    
    /**
     * @brief firstFrame
     * @return Returns the index of the first frame in the sequence held by this Reader.
     */
    int firstFrame() const;
    
    /**
     * @brief lastFrame
     * @return Returns the index of the last frame in the sequence held by this Reader.
     */
    int lastFrame() const;
    
    void open_file() { emit openFile(); }

    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     */
    std::string getFileName(int time,int view) const;


signals:
    
    void openFile();
    
private:
    
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
            
    int frameCount() const;
    
    static const std::string _typeNameStr;
    int _isInputImage;
};

/******************************OUTPUT_FILE_KNOB**************************************/

class OutputFile_Knob:  public QObject, public Knob<std::string>
{
    
    Q_OBJECT
public:

    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new OutputFile_Knob(holder, description, dimension,declaredByPlugin);
    }

    OutputFile_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);

    static const std::string& typeNameStatic();
    
    void setAsOutputImageFile() { _isOutputImage = true; }
    
    bool isOutputImageFile() const { return _isOutputImage; }
    
    void open_file() { emit openFile(_sequenceDialog && _isOutputImage); }
    
    void turnOffSequences() { _sequenceDialog = false; }
    
    bool isSequencesDialogEnabled() const { return _sequenceDialog; }
    
    QString generateFileNameAtTime(SequenceTime time,int view = 0) const;
    
signals:
    
    void openFile(bool);
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;

    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
    bool _isOutputImage;
    bool _sequenceDialog;
};


/******************************PATH_KNOB**************************************/
class Path_Knob:  public QObject, public Knob<std::string>
{
    
    Q_OBJECT
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Path_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Path_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    static const std::string& typeNameStatic();
    
    void open_file() { emit openFile(); }
    
    void setMultiPath(bool b);
    
    bool isMultiPath() const;
    
signals:
    
    void openFile();
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
    bool _isMultiPath;
};

#endif // NATRON_ENGINE_KNOBFILE_H_
