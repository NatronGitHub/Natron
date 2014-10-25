//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

/**
* @brief Simple wrap for the Knob class that is the API we want to expose to the Python
* Engine module.
**/

#ifndef PARAMETERWRAPPER_H
#define PARAMETERWRAPPER_H

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

class Path_Knob;
class String_Knob;
class File_Knob;
class OutputFile_Knob;
class Button_Knob;
class Color_Knob;
class Int_Knob;
class Double_Knob;
class Bool_Knob;
class Choice_Knob;
class Group_Knob;
class RichText_Knob;
class Page_Knob;
class Parametric_Knob;
class KnobI;

class Param
{
    boost::shared_ptr<KnobI> _knob;
public:
    
    Param(const boost::shared_ptr<KnobI>& knob);
    
    virtual ~Param();
    
    /**
     * @brief Returns the number of dimensions that the parameter have.
     * For example a size integer parameter could have 2 dimensions: width and height.
     **/
    int getNumDimensions() const;
    
    /**
     * @brief Returns the name of the Param internally. This is the identifier that allows the Python programmer
     * to find and identify a specific Param.
     **/
    std::string getScriptName() const;
};

class IntParam : public Param
{
    boost::shared_ptr<Int_Knob> _intKnob;
public:
    
    IntParam(const boost::shared_ptr<Int_Knob>& knob);
    
    virtual ~IntParam();
    
    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    int getValue(int dimension = 0) const;
};

#endif // PARAMETERWRAPPER_H
