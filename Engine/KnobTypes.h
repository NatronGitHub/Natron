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

#ifndef NATRON_ENGINE_KNOBTYPES_H_
#define NATRON_ENGINE_KNOBTYPES_H_

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
class OverlaySupport;
class StringAnimationManager;
/******************************INT_KNOB**************************************/

class Int_Knob: public QObject, public Knob<int>
{
    
    Q_OBJECT
    
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Int_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Int_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    void disableSlider();
    
    bool isSliderDisabled() const;
        
    static const std::string& typeNameStatic();
    
public:
    void setMinimum(int mini, int index = 0);
    
    void setMaximum(int maxi, int index = 0);
    
    void setDisplayMinimum(int mini, int index = 0);
    
    void setDisplayMaximum(int maxi, int index = 0);
    
    void setIncrement(int incr, int index = 0);
    
    void setIncrement(const std::vector<int> &incr);
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis);
    
    void setDisplayMinimumsAndMaximums(const std::vector<int> &minis, const std::vector<int> &maxis);
    
    const std::vector<int> &getMinimums() const;
    
    const std::vector<int> &getMaximums() const;
    
    const std::vector<int> &getIncrements() const;
    
    const std::vector<int> &getDisplayMinimums() const;
    
    const std::vector<int> &getDisplayMaximums() const;
    
    std::pair<int,int> getMinMaxForCurve(const Curve* curve) const;
    
    void setDimensionName(int dim,const std::string& name);
    
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;

signals:
    
    void minMaxChanged(int mini, int maxi, int index = 0);
    
    void displayMinMaxChanged(int mini,int maxi,int index = 0);
    
    void incrementChanged(int incr, int index = 0);
    
private:
        
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    
    std::vector<std::string> _dimensionNames;
    std::vector<int> _minimums, _maximums, _increments, _displayMins, _displayMaxs;
    bool _disableSlider;
    static const std::string _typeNameStr;
};

/******************************BOOL_KNOB**************************************/

class Bool_Knob:  public Knob<bool>
{
    
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Bool_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Bool_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    /// Can this type be animated?
    /// BooleanParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic() { return true; }
    
    static const std::string& typeNameStatic();
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
};

/******************************DOUBLE_KNOB**************************************/

class Double_Knob:  public QObject,public Knob<double>
{
    Q_OBJECT
    
public:
    
    enum NormalizedState {
        NORMALIZATION_NONE = 0, ///< indicating that the dimension holds a  non-normalized value.
        NORMALIZATION_X, ///< indicating that the dimension holds a value normalized against the X dimension of the project format
        NORMALIZATION_Y ///< indicating that the dimension holds a value normalized against the Y dimension of the project format
    };
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Double_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Double_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin );
    
    
    void disableSlider();
    
    bool isSliderDisabled() const;
    
    const std::vector<double> &getMinimums() const;
    
    const std::vector<double> &getMaximums() const;
    
    const std::vector<double> &getIncrements() const;
    
    const std::vector<int> &getDecimals() const;
    
    const std::vector<double> &getDisplayMinimums() const;
    
    const std::vector<double> &getDisplayMaximums() const;
    
    void setMinimum(double mini, int index = 0);
    
    void setMaximum(double maxi, int index = 0);
    
    void setDisplayMinimum(double mini, int index = 0);
    
    void setDisplayMaximum(double maxi, int index = 0);
    
    void setIncrement(double incr, int index = 0);
    
    void setDecimals(int decis, int index = 0);
    
    std::pair<double,double> getMinMaxForCurve(const Curve* curve) const;
    
    
    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis);
    
    void setDisplayMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis);
    
    void setIncrement(const std::vector<double> &incr);
    
    void setDecimals(const std::vector<int> &decis);
    
    static const std::string& typeNameStatic();
    
    void setDimensionName(int dim,const std::string& name);
    
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;
    
    NormalizedState getNormalizedState(int dimension) const
    {
        assert(dimension < 2 && dimension >= 0);
        if (dimension == 0) {
            return _normalizationXY.first;
        } else {
            return _normalizationXY.second;
        }
        
    }
    
    void setNormalizedState(int dimension,NormalizedState state) {
        assert(dimension < 2 && dimension >= 0);
        if (dimension == 0) {
            _normalizationXY.first = state;
        } else {
            _normalizationXY.second = state;
        }
    }
    
signals:
    void minMaxChanged(double mini, double maxi, int index = 0);
    
    void displayMinMaxChanged(double mini,double maxi,int index = 0);
    
    void incrementChanged(double incr, int index = 0);
    
    void decimalsChanged(int deci, int index = 0);
    
