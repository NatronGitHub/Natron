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

#ifndef NATRON_ENGINE_KNOBTYPES_H
#define NATRON_ENGINE_KNOBTYPES_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <string>
#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QVector>
#include <QMutex>

#include "Global/GlobalDefines.h"
#include "Engine/Knob.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/******************************KnobInt**************************************/

class KnobInt
    : public QObject, public Knob<int>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobInt(holder, label, dimension, declaredByPlugin);
    }

    KnobInt(KnobHolder* holder,
             const std::string &label,
             int dimension,
             bool declaredByPlugin);

    void disableSlider();

    bool isSliderDisabled() const;

    static const std::string & typeNameStatic();

public:

    void setIncrement(int incr, int index = 0);

    void setIncrement(const std::vector<int> &incr);

    const std::vector<int> &getIncrements() const;


Q_SIGNALS:


    void incrementChanged(int incr, int index = 0);

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    std::vector<int> _increments;
    bool _disableSlider;
    static const std::string _typeNameStr;
};

/******************************KnobBool**************************************/

class KnobBool
    :  public Knob<bool>
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobBool(holder, label, dimension, declaredByPlugin);
    }

    KnobBool(KnobHolder* holder,
              const std::string &label,
              int dimension,
              bool declaredByPlugin);

    /// Can this type be animated?
    /// BooleanParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};

/******************************KnobDouble**************************************/

class KnobDouble
    :  public QObject,public Knob<double>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum ValueIsNormalizedEnum
    {
        eValueIsNormalizedNone = 0, ///< indicating that the dimension holds a  non-normalized value.
        eValueIsNormalizedX, ///< indicating that the dimension holds a value normalized against the X dimension of the project format
        eValueIsNormalizedY ///< indicating that the dimension holds a value normalized against the Y dimension of the project format
    };

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobDouble(holder, label, dimension, declaredByPlugin);
    }

    KnobDouble(KnobHolder* holder,
                const std::string &label,
                int dimension,
                bool declaredByPlugin );

    virtual ~KnobDouble();

    void disableSlider();

    bool isSliderDisabled() const;

    const std::vector<double> &getIncrements() const;
    const std::vector<int> &getDecimals() const;

    void setIncrement(double incr, int index = 0);

    void setDecimals(int decis, int index = 0);

    void setIncrement(const std::vector<double> &incr);

    void setDecimals(const std::vector<int> &decis);

    static const std::string & typeNameStatic();

    ValueIsNormalizedEnum getValueIsNormalized(int dimension) const {
        return _valueIsNormalized[dimension];
    }

    void setValueIsNormalized(int dimension,
                              ValueIsNormalizedEnum state) {
        _valueIsNormalized[dimension] = state;
    }
    
    void setSpatial(bool spatial) {
        _spatial = spatial;
    }
    
    bool getIsSpatial() const {
        return _spatial;
    }

    /**
     * @brief Normalize the default values, set the _defaultValuesAreNormalized to true and
     * calls setDefaultValue with the good parameters.
     * Later when restoring the default values, this flag will be used to know whether we need
     * to denormalize the default stored values to the set the "live" values.
     * Hence this SHOULD NOT bet set for old deprecated < OpenFX 1.2 normalized parameters otherwise
     * they would be denormalized before being passed to the plug-in.
     *
     * If *all* the following conditions hold:
     * - this is a double value
     * - this is a non normalised spatial double parameter, i.e. kOfxParamPropDoubleType is set to one of
     *   - kOfxParamDoubleTypeX
     *   - kOfxParamDoubleTypeXAbsolute
     *   - kOfxParamDoubleTypeY
     *   - kOfxParamDoubleTypeYAbsolute
     *   - kOfxParamDoubleTypeXY
     *   - kOfxParamDoubleTypeXYAbsolute
     * - kOfxParamPropDefaultCoordinateSystem is set to kOfxParamCoordinatesNormalised
     * Knob<T>::resetToDefaultValue should denormalize
     * the default value, using the input size.
     * Input size be defined as the first available of:
     * - the RoD of the "Source" clip
     * - the RoD of the first non-mask non-optional input clip (in case there is no "Source" clip) (note that if these clips are not connected, you get the current project window, which is the default value for GetRegionOfDefinition)

     * see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
     * and http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#APIChanges_1_2_SpatialParameters
     **/
    void setDefaultValuesAreNormalized(bool normalized) {
        _defaultValuesAreNormalized = normalized;
    }

    /**
     * @brief Returns whether the default values are stored normalized or not.
     **/
    bool getDefaultValuesAreNormalized() const {
        return _defaultValuesAreNormalized;
    }

    /**
     * @brief Denormalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    void denormalize(int dimension,double time,double* value) const;

    /**
     * @brief Normalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setValueIsNormalized has been called!
     **/
    void normalize(int dimension,double time,double* value) const;

    void addSlavedTrack(const boost::shared_ptr<BezierCP> & cp)
    {
        _slavedTracks.push_back(cp);
    }

    void removeSlavedTrack(const boost::shared_ptr<BezierCP> & cp);

    const std::list< boost::shared_ptr<BezierCP> > & getSlavedTracks()
    {
        return _slavedTracks;
    }

    struct SerializedTrack
    {
        std::string rotoNodeName;
        std::string bezierName;
        int cpIndex;
        bool isFeather;
        int offsetTime;
    };

    void serializeTracks(std::list<SerializedTrack>* tracks);

    void restoreTracks(const std::list <SerializedTrack> & tracks,const NodesList & activeNodes);

    void setHasHostOverlayHandle(bool handle);
    
    bool getHasHostOverlayHandle() const;
    
    virtual bool useHostOverlayHandle() const OVERRIDE { return getHasHostOverlayHandle(); }
    
