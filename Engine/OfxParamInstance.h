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
#ifndef NATRON_ENGINE_OFXPARAMINSTANCE_H_
#define NATRON_ENGINE_OFXPARAMINSTANCE_H_

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <QStringList>
#include <QVector4D>
#include <QThreadStorage>


#include "Global/GlobalDefines.h"
//ofx
#include "ofxhImageEffect.h"
#include "ofxCore.h"
#include "Engine/ofxhParametricParam.h"


/*This file contains the classes that connect the knobs to the OpenFX params.
 Note that all the get(...) and set(...) functions are called BY PLUGIN and you should
 never call them. When the user interact with a knob, the onInstanceChanged() slot
 is called. In turn, the plug-in will fetch the value that has changed by calling get(...).
 */
class String_Knob;
class File_Knob;
class OutputFile_Knob;
class Button_Knob;
class Tab_Knob;
class Color_Knob;
class Int_Knob;
class Double_Knob;
class Bool_Knob;
class Choice_Knob;
class Group_Knob;
class Custom_Knob;
class RichText_Knob;
class Parametric_Knob;
class OfxEffectInstance;
class CurveWidget;
class Knob;

namespace Natron{
    class OfxOverlayInteract;
}

class OfxPushButtonInstance : public OFX::Host::Param::PushbuttonInstance {
    
public:
    OfxPushButtonInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    boost::shared_ptr<Knob> getKnob() const;

private:
    boost::shared_ptr<Button_Knob> _knob;
};



class OfxIntegerInstance :  public QObject, public OFX::Host::Param::IntegerInstance {
    
    Q_OBJECT
    
public:
    
    OfxIntegerInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();
    
    boost::shared_ptr<Knob> getKnob() const;

public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
    
private:
    boost::shared_ptr<Int_Knob> _knob;
};

class OfxDoubleInstance :  public QObject,  public OFX::Host::Param::DoubleInstance {
    
    Q_OBJECT
    
public:
    OfxDoubleInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&);
    OfxStatus get(OfxTime time, double&);
    OfxStatus set(double);
    OfxStatus set(OfxTime time, double);
    OfxStatus derive(OfxTime time, double&);
    OfxStatus integrate(OfxTime time1, OfxTime time2, double&);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();
    
    boost::shared_ptr<Knob> getKnob() const;

    bool isAnimated() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
    
private:
    boost::shared_ptr<Double_Knob> _knob;
};

class OfxBooleanInstance :  public QObject,  public OFX::Host::Param::BooleanInstance {
    
    Q_OBJECT
    
public:
    OfxBooleanInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(bool&);
    OfxStatus get(OfxTime time, bool&);
    OfxStatus set(bool);
    OfxStatus set(OfxTime time, bool);
    
    
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();

    
    boost::shared_ptr<Knob> getKnob() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);

private:
    boost::shared_ptr<Bool_Knob> _knob;
};

class OfxChoiceInstance : public QObject, public OFX::Host::Param::ChoiceInstance {
    
    Q_OBJECT
    
public:
    OfxChoiceInstance(OfxEffectInstance* node,  OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();

    boost::shared_ptr<Knob> getKnob() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);

private:
    std::vector<std::string> _entries;
    boost::shared_ptr<Choice_Knob> _knob;
};

class OfxRGBAInstance :  public QObject, public OFX::Host::Param::RGBAInstance {
    
    Q_OBJECT
    
public:
    OfxRGBAInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&,double&);
    OfxStatus set(double,double,double,double);
    OfxStatus set(OfxTime time, double,double,double,double);
    OfxStatus derive(OfxTime time, double&, double&, double&, double&);
    OfxStatus integrate(OfxTime time1, OfxTime time2, double&, double&, double&, double&);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();

    
    boost::shared_ptr<Knob> getKnob() const;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
    
private:
    boost::shared_ptr<Color_Knob> _knob;
};


class OfxRGBInstance :  public QObject,  public OFX::Host::Param::RGBInstance {
    
    Q_OBJECT
    
public:
    OfxRGBInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&);
    OfxStatus set(double,double,double);
    OfxStatus set(OfxTime time, double,double,double);
    OfxStatus derive(OfxTime time, double&, double&, double&);
    OfxStatus integrate(OfxTime time1, OfxTime time2, double&, double&, double&);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();

    
    boost::shared_ptr<Knob> getKnob() const;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
    
private:
    boost::shared_ptr<Color_Knob> _knob;
};

class OfxDouble2DInstance :  public QObject, public OFX::Host::Param::Double2DInstance {
    
    Q_OBJECT
    
public:
    OfxDouble2DInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&);
    OfxStatus get(OfxTime time,double&,double&);
    OfxStatus set(double,double);
    OfxStatus set(OfxTime time,double,double);
    OfxStatus derive(OfxTime time, double&, double&);
    OfxStatus integrate(OfxTime time1, OfxTime time2, double&, double&);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();

    
    boost::shared_ptr<Knob> getKnob() const;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
