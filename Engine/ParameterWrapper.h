/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

/**
* @brief Simple wrap for the Knob class that is the API we want to expose to the Python
* Engine module.
**/

#ifndef PARAMETERWRAPPER_H
#define PARAMETERWRAPPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/EngineFwd.h"

class Param
{
    
protected:
    boost::weak_ptr<KnobI> _knob;
    
public:
    
    Param(const boost::shared_ptr<KnobI>& knob);
    
    virtual ~Param();
    
    boost::shared_ptr<KnobI> getInternalKnob() const { return _knob.lock(); }
    
    /**
     * @brief Returns the parent of this parameter if any. If the parameter has no parent it is assumed to be a top-level
     * param of the effect.
     **/
    Param* getParent() const;
    
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
    
    /**
     * @brief Returns the label of the Param as shown on the GUI.
     **/
    std::string getLabel() const;
    
    /**
     * @brief Returns the type of the parameter. The list of known type is:
     * "Int" "Bool" "Double" "Button" "Choice" "Color" "String" "Group" "Page" "Parametric" "InputFile" "OutputFile" "Path"
     **/
    std::string getTypeName() const;
    
    /**
     * @brief Returns the help that describes the parameter, its effect, etc...
     **/
    std::string getHelp() const;
    void setHelp(const std::string& help);
    
    /**
     * @brief Returns true if this parameter is visible in the user interface, false if it is hidden.
     **/
    bool getIsVisible() const;
    
    /**
     * @brief Set the visibility of the parameter
     **/
    void setVisible(bool visible);
    
    void setVisibleByDefault(bool visible);
    
    /**
     * @brief Returns whether the given dimension is enabled, i.e: whether the user can interact with it or not.
     **/
    bool getIsEnabled(int dimension = 0) const;
    
    /**
     * @brief Set the given dimension enabledness
     **/
    void setEnabled(bool enabled,int dimension = 0);
    
    void setEnabledByDefault(bool enabled = 0);
    
    /**
     * @brief Returns whether this parameter is persistant or not. A persistant parameter will be saved into the project.
     **/
    bool getIsPersistant() const;
    
    /**
     * @brief Set the parameter persistancy.
     **/
    void setPersistant(bool persistant);
    
    /**
     * @brief Returns whether the parameter forces a new evaluation when its value changes. An evaluation is typically 
     * a request for a re-render of the current frame.
     **/
    bool getEvaluateOnChange() const;
    
    /**
     * @brief Set whether this Param evaluates on change.
     **/
    void setEvaluateOnChange(bool eval);
    
    /**
     * @brief Returns whether the parameter type can animate or not. For example a button parameter cannot animate.
     * This is "static" (per parameter type property) and does not mean the same thing that getIsAnimationEnabled().
     * A parameter can have the function getCanAnimate() return true but getIsAnimationEnabled() return false.
     **/
    bool getCanAnimate() const;
    
    /**
     * @brief Returns whether this parameter can animate.
     **/
    bool getIsAnimationEnabled() const;
    void setAnimationEnabled(bool e);
    
    /**
     * @brief Controls whether the next parameter defined after this parameter should add a new line or not.
     **/
    bool getAddNewLine();
    void setAddNewLine(bool a);

    /**
     * @brief Copies all the content of the other param: animation, expression and value.
     * The parameters must be compatible types. E.g: you cannot copy
     * a StringParam to an IntParam but you can convert a Doubleparam to an IntParam.
     **/
    bool copy(Param* other, int dimension = -1);
    
    /**
     * @brief Returns a pseudo-random number. This will always be the same for the same time on the timeline.
     * The version with the seed can be used to retrieve the same value for 2 successive randoms
     **/
    double random(double min = 0., double max = 1.) const;
    double random(unsigned int seed) const;
    
    int randomInt(int min, int max);
    int randomInt(unsigned int seed) const;
    
    /**
     * @brief Returns the raw value of the curve at the given dimension at the given time. Note that
     * if there's an expression this will not return the value of the expression but the value of the animation curve
     * if there is any.
     **/
    double curve(double time, int dimension = 0) const;
    
    bool setAsAlias(Param* other);
    
protected:
    
    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    void _addAsDependencyOf(int fromExprDimension,Param* param, int thisDimension);

};


class AnimatedParam : public Param
{
    
public:
    
    AnimatedParam(const boost::shared_ptr<KnobI>& knob);
    
    virtual ~AnimatedParam();