public Q_SLOTS:

    void onNodeDeactivated();
    void onNodeActivated();

Q_SIGNALS:

    void incrementChanged(double incr, int index = 0);

    void decimalsChanged(int deci, int index = 0);

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    
    bool _spatial;
    std::vector<double>  _increments;
    std::vector<int> _decimals;
    bool _disableSlider;
    std::list< boost::shared_ptr<BezierCP> > _slavedTracks;

    /// to support ofx deprecated normalizd params:
    /// the first and second dimensions of the double param( hence a pair ) have a normalized state.
    /// BY default they have eValueIsNormalizedNone
    /// if the double type is one of
    /// - kOfxParamDoubleTypeNormalisedX - normalised size wrt to the project's X dimension (1D only),
    /// - kOfxParamDoubleTypeNormalisedXAbsolute - normalised absolute position on the X axis (1D only)
    /// - kOfxParamDoubleTypeNormalisedY - normalised size wrt to the project's Y dimension(1D only),
    /// - kOfxParamDoubleTypeNormalisedYAbsolute - normalised absolute position on the Y axis (1D only)
    /// - kOfxParamDoubleTypeNormalisedXY - normalised to the project's X and Y size (2D only),
    /// - kOfxParamDoubleTypeNormalisedXYAbsolute - normalised to the projects X and Y size, and is an absolute position on the image plane,
    std::vector<ValueIsNormalizedEnum> _valueIsNormalized;

    ///For double params respecting the kOfxParamCoordinatesNormalised
    ///This tells us that only the default value is stored normalized.
    ///This SHOULD NOT bet set for old deprecated < OpenFX 1.2 normalized parameters.
    bool _defaultValuesAreNormalized;
    bool _hasHostOverlayHandle;
    static const std::string _typeNameStr;
};

/******************************KnobButton**************************************/