private:
    boost::shared_ptr<Double_Knob> _knob;
};


class OfxInteger2DInstance :  public QObject, public OFX::Host::Param::Integer2DInstance {
    Q_OBJECT
    
public:
    OfxInteger2DInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&,int&);
    OfxStatus get(OfxTime time,int&,int&);
    OfxStatus set(int,int);
    OfxStatus set(OfxTime time,int,int);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();

    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();


    boost::shared_ptr<Knob> getKnob() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);

private:
    boost::shared_ptr<Int_Knob> _knob;
};

class OfxDouble3DInstance :  public QObject, public OFX::Host::Param::Double3DInstance {
    Q_OBJECT
    
public:
    OfxDouble3DInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&);
    OfxStatus get(OfxTime time,double&,double&,double&);
    OfxStatus set(double,double,double);
    OfxStatus set(OfxTime time,double,double,double);
    OfxStatus derive(OfxTime time, double&, double&, double&);
    OfxStatus integrate(OfxTime time1, OfxTime time2, double&, double&, double&);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();

    // callback which should set secret state as appropriate
    virtual void setSecret();

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();
    

    boost::shared_ptr<Knob> getKnob() const;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
private:
    boost::shared_ptr<Double_Knob> _knob;
};

class OfxInteger3DInstance :  public QObject, public OFX::Host::Param::Integer3DInstance {
    Q_OBJECT
    
public:
    OfxInteger3DInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&,int&,int&);
    OfxStatus get(OfxTime time,int&,int&,int&);
    OfxStatus set(int,int,int);
    OfxStatus set(OfxTime time,int,int,int);

    // callback which should set enabled state as appropriate
    virtual void setEnabled();

    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();


    boost::shared_ptr<Knob> getKnob() const;

public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
private:
    boost::shared_ptr<Int_Knob> _knob;
};

class OfxGroupInstance : public OFX::Host::Param::GroupInstance {
public:

    OfxGroupInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor);

    void addKnob(boost::shared_ptr<Knob> k);

    boost::shared_ptr<Knob> getKnob() const;

    virtual ~OfxGroupInstance(){}

private:
    boost::shared_ptr<Group_Knob> _groupKnob;
    boost::shared_ptr<Tab_Knob> _tabKnob;
};