    /**
     * @brief Returns whether the given dimension has animation or not. A dimension is considered to have an animation when it has
     * at least 1 keyframe.
     **/
    bool getIsAnimated(int dimension = 0) const;
    
    /**
     * @brief Returns the number of keyframes for the given dimension.
     **/
    int getNumKeys(int dimension = 0) const;
    
    /**
     * @brief Returns the index of the keyframe at the given time for the given dimension.
     * Returns -1 if no keyframe could be found at the given time.
     **/
    int getKeyIndex(double time,int dimension = 0) const;
    
    /**
     * @brief Set in 'time' the time of the keyframe at the given index for the given dimension.
     * This function returns true if a keyframe was found at the given index, false otherwise.
     **/
    bool getKeyTime(int index,int dimension,double* time) const;
    
    /**
     * @brief Removes the keyframe at the given time and dimension if it matches any.
     **/
    void deleteValueAtTime(double time,int dimension = 0);
    
    /**
     * @brief Removes all animation for the given dimension.
     **/
    void removeAnimation(int dimension = 0);
    
    /**
     * @brief Compute the derivative at time as a double
     **/
    double getDerivativeAtTime(double time, int dimension = 0) const;
    
    /**
     * @brief Compute the integral of dimension from time1 to time2 as a double
     **/
    double getIntegrateFromTimeToTime(double time1, double time2, int dimension = 0) const;
    
    /**
     * @brief Get the current global time in the parameter context. Most generally this is the time on the timeline,
     * but when multiple frames are being rendered at different times, this is the time of the frame being rendered
     * by the caller thread.
     **/
    int getCurrentTime() const;
    
    /**
     * @brief Set an expression on the Param. This is a Python script, see documentation for more infos.
     **/
    bool setExpression(const std::string& expr,bool hasRetVariable,int dimension = 0);
    std::string getExpression(int dimension,bool* hasRetVariable) const;
};

/**
 * @brief Small helper struct that is returned by the get() function of all params type
 * so the user can write stuff like myParam.get().x
 **/
struct Int2DTuple
{
    int x,y;
};
struct Int3DTuple
{
    int x,y,z;
};
struct Double2DTuple
{
    double x,y;
};
struct Double3DTuple
{
    double x,y,z;
};
struct ColorTuple
{
    double r,g,b,a; //< Color params are 4-dimensional
};

class IntParam : public AnimatedParam
{
    
protected:
    boost::weak_ptr<KnobInt> _intKnob;
public:
    
    IntParam(const boost::shared_ptr<KnobInt>& knob);
    
    virtual ~IntParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    int get() const;
    int get(double frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(int x);
    void set(int x, double frame);
    
    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    int getValue(int dimension = 0) const;
    
    /**
     * @brief Set the value held by the parameter. If it is animated 
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(int value,int dimension = 0);
    
    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the 
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    int getValueAtTime(double time,int dimension = 0) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(int value,double time,int dimension = 0);
    
    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(int value,int dimension = 0);
    
    /**
     * @brief Return the default value for the given dimension
     **/
    int getDefaultValue(int dimension = 0) const;
    
    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(int dimension = 0);
    
    /**
     * @brief Set the minimum possible value for the given dimension. The minimum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values smaller than the minimum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMinimum(int minimum,int dimension = 0);
    
    /**
     * @brief Get the minimum for the given dimension.
     **/
    int getMinimum(int dimension = 0) const;
    
    /**
     * @brief Set the maximum possible value for the given dimension. The maximum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values greater than the maximum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMaximum(int maximum,int dimension = 0);
    
    /**
     * @brief Get the minimum for the given dimension.
     **/
    int getMaximum(int dimension = 0) const;
    
    /**
     * @brief Set the minimum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMinimum(int minimum,int dimension = 0);
    
    /**
     * @brief Get the display minimum for the given dimension
     **/
    int getDisplayMinimum(int dimension) const;
    
    /**
     * @brief Set the maximum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMaximum(int maximum,int dimension = 0);

    /**
     * @brief Get the display maximum for the given dimension
     **/
    int getDisplayMaximum(int dimension) const;

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    int addAsDependencyOf(int fromExprDimension,Param* param,int thisDimension);

};

class Int2DParam : public IntParam
{
public:
    
    Int2DParam(const boost::shared_ptr<KnobInt>& knob) : IntParam(knob) {}
    
    virtual ~Int2DParam() {}
    
