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
#include <QObject>
CLANG_DIAG_ON(deprecated)
#include <QVector>

#include "Engine/Knob.h"

#include "Global/GlobalDefines.h"

class Curve;
class CurveWidget;
/******************************INT_KNOB**************************************/

class Int_Knob: public Knob
{
    
    Q_OBJECT
    
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Int_Knob(holder, description, dimension);
    }
    
    Int_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
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
    
signals:
    
    void minMaxChanged(int mini, int maxi, int index = 0);
    
    void displayMinMaxChanged(int mini,int maxi,int index = 0);
    
    void incrementChanged(int incr, int index = 0);
    
private:
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    
    std::vector<int> _minimums, _maximums, _increments, _displayMins, _displayMaxs;
    bool _disableSlider;
    static const std::string _typeNameStr;
};

/******************************BOOL_KNOB**************************************/

class Bool_Knob: public Knob
{
    
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Bool_Knob(holder, description, dimension);
    }
    
    Bool_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
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

class Double_Knob: public Knob
{
    Q_OBJECT
    
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Double_Knob(holder, description, dimension);
    }
    
    Double_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    
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
    
signals:
    void minMaxChanged(double mini, double maxi, int index = 0);
    
    void displayMinMaxChanged(double mini,double maxi,int index = 0);
    
    void incrementChanged(double incr, int index = 0);
    
    void decimalsChanged(int deci, int index = 0);
    
private:
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    
    std::vector<double> _minimums, _maximums, _increments, _displayMins, _displayMaxs;
    std::vector<int> _decimals;
    bool _disableSlider;
    static const std::string _typeNameStr;
    
};

/******************************BUTTON_KNOB**************************************/

class Button_Knob: public Knob
{
    
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Button_Knob(holder, description, dimension);
    }
    
    Button_Knob(KnobHolder  *holder, const std::string &description, int dimension);
    
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

class Choice_Knob: public Knob
{
    Q_OBJECT
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Choice_Knob(holder, description, dimension);
    }
    
    Choice_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    /*Must be called right away after the constructor.*/
    void populate(const std::vector<std::string> &entries, const std::vector<std::string> &entriesHelp = std::vector<std::string>());
    
    const std::vector<std::string> &getEntries() const;
    
    const std::vector<std::string> &getEntriesHelp() const;
    
    int getActiveEntry() const;
    
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
/*****************************TABLE_KNOB**************************************/
//class Table_Knob: public Knob
//{
//    Q_OBJECT
//public:
//    
//    typedef std::map<std::string,std::vector<std::string> > TableEntries;
//
//    
//    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
//        return new Table_Knob(holder, description, dimension);
//    }
//    
//    ///The dimension parameter here define the number of cells in the table.
//    Table_Knob(KnobHolder *holder, const std::string &description, int dimension);
//    
//    ///append a new cell to the table up to 'dimension' cells.
//    ///Any row appended after the size reached the dimension of the table will be discarded.
//    void appendRow(const std::string& key,const std::vector<std::string> &choices);
//    
//    const TableEntries& getRows() const;
//    
//    void setRows(const Table_Knob::TableEntries& rows);
//    
//    static bool canAnimateStatic() { return false; }
//    
//    static const std::string& typeNameStatic();
//    
//    void setVerticalHeaders(const std::string& keyHeader,const std::string& choicesHeader);
//    
//    void getVerticalHeaders(std::string* keyHeader,std::string* choicesHeader);
//    
//signals:
//    
//    void populated();
//    
//private:
//    virtual bool canAnimate() const OVERRIDE FINAL;
//    
//    virtual const std::string& typeName() const OVERRIDE FINAL;
//    
//    
//    TableEntries _entries;
//    std::string _keyHeader,_choicesHeader;
//    static const std::string _typeNameStr;
//};

/******************************SEPARATOR_KNOB**************************************/

class Separator_Knob: public Knob
{
    
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Separator_Knob(holder, description, dimension);
    }
    
    Separator_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
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
class Color_Knob: public Knob
{
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Color_Knob(holder, description, dimension);
    }
    
    
    Color_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    virtual std::string getDimensionName(int dimension) const OVERRIDE FINAL;
    
    static const std::string& typeNameStatic();
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
};

/******************************STRING_KNOB**************************************/
class String_Knob: public Knob
{
public:
    struct StringKeyFrame {
        QString value;
        int time;
    };
    
    struct KeyFrame_compare_time {
        bool operator() (const StringKeyFrame& lhs, const StringKeyFrame& rhs) const {
            return lhs.time < rhs.time;
        }
    };
    