class OfxStringInstance : public QObject, public OFX::Host::Param::StringInstance {
    Q_OBJECT

public:
    OfxStringInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor);
    
    virtual OfxStatus get(std::string&) OVERRIDE;
    virtual OfxStatus get(OfxTime time, std::string&) OVERRIDE;
    virtual OfxStatus set(const char*) OVERRIDE;
    virtual OfxStatus set(OfxTime time, const char*) OVERRIDE;
    
    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(va_list arg) OVERRIDE;
    
    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(OfxTime time, va_list arg) OVERRIDE;

    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();

    // returns true if it is a file param and that it is not empty
    bool isValid() const;
    
    void ifFileKnobPopDialog();
    
    /*Returns the frame name according to the current pattern stored by this param
     for the frame frameIndex.*/
    std::string filenameFromPattern(int frameIndex) const;
    
    /**
     * @brief getRandomFrameName Valid only for OfxStringInstance's of type kOfxParamStringIsFilePath with
     * an OfxImageEffectInstance of type kOfxImageEffectContextGenerator
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    const QString getRandomFrameName(int f) const;
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();

    
    boost::shared_ptr<Knob> getKnob() const;
    
    virtual ~OfxStringInstance(){}
    
public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
    
public slots:
    void onFrameRangeChanged(int,int);

private:
    OfxEffectInstance* _node;
    boost::shared_ptr<File_Knob> _fileKnob;
    boost::shared_ptr<OutputFile_Knob> _outputFileKnob;
    boost::shared_ptr<String_Knob> _stringKnob;
    boost::shared_ptr<RichText_Knob> _multiLineKnob;
    QThreadStorage<std::string> _localString;
};


class OfxCustomInstance : public QObject, public OFX::Host::Param::CustomInstance {
Q_OBJECT
public:
    OfxCustomInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor);

    virtual OfxStatus get(std::string&) OVERRIDE;
    virtual OfxStatus get(OfxTime time, std::string&) OVERRIDE;
    virtual OfxStatus set(const char*) OVERRIDE;
    virtual OfxStatus set(OfxTime time, const char*) OVERRIDE;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(va_list arg) OVERRIDE;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(OfxTime time, va_list arg) OVERRIDE;


    // callback which should set enabled state as appropriate
    virtual void setEnabled();

    // callback which should set secret state as appropriate
    virtual void setSecret();

    boost::shared_ptr<Knob> getKnob() const;

    virtual ~OfxCustomInstance(){}
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const ;
    virtual OfxStatus getKeyTime(int nth, OfxTime& time) const ;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const ;
    virtual OfxStatus deleteKey(OfxTime time) ;
    virtual OfxStatus deleteAllKeys();


public slots:
    
    void onKnobAnimationLevelChanged(int lvl);
    
private:
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(
                                                           const void*            handleRaw,
                                                           OfxPropertySetHandle   inArgsRaw,
                                                           OfxPropertySetHandle   outArgsRaw);

    OfxEffectInstance* _node;
    boost::shared_ptr<Custom_Knob> _knob;
    customParamInterpolationV1Entry_t _customParamInterpolationV1Entry;
    QThreadStorage<std::string> _localString;
};


class OfxParametricInstance : public QObject, public OFX::Host::ParametricParam::ParametricInstance {
    
    Q_OBJECT
    
    Natron::OfxOverlayInteract* _overlayInteract;
    OfxEffectInstance* _effect;

public:
    
    explicit OfxParametricInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor);
    
    virtual ~OfxParametricInstance();
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE;
    
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE;
    
    /// callback which should update label
    virtual void setLabel() OVERRIDE;
    
    /// callback which should set
    virtual void setDisplayRange() OVERRIDE;
    
    
    /** @brief Evaluates a parametric parameter
     
     \arg curveIndex            which dimension to evaluate
     \arg time                  the time to evaluate to the parametric param at
     \arg parametricPosition    the position to evaluate the parametric param at
     \arg returnValue           pointer to a double where a value is returned
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrBadIndex   - the curve index was invalid
     */
    virtual OfxStatus getValue(int curveIndex,OfxTime time,double parametricPosition,double *returnValue);
    
    
    /** @brief Returns the number of control points in the parametric param.
     
     \arg curveIndex            which dimension to check
     \arg time                  the time to check
     \arg returnValue           pointer to an integer where the value is returned.
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrBadIndex   - the curve index was invalid
     */
    virtual OfxStatus getNControlPoints(int curveIndex,double time,int *returnValue);
    
    
    /** @brief Returns the key/value pair of the nth control point.
     
     \arg curveIndex            which dimension to check
     \arg time                  the time to check
     \arg nthCtl                the nth control point to get the value of
     \arg key                   pointer to a double where the key will be returned
     \arg value                 pointer to a double where the value will be returned
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrUnknown    - if the type is unknown
     */
    virtual OfxStatus getNthControlPoint(int curveIndex,
                                         double time,
                                         int    nthCtl,
                                         double *key,
                                         double *value);
    
    
    /** @brief Modifies an existing control point on a curve
     
     \arg curveIndex            which dimension to set
     \arg time                  the time to set the value at
     \arg nthCtl                the control point to modify
     \arg key                   key of the control point
     \arg value                 value of the control point
     \arg addAnimationKey       if the param is an animatable, setting this to true will
     force an animation keyframe to be set as well as a curve key,
     otherwise if false, a key will only be added if the curve is already
     animating.
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrUnknown    - if the type is unknown
     
     This modifies an existing control point. Note that by changing key, the order of the
     control point may be modified (as you may move it before or after anther point). So be
     careful when iterating over a curves control points and you change a key.
     */
    virtual OfxStatus setNthControlPoint(int   curveIndex,
                                         double time,
                                         int   nthCtl,
                                         double key,
                                         double value,
                                         bool addAnimationKey
                                         );
    
    /** @brief Adds a control point to the curve.
     
     \arg curveIndex            which dimension to set
     \arg time                  the time to set the value at
     \arg key                   key of the control point
     \arg value                 value of the control point
     \arg addAnimationKey       if the param is an animatable, setting this to true will
     force an animation keyframe to be set as well as a curve key,
     otherwise if false, a key will only be added if the curve is already
     animating.
     
     @returns
     - ::kOfxStatOK            - all was fine
     - ::kOfxStatErrUnknown    - if the type is unknown
     
     This will add a new control point to the given dimension of a parametric parameter. If a key exists
     sufficiently close to 'key', then it will be set to the indicated control point.
     */
    virtual OfxStatus addControlPoint(int   curveIndex,
                                      double time,
                                      double key,
                                      double value,
                                      bool addAnimationKey);
    
    /** @brief Deletes the nth control point from a parametric param.
     
     \arg curveIndex            which dimension to delete
     \arg nthCtl                the control point to delete
     */
    virtual OfxStatus  deleteControlPoint(int   curveIndex,int   nthCtl);
    
    /** @brief Delete all curve control points on the given param.
     
     \arg curveIndex            which dimension to clear
     */
    virtual OfxStatus  deleteAllControlPoints(int   curveIndex);
    
    boost::shared_ptr<Knob> getKnob() const;
    
public slots:
    
    void onCustomBackgroundDrawingRequested();
    
    void initializeInteract(CurveWidget* widget);

   
private:
    boost::shared_ptr<Parametric_Knob> _knob;
};

#endif // NATRON_ENGINE_OFXPARAMINSTANCE_H_