    Int2DTuple get() const;
    Int2DTuple get(double frame) const;
    void set(int x, int y);
    void set(int x, int y, double frame);
};

class Int3DParam : public Int2DParam
{
public:
    
    Int3DParam(const boost::shared_ptr<KnobInt>& knob) : Int2DParam(knob) {}
    
    virtual ~Int3DParam() {}
    
    Int3DTuple get() const;
    Int3DTuple get(double frame) const;
    void set(int x, int y, int z);
    void set(int x, int y, int z, double frame);
};



class DoubleParam : public AnimatedParam
{
    
protected:
    boost::weak_ptr<KnobDouble> _doubleKnob;
public:
    
    DoubleParam(const boost::shared_ptr<KnobDouble>& knob);
    
    virtual ~DoubleParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    double get() const;
    double get(double frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(double x);
    void set(double x, double frame);
    
    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    double getValue(int dimension = 0) const;
    
    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(double value,int dimension = 0);
    
    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    double getValueAtTime(double time,int dimension = 0) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(double value,double time,int dimension = 0);
    
    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(double value,int dimension = 0);
    
    /**
     * @brief Return the default value for the given dimension
     **/
    double getDefaultValue(int dimension = 0) const;
    
    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(int dimension = 0);
    
    /**
     * @brief Set the minimum possible value for the given dimension. The minimum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values smaller than the minimum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMinimum(double minimum,int dimension = 0);
    
    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMinimum(int dimension = 0) const;
    
    /**
     * @brief Set the maximum possible value for the given dimension. The maximum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values greater than the maximum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMaximum(double maximum,int dimension = 0);
    
    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMaximum(int dimension = 0) const;
    
    /**
     * @brief Set the minimum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMinimum(double minimum,int dimension = 0);
    
    /**
     * @brief Get the display minimum for the given dimension
     **/
    double getDisplayMinimum(int dimension) const;
    
    /**
     * @brief Set the maximum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMaximum(double maximum,int dimension = 0);
    
    /**
     * @brief Get the display maximum for the given dimension
     **/
    double getDisplayMaximum(int dimension) const;
    
    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    double addAsDependencyOf(int fromExprDimension,Param* param,int thisDimension);

};

class Double2DParam : public DoubleParam
{
public:
    
    Double2DParam(const boost::shared_ptr<KnobDouble>& knob) : DoubleParam(knob) {}
    
    virtual ~Double2DParam() {}
    
    Double2DTuple get() const;
    Double2DTuple get(double frame) const;
    void set(double x, double y);
    void set(double x, double y, double frame);
    
    void setUsePointInteract(bool use);
};

class Double3DParam : public Double2DParam
{
public:
    
    Double3DParam(const boost::shared_ptr<KnobDouble>& knob) : Double2DParam(knob) {}
    
    virtual ~Double3DParam() {}
    
    Double3DTuple get() const;
    Double3DTuple get(double frame) const;
    void set(double x, double y, double z);
    void set(double x, double y, double z, double frame);
};


class ColorParam : public AnimatedParam
{
    
protected:
    boost::weak_ptr<KnobColor> _colorKnob;
public:
    
    ColorParam(const boost::shared_ptr<KnobColor>& knob);
    
    virtual ~ColorParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    ColorTuple get() const;
    ColorTuple get(double frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(double r, double g, double b, double a);
    void set(double r, double g, double b, double a, double frame);

    
    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    double getValue(int dimension = 0) const;
    
    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(double value,int dimension = 0);
    
    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    double getValueAtTime(double time,int dimension = 0) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(double value,double time,int dimension = 0);
    
    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(double value,int dimension = 0);
    
    /**
     * @brief Return the default value for the given dimension
     **/
    double getDefaultValue(int dimension = 0) const;
    
    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(int dimension = 0);
    
    /**
     * @brief Set the minimum possible value for the given dimension. The minimum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values smaller than the minimum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMinimum(double minimum,int dimension = 0);
    
    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMinimum(int dimension = 0) const;
    
    /**
     * @brief Set the maximum possible value for the given dimension. The maximum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values greater than the maximum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMaximum(double maximum,int dimension = 0);
    
    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMaximum(int dimension = 0) const;
    
    /**
     * @brief Set the minimum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMinimum(double minimum,int dimension = 0);
    
    /**
     * @brief Get the display minimum for the given dimension
     **/
    double getDisplayMinimum(int dimension) const;
    
    /**
     * @brief Set the maximum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMaximum(double maximum,int dimension = 0);
    
    /**
     * @brief Get the display maximum for the given dimension
     **/
    double getDisplayMaximum(int dimension) const;
    
    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    double addAsDependencyOf(int fromExprDimension,Param* param,int thisDimension);
    
};

class ChoiceParam : public AnimatedParam
{
    
protected:
    boost::weak_ptr<KnobChoice> _choiceKnob;
public:
    