class KnobButton
    : public Knob<bool>
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobButton(holder, label, dimension, declaredByPlugin);
    }

    KnobButton(KnobHolder* holder,
                const std::string &label,
                int dimension,
                bool declaredByPlugin);
    static const std::string & typeNameStatic();

    void setAsRenderButton()
    {
        _renderButton = true;
    }

    bool isRenderButton() const
    {
        return _renderButton;
    }

    void setIconFilePath(const std::string & filePath)
    {
        _iconFilePath = filePath;
    }

    const std::string & getIconFilePath() const
    {
        return _iconFilePath;
    }
    
    void trigger();

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _renderButton;
    std::string _iconFilePath;
};

/******************************KnobChoice**************************************/

class KnobChoice
    : public QObject,public Knob<int>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobChoice(holder, label, dimension, declaredByPlugin);
    }

    KnobChoice(KnobHolder* holder,
                const std::string &label,
                int dimension,
                bool declaredByPlugin);

    virtual ~KnobChoice();

    /*Must be called right away after the constructor.*/
    void populateChoices( const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp = std::vector<std::string>() );
    
    void resetChoices();
    
    void appendChoice(const std::string& entry, const std::string& help = std::string());
    
    std::vector<std::string> getEntries_mt_safe() const;
    const std::string& getEntry(int v) const;
    std::vector<std::string> getEntriesHelp_mt_safe() const;
    std::string getActiveEntryText_mt_safe() const;
    
    int getNumEntries() const;

    /// Can this type be animated?
    /// ChoiceParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();
    std::string getHintToolTipFull() const;
    
    void choiceRestoration(KnobChoice* knob,const ChoiceExtraData* data);
    
    /**
     * @brief When set the menu will have a "New" entry which the user can select to create a new entry on its own.
     **/
    void setHostCanAddOptions(bool add);
    
    bool getHostCanAddOptions() const;

    void setCascading(bool cascading)
    {
        _isCascading = cascading;
    }
    
    bool isCascading() const
    {
        return _isCascading;
    }

    /// set the KnobChoice value from the label
    ValueChangedReturnCodeEnum setValueFromLabel(const std::string & value,
                                                 int dimension,
                                                 bool turnOffAutoKeying = false);
    
    /// set the KnobChoice default value from the label
    void setDefaultValueFromLabel(const std::string & value,int dimension = 0);

public Q_SLOTS:
    
    void onOriginalKnobPopulated();
    void onOriginalKnobEntriesReset();
    void onOriginalKnobEntryAppend(const QString& text,const QString& help);
    
Q_SIGNALS:

    void populated();
    void entriesReset();
    void entryAppended(QString,QString);

private:


    void findAndSetOldChoice(const std::vector<std::string>& newEntries);
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;
    virtual void handleSignalSlotsForAliasLink(const KnobPtr& alias,bool connect) OVERRIDE FINAL;
    virtual void onInternalValueChanged(int dimension, double time, ViewIdx view) OVERRIDE FINAL;
    
private:
    
    mutable QMutex _entriesMutex;
    std::vector<std::string> _entries;
    std::vector<std::string> _entriesHelp;
    
#pragma message WARN("When enabling multi-view knobs, make this multi-view too")
    std::string _lastValidEntry; // protected by _entriesMutex
    bool _addNewChoice;
    static const std::string _typeNameStr;
    bool _isCascading;
};

/******************************KnobSeparator**************************************/

class KnobSeparator
    : public Knob<bool>
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobSeparator(holder, label, dimension, declaredByPlugin);
    }

    KnobSeparator(KnobHolder* holder,
                   const std::string &label,
                   int dimension,
                   bool declaredByPlugin);
    static const std::string & typeNameStatic();

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};

/******************************RGBA_KNOB**************************************/

/**
 * @brief A color knob with of variable dimension. Each color is a double ranging in [0. , 1.]
 * In dimension 1 the knob will have a single channel being a gray-scale
 * In dimension 3 the knob will have 3 channel R,G,B
 * In dimension 4 the knob will have R,G,B and A channels.
 **/
