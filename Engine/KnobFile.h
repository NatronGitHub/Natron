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

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QString>

#include "Engine/Knob.h"

#include "Global/Macros.h"

class StringAnimationManager;
/******************************FILE_KNOB**************************************/

class File_Knob : public Knob
{
    
    Q_OBJECT
    
public:

    static Knob *BuildKnob(KnobHolder  *holder, const std::string &description, int dimension) {
        return new File_Knob(holder, description, dimension);
    }

    File_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    virtual ~File_Knob();

    static const std::string& typeNameStatic();
    
    void setAsInputImage() { _isInputImage = true; }
    
    bool isInputImageFile() const { return _isInputImage; }
    
    void setFiles(const QStringList& files);

    void setFiles(const std::map<int, QString>& fileSequence);
    
    void getFiles(QStringList* files);
    
    static void filesListToSequence(const QStringList& files,std::map<int, QString>* sequence);
    
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
    
    void open_file() { emit openFile(isAnimationEnabled()); }

    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    QString getRandomFrameName(int f, bool loadNearestIfNotFound) const;



    
signals:
    
    void openFile(bool);
    
private:
    
    virtual void onKeyFrameRemoved(int dimension, double time) OVERRIDE FINAL;
    
    virtual void onKeyframesRemoved(int dimension) OVERRIDE FINAL;
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
    virtual void cloneExtraData(const Knob &other) OVERRIDE FINAL;
    
    ///called by setValueAtTime
    virtual Natron::Status variantToKeyFrameValue(int time,const Variant& v,double* returnValue) OVERRIDE FINAL;
    
    ///called by getValueAtTime
    virtual void variantFromInterpolatedValue(double interpolated,Variant* returnValue) const OVERRIDE FINAL;
    
    virtual void loadExtraData(const QString& str) OVERRIDE FINAL;
    
    virtual QString saveExtraData() const OVERRIDE FINAL;


    int frameCount() const;
    
    /**
     * @brief nearestFrame
     * @return Returns the index of the nearest frame in the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to modify.
     */
    int nearestFrame(int f) const;


    static const std::string _typeNameStr;
    StringAnimationManager* _animation;
    int _isInputImage;
};

/******************************OUTPUT_FILE_KNOB**************************************/

class OutputFile_Knob: public Knob
{
    
    Q_OBJECT
public:

    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new OutputFile_Knob(holder, description, dimension);
    }

    OutputFile_Knob(KnobHolder *holder, const std::string &description, int dimension);

    std::string getFileName() const;

    void setFile(const QString& file);

    static const std::string& typeNameStatic();
    
    void setAsOutputImageFile() { _isOutputImage = true; }
    
    bool isOutputImageFile() const { return _isOutputImage; }
    
    void open_file() { emit openFile(_sequenceDialog && _isOutputImage); }
    
    void turnOffSequences() { _sequenceDialog = false; }
    
    bool isSequencesDialogEnabled() const { return _sequenceDialog; }
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
class Path_Knob: public Knob
{
    
    Q_OBJECT
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Path_Knob(holder, description, dimension);
    }
    
    Path_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
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