    ChoiceParam(const boost::shared_ptr<KnobChoice>& knob);
    
    virtual ~ChoiceParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    int get() const;
    int get(double frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(int x);
    void set(int x, double frame);
    
    /*
     * @brief Set the value from label if it exists. 
     * The label will be compared without case sensitivity to existing entries. If it's not found, nothing is done.
     */
    void set(const std::string& label);
    
    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    int getValue() const;
    
    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(int value);
    
    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    int getValueAtTime(double time) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(int value,double time);
    
    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(int value);
    
    /**
     * @brief Set the default value from an existing entry. If it does not match (without case sensitivity) an existing entry, nothing is done.
     **/
    void setDefaultValue(const std::string& value);
    
    /**
     * @brief Return the default value for the given dimension
     **/
    int getDefaultValue() const;
    
    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue();
    
    /**
     * @brief Add a new option to the drop-down menu
     **/
    void addOption(const std::string& option,const std::string& help);
    
    /**
     * @brief Set all options at once
     **/
    void setOptions(const std::list<std::pair<std::string,std::string> >& options);
    
    /**
     * @brief Returns the option at the given index
     **/
    std::string getOption(int index) const;
    
    /**
     * @brief Returns the count of options
     **/
    int getNumOptions() const;
    
    /**
     * @brief Returns all options
     **/
    std::vector<std::string> getOptions() const;
    
    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    int addAsDependencyOf(int fromExprDimension,Param* param,int thisDimension);
    
};

class BooleanParam : public AnimatedParam
{
    
protected:
    boost::weak_ptr<KnobBool> _boolKnob;
public:
    
    BooleanParam(const boost::shared_ptr<KnobBool>& knob);
    
    virtual ~BooleanParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    bool get() const;
    bool get(double frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(bool x);
    void set(bool x, double frame);
    
    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    bool getValue() const;
    
    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(bool value);
    
    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    bool getValueAtTime(double time) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(bool value,double time);
    
    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(bool value);
    
    /**
     * @brief Return the default value for the given dimension
     **/
    bool getDefaultValue() const;
    
    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue();
    
    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    bool addAsDependencyOf(int fromExprDimension,Param* param,int thisDimension);
    
};

/////////////StringParam

class StringParamBase : public AnimatedParam
{
    
protected:
    boost::weak_ptr<Knob<std::string> > _stringKnob;
public:
    
    StringParamBase(const boost::shared_ptr<Knob<std::string> >& knob);
    
    virtual ~StringParamBase();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    std::string get() const;
    std::string get(double frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(const std::string& x);
    void set(const std::string& x, double frame);
    
    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    std::string getValue() const;
    
    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(const std::string& value);
    
    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    std::string getValueAtTime(double time) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(const std::string& value,double time);
    
    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(const std::string& value);
    
    /**
     * @brief Return the default value for the given dimension
     **/
    std::string getDefaultValue() const;
    
    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue();
    
    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    std::string addAsDependencyOf(int fromExprDimension,Param* param,int thisDimension);
    
   
    
};

class StringParam : public StringParamBase
{
    boost::weak_ptr<KnobString> _sKnob;
public:
    
    enum TypeEnum {
        eStringTypeLabel = 0, //< A label shown on gui, cannot animate
        eStringTypeMultiLine, //< A text area in plain text
        eStringTypeRichTextMultiLine, //< A text area with Qt Html support
        eStringTypeCustom, //< A custom string where anything can be passed, it animates but is not editable by the user
        eStringTypeDefault, //< Same as custom except that it is editable
    };
    
    StringParam(const boost::shared_ptr<KnobString>& knob);
    
    virtual ~StringParam();
    
    /**
     * @brief Set the type of the string, @see TypeEnum.
     * This is not dynamic and has to be called right away after creating the parameter!
     **/
    void setType(TypeEnum type);
    
};

class FileParam : public StringParamBase
{
    boost::shared_ptr<KnobFile> _sKnob;
public:

    
    FileParam(const boost::shared_ptr<KnobFile>& knob);
    