private:
    
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    
    std::vector<std::string> _dimensionNames;
    std::vector<double> _minimums, _maximums, _increments, _displayMins, _displayMaxs;
    std::vector<int> _decimals;
    bool _disableSlider;
    
    
    /// to support ofx deprecated normalizd params:
    /// the first and second dimensions of the double param( hence a pair ) have a normalized state.
    /// BY default they have NORMALIZATION_NONE
    std::pair<NormalizedState, NormalizedState> _normalizationXY;
    
    static const std::string _typeNameStr;
    
};

/******************************BUTTON_KNOB**************************************/

class Button_Knob: public Knob<bool>
{
    
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Button_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Button_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    static const std::string& typeNameStatic();
    
    void setAsRenderButton() { _renderButton = true; }
    
    bool isRenderButton() const { return _renderButton; }
    
private:
    
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
    bool _renderButton;
};

/******************************CHOICE_KNOB**************************************/

class Choice_Knob: public QObject,public Knob<int>
{
    Q_OBJECT
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Choice_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Choice_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);

    virtual ~Choice_Knob();
    
    /*Must be called right away after the constructor.*/
    void populateChoices(const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp = std::vector<std::string>());
    
    const std::vector<std::string> &getEntries() const;
    
    const std::vector<std::string> &getEntriesHelp() const;
    
    const std::string &getActiveEntryText() const;
    
    /// Can this type be animated?
    /// ChoiceParam animation may not be quite perfect yet,
    /// @see Curve::getValueAt() for the animation code.
    static bool canAnimateStatic() { return true; }
    
    static const std::string& typeNameStatic();
    
signals:
    
    void populated();
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    std::vector<std::string> _entries;
    std::vector<std::string> _entriesHelp;
    static const std::string _typeNameStr;
};

/******************************SEPARATOR_KNOB**************************************/

class Separator_Knob: public Knob<bool>
{
    
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Separator_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Separator_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    static const std::string& typeNameStatic();
    
private:
    
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
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
class Color_Knob:  public QObject, public Knob<double>
{
    Q_OBJECT
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Color_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    
    Color_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);

    const std::vector<double> &getMinimums() const;

    const std::vector<double> &getMaximums() const;

    const std::vector<double> &getDisplayMinimums() const;

    const std::vector<double> &getDisplayMaximums() const;

    void setMinimum(double mini, int index);

    void setMaximum(double maxi, int index);

    void setDisplayMinimum(double mini, int index);

    void setDisplayMaximum(double maxi, int index);

    std::pair<double,double> getMinMaxForCurve(const Curve* curve) const;

    /*minis & maxis must have the same size*/
    void setMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis);

    void setDisplayMinimumsAndMaximums(const std::vector<double> &minis, const std::vector<double> &maxis);

    static const std::string& typeNameStatic();
    
    void setDimensionName(int dim,const std::string& dimension);
    
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;

    bool areAllDimensionsEnabled() const;
    
    void activateAllDimensions() { emit mustActivateAllDimensions(); }
    
    void setPickingEnabled(bool enabled) { emit pickingEnabled(enabled); }
    
public slots:
    
    void onDimensionSwitchToggled(bool b);
    
signals:
    
    void pickingEnabled(bool);
    
    void minMaxChanged(double mini, double maxi, int index = 0);

    void displayMinMaxChanged(double mini,double maxi,int index = 0);

    void mustActivateAllDimensions();
    
private:
    

    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    bool _allDimensionsEnabled;
    std::vector<std::string> _dimensionNames;
    std::vector<double> _minimums, _maximums, _displayMins, _displayMaxs;
    static const std::string _typeNameStr;
};

/******************************STRING_KNOB**************************************/



class String_Knob: public AnimatingString_KnobHelper
{
public:
    
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new String_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    String_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    virtual ~String_Knob();
    
    /// Can this type be animated?
    /// String animation consists in setting constant strings at
    /// each keyframe, which are valid until the next keyframe.
    /// It can be useful for titling/subtitling.
    static bool canAnimateStatic() { return true; }
    
    static const std::string& typeNameStatic();
    
    void setAsMultiLine() { _multiLine = true;  }
    
    void setUsesRichText(bool useRichText) { _richText = useRichText; }
    
    bool isMultiLine() const { return _multiLine; }
    
    bool usesRichText() const { return _richText; }
    
    void setAsLabel() {
        setAnimationEnabled(false); //< labels cannot animate
        _isLabel = true;
    }

    bool isLabel() const { return _isLabel; }

    void setAsCustom() { _isCustom = true; }
    
    bool isCustomKnob() const { return _isCustom; }
        
    private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
    
