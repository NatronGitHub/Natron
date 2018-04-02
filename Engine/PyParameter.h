/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_ParameterWrapper_h
#define Engine_ParameterWrapper_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <vector>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

/**
 * @brief Simple wrap for the Knob class that is the API we want to expose to the Python
 * Engine module.
 **/

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

/**
 * @brief Specify that an action (a setter) must be applied on all split views in the parameter
 **/
#define kPyParamViewSetSpecAll "All"

/**
 * @brief Specify that an action (a setter) must be applied on the main view (all unsplit views) only
 **/
#define kPyParamViewIdxMain "Main"

/**
 * @brief Specify that an action (a setter) must be applied on all dimensions of a parameter
 **/
#define kPyParamDimSpecAll -1

#define PythonSetNullError() (PyErr_SetString(PyExc_RuntimeError, Param::tr("Value is Null").toStdString().c_str()))
#define PythonSetInvalidDimensionError(index) (PyErr_SetString(PyExc_IndexError, Param::tr("%1: Dimension out of range").arg(QString::number(index)).toStdString().c_str()))
#define PythonSetNonUserKnobError() (PyErr_SetString(PyExc_ValueError, Param::tr("Cannot do this on a non-user parameter").toStdString().c_str()))
#define PythonSetInvalidViewName(view) (PyErr_SetString(PyExc_ValueError, Param::tr("%1: Invalid view").arg(view).toStdString().c_str()))


class Param
{
    Q_DECLARE_TR_FUNCTIONS(Param)
    
protected:
    KnobIWPtr _knob;

public:

    Param(const KnobIPtr& knob);

    virtual ~Param();

    KnobIPtr getInternalKnob() const { return _knob.lock(); }

    /**
     * @brief Returns the parent of this parameter if any. If the parameter has no parent it is assumed to be a top-level
     * param of the effect.
     **/
    Param* getParent() const;

    /*
     @brief If the holder of this parameter is an effect, this is a pointer to the effect.
     If the holder of this parameter is a table item, this will return the effect holding the item
     itself.
     */
    Effect* getParentEffect() const;

    /*
     @brief If the holder of this parameter is a table item, this is a pointer to the table item
     */
    ItemBase* getParentItemBase() const;

    /*
     @brief If the holder of this parameter is the app itself (so it is a project setting), this is a pointer
     to the app. If the holder of this parameter is an effect, this is a pointer to the application containing
     the effect. If the holder of this parameter is a table item, this will return the application
     containing the effect holding the item itself.
     */
    App* getApp() const;

    /**
     * @brief Returns the number of dimensions that the parameter have.
     * For example a size integer parameter could have 2 dimensions: width and height.
     **/
    int getNumDimensions() const;

    /**
     * @brief Returns the name of the Param internally. This is the identifier that allows the Python programmer
     * to find and identify a specific Param.
     **/
    QString getScriptName() const;

    /**
     * @brief Returns the label of the Param as shown on the GUI.
     **/
    QString getLabel() const;
    void setLabel(const QString& label);

    /**
     * @brief Set the icon file-path that should be used for the button.
     * This can only be called right away after the parameter has been created.
     **/
    void setIconFilePath(const QString& icon, bool checked = false);

    /**
     * @brief Returns the type of the parameter. The list of known type is:
     * "Int" "Bool" "Double" "Button" "Choice" "Color" "String" "Group" "Page" "Parametric" "InputFile" "OutputFile" "Path"
     **/
    QString getTypeName() const;

    /**
     * @brief Returns the help that describes the parameter, its effect, etc...
     **/
    QString getHelp() const;
    void setHelp(const QString& help);

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
    bool getIsEnabled() const;

    /**
     * @brief Set the given dimension enabledness
     **/
    void setEnabled(bool enabled);

    /**
     * @brief Returns whether this parameter is persistent or not. A persistent parameter will be saved into the project.
     **/
    bool getIsPersistent() const;

    /**
     * @brief Set the parameter persistency.
     **/
    void setPersistent(bool persistent);

    void setExpressionCacheEnabled(bool enabled);
    bool isExpressionCacheEnabled() const;

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
     * @brief Does this parameter have a viewer interface ?
     **/
    bool getHasViewerUI() const;

