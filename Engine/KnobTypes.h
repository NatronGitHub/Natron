//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_KNOBTYPES_H_
#define NATRON_ENGINE_KNOBTYPES_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <vector>
#include <string>
#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QVector>
#include <QMutex>

#include "Engine/Knob.h"

#include "Global/GlobalDefines.h"

class Curve;
class ChoiceExtraData;
class OverlaySupport;
class StringAnimationManager;
class BezierCP;
namespace Natron {
class Node;
}
/******************************INT_KNOB**************************************/

class Int_Knob
    : public QObject, public Knob<int>
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
        return new Int_Knob(holder, description, dimension,declaredByPlugin);
    }

    Int_Knob(KnobHolder* holder,
             const std::string &description,
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

/******************************BOOL_KNOB**************************************/

class Bool_Knob
    :  public Knob<bool>
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new Bool_Knob(holder, description, dimension,declaredByPlugin);
    }

    Bool_Knob(KnobHolder* holder,
              const std::string &description,
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

/******************************DOUBLE_KNOB**************************************/

class Double_Knob
    :  public QObject,public Knob<double>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum NormalizedStateEnum
    {
        eNormalizedStateNone = 0, ///< indicating that the dimension holds a  non-normalized value.
        eNormalizedStateX, ///< indicating that the dimension holds a value normalized against the X dimension of the project format
        eNormalizedStateY ///< indicating that the dimension holds a value normalized against the Y dimension of the project format
    };

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new Double_Knob(holder, description, dimension,declaredByPlugin);
    }

    Double_Knob(KnobHolder* holder,
                const std::string &description,
                int dimension,
                bool declaredByPlugin );

    virtual ~Double_Knob();

    void disableSlider();

    bool isSliderDisabled() const;

    const std::vector<double> &getIncrements() const;
    const std::vector<int> &getDecimals() const;

    void setIncrement(double incr, int index = 0);

    void setDecimals(int decis, int index = 0);

    void setIncrement(const std::vector<double> &incr);

    void setDecimals(const std::vector<int> &decis);

    static const std::string & typeNameStatic();

    NormalizedStateEnum getNormalizedState(int dimension) const
    {
        assert(dimension < 2 && dimension >= 0);
        if (dimension == 0) {
            return _normalizationXY.first;
        } else {
            return _normalizationXY.second;
        }
    }

    void setNormalizedState(int dimension,
                            NormalizedStateEnum state)
    {
        assert(dimension < 2 && dimension >= 0);
        if (dimension == 0) {
            _normalizationXY.first = state;
        } else {
            _normalizationXY.second = state;
        }
    }
    
    void setSpatial(bool spatial);
    bool getIsSpatial() const;

    /**
     * @brief Normalize the default values, set the _defaultStoredNormalized to true and
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
    void setDefaultValuesNormalized(int dims,double defaults[]);

    /**
     * @brief Same as setDefaultValuesNormalized but for 1 dimensional doubles
     **/
    void setDefaultValuesNormalized(double def)
    {
        double d[1];

        d[0] = def;
        setDefaultValuesNormalized(1,d);
    }

    /**
     * @brief Returns whether the default values are stored normalized or not.
     **/
    bool areDefaultValuesNormalized() const;

    /**
     * @brief Denormalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setNormalizedState has been called!
     **/
    void denormalize(int dimension,double time,double* value) const;

    /**
     * @brief Normalize the given value according to the RoD of the attached effect's input's RoD.
     * WARNING: Can only be called once setNormalizedState has been called!
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

    void restoreTracks(const std::list <SerializedTrack> & tracks,const std::list<boost::shared_ptr<Natron::Node> > & activeNodes);

    void setHasNativeOverlayHandle(bool handle);
    
    bool getHasNativeOverlayHandle() const;
    
    virtual bool useNativeOverlayHandle() const OVERRIDE { return getHasNativeOverlayHandle(); }
    
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
    /// BY default they have eNormalizedStateNone
    std::pair<NormalizedStateEnum, NormalizedStateEnum> _normalizationXY;

    ///For double params respecting the kOfxParamCoordinatesNormalised
    ///This tells us that only the default value is stored normalized.
    ///This SHOULD NOT bet set for old deprecated < OpenFX 1.2 normalized parameters.
    bool _defaultStoredNormalized;
    bool _hasNativeOverlayHandle;
    static const std::string _typeNameStr;
};

/******************************BUTTON_KNOB**************************************/

class Button_Knob
    : public Knob<bool>
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new Button_Knob(holder, description, dimension,declaredByPlugin);
    }

    Button_Knob(KnobHolder* holder,
                const std::string &description,
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

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:
    static const std::string _typeNameStr;
    bool _renderButton;
    std::string _iconFilePath;
};

/******************************CHOICE_KNOB**************************************/

class Choice_Knob
    : public QObject,public Knob<int>
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
        return new Choice_Knob(holder, description, dimension,declaredByPlugin);
    }

    Choice_Knob(KnobHolder* holder,
                const std::string &description,
                int dimension,
                bool declaredByPlugin);

    virtual ~Choice_Knob();

    /*Must be called right away after the constructor.*/
    void populateChoices( const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp = std::vector<std::string>() );
    
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
    
    void choiceRestoration(Choice_Knob* knob,const ChoiceExtraData* data);
    
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

    /// set the Choice_Knob value from the label
    ValueChangedReturnCodeEnum setValueFromLabel(const std::string & value,
                                                 int dimension,
                                                 bool turnOffAutoKeying = false);
    /// set the Choice_Knob default value from the label
    void setDefaultValueFromLabel(const std::string & value,int dimension = 0);

Q_SIGNALS:

    void populated();

private:


    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;