    virtual ~FileParam();
  
    /**
     * @brief If enabled is true then the dialog associated to this parameter will be able to display sequences.
     * By default this is set to false. This property is not dynamic and should be set right away after parameter creation.
     **/
    void setSequenceEnabled(bool enabled);
    
    /**
     * @brief Forces the GUI to pop-up the dialog
     **/
    void openFile();
};

class OutputFileParam : public StringParamBase
{
    boost::weak_ptr<KnobOutputFile> _sKnob;
public:
    
    
    OutputFileParam(const boost::shared_ptr<KnobOutputFile>& knob);
    
    virtual ~OutputFileParam();
    
    /**
     * @brief If enabled is true then the dialog associated to this parameter will be able to display sequences.
     * By default this is set to false. This property is not dynamic and should be set right away after parameter creation.
     **/
    void setSequenceEnabled(bool enabled);
    
    /**
     * @brief Forces the GUI to pop-up the dialog
     **/
    void openFile();
};

class PathParam : public StringParamBase
{
    boost::weak_ptr<KnobPath> _sKnob;
public:
    
    
    PathParam(const boost::shared_ptr<KnobPath>& knob);
    
    virtual ~PathParam();
    
    /**
     * @brief When set, instead of being a regular line-edit with a button to select a directory, the parameter will 
     * be a table much like the Project paths table of the Project settings. This cannot animate.
     **/
    void setAsMultiPathTable();
};

/////////////////ButtonParam

class ButtonParam : public Param
{
    
protected:
    boost::weak_ptr<KnobButton> _buttonKnob;
public:
    
    ButtonParam(const boost::shared_ptr<KnobButton>& knob);
    
    virtual ~ButtonParam();
    
    /**
     * @brief Set the icon file-path that should be used for the button.
     * This can only be called right away after the parameter has been created.
     **/
    void setIconFilePath(const std::string& icon);
    
    void trigger();
};


class SeparatorParam : public Param
{
    
protected:
    boost::weak_ptr<KnobSeparator> _separatorKnob;
public:
    
    SeparatorParam(const boost::shared_ptr<KnobSeparator>& knob);
    
    virtual ~SeparatorParam();
};

class GroupParam : public Param
{
    
protected:
    boost::weak_ptr<KnobGroup> _groupKnob;
public:
    
    GroupParam(const boost::shared_ptr<KnobGroup>& knob);
    
    virtual ~GroupParam();
    
    /**
     * @brief Add a param as a child of this group
     **/
    void addParam(const Param* param);
    
    /**
     * @brief Set this group as a tab. If the holding node doesn't have a tab to contain this tab, it will create then a tab widget container.
     **/
    void setAsTab();
    
    /**
     * @brief Set whether the group should be folded or expanded.
     **/
    void setOpened(bool opened);
    bool getIsOpened() const;
};

class PageParam : public Param
{
    
protected:
    boost::weak_ptr<KnobPage> _pageKnob;
public:
    
    PageParam(const boost::shared_ptr<KnobPage>& knob);
    
    virtual ~PageParam();
    
    /**
     * @brief Add a param as a child of this group. All params should belong to a page, otherwise they will end up in the default page.
     **/
    void addParam(const Param* param);
 
};

class ParametricParam : public Param
{
    
protected:
    boost::weak_ptr<KnobParametric> _parametricKnob;
public:
    
    ParametricParam(const boost::shared_ptr<KnobParametric>& knob);
    
    virtual ~ParametricParam();
   
    void setCurveColor(int dimension,double r,double g,double b);
    
    void getCurveColor(int dimension, ColorTuple& ret) const;
    
    Natron::StatusEnum addControlPoint(int dimension,double key,double value);
    
    double getValue(int dimension,double parametricPosition) const;
    
    int getNControlPoints(int dimension) const ;
    
    Natron::StatusEnum getNthControlPoint(int dimension,
                                          int nthCtl,
                                          double *key,
                                          double *value,
                                          double *leftDerivative,
                                          double *rightDerivative) const;
    
    Natron::StatusEnum setNthControlPoint(int dimension,
                                          int nthCtl,
                                          double key,
                                          double value,
                                          double leftDerivative,
                                          double rightDerivative) ;
    
    Natron::StatusEnum deleteControlPoint(int dimension, int nthCtl) ;
    
    Natron::StatusEnum deleteAllControlPoints(int dimension) ;
    
};


#endif // PARAMETERWRAPPER_H