    typedef std::set<StringKeyFrame,KeyFrame_compare_time> Keyframes;
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new String_Knob(holder, description, dimension);
    }
    
    String_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    
    /// Can this type be animated?
    /// String animation consists in setting constant strings at
    /// each keyframe, which are valid until the next keyframe.
    /// It can be useful for titling/subtitling.
    static bool canAnimateStatic() { return true; }
    
    static const std::string& typeNameStatic();
    
    void setAsMultiLine() { _multiLine = true; }
    
    bool isMultiLine() const { return _multiLine; }
    
    void setAsCustom() { _isCustom = true; }
    
    bool isCustomKnob() const { return _isCustom; }
    
    const String_Knob::Keyframes& getKeyFrames() const { return _keyframes; }
    
    ///for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(
                                                          const void*            handleRaw,
                                                          OfxPropertySetHandle   inArgsRaw,
                                                          OfxPropertySetHandle   outArgsRaw);
    
    void setCustomInterpolation(customParamInterpolationV1Entry_t func,void* ofxParamHandle) {
        _customInterpolation = func;
        _ofxParamHandle = ofxParamHandle;
    }
    
    virtual Variant getValueAtTime(double time, int dimension) const OVERRIDE FINAL;
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
    virtual Natron::Status variantToKeyFrameValue(int time,const Variant& v,double* returnValue) OVERRIDE FINAL;
    
    virtual void variantFromInterpolatedValue(double interpolated,Variant* returnValue) const OVERRIDE FINAL;
    
    virtual void cloneExtraData(const Knob& other) OVERRIDE FINAL;
    
    virtual void loadExtraData(const QString& str) OVERRIDE FINAL;
    
    virtual QString saveExtraData() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
    
  
    Keyframes _keyframes;
    bool _multiLine;
    bool _isCustom;
    customParamInterpolationV1Entry_t _customInterpolation;
    void* _ofxParamHandle;

};

/******************************GROUP_KNOB**************************************/
class Group_Knob: public Knob
{
    Q_OBJECT
    
    std::vector< boost::shared_ptr<Knob> > _children;
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Group_Knob(holder, description, dimension);
    }
    
    Group_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    void addKnob(boost::shared_ptr<Knob> k);
    
    const std::vector< boost::shared_ptr<Knob> > &getChildren() const;
    
    static const std::string& typeNameStatic();
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    static const std::string _typeNameStr;
};

/******************************TAB_KNOB**************************************/

class Tab_Knob : public Knob
{
    Q_OBJECT
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Tab_Knob(holder, description, dimension);
    }
    
    Tab_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
    void addKnob(boost::shared_ptr<Knob> k);
    
    const std::vector< boost::shared_ptr<Knob> >  &getKnobs() const { return _children; }
    
    static const std::string& typeNameStatic();
    
private:
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
private:
    
    std::vector< boost::shared_ptr<Knob> > _children;
    
    static const std::string _typeNameStr;
};


/******************************Parametric_Knob**************************************/

class Parametric_Knob : public Knob{
    
    Q_OBJECT
    
    std::vector< boost::shared_ptr<Curve> > _curves;
    std::vector<RGBAColourF> _curvesColor;
    std::vector<std::string> _curveLabels;
    
public:
    
    static Knob *BuildKnob(KnobHolder *holder, const std::string &description, int dimension) {
        return new Parametric_Knob(holder, description, dimension);
    }
    
    Parametric_Knob(KnobHolder *holder, const std::string &description, int dimension);
    
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
    
public slots:
    
    virtual void drawCustomBackground(){
        emit customBackgroundRequested();
    }
    
    virtual void initializeOverlayInteract(CurveWidget* widget){
        emit mustInitializeOverlayInteract(widget);
    }
    
    virtual void resetToDefault(const QVector<int>& dimensions){
        
        emit mustResetToDefault(dimensions);
    }

    
signals:
    
    //emitted by drawCustomBackground()
    //if you can't overload drawCustomBackground()
    void customBackgroundRequested();
    
    void mustInitializeOverlayInteract(CurveWidget*);
    
    ///emitted when the state of a curve changed at the indicated dimension
    void curveChanged(int);
    
    void mustResetToDefault(QVector<int>);
    
private:
    
    virtual bool canAnimate() const OVERRIDE FINAL;
    
    virtual const std::string& typeName() const OVERRIDE FINAL;
    
    virtual void appendExtraDataToHash(std::vector<U64>* hash) const OVERRIDE FINAL;
    
    virtual void cloneExtraData(const Knob& other) OVERRIDE FINAL;
    
    virtual void loadExtraData(const QString& str) OVERRIDE FINAL;
    
    virtual QString saveExtraData() const OVERRIDE FINAL;
    
    static const std::string _typeNameStr;
};

#endif // NATRON_ENGINE_KNOBTYPES_H_