class KnobColor
    :  public QObject, public Knob<double>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobColor(holder, label, dimension, declaredByPlugin);
    }

    KnobColor(KnobHolder* holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);
    
    static const std::string & typeNameStatic();

    bool areAllDimensionsEnabled() const;

    void activateAllDimensions()
    {
        Q_EMIT mustActivateAllDimensions();
    }

    void setPickingEnabled(bool enabled)
    {
        Q_EMIT pickingEnabled(enabled);
    }

    
    /**
     * @brief When simplified, the GUI of the knob should not have any spinbox and sliders but just a label to click and openup a color dialog
     **/
    void setSimplified(bool simp);
    bool isSimplified() const;
    

public Q_SLOTS:

    void onDimensionSwitchToggled(bool b);

Q_SIGNALS:

    void pickingEnabled(bool);

    void minMaxChanged(double mini, double maxi, int index = 0);

    void displayMinMaxChanged(double mini,double maxi,int index = 0);

    void mustActivateAllDimensions();

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    bool _allDimensionsEnabled;
    bool _simplifiedMode;
    static const std::string _typeNameStr;
};

/******************************KnobString**************************************/


class KnobString
    : public AnimatingKnobStringHelper
{
public:


    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobString(holder, label, dimension, declaredByPlugin);
    }

    KnobString(KnobHolder* holder,
                const std::string &label,
                int dimension,
                bool declaredByPlugin);

    virtual ~KnobString();

    /// Can this type be animated?
    /// String animation consists in setting constant strings at
    /// each keyframe, which are valid until the next keyframe.
    /// It can be useful for titling/subtitling.
    static bool canAnimateStatic()
    {
        return true;
    }

    static const std::string & typeNameStatic();

    void setAsMultiLine()
    {
        _multiLine = true;
    }

    void setUsesRichText(bool useRichText)
    {
        _richText = useRichText;
    }

    bool isMultiLine() const
    {
        return _multiLine;
    }

    bool usesRichText() const
    {
        return _richText;
    }
    
    void setAsCustomHTMLText(bool custom) {
        _customHtmlText = custom;
    }
    
    bool isCustomHTMLText() const
    {
        return _customHtmlText;
    }

    void setAsLabel();
    
    bool isLabel() const
    {
        return _isLabel;
    }

    void setAsCustom()
    {
        _isCustom = true;
    }

    bool isCustomKnob() const
    {
        return _isCustom;
    }
    
    /**
     * @brief Relevant for multi-lines with rich text enables. It tells if
     * the string has content without the html tags
     **/
    bool hasContentWithoutHtmlTags() const;

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _multiLine;
    bool _richText;
    bool _customHtmlText;
    bool _isLabel;
    bool _isCustom;
};

/******************************KnobGroup**************************************/
class KnobGroup
    :  public QObject, public Knob<bool>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    std::vector< boost::weak_ptr<KnobI> > _children;
    bool _isTab;

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobGroup(holder, label, dimension, declaredByPlugin);
    }

    KnobGroup(KnobHolder* holder,
               const std::string &label,
               int dimension,
               bool declaredByPlugin);

    void addKnob(const KnobPtr& k);
    void removeKnob(KnobI* k);
    
    bool moveOneStepUp(KnobI* k);
    bool moveOneStepDown(KnobI* k);
    
    void insertKnob(int index, const KnobPtr& k);

    std::vector< KnobPtr > getChildren() const;

    void setAsTab();

    bool isTab() const;

    static const std::string & typeNameStatic();

private:

    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
};


/******************************PAGE_KNOB**************************************/

class KnobPage
    :  public QObject,public Knob<bool>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobPage(holder, label, dimension, declaredByPlugin);
    }

    KnobPage(KnobHolder* holder,
              const std::string &label,
              int dimension,
              bool declaredByPlugin);

    void addKnob(const KnobPtr& k);
    

    bool moveOneStepUp(KnobI* k);
    bool moveOneStepDown(KnobI* k);
    
    void removeKnob(KnobI* k);
    
    void insertKnob(int index, const KnobPtr& k);

    std::vector< KnobPtr >  getChildren() const;

    static const std::string & typeNameStatic();