    bool _multiLine;
    bool _richText;
    bool _isLabel;
    bool _isCustom;


};

/******************************GROUP_KNOB**************************************/
class Group_Knob:  public QObject, public Knob<bool>
{
    Q_OBJECT
    
    std::vector< boost::shared_ptr<KnobI> > _children;
    bool _isTab;
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Group_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Group_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    void addKnob(boost::shared_ptr<KnobI> k);
    
    const std::vector< boost::shared_ptr<KnobI> > &getChildren() const;
    
    void setAsTab() ;
    
    bool isTab() const;
    
    static const std::string& typeNameStatic();
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
};



/******************************PAGE_KNOB**************************************/

class Page_Knob :  public QObject,public Knob<bool>
{
    Q_OBJECT
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Page_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Page_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin);
    
    void addKnob(boost::shared_ptr<KnobI> k);
    
    const std::vector< boost::shared_ptr<KnobI> >& getChildren() const { return _children; }
    
    static const std::string& typeNameStatic();
    
private:
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    
    std::vector< boost::shared_ptr<KnobI> > _children;
    
    static const std::string _typeNameStr;
};


/******************************Parametric_Knob**************************************/

class Parametric_Knob :  public QObject, public Knob<double>
{
    
    Q_OBJECT
    
    mutable QMutex _curvesMutex;
    std::vector< boost::shared_ptr<Curve> > _curves;
    std::vector<RGBAColourF> _curvesColor;
    std::vector<std::string> _curveLabels;
    
public:
    
    static KnobHelper *BuildKnob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin = true) {
        return new Parametric_Knob(holder, description, dimension,declaredByPlugin);
    }
    
    Parametric_Knob(KnobHolder* holder, const std::string &description, int dimension,bool declaredByPlugin );
    
    void setCurveColor(int dimension,double r,double g,double b);
    
    void getCurveColor(int dimension,double* r,double* g,double* b);
    
    void setCurveLabel(int dimension,const std::string& str);
    
    const std::string& getCurveLabel(int dimension) const WARN_UNUSED_RETURN;
    
    void setParametricRange(double min,double max);
    
    std::pair<double,double> getParametricRange() const WARN_UNUSED_RETURN;
    
    virtual std::string getDimensionName(int dimension) const WARN_UNUSED_RETURN;
    
    boost::shared_ptr<Curve> getParametricCurve(int dimension) const;
    
    Natron::Status addControlPoint(int dimension,double key,double value) WARN_UNUSED_RETURN;
    
    Natron::Status getValue(int dimension,double parametricPosition,double *returnValue) WARN_UNUSED_RETURN;
    
    Natron::Status getNControlPoints(int dimension,int *returnValue) WARN_UNUSED_RETURN;
    
    Natron::Status getNthControlPoint(int dimension,
                                      int    nthCtl,
                                      double *key,
                                      double *value) WARN_UNUSED_RETURN;
    
    Natron::Status setNthControlPoint(int   dimension,
                                      int   nthCtl,
                                      double key,
                                      double value) WARN_UNUSED_RETURN;
    
    Natron::Status deleteControlPoint(int dimension, int nthCtl) WARN_UNUSED_RETURN;
    
    Natron::Status deleteAllControlPoints(int dimension) WARN_UNUSED_RETURN;
    
    
    static const std::string& typeNameStatic() WARN_UNUSED_RETURN;
    
    void saveParametricCurves(std::list< Curve >* curves) const;
    
    void loadParametricCurves(const std::list< Curve >& curves);
    
public slots:
    
    virtual void drawCustomBackground(){
        emit customBackgroundRequested();
    }
    
    virtual void initializeOverlayInteract(OverlaySupport* widget){
        emit mustInitializeOverlayInteract(widget);
    }
    
    virtual void resetToDefault(const QVector<int>& dimensions){
        
        emit mustResetToDefault(dimensions);
    }

    
signals:
    
    //emitted by drawCustomBackground()
    //if you can't overload drawCustomBackground()
    void customBackgroundRequested();
    
    void mustInitializeOverlayInteract(OverlaySupport*);
    
    ///emitted when the state of a curve changed at the indicated dimension
    void curveChanged(int);
    
    void mustResetToDefault(QVector<int>);
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
    virtual void cloneExtraData(const boost::shared_ptr<KnobI>& other) OVERRIDE FINAL;
    
    virtual void cloneExtraData(const boost::shared_ptr<KnobI>& other, SequenceTime offset, const RangeD* range) OVERRIDE FINAL;
    
    static const std::string _typeNameStr;
};

#endif // NATRON_ENGINE_KNOBTYPES_H_
