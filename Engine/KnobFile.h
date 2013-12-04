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
#include <QtCore/QObject>
#include <QtCore/QMutex>

#include "Engine/Knob.h"

#include "Global/Macros.h"

/******************************FILE_KNOB**************************************/

class File_Knob : public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder*  holder, const std::string& description,int dimension) {
        return new File_Knob(holder,description,dimension);
    }
    
    File_Knob(KnobHolder* holder, const std::string& description, int dimension)
    : Knob(holder,description,dimension)
    {}

private:
    virtual bool canAnimate() const OVERRIDE FINAL { return false; }
    
    virtual std::string typeName() const OVERRIDE FINAL {return "InputFile";}

public:
    void openFile(){
        emit shouldOpenFile();
    }

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
    
private:
    int frameCount() const{return _filesSequence.size();}
    
    /**
     * @brief nearestFrame
     * @return Returns the index of the nearest frame in the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to modify.
     */
    int nearestFrame(int f) const;
    
public:
    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    QString getRandomFrameName(int f,bool loadNearestIfNotFound) const;
private:
    virtual void cloneExtraData(const Knob& other) OVERRIDE FINAL;
    
    virtual void processNewValue() OVERRIDE FINAL;
    
signals:
    void shouldOpenFile();
    
private:
    mutable QMutex _fileSequenceLock;
    std::map<int,QString> _filesSequence;///mapping <frameNumber,fileName>
};

/******************************OUTPUT_FILE_KNOB**************************************/

class OutputFile_Knob:public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobHolder* holder, const std::string& description,int dimension){
        return new OutputFile_Knob(holder,description,dimension);
    }
    
    OutputFile_Knob(KnobHolder* holder, const std::string& description,int dimension):
        Knob(holder,description,dimension)
    {}

    std::string getFileName() const;

    void openFile(){
        emit shouldOpenFile();
    }
    
    
signals:
    
    void shouldOpenFile();

private:

    virtual bool canAnimate() const OVERRIDE FINAL { return false; }

    virtual std::string typeName() const OVERRIDE FINAL {return "OutputFile";}
};


#endif // NATRON_ENGINE_KNOBFILE_H_