private:
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    std::vector< boost::weak_ptr<KnobI> > _children;
    static const std::string _typeNameStr;
};


/******************************KnobParametric**************************************/

class KnobParametric
    :  public QObject, public Knob<double>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    mutable QMutex _curvesMutex;
    std::vector< boost::shared_ptr<Curve> > _curves, _defaultCurves;
    std::vector<RGBAColourF> _curvesColor;

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &label,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new KnobParametric(holder, label, dimension, declaredByPlugin);
    }

    KnobParametric(KnobHolder* holder,
                    const std::string &label,
                    int dimension,
                    bool declaredByPlugin );

    void setCurveColor(int dimension,double r,double g,double b);

    void getCurveColor(int dimension,double* r,double* g,double* b);

    void setParametricRange(double min,double max);
    
    void setDefaultCurvesFromCurves();

    std::pair<double,double> getParametricRange() const WARN_UNUSED_RETURN;
    boost::shared_ptr<Curve> getParametricCurve(int dimension) const;
    boost::shared_ptr<Curve> getDefaultParametricCurve(int dimension) const;
    StatusEnum addControlPoint(int dimension, double key, double value, KeyframeTypeEnum interpolation = eKeyframeTypeSmooth) WARN_UNUSED_RETURN;
    StatusEnum getValue(int dimension,double parametricPosition,double *returnValue) const WARN_UNUSED_RETURN;
    StatusEnum getNControlPoints(int dimension,int *returnValue) const WARN_UNUSED_RETURN;
    StatusEnum getNthControlPoint(int dimension,
                                      int nthCtl,
                                      double *key,
                                      double *value) const WARN_UNUSED_RETURN;
    StatusEnum getNthControlPoint(int dimension,
                                          int nthCtl,
                                          double *key,
                                          double *value,
                                          double *leftDerivative,
                                          double *rightDerivative) const WARN_UNUSED_RETURN;
    StatusEnum setNthControlPoint(int dimension,
                                      int nthCtl,
                                      double key,
                                      double value) WARN_UNUSED_RETURN;
    
    StatusEnum setNthControlPoint(int dimension,
                                          int nthCtl,
                                          double key,
                                          double value,
                                          double leftDerivative,
                                          double rightDerivative) WARN_UNUSED_RETURN;

    
    StatusEnum deleteControlPoint(int dimension, int nthCtl) WARN_UNUSED_RETURN;
    StatusEnum deleteAllControlPoints(int dimension) WARN_UNUSED_RETURN;
    static const std::string & typeNameStatic() WARN_UNUSED_RETURN;

    void saveParametricCurves(std::list< Curve >* curves) const;

    void loadParametricCurves(const std::list< Curve > & curves);

public Q_SLOTS:

    virtual void drawCustomBackground()
    {
        Q_EMIT customBackgroundRequested();
    }

    virtual void initializeOverlayInteract(OverlaySupport* widget)
    {
        Q_EMIT mustInitializeOverlayInteract(widget);
    }

    
Q_SIGNALS:

    //emitted by drawCustomBackground()
    //if you can't overload drawCustomBackground()
    void customBackgroundRequested();

    void mustInitializeOverlayInteract(OverlaySupport*);

    ///emitted when the state of a curve changed at the indicated dimension
    void curveChanged(int);

    void curveColorChanged(int);
private:

    virtual void resetExtraToDefaultValue(int dimension) OVERRIDE FINAL;
    virtual bool hasModificationsVirtual(int dimension) const OVERRIDE FINAL;
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;
    virtual void cloneExtraData(KnobI* other,int dimension = -1) OVERRIDE FINAL;
    virtual bool cloneExtraDataAndCheckIfChanged(KnobI* other,int dimension = -1) OVERRIDE FINAL;
    virtual void cloneExtraData(KnobI* other, double offset, const RangeD* range,int dimension = -1) OVERRIDE FINAL;
    static const std::string _typeNameStr;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_KNOBTYPES_H