    /**
     * @brief Is this parameter visible in the Viewer UI? Only valid for parameters with a viewer ui
     **/
    void setViewerUIVisible(bool visible);
    bool getViewerUIVisible() const;

    /**
     * @brief Controls the layout type of this parameter if it is present in the viewer interface of the Effect holding it
     **/
    void setViewerUILayoutType(NATRON_NAMESPACE::ViewerContextLayoutTypeEnum type);
    NATRON_NAMESPACE::ViewerContextLayoutTypeEnum getViewerUILayoutType() const;

    /**
     * @brief Controls the item spacing after this parameter if it is present in the viewer interface of the Effect holding it
     **/
    void setViewerUIItemSpacing(int spacingPx);
    int getViewerUIItemSpacing() const;

    /**
     * @brief Controls the label icon of this parameter in the viewer interface of the Effect holding it.
     * For buttons, if checked it false, the icon will be used when the button is unchecked, if checked it will be used
     * when the button is checked.
     **/
    void setViewerUIIconFilePath(const QString& icon, bool checked = false);
    QString getViewerUIIconFilePath(bool checked = false) const;

    /**
     * @brief Controls the label of this parameter if it is present in the viewer interface of the Effect holding it
     **/
    void setViewerUILabel(const QString& label);
    QString getViewerUILabel() const;

    /**
     * @brief Copies all the content of the other param: animation, expression and value.
     * The parameters must be compatible types. E.g: you cannot copy
     * a StringParam to an IntParam but you can convert a Doubleparam to an IntParam.
     **/
    bool copy(Param* other,
              int thisDimension = kPyParamDimSpecAll,
              int otherDimension = kPyParamDimSpecAll,
              const QString& thisView = QLatin1String(kPyParamViewSetSpecAll),
              const QString& otherView = QLatin1String(kPyParamViewSetSpecAll));

    bool linkTo(Param* other,
                int thisDimension = kPyParamDimSpecAll,
                int otherDimension = kPyParamDimSpecAll,
                const QString& thisView = QLatin1String(kPyParamViewSetSpecAll),
                const QString& otherView = QLatin1String(kPyParamViewSetSpecAll));

    bool slaveTo(Param* other,
                int thisDimension = kPyParamDimSpecAll,
                int otherDimension = kPyParamDimSpecAll,
                const QString& thisView = QLatin1String(kPyParamViewSetSpecAll),
                const QString& otherView = QLatin1String(kPyParamViewSetSpecAll));

    void unlink(int dimension = kPyParamDimSpecAll, const QString& thisView = QLatin1String(kPyParamViewSetSpecAll));

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
    double curve(double time, int dimension = 0, const QString& thisView = QLatin1String(kPyParamViewIdxMain)) const;
    double curve(int time, int dimension = 0, const QString& thisView = QLatin1String(kPyParamViewIdxMain)) const
    {
        return curve((double)time, dimension, thisView);
    }

    bool setAsAlias(Param* other);

    void beginChanges();

    void endChanges();

protected:

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    void _addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView);

    bool getViewSetSpecFromViewName(const QString& viewName, ViewSetSpec* view) const;
    bool getViewIdxFromViewName(const QString& viewName, ViewIdx* view) const;

    KnobIPtr getRenderCloneKnobInternal() const;

    template <typename T>
    boost::shared_ptr<T> getRenderCloneKnob() const
    {
        return boost::dynamic_pointer_cast<T>(getRenderCloneKnobInternal());
    }
    
};


