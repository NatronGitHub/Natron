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
    
    /**
     * @brief Returns true if this parameter is visible in the user interface, false if it is hidden.
     **/
    bool getIsVisible() const;
    
    /**
     * @brief Set the visibility of the parameter
     **/
    void setVisible(bool visible);
    
    /**
     * @brief Returns whether the given dimension is enabled, i.e: whether the user can interact with it or not.
     **/
    bool getIsEnabled(int dimension = 0) const;
    
    /**
     * @brief Set the given dimension enabledness
     **/
    void setEnabled(bool enabled,int dimension = 0);
    
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
    int getKeyIndex(int time,int dimension = 0) const;
    
    /**
     * @brief Set in 'time' the time of the keyframe at the given index for the given dimension.
     * This function returns true if a keyframe was found at the given index, false otherwise.
     **/
    bool getKeyTime(int index,int dimension,double* time) const;
    
    /**
     * @brief Removes the keyframe at the given time and dimension if it matches any.
     **/
    void deleteValueAtTime(int time,int dimension = 0);
    
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
    void setExpression(const std::string& expr,bool hasRetVariable,int dimension = 0);
    
protected:
    
    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    void _addAsDependencyOf(int fromExprDimension,Param* param);

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

class IntParam : public Param
{
    
protected:
    boost::shared_ptr<Int_Knob> _intKnob;
public:
    
    IntParam(const boost::shared_ptr<Int_Knob>& knob);
    
    virtual ~IntParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    int get() const;
    int get(int frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(int x);
    void set(int x, int frame);
    
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
    int getValueAtTime(int time,int dimension = 0) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(int value,int time,int dimension = 0);
    
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
    int addAsDependencyOf(int fromExprDimension,Param* param);

};

class Int2DParam : public IntParam
{
public:
    
    Int2DParam(const boost::shared_ptr<Int_Knob>& knob) : IntParam(knob) {}
    
    virtual ~Int2DParam() {}
    
    void get(Int2DTuple& ret) const;
    void get(int frame, Int2DTuple& ret) const;
    void set(int x, int y);
    void set(int x, int y, int frame);
};

class Int3DParam : public Int2DParam
{
public:
    
    Int3DParam(const boost::shared_ptr<Int_Knob>& knob) : Int2DParam(knob) {}
    
    virtual ~Int3DParam() {}
    
    void get(Int3DTuple& ret) const;
    void get(int frame, Int3DTuple& ret) const;
    void set(int x, int y, int z);
    void set(int x, int y, int z, int frame);
};



class DoubleParam : public Param
{
    
protected:
    boost::shared_ptr<Double_Knob> _doubleKnob;
public:
    
    DoubleParam(const boost::shared_ptr<Double_Knob>& knob);
    
    virtual ~DoubleParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    double get() const;
    double get(int frame) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(double x);
    void set(double x, int frame);
    
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
    double getValueAtTime(int time,int dimension = 0) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(double value,int time,int dimension = 0);
    
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
    double addAsDependencyOf(int fromExprDimension,Param* param);

};

class Double2DParam : public DoubleParam
{
public:
    
    Double2DParam(const boost::shared_ptr<Double_Knob>& knob) : DoubleParam(knob) {}
    
    virtual ~Double2DParam() {}
    
    void get(Double2DTuple& ret) const;
    void get(int frame, Double2DTuple& ret) const;
    void set(double x, double y);
    void set(double x, double y, int frame);
};

class Double3DParam : public Double2DParam
{
public:
    
    Double3DParam(const boost::shared_ptr<Double_Knob>& knob) : Double2DParam(knob) {}
    
    virtual ~Double3DParam() {}
    
    void get(Double3DTuple& ret) const;
    void get(int frame, Double3DTuple& ret) const;
    void set(double x, double y, double z);
    void set(double x, double y, double z, int frame);
};


class ColorParam : public Param
{
    
protected:
    boost::shared_ptr<Color_Knob> _colorKnob;
public:
    
    ColorParam(const boost::shared_ptr<Color_Knob>& knob);
    
    virtual ~ColorParam();
    
    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    void get(ColorTuple& ret) const;
    void get(int frame,ColorTuple& ret) const;
    
    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(double r, double g, double b, double a);
    void set(double r, double g, double b, double a, int frame);
    
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
    double getValueAtTime(int time,int dimension = 0) const;
    
    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(double value,int time,int dimension = 0);
    
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
    double addAsDependencyOf(int fromExprDimension,Param* param);
    
};

#endif // PARAMETERWRAPPER_H
