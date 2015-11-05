/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_ENGINE_KNOBFILE_H
#define NATRON_ENGINE_KNOBFILE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
#include "Engine/EngineFwd.h"

namespace SequenceParsing {
class SequenceFromFiles;
}
/******************************KnobFile**************************************/

class KnobFile
    : public QObject, public AnimatingKnobStringHelper
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobFile(holder, description, dimension,declaredByPlugin);
    }

    KnobFile(KnobHolder* holder,
              const std::string &description,
              int dimension,
              bool declaredByPlugin);

    virtual ~KnobFile();

    static const std::string & typeNameStatic();

    void setAsInputImage()
    {
        _isInputImage = true;
    }

    bool isInputImageFile() const
    {
        return _isInputImage;
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

    void open_file()
    {
        Q_EMIT openFile();
    }

    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     */
    std::string getFileName(int time) const;

Q_SIGNALS:

    void openFile();

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

    int frameCount() const;

    static const std::string _typeNameStr;
    int _isInputImage;
};

/******************************KnobOutputFile**************************************/

class KnobOutputFile
    :  public QObject, public Knob<std::string>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobOutputFile(holder, description, dimension,declaredByPlugin);
    }

    KnobOutputFile(KnobHolder* holder,
                    const std::string &description,
                    int dimension,
                    bool declaredByPlugin);
    static const std::string & typeNameStatic();

    void setAsOutputImageFile()
    {
        _isOutputImage = true;
    }

    bool isOutputImageFile() const
    {
        return _isOutputImage;
    }

    void open_file()
    {
        Q_EMIT openFile(_sequenceDialog && _isOutputImage);
    }

    void turnOffSequences()
    {
        _sequenceDialog = false;
    }

    bool isSequencesDialogEnabled() const
    {
        return _sequenceDialog;
    }

    QString generateFileNameAtTime(SequenceTime time) const;

Q_SIGNALS:

    void openFile(bool);

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _isOutputImage;
    bool _sequenceDialog;
};


/******************************KnobPath**************************************/


/**
 * @brief A Path knob could also be called Environment_variable_Knob. 
 * The string is encoded the following way:
 * <Name>Lala</Name><Value>MyValue</Value>
 * Split all the ';' characters to get all different variables
 * then for each variable split the ':' to get the name and the value of the variable.
 **/
class KnobPath
    : public Knob<std::string>
{

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobPath(holder, description, dimension,declaredByPlugin);
    }

    KnobPath(KnobHolder* holder,
              const std::string &description,
              int dimension,
              bool declaredByPlugin);
    static const std::string & typeNameStatic();

    void setMultiPath(bool b);

    bool isMultiPath() const;

    void getPaths(std::list<std::string>* paths) const;
    
    ///Doesn't work if isMultiPath() == false
    void getVariables(std::list<std::pair<std::string,std::string> >* paths) const;
    
    static std::string encodeToMultiPathFormat(const std::list<std::pair<std::string,std::string> >& paths);
    
    void setPaths(const std::list<std::pair<std::string,std::string> >& paths);
    
    void prependPath(const std::string& path) ;
    void appendPath(const std::string& path) ;

    /*
     @brief same as setMultiPath except that there will be only variable names, no values
     */
    void setAsStringList(bool b);
    bool getIsStringList() const;
    
private:
    
    static std::string generateUniquePathID(const std::list<std::pair<std::string,std::string> >& paths);

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _isMultiPath;
    bool _isStringList;
};

#endif // NATRON_ENGINE_KNOBFILE_H