class AnimatedParam
    : public Param
{
public:

    AnimatedParam(const KnobIPtr& knob);

    virtual ~AnimatedParam();

    /**
     * @brief Returns whether the given dimension has animation or not. A dimension is considered to have an animation when it has
     * at least 1 keyframe.
     **/
    bool getIsAnimated(int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Returns the number of keyframes for the given dimension.
     **/
    int getNumKeys(int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Returns the index of the keyframe at the given time for the given dimension.
     * Returns -1 if no keyframe could be found at the given time.
     **/
    int getKeyIndex(double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set in 'time' the time of the keyframe at the given index for the given dimension.
     * This function returns true if a keyframe was found at the given index, false otherwise.
     **/
    bool getKeyTime(int index, int dimension, double* time, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Removes the keyframe at the given time and dimension if it matches any.
     **/
    void deleteValueAtTime(double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Removes all animation for the given dimension.
     **/
    void removeAnimation(int dimension = kPyParamDimSpecAll, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Compute the derivative at time as a double
     **/
    double getDerivativeAtTime(double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Compute the integral of dimension from time1 to time2 as a double
     **/
    double getIntegrateFromTimeToTime(double time1, double time2, int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Get the current global time in the parameter context. Most generally this is the time on the timeline,
     * but when multiple frames are being rendered at different times, this is the time of the frame being rendered
     * by the caller thread.
     **/
    double getCurrentTime() const;

    /**
     * @brief Set an expression on the Param. This is a Python script, see documentation for more infos.
     **/
    bool setExpression(const QString& expr, bool hasRetVariable, int dimension = kPyParamDimSpecAll, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    QString getExpression(int dimension, bool* hasRetVariable, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    bool setInterpolationAtTime(double time, NATRON_NAMESPACE::KeyframeTypeEnum interpolation, int dimension = -1, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void splitView(const QString& viewName);

    void unSplitView(const QString& viewName);

    QStringList getViewsList() const;
};

/**
 * @brief Small helper struct that is returned by the get() function of all params type
 * so the user can write stuff like myParam.get().x
 **/
struct Int2DTuple
{
    int x, y;
};

struct Int3DTuple
{
    int x, y, z;
};

struct Double2DTuple
{
    double x, y;
};

struct Double3DTuple
{
    double x, y, z;
};

struct ColorTuple
{
    double r, g, b, a; //< Color params are 4-dimensional
};

class IntParam
    : public AnimatedParam
{
protected:
    KnobIntWPtr _intKnob;

public:

    IntParam(const KnobIntPtr& knob);

    virtual ~IntParam();

    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    int get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    int get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(int x, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(int x, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    int getValue(int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(int value, int dimension = 0, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    int getValueAtTime(double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(int value, double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(int value, int dimension = 0);

    /**
     * @brief Return the default value for the given dimension
     **/
    int getDefaultValue(int dimension = 0) const;

    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(int dimension = -1, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the minimum possible value for the given dimension. The minimum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values smaller than the minimum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMinimum(int minimum, int dimension = 0);

    /**
     * @brief Get the minimum for the given dimension.
     **/
    int getMinimum(int dimension = 0) const;

    /**
     * @brief Set the maximum possible value for the given dimension. The maximum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values greater than the maximum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMaximum(int maximum, int dimension = 0);

    /**
     * @brief Get the minimum for the given dimension.
     **/
    int getMaximum(int dimension = 0) const;

    /**
     * @brief Set the minimum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMinimum(int minimum, int dimension = 0);

    /**
     * @brief Get the display minimum for the given dimension
     **/
    int getDisplayMinimum(int dimension) const;

    /**
     * @brief Set the maximum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMaximum(int maximum, int dimension = 0);

    /**
     * @brief Get the display maximum for the given dimension
     **/
    int getDisplayMaximum(int dimension) const;

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    int addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView);
};

class Int2DParam
    : public IntParam
{
public:

    Int2DParam(const KnobIntPtr& knob)
        : IntParam(knob) {}

    virtual ~Int2DParam() {}

    Int2DTuple get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    Int2DTuple get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    void set(int x, int y, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(int x, int y, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
};

class Int3DParam
    : public Int2DParam
{
public:

    Int3DParam(const KnobIntPtr& knob)
        : Int2DParam(knob) {}

    virtual ~Int3DParam() {}

    Int3DTuple get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    Int3DTuple get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    void set(int x, int y, int z, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(int x, int y, int z, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
};


class DoubleParam
    : public AnimatedParam
{
protected:
    KnobDoubleWPtr _doubleKnob;

public:

    DoubleParam(const KnobDoublePtr& knob);

    virtual ~DoubleParam();

    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    double get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    double get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(double x, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(double x, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    double getValue(int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(double value, int dimension = 0, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    double getValueAtTime(double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(double value, double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(double value, int dimension = 0);

    /**
     * @brief Return the default value for the given dimension
     **/
    double getDefaultValue(int dimension = 0) const;

    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(int dimension = -1, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the minimum possible value for the given dimension. The minimum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values smaller than the minimum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMinimum(double minimum, int dimension = 0);

    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMinimum(int dimension = 0) const;

    /**
     * @brief Set the maximum possible value for the given dimension. The maximum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values greater than the maximum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMaximum(double maximum, int dimension = 0);

    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMaximum(int dimension = 0) const;

    /**
     * @brief Set the minimum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMinimum(double minimum, int dimension = 0);

    /**
     * @brief Get the display minimum for the given dimension
     **/
    double getDisplayMinimum(int dimension) const;

    /**
     * @brief Set the maximum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMaximum(double maximum, int dimension = 0);

    /**
     * @brief Get the display maximum for the given dimension
     **/
    double getDisplayMaximum(int dimension) const;

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    double addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView);
};

class Double2DParam
    : public DoubleParam
{
public:

    Double2DParam(const KnobDoublePtr& knob)
        : DoubleParam(knob) {}

    virtual ~Double2DParam() {}

    Double2DTuple get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    Double2DTuple get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    void set(double x, double y, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(double x, double y, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void setUsePointInteract(bool use);
    void setCanAutoFoldDimensions(bool can);
};

class Double3DParam
    : public Double2DParam
{
public:

    Double3DParam(const KnobDoublePtr& knob)
        : Double2DParam(knob) {}

    virtual ~Double3DParam() {}

    Double3DTuple get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    Double3DTuple get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    void set(double x, double y, double z, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(double x, double y, double z, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
};


class ColorParam
    : public AnimatedParam
{
protected:
    KnobColorWPtr _colorKnob;

public:

    ColorParam(const KnobColorPtr& knob);

    virtual ~ColorParam();

    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    ColorTuple get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    ColorTuple get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(double r, double g, double b, double a, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(double r, double g, double b, double a, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));


    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    double getValue(int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(double value, int dimension = 0, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    double getValueAtTime(double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(double value, double time, int dimension = 0, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(double value, int dimension = 0);

    /**
     * @brief Return the default value for the given dimension
     **/
    double getDefaultValue(int dimension = 0) const;

    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(int dimension = -1, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the minimum possible value for the given dimension. The minimum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values smaller than the minimum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMinimum(double minimum, int dimension = 0);

    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMinimum(int dimension = 0) const;

    /**
     * @brief Set the maximum possible value for the given dimension. The maximum will not limit the user on the GUI, i.e: he/she
     * will still be able to input values greater than the maximum, but values returned by getValue() or getValueAtTime() will
     * return a value clamped to it.
     **/
    void setMaximum(double maximum, int dimension = 0);

    /**
     * @brief Get the minimum for the given dimension.
     **/
    double getMaximum(int dimension = 0) const;

    /**
     * @brief Set the minimum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMinimum(double minimum, int dimension = 0);

    /**
     * @brief Get the display minimum for the given dimension
     **/
    double getDisplayMinimum(int dimension) const;

    /**
     * @brief Set the maximum to be displayed on a slider if this parameter has a slider.
     **/
    void setDisplayMaximum(double maximum, int dimension = 0);

    /**
     * @brief Get the display maximum for the given dimension
     **/
    double getDisplayMaximum(int dimension) const;

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    double addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView);
};

class ChoiceParam
    : public AnimatedParam
{
protected:
    KnobChoiceWPtr _choiceKnob;

public:

    ChoiceParam(const KnobChoicePtr& knob);

    virtual ~ChoiceParam();

    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    int get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    int get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(int x, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(int x, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /*
     * @brief Set the value from label if it exists.
     * The label will be compared without case sensitivity to existing entries. If it's not found, nothing is done.
     */
    void set(const QString& label, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    int getValue(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(int value, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    void setValue(const QString& label,  const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    int getValueAtTime(double time, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(int value, double time, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(int value);

    /**
     * @brief Set the default value from an existing entry. If it does not match (without case sensitivity) an existing entry, nothing is done.
     **/
    void setDefaultValue(const QString& value);

    /**
     * @brief Return the default value for the given dimension
     **/
    int getDefaultValue() const;

    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Add a new option to the drop-down menu
     **/
    void addOption(const QString& optionID, const QString& optionLabel, const QString& optionHelp);

    /**
     * @brief Set all options at once
     **/
    void setOptions(const std::list<QString>& optionIDs,
                    const std::list<QString>& optionLabels,
                    const std::list<QString>& optionHelps);

    // Backward compatibility with Natron 2
    void setOptions(const std::list<std::pair<QString, QString> >& options);

    /**
     * @brief Returns the option at the given index
     **/
    bool getOption(int index, QString* optionID, QString* optionLabel, QString* optionHelp) const;

    /**
     * @brief Returns the current option
     **/
    void getActiveOption(QString* optionID, QString* optionLabel, QString* optionHelp, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Returns the count of options
     **/
    int getNumOptions() const;

    /**
     * @brief Returns all options
     **/
    void getOptions(std::list<QString>* optionIDs,
                    std::list<QString>* optionLabels,
                    std::list<QString>* optionHelps) const;

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    int addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView);
};

class BooleanParam
    : public AnimatedParam
{
protected:
    KnobBoolWPtr _boolKnob;

public:

    BooleanParam(const KnobBoolPtr& knob);

    virtual ~BooleanParam();

    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    bool get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    bool get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(bool x, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(bool x, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    bool getValue(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(bool value, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    bool getValueAtTime(double time, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(bool value, double time, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

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
    void restoreDefaultValue(const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    bool addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView);
};

/////////////StringParam

class StringParamBase
    : public AnimatedParam
{
protected:
    boost::weak_ptr<KnobStringBase> _stringKnob;

public:

    StringParamBase(const KnobStringBasePtr& knob);

    virtual ~StringParamBase();

    /**
     * @brief Convenience function that calls getValue() for all dimensions and store them in a tuple-like struct.
     **/
    QString get(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;
    QString get(double frame, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Convenience functions for multi-dimensional setting of values
     **/
    void set(const QString& x, const QString& view = QLatin1String(kPyParamViewSetSpecAll));
    void set(const QString& x, double frame, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Returns the value held by the parameter. If it is animated, getValueAtTime
     * will be called instead at the current's timeline position.
     **/
    QString getValue(const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set the value held by the parameter. If it is animated
     * this function will either add a new keyframe or modify a keyframe already existing at the current time.
     **/
    void setValue(const QString& value, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief If this parameter is animated for the given dimension, this function returns a value interpolated between the
     * 2 keyframes surrounding the given time. If time is exactly one keyframe then the value of the keyframe is returned.
     * If this parameter is not animated for the given dimension, then this function returns the same as getValue(int)
     **/
    QString getValueAtTime(double time, const QString& view = QLatin1String(kPyParamViewIdxMain)) const;

    /**
     * @brief Set a new keyframe on the parameter at the given time. If a keyframe already exists, it will modify it.
     **/
    void setValueAtTime(const QString& value, double time, const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Set the default value for the given dimension
     **/
    void setDefaultValue(const QString& value);

    /**
     * @brief Return the default value for the given dimension
     **/
    QString getDefaultValue() const;

    /**
     * @brief Restores the default value for the given dimension
     **/
    void restoreDefaultValue(const QString& view = QLatin1String(kPyParamViewSetSpecAll));

    /**
     * @brief Adds this Param as a dependency of the given Param. This is used mainly by the GUI to notify the user
     * when a dependency (through expressions) is destroyed (because the holding node has been removed).
     * You should not call this directly.
     **/
    QString addAsDependencyOf(Param* param, int fromExprDimension, int thisDimension, const QString& fromExprView, const QString& thisView);
};

class StringParam
    : public StringParamBase
{
    KnobStringWPtr _sKnob;

public:

    enum TypeEnum
    {
        eStringTypeLabel = 0, //< A label shown on gui, cannot animate
        eStringTypeMultiLine, //< A text area in plain text
        eStringTypeRichTextMultiLine, //< A text area with Qt Html support
        eStringTypeCustom, //< A custom string where anything can be passed, it animates but is not editable by the user
        eStringTypeDefault, //< Same as custom except that it is editable
    };

    StringParam(const KnobStringPtr& knob);

    virtual ~StringParam();

    /**
     * @brief Set the type of the string, @see TypeEnum.
     * This is not dynamic and has to be called right away after creating the parameter!
     **/
    void setType(TypeEnum type);
};

class FileParam
    : public StringParamBase
{
    KnobFileWPtr _sKnob;

public:


    FileParam(const KnobFilePtr& knob);

    virtual ~FileParam();

    /**
     * @brief DEPRECATED, use setDialogType instead.
     * If enabled is true then the dialog associated to this parameter will be able to display sequences.
     * By default this is set to false. This property is not dynamic and should be set right away after parameter creation.
     **/
    void setSequenceEnabled(bool enabled);

    /**
     * @brief Set the type of dialog to use
     **/
    void setDialogType(bool fileExisting, bool useSequences, const std::vector<std::string>& filters);

    /**
     * @brief Forces the GUI to pop-up the dialog
     **/
    void openFile();

    /**
     * @brief Force a refresh of the file
     **/
    void reloadFile();
};

class PathParam
    : public StringParamBase
{
    KnobPathWPtr _sKnob;

public:


    PathParam(const KnobPathPtr& knob);

    virtual ~PathParam();

    /**
     * @brief When set, instead of being a regular line-edit with a button to select a directory, the parameter will
     * be a table much like the Project paths table of the Project settings. This cannot animate.
     **/
    void setAsMultiPathTable();

    bool isMultiPathTable() const;

    void setTable(const std::list<std::vector<std::string> >& table);

    void getTable(std::list<std::vector<std::string> >* table) const;
};

/////////////////ButtonParam

class ButtonParam
    : public Param
{
protected:
    KnobButtonWPtr _buttonKnob;

public:

    ButtonParam(const KnobButtonPtr& knob);

    virtual ~ButtonParam();

    bool isCheckable() const;

    void setDown(bool down);

    bool isDown() const;

    void trigger();
};


class SeparatorParam
    : public Param
{
protected:
    KnobSeparatorWPtr _separatorKnob;

public:

    SeparatorParam(const KnobSeparatorPtr& knob);

    virtual ~SeparatorParam();
};

class GroupParam
    : public Param
{
protected:
    KnobGroupWPtr _groupKnob;

public:

    GroupParam(const KnobGroupPtr& knob);

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

class PageParam
    : public Param
{
protected:
    KnobPageWPtr _pageKnob;

public:

    PageParam(const KnobPagePtr& knob);

    virtual ~PageParam();

    /**
     * @brief Add a param as a child of this group. All params should belong to a page, otherwise they will end up in the default page.
     **/
    void addParam(const Param* param);
};

class ParametricParam
    : public Param
{
protected:
    KnobParametricWPtr _parametricKnob;

public:

    ParametricParam(const KnobParametricPtr& knob);

    virtual ~ParametricParam();

    void setCurveColor(int dimension, double r, double g, double b);

    void getCurveColor(int dimension, ColorTuple& ret) const;

    bool addControlPoint(int dimension, double key, double value, NATRON_NAMESPACE::KeyframeTypeEnum interpolation = eKeyframeTypeSmooth);
    bool addControlPoint(int dimension, double key, double value, double leftDerivative, double rightDerivative, NATRON_NAMESPACE::KeyframeTypeEnum interpolation = eKeyframeTypeSmooth);

    double getValue(int dimension, double parametricPosition) const;

    int getNControlPoints(int dimension) const;

    // NATRON_NAMESPACE is necessary for shiboken
    bool getNthControlPoint(int dimension,
                            int nthCtl,
                            double *key,
                            double *value,
                            double *leftDerivative,
                            double *rightDerivative) const;
    bool setNthControlPoint(int dimension,
                            int nthCtl,
                            double key,
                            double value,
                            double leftDerivative,
                            double rightDerivative);
    bool setNthControlPointInterpolation(int dimension,
                                         int nThCtl,
                                         KeyframeTypeEnum interpolation);
    bool deleteControlPoint(int dimension, int nthCtl);
    bool deleteAllControlPoints(int dimension);

    void setDefaultCurvesFromCurrentCurves();
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // Engine_ParameterWrapper_h