private:
    
    mutable QMutex _entriesMutex;
    std::vector<std::string> _entries;
    std::vector<std::string> _entriesHelp;
    bool _addNewChoice;
    static const std::string _typeNameStr;
    bool _isCascading;
};

/******************************SEPARATOR_KNOB**************************************/

class Separator_Knob
    : public Knob<bool>
{
public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new Separator_Knob(holder, description, dimension,declaredByPlugin);
    }

    Separator_Knob(KnobHolder* holder,
                   const std::string &description,
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
class Color_Knob
    :  public QObject, public Knob<double>
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
        return new Color_Knob(holder, description, dimension,declaredByPlugin);
    }

    Color_Knob(KnobHolder* holder,
               const std::string &description,
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

/******************************STRING_KNOB**************************************/


class String_Knob
    : public AnimatingString_KnobHelper
{
public:


    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new String_Knob(holder, description, dimension,declaredByPlugin);
    }

    String_Knob(KnobHolder* holder,
                const std::string &description,
                int dimension,
                bool declaredByPlugin);

    virtual ~String_Knob();

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

    void setAsLabel()
    {
        setAnimationEnabled(false); //< labels cannot animate
        _isLabel = true;
    }

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
    bool _isLabel;
    bool _isCustom;
};

/******************************GROUP_KNOB**************************************/
class Group_Knob
    :  public QObject, public Knob<bool>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    std::vector< boost::weak_ptr<KnobI> > _children;
    bool _isTab;

public:

    static KnobHelper * BuildKnob(KnobHolder* holder,
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new Group_Knob(holder, description, dimension,declaredByPlugin);
    }

    Group_Knob(KnobHolder* holder,
               const std::string &description,
               int dimension,
               bool declaredByPlugin);

    void addKnob(boost::shared_ptr<KnobI> k);
    void removeKnob(KnobI* k);
    
    void moveOneStepUp(KnobI* k);
    void moveOneStepDown(KnobI* k);
    
    void insertKnob(int index, const boost::shared_ptr<KnobI>& k);

    std::vector< boost::shared_ptr<KnobI> > getChildren() const;

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

class Page_Knob
    :  public QObject,public Knob<bool>
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
        return new Page_Knob(holder, description, dimension,declaredByPlugin);
    }

    Page_Knob(KnobHolder* holder,
              const std::string &description,
              int dimension,
              bool declaredByPlugin);

    void addKnob(const boost::shared_ptr<KnobI>& k);
    

    void moveOneStepUp(KnobI* k);
    void moveOneStepDown(KnobI* k);
    
    void removeKnob(KnobI* k);
    
    void insertKnob(int index, const boost::shared_ptr<KnobI>& k);

    std::vector< boost::shared_ptr<KnobI> >  getChildren() const;

    static const std::string & typeNameStatic();

private:
    virtual bool canAnimate() const OVERRIDE FINAL;
    virtual const std::string & typeName() const OVERRIDE FINAL;

private:

    std::vector< boost::weak_ptr<KnobI> > _children;
    static const std::string _typeNameStr;
};


/******************************Parametric_Knob**************************************/

class Parametric_Knob
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
                                  const std::string &description,
                                  int dimension,
                                  bool declaredByPlugin = true)
    {
        return new Parametric_Knob(holder, description, dimension,declaredByPlugin);
    }

    Parametric_Knob(KnobHolder* holder,
                    const std::string &description,
                    int dimension,
                    bool declaredByPlugin );

    void setCurveColor(int dimension,double r,double g,double b);

    void getCurveColor(int dimension,double* r,double* g,double* b);

    void setParametricRange(double min,double max);
    
    void setDefaultCurvesFromCurves();

    std::pair<double,double> getParametricRange() const WARN_UNUSED_RETURN;
    boost::shared_ptr<Curve> getParametricCurve(int dimension) const;
    boost::shared_ptr<Curve> getDefaultParametricCurve(int dimension) const;
    Natron::StatusEnum addControlPoint(int dimension,double key,double value) WARN_UNUSED_RETURN;
    Natron::StatusEnum addHorizontalControlPoint(int dimension,double key,double value) WARN_UNUSED_RETURN;
    Natron::StatusEnum getValue(int dimension,double parametricPosition,double *returnValue) const WARN_UNUSED_RETURN;
    Natron::StatusEnum getNControlPoints(int dimension,int *returnValue) const WARN_UNUSED_RETURN;
    Natron::StatusEnum getNthControlPoint(int dimension,
                                      int nthCtl,
                                      double *key,
                                      double *value) const WARN_UNUSED_RETURN;
    Natron::StatusEnum getNthControlPoint(int dimension,
                                          int nthCtl,
                                          double *key,
                                          double *value,
                                          double *leftDerivative,
                                          double *rightDerivative) const WARN_UNUSED_RETURN;
    Natron::StatusEnum setNthControlPoint(int dimension,
                                      int nthCtl,
                                      double key,
                                      double value) WARN_UNUSED_RETURN;
    
    Natron::StatusEnum setNthControlPoint(int dimension,
                                          int nthCtl,
                                          double key,
                                          double value,
                                          double leftDerivative,
                                          double rightDerivative) WARN_UNUSED_RETURN;

    
    Natron::StatusEnum deleteControlPoint(int dimension, int nthCtl) WARN_UNUSED_RETURN;
    Natron::StatusEnum deleteAllControlPoints(int dimension) WARN_UNUSED_RETURN;
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
    virtual void cloneExtraData(KnobI* other, SequenceTime offset, const RangeD* range,int dimension = -1) OVERRIDE FINAL;
    static const std::string _typeNameStr;
};

#endif // NATRON_ENGINE_KNOBTYPES_H_
