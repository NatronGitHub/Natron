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

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Knob.h"
#include "KnobImpl.h"

#include <algorithm> // min, max
#include <stdexcept>

#include <QtCore/QDataStream>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/KnobSerialization.h"
#include "Engine/ThreadStorage.h"
#include "Engine/Transform.h"

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/AppInstance.h"
#include "Engine/Hash64.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/DockablePanelI.h"

class Page_Knob;

using namespace Natron;
using std::make_pair; using std::pair;

KnobSignalSlotHandler::KnobSignalSlotHandler(const boost::shared_ptr<KnobI>& knob)
: QObject()
, k(knob)
{

}

void
KnobSignalSlotHandler::onAnimationRemoved(int dimension)
{
    getKnob()->onAnimationRemoved(dimension);
}

void
KnobSignalSlotHandler::onMasterChanged(int dimension)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    
    assert(handler);
    getKnob()->onMasterChanged(handler->getKnob().get(), dimension);
}

void
KnobSignalSlotHandler::onExprDependencyChanged(int dimension)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    
    assert(handler);
    getKnob()->onExprDependencyChanged(handler->getKnob().get(), dimension);
    
}

void
KnobSignalSlotHandler::onMasterKeyFrameSet(SequenceTime time,int dimension,int reason,bool added)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    boost::shared_ptr<KnobI> master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameSet(time, dimension, reason, added);
}

void
KnobSignalSlotHandler::onMasterKeyFrameRemoved(SequenceTime time,int dimension,int reason)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    boost::shared_ptr<KnobI> master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameRemoved(time, dimension, reason);
}

void
KnobSignalSlotHandler::onMasterKeyFrameMoved(int dimension,int oldTime,int newTime)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    boost::shared_ptr<KnobI> master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameMoved(dimension, oldTime, newTime);
}

void
KnobSignalSlotHandler::onMasterAnimationRemoved(int dimension)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    boost::shared_ptr<KnobI> master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT animationRemoved(dimension);
}


/***************** KNOBI**********************/

bool
KnobI::slaveTo(int dimension,
               const boost::shared_ptr<KnobI> & other,
               int otherDimension,
               bool ignoreMasterPersistence)
{
    return slaveTo(dimension, other, otherDimension, Natron::eValueChangedReasonPluginEdited,ignoreMasterPersistence);
}

void
KnobI::onKnobSlavedTo(int dimension,
                      const boost::shared_ptr<KnobI> &  other,
                      int otherDimension)
{
    slaveTo(dimension, other, otherDimension, Natron::eValueChangedReasonUserEdited);
}

void
KnobI::unSlave(int dimension,
               bool copyState)
{
    unSlave(dimension, Natron::eValueChangedReasonPluginEdited,copyState);
}

void
KnobI::onKnobUnSlaved(int dimension)
{
    unSlave(dimension, Natron::eValueChangedReasonUserEdited,true);
}

void
KnobI::deleteValueAtTime(int time,
                         int dimension)
{
    deleteValueAtTime(time, dimension, Natron::eValueChangedReasonPluginEdited);
}

void
KnobI::removeAnimation(int dimension)
{
    if (canAnimate()) {
        removeAnimation(dimension, Natron::eValueChangedReasonPluginEdited);
    }
}


void
KnobI::onAnimationRemoved(int dimension)
{
    if (canAnimate()) {
        removeAnimation(dimension, Natron::eValueChangedReasonUserEdited);
    }
}

Page_Knob*
KnobI::getTopLevelPage()
{
    boost::shared_ptr<KnobI> parentKnob = getParentKnob();
    KnobI* parentKnobTmp = parentKnob.get();
    while (parentKnobTmp) {
        boost::shared_ptr<KnobI> parent = parentKnobTmp->getParentKnob();
        if (!parent) {
            break;
        } else {
            parentKnobTmp = parent.get();
        }
    }

    ////find in which page the knob should be
    Page_Knob* isTopLevelParentAPage = dynamic_cast<Page_Knob*>(parentKnobTmp);
    return isTopLevelParentAPage;
}


/***********************************KNOB HELPER******************************************/

///for each dimension, the dimension of the master this dimension is linked to, and a pointer to the master
typedef std::vector< std::pair< int,boost::shared_ptr<KnobI> > > MastersMap;

///a curve for each dimension
typedef std::vector< boost::shared_ptr<Curve> > CurvesMap;

struct Expr
{
    std::string expression; //< the one modified by Natron
    std::string originalExpression; //< the one input by the user
    
    bool hasRet;
    std::list<KnobI*> dependencies;
    
    //PyObject* code;
    
    Expr() : expression(), originalExpression(), hasRet(false) /*, code(0)*/{}
};


struct KnobHelperPrivate
{
    KnobHelper* publicInterface;
    KnobHolder* holder;
    std::string description; //< the text label that will be displayed  on the GUI
    bool descriptionVisible;
    std::string name; //< the knob can have a name different than the label displayed on GUI.
    std::string originalName; //< the original name passed to setName() by the user
    //By default this is the same as _description but can be set by calling setName().
    bool newLine;
    bool addSeparator;
    int itemSpacing;
    boost::weak_ptr<KnobI> parentKnob;
    bool IsSecret;
    std::vector<bool> enabled;
    bool CanUndo;
    
    QMutex evaluateOnChangeMutex;
    bool evaluateOnChange; //< if true, a value change will never trigger an evaluation
    bool IsPersistant; //will it be serialized?
    std::string tooltipHint;
    bool isAnimationEnabled;
    int dimension;
    /* the keys for a specific dimension*/
    CurvesMap curves;
    
    ////curve links
    ///A slave link CANNOT be master at the same time (i.e: if _slaveLinks[i] != NULL  then _masterLinks[i] == NULL )
    mutable QReadWriteLock mastersMutex; //< protects _masters & ignoreMasterPersistence & listeners
    MastersMap masters; //from what knob is slaved each curve if any
    bool ignoreMasterPersistence; //< when true masters will not be serialized
    
    ///This is a list of all the knobs that have expressions/links to this knob. It could be named "slaves" but
    ///in the future we will also add expressions.
    std::list<boost::weak_ptr<KnobI> > listeners;
    
    mutable QMutex animationLevelMutex;
    std::vector<Natron::AnimationLevelEnum> animationLevel; //< indicates for each dimension whether it is static/interpolated/onkeyframe
    bool declaredByPlugin; //< was the knob declared by a plug-in or added by Natron
    bool dynamicallyCreated; //< true if the knob was dynamically created by the user (either via python or via the gui)
    bool userKnob; //< true if it was created by the user and should be put into the "User" page
    
    ///Pointer to the ofx param overlay interact
    boost::shared_ptr<OfxParamOverlayInteract> customInteract;
    
    ///Pointer to the knobGui interface if it has any
    KnobGuiI* gui;
    
    mutable QMutex mustCloneGuiCurvesMutex;
    /// Set to true if gui curves were modified by the user instead of the real internal curves.
    /// If true then when finished rendering, the knob should clone the guiCurves into the internal curves.
    std::vector<bool> mustCloneGuiCurves;
    std::vector<bool> mustCloneInternalCurves;
    
    ///Used by deQueueValuesSet to know whether we should clear expressions results or not
    std::vector<bool> mustClearExprResults;
    
    ///A blind handle to the ofx param, needed for custom overlay interacts
    void* ofxParamHandle;
    
    ///This is to deal with multi-instance effects such as the Tracker: instance specifics knobs are
    ///not shared between instances whereas non instance specifics are shared.
    bool isInstanceSpecific;
    
    std::vector<std::string> dimensionNames;
    
    mutable QMutex expressionMutex;
    std::vector<Expr> expressions;
    
    mutable QMutex lastRandomHashMutex;
    mutable U32 lastRandomHash;
    
    ///Used to prevent recursive calls for expressions
    mutable ThreadStorage<int> expressionRecursionLevel;
    
    mutable QMutex hasModificationsMutex;
    std::vector<bool> hasModifications;
    
    mutable QMutex valueChangedBlockedMutex;
    bool valueChangedBlocked;
    
    KnobHelperPrivate(KnobHelper* publicInterface_,
                      KnobHolder*  holder_,
                      int dimension_,
                      const std::string & description_,
                      bool declaredByPlugin_)
    : publicInterface(publicInterface_)
    , holder(holder_)
    , description(description_)
    , descriptionVisible(true)
    , name( description_.c_str() )
    , originalName(description.c_str())
    , newLine(true)
    , addSeparator(false)
    , itemSpacing(0)
    , parentKnob()
    , IsSecret(false)
    , enabled(dimension_)
    , CanUndo(true)
    , evaluateOnChangeMutex()
    , evaluateOnChange(true)
    , IsPersistant(true)
    , tooltipHint()
    , isAnimationEnabled(true)
    , dimension(dimension_)
    , curves(dimension_)
    , mastersMutex()
    , masters(dimension_)
    , ignoreMasterPersistence(false)
    , listeners()
    , animationLevelMutex()
    , animationLevel(dimension_)
    , declaredByPlugin(declaredByPlugin_)
    , dynamicallyCreated(false)
    , userKnob(false)
    , customInteract()
    , gui(0)
    , mustCloneGuiCurvesMutex()
    , mustCloneGuiCurves()
    , mustCloneInternalCurves()
    , mustClearExprResults()
    , ofxParamHandle(0)
    , isInstanceSpecific(false)
    , dimensionNames(dimension_)
    , expressionMutex()
    , expressions()
    , lastRandomHash(0)
    , expressionRecursionLevel()
    , hasModificationsMutex()
    , hasModifications()
    , valueChangedBlockedMutex()
    , valueChangedBlocked(false)
    {
        mustCloneGuiCurves.resize(dimension);
        mustCloneInternalCurves.resize(dimension);
        mustClearExprResults.resize(dimension);
        expressions.resize(dimension);
        hasModifications.resize(dimension);
        for (int i = 0; i < dimension_; ++i) {
            mustCloneGuiCurves[i] = false;
            mustCloneInternalCurves[i] = false;
            mustClearExprResults[i] = false;
            hasModifications[i] = false;
        }
    }
    
    void parseListenersFromExpression(int dimension);
    
    std::string declarePythonVariables(bool addTab, int dimension);
};



KnobHelper::KnobHelper(KnobHolder* holder,
                       const std::string & description,
                       int dimension,
                       bool declaredByPlugin)
: _signalSlotHandler()
, _imp( new KnobHelperPrivate(this,holder,dimension,description,declaredByPlugin) )
{
}

KnobHelper::~KnobHelper()
{
    
}

void
KnobHelper::incrementExpressionRecursionLevel() const
{
    if (!_imp->expressionRecursionLevel.hasLocalData()) {
        _imp->expressionRecursionLevel.localData() = 1;
    } else {
        int& tls = _imp->expressionRecursionLevel.localData();
        ++tls;
    }
    
}

void
KnobHelper::decrementExpressionRecursionLevel() const
{
    assert(_imp->expressionRecursionLevel.hasLocalData());
    int& tls = _imp->expressionRecursionLevel.localData();
    --tls;
}

int
KnobHelper::getExpressionRecursionLevel() const
{
    if (!_imp->expressionRecursionLevel.hasLocalData()) {
        return 0;
    } else {
        int& tls = _imp->expressionRecursionLevel.localData();
        return tls;
    }
}

void
KnobHelper::deleteKnob()
{
    std::list<boost::weak_ptr<KnobI> > listenersCpy = _imp->listeners;
    for (std::list<boost::weak_ptr<KnobI> >::iterator it = listenersCpy.begin(); it != listenersCpy.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->lock();
        if (!knob) {
            continue;
        }
        for (int i = 0; i < knob->getDimension(); ++i) {
            knob->clearExpression(i,true);
        }
    }
    
    for (int i = 0; i < getDimension(); ++i) {
        clearExpression(i, true);
    }
    
    boost::shared_ptr<KnobI> parent = _imp->parentKnob.lock();
    if (parent) {
        Group_Knob* isGrp =  dynamic_cast<Group_Knob*>(parent.get());
        Page_Knob* isPage = dynamic_cast<Page_Knob*>(parent.get());
        if (isGrp) {
            isGrp->removeKnob(this);
        } else if (isPage) {
            isPage->removeKnob(this);
        } else {
            assert(false);
        }
    }
    Group_Knob* isGrp =  dynamic_cast<Group_Knob*>(this);
    Page_Knob* isPage = dynamic_cast<Page_Knob*>(this);
    if (isGrp) {
        std::vector<boost::shared_ptr<KnobI> > children = isGrp->getChildren();
        for (std::vector<boost::shared_ptr<KnobI> >::iterator it = children.begin(); it != children.end(); ++it) {
            _imp->holder->removeDynamicKnob(it->get());
        }
    } else if (isPage) {
        std::vector<boost::shared_ptr<KnobI> > children = isPage->getChildren();
        for (std::vector<boost::shared_ptr<KnobI> >::iterator it = children.begin(); it != children.end(); ++it) {
            _imp->holder->removeDynamicKnob(it->get());
        }
    }
    if (_imp->gui) {
        _imp->gui->onKnobDeletion();
    }
    
    KnobHolder* holder = getHolder();
    if (holder) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (effect) {
            if (useNativeOverlayHandle()) {
                effect->getNode()->removeDefaultOverlay(this);
            }
            effect->getNode()->removeParameterFromPython(getName());
        }
    }
    _signalSlotHandler.reset();
}

void
KnobHelper::setKnobGuiPointer(KnobGuiI* ptr)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->gui = ptr;
}

KnobGuiI*
KnobHelper::getKnobGuiPointer() const
{
    return _imp->gui;
}

bool
KnobHelper::isDeclaredByPlugin() const
{
    return _imp->declaredByPlugin;
}

void
KnobHelper::setAsInstanceSpecific()
{
    _imp->isInstanceSpecific = true;
}

bool
KnobHelper::isInstanceSpecific() const
{
    return _imp->isInstanceSpecific;
}

void
KnobHelper::setDynamicallyCreated()
{
    _imp->dynamicallyCreated = true;
}

bool
KnobHelper::isDynamicallyCreated() const
{
    return _imp->dynamicallyCreated;
}

void
KnobHelper::setAsUserKnob()
{
    _imp->userKnob = true;
    _imp->dynamicallyCreated = true;
}

bool
KnobHelper::isUserKnob() const
{
    return _imp->userKnob;
}

void
KnobHelper::populate()
{
    
    boost::shared_ptr<KnobI> thisKnob = shared_from_this();
    boost::shared_ptr<KnobSignalSlotHandler> handler( new KnobSignalSlotHandler(thisKnob) );
    setSignalSlotHandler(handler);

    Color_Knob* isColor = dynamic_cast<Color_Knob*>(this);
    Separator_Knob* isSep = dynamic_cast<Separator_Knob*>(this);
    if (isSep) {
        _imp->IsPersistant = false;
    }
    for (int i = 0; i < _imp->dimension; ++i) {
        _imp->enabled[i] = true;
        if (canAnimate()) {
            _imp->curves[i] = boost::shared_ptr<Curve>( new Curve(this,i) );
        }
        _imp->animationLevel[i] = Natron::eAnimationLevelNone;
        
        
        if (!isColor) {
            switch (i) {
                case 0:
                    _imp->dimensionNames[i] = "x";
                    break;
                case 1:
                    _imp->dimensionNames[i] = "y";
                    break;
                case 2:
                    _imp->dimensionNames[i] = "z";
                    break;
                case 3:
                    _imp->dimensionNames[i] = "w";
                    break;
                default:
                    break;
            }
            
        } else {
            switch (i) {
                case 0:
                    _imp->dimensionNames[i] = "r";
                    break;
                case 1:
                    _imp->dimensionNames[i] = "g";
                    break;
                case 2:
                    _imp->dimensionNames[i] = "b";
                    break;
                case 3:
                    _imp->dimensionNames[i] = "a";
                    break;
                default:
                    break;
            }
        }
        
    }
    
}

std::string
KnobHelper::getDimensionName(int dimension) const
{
    assert( dimension < (int)_imp->dimensionNames.size() && dimension >= 0);
    return _imp->dimensionNames[dimension];
    
}

void
KnobHelper::setDimensionName(int dim,const std::string & name)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->dimensionNames[dim] = name;
}

template <typename T>
const std::string &
Knob<T>::typeName() const {
    static std::string knobNoTypeName("NoType");
    return knobNoTypeName;
}

template <typename T>
bool
Knob<T>::canAnimate() const {
    return false;
}

void
KnobHelper::setSignalSlotHandler(const boost::shared_ptr<KnobSignalSlotHandler> & handler)
{
    _signalSlotHandler = handler;
}

void
KnobHelper::deleteValueAtTime(int time,
                              int dimension,
                              Natron::ValueChangedReasonEnum reason)
{
    if ( dimension > (int)_imp->curves.size() || dimension < 0) {
        throw std::invalid_argument("KnobHelper::deleteValueAtTime(): Dimension out of range");
    }

    if (!canAnimate() || !isAnimated(dimension)) {
        return;
    }
    
    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }

    assert(curve);

    try {
        curve->removeKeyFrameWithTime( (double)time );
    } catch (const std::exception & e) {
        //qDebug() << e.what();
    }
    
    //virtual portion
    keyframeRemoved_virtual(dimension, time);
    

    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }


    if (!useGuiCurve) {
        
        if (_signalSlotHandler) {
            _signalSlotHandler->s_updateDependencies(dimension);
        }
        checkAnimationLevel(dimension);
        guiCurveCloneInternalCurve(dimension, reason);
        evaluateValueChange(dimension,reason);
    }
    
    if (_signalSlotHandler && reason != eValueChangedReasonUserEdited) {
        _signalSlotHandler->s_keyFrameRemoved(time,dimension,(int)reason);
    }
    
}

void
KnobHelper::onKeyFrameRemoved(SequenceTime time,int dimension)
{
    deleteValueAtTime(time,dimension,Natron::eValueChangedReasonUserEdited);
}

bool
KnobHelper::moveValueAtTime(int time,int dimension,double dt,double dv,KeyFrame* newKey)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());

    if (!canAnimate() || !isAnimated(dimension)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    assert(curve);
        
    KeyFrame k;
    int keyindex = curve->keyFrameIndex(time);
    if (keyindex == -1) {
        return false;
    }
    
    bool gotKey = curve->getKeyFrameWithIndex(keyindex, &k);
    if (!gotKey) {
        return false;
    }
    
    double newX = k.getTime() + dt;
    double newY = k.getValue() + dv;
    
    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        newY = std::floor(newY + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        newY = newY < 0.5 ? 0 : 1;
    }
    
    ///Make sure string animation follows up
    AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>(this);
    std::string v;
    if (isString) {
        isString->stringFromInterpolatedValue(k.getValue(), &v);
    }
    keyframeRemoved_virtual(dimension,time);
    if (isString) {
        double ret;
        isString->stringToKeyFrameValue(newX, v, &ret);
    }
    
    
    try {
        *newKey = curve->setKeyFrameValueAndTime(newX,newY, keyindex, NULL);
    } catch (...) {
        return false;
    }
    
    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameMoved(dimension,time,newX);
    }
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, Natron::eValueChangedReasonPluginEdited);
        guiCurveCloneInternalCurve(dimension, Natron::eValueChangedReasonPluginEdited);
    }
    return true;
    
}

bool
KnobHelper::transformValueAtTime(int time,int dimension,const Transform::Matrix3x3& matrix,KeyFrame* newKey)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());
    
    if (!canAnimate() || !isAnimated(dimension)) {
        return false;
    }
    
    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    assert(curve);
    
    KeyFrame k;
    int keyindex = curve->keyFrameIndex(time);
    if (keyindex == -1) {
        return false;
    }
    
    bool gotKey = curve->getKeyFrameWithIndex(keyindex, &k);
    if (!gotKey) {
        return false;
    }
    
    Transform::Point3D p;
    p.x = k.getTime();
    p.y = k.getValue();
    p.z = 1;
    
    p = Transform::matApply(matrix, p);
    
    //clamp p.x to integers for keyframe times
    p.x = std::floor(p.x + 0.5);
    
    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        p.y = std::floor(p.y + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        p.y = p.y < 0.5 ? 0 : 1;
    }
    
    ///Make sure string animation follows up
    AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>(this);
    std::string v;
    if (isString) {
        isString->stringFromInterpolatedValue(k.getValue(), &v);
    }
    keyframeRemoved_virtual(dimension,time);
    if (isString) {
        double ret;
        isString->stringToKeyFrameValue(p.x, v, &ret);
    }
    
    
    try {
        *newKey = curve->setKeyFrameValueAndTime(p.x,p.y, keyindex, NULL);
    } catch (...) {
        return false;
    }
    
    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameMoved(dimension,time,p.x);
    }
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, Natron::eValueChangedReasonPluginEdited);
        guiCurveCloneInternalCurve(dimension , eValueChangedReasonPluginEdited);
    }
    return true;

}

void
KnobHelper::cloneCurve(int dimension,const Curve& curve)
{
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());
    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> thisCurve;
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        thisCurve = _imp->curves[dimension];
    } else {
        thisCurve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    assert(thisCurve);
    
    if (_signalSlotHandler) {
        _signalSlotHandler->s_animationAboutToBeRemoved(dimension);
        _signalSlotHandler->s_animationRemoved(dimension);
    }
    animationRemoved_virtual(dimension);
    thisCurve->clone(curve);
    if (!useGuiCurve) {
        evaluateValueChange(dimension, Natron::eValueChangedReasonPluginEdited);
        guiCurveCloneInternalCurve(dimension, eValueChangedReasonPluginEdited);
    }
    
    if (_signalSlotHandler) {
        std::list<SequenceTime> keysList;
        KeyFrameSet keys = thisCurve->getKeyFrames_mt_safe();
        for (KeyFrameSet::iterator it = keys.begin(); it!=keys.end(); ++it) {
            keysList.push_back(it->getTime());
        }
        if (!keysList.empty()) {
            _signalSlotHandler->s_multipleKeyFramesSet(keysList, dimension, (int)Natron::eValueChangedReasonNatronInternalEdited);
        }
    }
}

bool
KnobHelper::setInterpolationAtTime(int dimension,int time,Natron::KeyframeTypeEnum interpolation,KeyFrame* newKey)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());

    if (!canAnimate() || !isAnimated(dimension)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    assert(curve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }
    
    *newKey = curve->setKeyFrameInterpolation(interpolation, keyIndex);
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, Natron::eValueChangedReasonPluginEdited);
        guiCurveCloneInternalCurve(dimension, eValueChangedReasonPluginEdited);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_refreshGuiCurve(dimension);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameInterpolationChanged(time, dimension);
    }
    return true;
}

bool
KnobHelper::moveDerivativesAtTime(int dimension,int time,double left,double right)
{
    assert(QThread::currentThread() == qApp->thread());
    if ( dimension > (int)_imp->curves.size() || dimension < 0) {
        throw std::invalid_argument("KnobHelper::setInterpolationAtTime(): Dimension out of range");
    }
    
    if (!canAnimate() || !isAnimated(dimension)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }

    assert(curve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }
    
    curve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
    curve->setKeyFrameDerivatives(left, right, keyIndex);
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, Natron::eValueChangedReasonPluginEdited);
        guiCurveCloneInternalCurve(dimension, eValueChangedReasonPluginEdited);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_refreshGuiCurve(dimension);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_derivativeMoved(time, dimension);
    }
    return true;
}

bool
KnobHelper::moveDerivativeAtTime(int dimension,int time,double derivative,bool isLeft)
{
    assert(QThread::currentThread() == qApp->thread());
    if ( dimension > (int)_imp->curves.size() || dimension < 0) {
        throw std::invalid_argument("KnobHelper::setInterpolationAtTime(): Dimension out of range");
    }
    
    if (!canAnimate() || !isAnimated(dimension)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    assert(curve);
    
    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }
    
    curve->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
    if (isLeft) {
        curve->setKeyFrameLeftDerivative(derivative, keyIndex);
    } else {
        curve->setKeyFrameRightDerivative(derivative, keyIndex);
    }
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, Natron::eValueChangedReasonPluginEdited);
        guiCurveCloneInternalCurve(dimension, eValueChangedReasonPluginEdited);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_refreshGuiCurve(dimension);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_derivativeMoved(time, dimension);
    }
    return true;
}

void
KnobHelper::removeAnimation(int dimension,
                            Natron::ValueChangedReasonEnum reason)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(0 <= dimension);
    if ( (dimension < 0) || ( (int)_imp->curves.size() <= dimension ) ) {
        throw std::invalid_argument("KnobHelper::removeAnimation(): Dimension out of range");
    }


    if (!canAnimate() || !isAnimated(dimension)) {
        return ;
    }

    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    
    if ( _signalSlotHandler && (reason != Natron::eValueChangedReasonUserEdited) ) {
        _signalSlotHandler->s_animationAboutToBeRemoved(dimension);
    }

    assert(curve);
    if (curve) {
        curve->clearKeyFrames();
    }
    
    if ( _signalSlotHandler && (reason != Natron::eValueChangedReasonUserEdited) ) {
        _signalSlotHandler->s_animationRemoved(dimension);
    }
    
    animationRemoved_virtual(dimension);

    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }
    
    
    if (!useGuiCurve) {
        //virtual portion
        evaluateValueChange(dimension, reason);
        guiCurveCloneInternalCurve(dimension, reason);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_refreshGuiCurve(dimension);
        }
    }
}

void
KnobHelper::clearExpressionsResultsIfNeeded(std::map<int,Natron::ValueChangedReasonEnum>& modifiedDimensions)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    for (int i = 0; i < getDimension(); ++i) {
        if (_imp->mustClearExprResults[i]) {
            clearExpressionsResults(i);
            _imp->mustClearExprResults[i] = false;
            modifiedDimensions.insert(std::make_pair(i, Natron::eValueChangedReasonNatronInternalEdited));
        }
    }
}

void
KnobHelper::cloneInternalCurvesIfNeeded(std::map<int,Natron::ValueChangedReasonEnum>& modifiedDimensions)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    for (int i = 0; i < getDimension(); ++i) {
        if (_imp->mustCloneInternalCurves[i]) {
            guiCurveCloneInternalCurve(i, eValueChangedReasonNatronInternalEdited);
            _imp->mustCloneInternalCurves[i] = false;
            modifiedDimensions.insert(std::make_pair(i,Natron::eValueChangedReasonNatronInternalEdited));
        }
    }
}

void
KnobHelper::setInternalCurveHasChanged(int dimension, bool changed)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    _imp->mustCloneInternalCurves[dimension] = changed;
}

void
KnobHelper::cloneGuiCurvesIfNeeded(std::map<int,Natron::ValueChangedReasonEnum>& modifiedDimensions)
{
    if (!canAnimate()) {
        return;
    }

    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    for (int i = 0; i < getDimension(); ++i) {
        if (_imp->mustCloneGuiCurves[i]) {
            boost::shared_ptr<Curve> curve = getCurve(i);
            assert(curve);
            boost::shared_ptr<Curve> guicurve = _imp->gui->getCurve(i);
            assert(guicurve);
            curve->clone(*guicurve);
            _imp->mustCloneGuiCurves[i] = false;
            
            modifiedDimensions.insert(std::make_pair(i,Natron::eValueChangedReasonUserEdited));
        }
    }
    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }
}

void
KnobHelper::guiCurveCloneInternalCurve(int dimension,Natron::ValueChangedReasonEnum reason)
{
    if (!canAnimate()) {
        return;
    }

    if (_imp->gui) {
        boost::shared_ptr<Curve> guicurve = _imp->gui->getCurve(dimension);
        assert(guicurve);
        guicurve->clone(*(_imp->curves[dimension]));
        if (_signalSlotHandler && reason != eValueChangedReasonUserEdited) {
            _signalSlotHandler->s_refreshGuiCurve(dimension);
        }
    }
}

boost::shared_ptr<Curve>
KnobHelper::getGuiCurve(int dimension) const
{
    if (!canAnimate()) {
        return boost::shared_ptr<Curve>();
    }

    if (_imp->gui) {
        return _imp->gui->getCurve(dimension);
    } else {
        return boost::shared_ptr<Curve>();
    }
}

void
KnobHelper::setGuiCurveHasChanged(int dimension,bool changed)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    _imp->mustCloneGuiCurves[dimension] = changed;
}

boost::shared_ptr<Curve> KnobHelper::getCurve(int dimension,bool byPassMaster) const
{

    if (dimension < 0 || dimension >= (int)_imp->curves.size() ) {
        return boost::shared_ptr<Curve>();
    }

    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return master.second->getCurve(master.first);
    }
    
    return _imp->curves[dimension];
}

bool
KnobHelper::isAnimated(int dimension) const
{
    if (!canAnimate()) {
        return false;
    }
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    return curve->isAnimated();
}

const std::vector<boost::shared_ptr<Curve> > &
KnobHelper::getCurves() const
{
    return _imp->curves;
}

int
KnobHelper::getDimension() const
{
    return _imp->dimension;
}

void
KnobHelper::beginChanges()
{
    if (_imp->holder) {
        _imp->holder->beginChanges();
    }
}

void
KnobHelper::endChanges()
{
    if (_imp->holder) {
        _imp->holder->endChanges();
    }
}

void
KnobHelper::blockValueChanges()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);
    _imp->valueChangedBlocked = true;
}

void
KnobHelper::unblockValueChanges()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);
    _imp->valueChangedBlocked = false;
}

bool
KnobHelper::isValueChangesBlocked() const
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);
    return _imp->valueChangedBlocked;
}

void
KnobHelper::evaluateValueChange(int dimension,
                                Natron::ValueChangedReasonEnum reason)
{
    

    AppInstance* app = 0;
    if (_imp->holder) {
        app = _imp->holder->getApp();
    }
    
    bool guiFrozen = app && _imp->gui && _imp->gui->isGuiFrozenForPlayback();

    /// For eValueChangedReasonTimeChanged we never call the instanceChangedAction and evaluate otherwise it would just throttle down
    /// the application responsiveness
    if (reason != Natron::eValueChangedReasonTimeChanged && _imp->holder) {
        if ( ( app && !app->getProject()->isLoadingProject() /*&& !app->isCreatingPythonGroup()*/) || !app ) {
            
            if (_imp->holder->isEvaluationBlocked()) {
                _imp->holder->appendValueChange(this,reason);
            } else {
                _imp->holder->beginChanges();
                _imp->holder->appendValueChange(this,reason);
                _imp->holder->endChanges();
            }
            
        }
    }
    
    if (!guiFrozen  && _signalSlotHandler) {
        computeHasModifications();
        _signalSlotHandler->s_valueChanged(dimension,(int)reason);
        _signalSlotHandler->s_updateDependencies(dimension);
        checkAnimationLevel(dimension);
    }
}

void
KnobHelper::setAddNewLine(bool newLine)
{
    _imp->newLine = newLine;
}

bool
KnobHelper::isNewLineActivated() const
{
    return _imp->newLine;
}

void
KnobHelper::setAddSeparator(bool addSep)
{
    _imp->addSeparator = addSep;
}

bool
KnobHelper::isSeparatorActivated() const
{
    return _imp->addSeparator;
}

void
KnobHelper::setSpacingBetweenItems(int spacing)
{
    _imp->itemSpacing = spacing;
}

void
KnobHelper::setEnabled(int dimension,
                       bool b)
{
    _imp->enabled[dimension] = b;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void
KnobHelper::setAllDimensionsEnabled(bool b)
{
    for (U32 i = 0; i < _imp->enabled.size(); ++i) {
        _imp->enabled[i] = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void
KnobHelper::setSecret(bool b)
{
    _imp->IsSecret = b;
    
    ///the knob was revealed , refresh its gui to the current time
    if ( !b && _imp->holder && _imp->holder->getApp() ) {
        onTimeChanged( _imp->holder->getApp()->getTimeLine()->currentFrame() );
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_secretChanged();
    }
}

int
KnobHelper::determineHierarchySize() const
{
    int ret = 0;
    boost::shared_ptr<KnobI> current = getParentKnob();
    
    while (current) {
        ++ret;
        current = current->getParentKnob();
    }
    
    return ret;
}

const std::string &
KnobHelper::getDescription() const
{
    return _imp->description;
}

void
KnobHelper::setDescription(const std::string& description)
{
    _imp->description = description;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_descriptionChanged();
    }
}

void
KnobHelper::hideDescription()
{
    _imp->descriptionVisible = false;
}

bool
KnobHelper::isDescriptionVisible() const
{
    return _imp->descriptionVisible;
}

bool
KnobHelper::hasAnimation() const
{
    
    for (int i = 0; i < getDimension(); ++i) {
        if (getKeyFramesCount(i) > 0) {
            return true;
        }
        if (!getExpression(i).empty()) {
            return true;
        }
    }
    
    return false;
}

static std::size_t getMatchingParenthesisPosition(std::size_t openingParenthesisPos,
                                                  char openingChar,
                                                  char closingChar,
                                                  const std::string& str) {
    assert(openingParenthesisPos < str.size() && str.at(openingParenthesisPos) == openingChar);
    
    int noOpeningParenthesisFound = 0;
    int i = openingParenthesisPos + 1;
    
    while (i < (int)str.size()) {
        if (str.at(i) == closingChar) {
            if (noOpeningParenthesisFound == 0) {
                break;
            } else {
                --noOpeningParenthesisFound;
            }
        } else if (str.at(i) == openingChar) {
            ++noOpeningParenthesisFound;
        }
        ++i;
    }
    if (i >= (int)str.size()) {
        return std::string::npos;
    }
    return i;
}

static bool parseTokenFrom(int fromDim,
                           const std::string& str,
                           const std::string& token,
                           std::size_t inputPos,
                            std::size_t *tokenStart,
                           std::string* output)
{
    std::size_t pos;

//    std::size_t tokenSize;
    bool foundMatchingToken = false;
    while (!foundMatchingToken) {
        
        pos = str.find(token,inputPos);
        if (pos == std::string::npos) {
            return false;
        }
        
        *tokenStart = pos;
        pos += token.size();
        
        ///Find nearest opening parenthesis
        for (; pos < str.size(); ++pos) {
            if (str.at(pos) == '(') {
                foundMatchingToken = true;
                break;
            } else if (str.at(pos) != ' ') {
                //We didn't find a good token
                break;
            }
        }
        
        if (pos >= str.size()) {
            throw std::invalid_argument("Invalid expr");
        }
        
        if (!foundMatchingToken) {
            inputPos = pos;
        }

    }
    
    std::size_t endingParenthesis = getMatchingParenthesisPosition(pos, '(', ')',  str);
    if (endingParenthesis == std::string::npos) {
        throw std::invalid_argument("Invalid expr");
    }
    
    std::stringstream ss;
    ss << ".addAsDependencyOf(" << fromDim << ",thisParam)\n";
    std::string toInsert = ss.str();
    
   // tokenSize = endingParenthesis - tokenStart + 1;
    if (*tokenStart < 2 || str[*tokenStart - 1] != '.') {
        throw std::invalid_argument("Invalid expr");
    }
    
    std::locale loc;
    //Find the start of the symbol
    int i = (int)*tokenStart - 2;
    int nClosingParenthesisMet = 0;
    while (i >= 0) {
        if (str[i] == ')') {
            ++nClosingParenthesisMet;
        }
        if (std::isspace(str[i],loc) ||
            str[i] == '+' ||
            str[i] == '-' ||
            str[i] == '*' ||
            str[i] == '/' ||
            str[i] == '%' ||
            (str[i] == '(' && !nClosingParenthesisMet)) {
            break;
        }
        --i;
    }
    ++i;
    std::string varName = str.substr(i, *tokenStart - 1 - i);
    output->append(varName + toInsert);
    //assert(*tokenSize > 0);
    return true;
}

static bool extractAllOcurrences(const std::string& str,const std::string& token,int fromDim,std::string *outputScript)
{
    
    std::size_t tokenStart;
    bool couldFindToken;
    try {
        couldFindToken = parseTokenFrom(fromDim, str, token, 0, &tokenStart, outputScript);
    } catch (...) {
        return false;
    }
    
    while (couldFindToken) {
        try {
            couldFindToken = parseTokenFrom(fromDim, str, token, tokenStart + 1, &tokenStart, outputScript);
        } catch (...) {
            return false;
        }
    }
    return true;
}

std::string
KnobHelperPrivate::declarePythonVariables(bool addTab, int dim)
{
    if (!holder) {
        throw std::runtime_error("This parameter cannot have an expression");
    }
    
    EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
    if (!effect) {
        throw std::runtime_error("This parameter cannot have an expression");
    }
    
    NodePtr node = effect->getNode();
    assert(node);
    
    boost::shared_ptr<NodeCollection> collection = node->getGroup();
    if (!collection) {
        throw std::runtime_error("This parameter cannot have an expression");
    }
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>(collection.get());
    std::string appID = node->getApp()->getAppIDString();

    std::string tabStr = addTab ? "    " : "";
    std::stringstream ss;
    if (appID != "app") {
        ss << tabStr << "app = " << appID << "\n";
    }
    if (isParentGrp) {
        ss << tabStr << "thisGroup = " << appID << "." << isParentGrp->getNode()->getFullyQualifiedName() << "\n";
    } else {
        ss << tabStr << "thisGroup = " << appID << "\n";
    }
    ss << tabStr << "thisNode = " << appID << "." << node->getFullyQualifiedName() <<  "\n";
    
    ///Now define the variables in the scope
    ss << tabStr << "thisParam = thisNode." << name << "\n";
    ss << tabStr << "random = thisParam.random\n";
    ss << tabStr << "randomInt = thisParam.randomInt\n";
    ss << tabStr << "curve = thisParam.curve\n";
    if (dimension != -1) {
        ss << tabStr << "dimension = " << dim << "\n";
    }
    
    //Define all nodes reachable through expressions in the scope
    std::string mustDeleteContainer;
    if (isParentGrp) {
        std::string containerName = isParentGrp->getNode()->getFullyQualifiedName();
        ss << tabStr << containerName  << " = " << appID << "." <<  containerName  << "\n";
    }
    
    NodeList siblings = collection->getNodes();
    for (NodeList::iterator it = siblings.begin(); it != siblings.end(); ++it) {
        if ((*it)->isActivated() && !(*it)->getParentMultiInstance()) {
            std::string fullName = (*it)->getFullyQualifiedName();
            ss << tabStr << fullName << " = " << appID << "." << fullName << "\n";
        }
    }
    
    NodeGroup* isHolderGrp = dynamic_cast<NodeGroup*>(effect);
    if (isHolderGrp) {
        NodeList children = isHolderGrp->getNodes();
        for (NodeList::iterator it = children.begin(); it != children.end(); ++it) {
            if ((*it)->isActivated() && !(*it)->getParentMultiInstance()) {
                std::string name = (*it)->getFullyQualifiedName();
                ss << tabStr << name << " = " << appID << "." << name << "\n";
            }
        }
    }
    return ss.str();
}

void
KnobHelperPrivate::parseListenersFromExpression(int dimension)
{
    
    //Extract pointers to knobs referred to by the expression
    //Our heuristic is quite simple: we replace in the python code all calls to:
    // - getValue
    // - getValueAtTime
    // - getDerivativeAtTime
    // - getIntegrateFromTimeToTime
    // - get
    // And replace them by addAsDependencyOf(thisParam) which will register the parameters as a dependency of this parameter

    std::string expressionCopy;
    
    {
        QMutexLocker k(&expressionMutex);
        expressionCopy = expressions[dimension].originalExpression;
    }
    
    std::string script;
    if  (!extractAllOcurrences(expressionCopy, "getValue",dimension,&script)) {
        return ;
    }
    
    if (!extractAllOcurrences(expressionCopy, "getValueAtTime", dimension,&script)) {
        return;
    }
    
    if (!extractAllOcurrences(expressionCopy, "getDerivativeAtTime", dimension,&script)) {
        return;
    }
    
    if (!extractAllOcurrences(expressionCopy, "getIntegrateFromTimeToTime", dimension,&script)) {
        return;
    }
    
    if (!extractAllOcurrences(expressionCopy, "get", dimension,&script)) {
        return;
    }
    
    std::string declarations = declarePythonVariables(false, dimension);
    script = declarations + "\n" + script;
    ///This will register the listeners
    std::string error;
    bool success = Natron::interpretPythonScript(script, &error,NULL);
    if (!error.empty()) {
        qDebug() << error.c_str();
    }
    assert(success);
    Q_UNUSED(success);
}



std::string
KnobHelper::validateExpression(const std::string& expression,int dimension,bool hasRetVariable,std::string* resultAsString)
{
    Natron::PythonGILLocker pgl;
    
    if (expression.empty()) {
        throw std::invalid_argument("empty expression");;
    }
    
    
    
    std::string exprCpy = expression;
    
    //if !hasRetVariable the expression is expected to be single-line
    if (!hasRetVariable) {
        std::size_t foundNewLine = expression.find("\n");
        if (foundNewLine != std::string::npos) {
            throw std::invalid_argument("unexpected new line character \'\\n\'");
        }
        //preprend the line with "ret = ..."
        std::string toInsert("    ret = ");
        exprCpy.insert(0, toInsert);
    } else {
        exprCpy.insert(0, "    ");
        std::size_t foundNewLine = exprCpy.find("\n");
        while (foundNewLine != std::string::npos) {
            exprCpy.insert(foundNewLine + 1, "    ");
            foundNewLine = exprCpy.find("\n", foundNewLine + 1);
        }
        
    }
    
    KnobHolder* holder = getHolder();
    if (!holder) {
        throw std::runtime_error("This parameter cannot have an expression");
    }
    
    EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
    if (!effect) {
        throw std::runtime_error("This parameter cannot have an expression");
    }
    
    NodePtr node = effect->getNode();
    assert(node);
    std::string appID = getHolder()->getApp()->getAppIDString();
    std::string exprFuncPrefix = appID + "." + node->getFullyQualifiedName() + "." + getName() + ".";
    std::string exprFuncName;
    {
        std::stringstream tmpSs;
        tmpSs << "expression" << dimension;
        exprFuncName = tmpSs.str();
    }
    
    exprCpy.append("\n    return ret\n");
    
    ///Now define the thisNode variable

    std::stringstream ss;
    ss << "def "  << exprFuncName << "(frame):\n";
    ss << _imp->declarePythonVariables(true, dimension);

    
    std::string script = ss.str();
    script.append(exprCpy);
    script.append(exprFuncPrefix + exprFuncName + " = " + exprFuncName);

    ///Try to compile the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    ///with the error.
    std::string error;
    std::string funcExecScript = "ret = " + exprFuncPrefix + exprFuncName;

    {
        EXPR_RECURSION_LEVEL();
        
        if (!interpretPythonScript(script, &error, 0)) {
            throw std::runtime_error(error);
        }
        
        std::stringstream ss;
        ss << funcExecScript <<'('<<getCurrentTime()<<")\n";
        if (!interpretPythonScript(ss.str(), &error, 0)) {
            throw std::runtime_error(error);
        }
        
        PyObject *ret = PyObject_GetAttrString(Natron::getMainModule(),"ret"); //get our ret variable created above
        
        if (!ret || PyErr_Occurred()) {
#ifdef DEBUG
            PyErr_Print();
#endif
            throw std::runtime_error("return value must be assigned to the \"ret\" variable");
        }
  

        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(this);
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(this);
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(this);
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(this);
        if (isDouble) {
            double r = isDouble->pyObjectToType(ret);
            *resultAsString = QString::number(r).toStdString();
        } else if (isInt) {
            int r = isInt->pyObjectToType(ret);
            *resultAsString = QString::number(r).toStdString();
        } else if (isBool) {
            bool r = isBool->pyObjectToType(ret);
            *resultAsString = r ? "True" : "False";
        } else {
            assert(isString);
            *resultAsString = isString->pyObjectToType(ret);
        }

        
    }
 
    
    return funcExecScript;
}

void
KnobHelper::setExpressionInternal(int dimension,const std::string& expression,bool hasRetVariable,bool clearResults)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON
	return;
#endif
    assert(dimension >= 0 && dimension < getDimension());
    
    Natron::PythonGILLocker pgl;
    
    ///Clear previous expr
    clearExpression(dimension,clearResults);
    
    if (expression.empty()) {
        return ;
    }
    
    std::string exprResult;
    std::string exprCpy = validateExpression(expression, dimension, hasRetVariable,&exprResult);
    
    //Set internal fields

    {
        QMutexLocker k(&_imp->expressionMutex);
        _imp->expressions[dimension].hasRet = hasRetVariable;
        _imp->expressions[dimension].expression = exprCpy;
        _imp->expressions[dimension].originalExpression = expression;
        
        ///This may throw an exception upon failure
        //compilePyScript(exprCpy, &_imp->expressions[dimension].code);
    }
  

    //Parse listeners of the expression, to keep track of dependencies to indicate them to the user.
    if (getHolder()) {
        {
            EXPR_RECURSION_LEVEL();
            _imp->parseListenersFromExpression(dimension);
        }
        getHolder()->updateHasAnimation();
    }
    
    //Notify the expr. has changed
    expressionChanged(dimension);
}

bool
KnobHelper::isExpressionUsingRetVariable(int dimension) const
{
    QMutexLocker k(&_imp->expressionMutex);
    return _imp->expressions[dimension].hasRet;
}

bool
KnobHelper::getExpressionDependencies(int dimension, std::list<KnobI*>& dependencies) const
{
    QMutexLocker k(&_imp->expressionMutex);
    if (!_imp->expressions[dimension].expression.empty()) {
        dependencies = _imp->expressions[dimension].dependencies;
        return true;
    }
    return false;
}

void
KnobHelper::clearExpression(int dimension,bool clearResults)
{
    Natron::PythonGILLocker pgl;
    bool hadExpression;
    {
        QMutexLocker k(&_imp->expressionMutex);
        hadExpression = !_imp->expressions[dimension].originalExpression.empty();
        _imp->expressions[dimension].expression.clear();
        _imp->expressions[dimension].originalExpression.clear();
        //Py_XDECREF(_imp->expressions[dimension].code); //< new ref
        //_imp->expressions[dimension].code = 0;
    }
    {
        std::list<KnobI*> dependencies;
        {
            QWriteLocker kk(&_imp->mastersMutex);
            dependencies = _imp->expressions[dimension].dependencies;
            _imp->expressions[dimension].dependencies.clear();
        }
        for (std::list<KnobI*>::iterator it = dependencies.begin();
             it != dependencies.end(); ++it) {
            
            KnobHelper* other = dynamic_cast<KnobHelper*>(*it);
            assert(other);
            
            std::list<boost::weak_ptr<KnobI> > otherListeners;
            {
                QReadLocker otherMastersLocker(&other->_imp->mastersMutex);
                otherListeners = other->_imp->listeners;
            }
            
            for (std::list<boost::weak_ptr<KnobI> >::iterator it = otherListeners.begin(); it != otherListeners.end(); ++it) {
                boost::shared_ptr<KnobI> knob = it->lock();
                if (knob.get() == this) {
                    if (knob) {
                        if (knob->getHolder() && knob->getSignalSlotHandler() ) {
                            getHolder()->onKnobSlaved(this, other,dimension,false );
                        }
                        QObject::disconnect(other->getSignalSlotHandler().get(), SIGNAL(updateDependencies(int)), _signalSlotHandler.get(),
                                            SLOT(onExprDependencyChanged(int)));
                    }
                    otherListeners.erase(it);
                    break;
                }
            }
            
            {
                QWriteLocker otherMastersLocker(&other->_imp->mastersMutex);
                other->_imp->listeners = otherListeners;
            }
        }
    }
    if (clearResults) {
        clearExpressionsResults(dimension);
    }

    if (hadExpression) {
        expressionChanged(dimension);
    }
    
}

void
KnobHelper::expressionChanged(int dimension)
{
    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }
    
    if (_signalSlotHandler) {
        _signalSlotHandler->s_expressionChanged(dimension);
    }
    computeHasModifications();
}

PyObject*
KnobHelper::executeExpression(double time, int dimension) const
{
    
    std::string expr;
    {
        QMutexLocker k(&_imp->expressionMutex);
        expr = _imp->expressions[dimension].expression;
    }
    
    //returns a new ref, this function's documentation is not clear onto what it returns...
    //https://docs.python.org/2/c-api/veryhigh.html
    PyObject* mainModule = getMainModule();
    PyObject* globalDict = PyModule_GetDict(mainModule);
    
    std::stringstream ss;
    ss << expr << '(' << time << ")\n";
    std::string script = ss.str();
    PyObject* v = PyRun_String(script.c_str(), Py_file_input, globalDict, 0);
    Py_XDECREF(v);
    
    
    
    if (PyErr_Occurred()) {
#ifdef DEBUG
        PyErr_Print();
        ///Gui session, do stdout, stderr redirection
        if (PyObject_HasAttrString(mainModule, "catchErr")) {
            std::string error;
            PyObject* errCatcher = PyObject_GetAttrString(mainModule,"catchErr"); //get our catchOutErr created above, new ref
            PyObject *errorObj = 0;
            if (errCatcher) {
                errorObj = PyObject_GetAttrString(errCatcher,"value"); //get the  stderr from our catchErr object, new ref
                assert(errorObj);
                error = PY3String_asString(errorObj);
                PyObject* unicode = PyUnicode_FromString("");
                PyObject_SetAttrString(errCatcher, "value", unicode);
                Py_DECREF(errorObj);
                Py_DECREF(errCatcher);
                qDebug() << "Expression dump:\n=========================================================";
                qDebug() << expr.c_str();
                qDebug() << error.c_str();
            }

        }

#endif
        throw std::runtime_error("Failed to execute expression");
    }
    PyObject *ret = PyObject_GetAttrString(mainModule,"ret"); //get our ret variable created above
    
    if (!ret || PyErr_Occurred()) {
#ifdef DEBUG
        PyErr_Print();
#endif
        throw std::runtime_error("return value must be assigned to the \"ret\" variable");
    }
    return ret;
    
}

std::string
KnobHelper::getExpression(int dimension) const
{
    if (dimension == -1) {
        dimension = 0;
    }
    QMutexLocker k(&_imp->expressionMutex);
    return _imp->expressions[dimension].originalExpression;
}

KnobHolder*
KnobHelper::getHolder() const
{
    return _imp->holder;
}

void
KnobHelper::setAnimationEnabled(bool val)
{
    if (!canAnimate()) {
        return;
    }
    _imp->isAnimationEnabled = val;
}

bool
KnobHelper::isAnimationEnabled() const
{
    return canAnimate() && _imp->isAnimationEnabled;
}

void
KnobHelper::setName(const std::string & name)
{
    _imp->originalName = name;
    _imp->name = Natron::makeNameScriptFriendly(name);
    
    if (!getHolder()) {
        return;
    }
    //Try to find a duplicate
    int no = 1;
    
    bool foundItem;
    std::string finalName;
    do {
        std::stringstream ss;
        ss << _imp->name;
        if (no > 1) {
            ss << no;
        }
        finalName = ss.str();
        if (getHolder()->getOtherKnobByName(finalName, this)) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);
    _imp->name = finalName;
}

const std::string &
KnobHelper::getName() const
{
    return _imp->name;
}

const std::string &
KnobHelper::getOriginalName() const
{
    return _imp->originalName;
}

void
KnobHelper::setParentKnob(boost::shared_ptr<KnobI> knob)
{
    _imp->parentKnob = knob;
}

boost::shared_ptr<KnobI> KnobHelper::getParentKnob() const
{
    return _imp->parentKnob.lock();
}

bool
KnobHelper::getIsSecret() const
{
    return _imp->IsSecret;
}

void
KnobHelper::setIsFrozen(bool frozen)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_setFrozen(frozen);
    }
}

bool
KnobHelper::isEnabled(int dimension) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    
    return _imp->enabled[dimension];
}

void
KnobHelper::setDirty(bool d)
{
    _signalSlotHandler->s_setDirty(d);
}

void
KnobHelper::setEvaluateOnChange(bool b)
{
    QMutexLocker k(&_imp->evaluateOnChangeMutex);
    _imp->evaluateOnChange = b;
}

bool
KnobHelper::getIsPersistant() const
{
    return _imp->IsPersistant;
}

void
KnobHelper::setIsPersistant(bool b)
{
    _imp->IsPersistant = b;
}

void
KnobHelper::setCanUndo(bool val)
{
    _imp->CanUndo = val;
}

bool
KnobHelper::getCanUndo() const
{
    return _imp->CanUndo;
}

bool
KnobHelper::getEvaluateOnChange() const
{
    QMutexLocker k(&_imp->evaluateOnChangeMutex);
    return _imp->evaluateOnChange;
}

void
KnobHelper::setHintToolTip(const std::string & hint)
{
    _imp->tooltipHint = hint;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
    
}

const std::string &
KnobHelper::getHintToolTip() const
{
    return _imp->tooltipHint;
}

void
KnobHelper::setCustomInteract(const boost::shared_ptr<OfxParamOverlayInteract> & interactDesc)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->customInteract = interactDesc;
}

boost::shared_ptr<OfxParamOverlayInteract> KnobHelper::getCustomInteract() const
{
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->customInteract;
}

void
KnobHelper::swapOpenGLBuffers()
{
}

void
KnobHelper::redraw()
{
    if (_imp->gui) {
        _imp->gui->redraw();
    }
}

void
KnobHelper::getViewportSize(double &width,
                            double &height) const
{
    if (_imp->gui) {
        _imp->gui->getViewportSize(width, height);
    } else {
        width = 0;
        height = 0;
    }
}

void
KnobHelper::getPixelScale(double & xScale,
                          double & yScale) const
{
    if (_imp->gui) {
        _imp->gui->getPixelScale(xScale, yScale);
    } else {
        xScale = 0;
        yScale = 0;
    }
}

void
KnobHelper::getBackgroundColour(double &r,
                                double &g,
                                double &b) const
{
    if (_imp->gui) {
        _imp->gui->getBackgroundColour(r, g, b);
    } else {
        r = 0;
        g = 0;
        b = 0;
    }
}

void
KnobHelper::saveOpenGLContext()
{
    if (_imp->gui) {
        _imp->gui->saveOpenGLContext();
    }
}

void
KnobHelper::restoreOpenGLContext()
{
    if (_imp->gui) {
        _imp->gui->restoreOpenGLContext();
    }
}

void
KnobHelper::setOfxParamHandle(void* ofxParamHandle)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->ofxParamHandle = ofxParamHandle;
}

void*
KnobHelper::getOfxParamHandle() const
{
    assert( QThread::currentThread() == qApp->thread() );
    
    return _imp->ofxParamHandle;
}

bool
KnobHelper::isMastersPersistenceIgnored() const
{
    QReadLocker l(&_imp->mastersMutex);
    
    return _imp->ignoreMasterPersistence;
}

void
KnobHelper::copyAnimationToClipboard() const
{
    if (_imp->gui) {
        _imp->gui->copyAnimationToClipboard();
    }
}

bool
KnobHelper::slaveTo(int dimension,
                    const boost::shared_ptr<KnobI> & other,
                    int otherDimension,
                    Natron::ValueChangedReasonEnum reason,
                    bool ignoreMasterPersistence)
{
    assert(other.get() != this);
    assert( 0 <= dimension && dimension < (int)_imp->masters.size() );
    if (other->isSlave(otherDimension)) {
        return false;
    }

    assert( !other->isSlave(otherDimension) );
    if (dynamic_cast<Button_Knob*>(this)) {
        return false;
    }
    {
        QWriteLocker l(&_imp->mastersMutex);
        if (_imp->masters[dimension].second) {
            return false;
        }
        _imp->ignoreMasterPersistence = ignoreMasterPersistence;
        _imp->masters[dimension].second = other;
        _imp->masters[dimension].first = otherDimension;
    }
    
    KnobHelper* helper = dynamic_cast<KnobHelper*>( other.get() );
    assert(helper);

    if (helper && helper->_signalSlotHandler && _signalSlotHandler) {

        //QObject::connect( helper->_signalSlotHandler.get(), SIGNAL( updateSlaves(int) ), _signalSlotHandler.get(), SLOT( onMasterChanged(int) ) );
        QObject::connect( helper->_signalSlotHandler.get(), SIGNAL( keyFrameSet(SequenceTime,int,int,bool) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameSet(SequenceTime,int,int,bool) ) );
        QObject::connect( helper->_signalSlotHandler.get(), SIGNAL( keyFrameRemoved(SequenceTime,int,int) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameRemoved(SequenceTime,int,int)) );
        
        QObject::connect( helper->_signalSlotHandler.get(), SIGNAL( keyFrameMoved(int,int,int) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameMoved(int,int,int) ) );
        QObject::connect( helper->_signalSlotHandler.get(), SIGNAL(animationRemoved(int) ),
                         _signalSlotHandler.get(), SLOT(onMasterAnimationRemoved(int)) );
        
    }
    
    bool hasChanged = cloneAndCheckIfChanged(other.get(),dimension);
    
    if (_signalSlotHandler) {
        ///Notify we want to refresh
        if (reason == Natron::eValueChangedReasonPluginEdited) {
            _signalSlotHandler->s_knobSlaved(dimension,true);
        }
    }
    
    if (hasChanged) {
        evaluateValueChange(dimension, reason);
    }

    ///Register this as a listener of the master
    if (helper) {
        helper->addListener(false,dimension,shared_from_this());
    }
    
    return true;
}

std::pair<int,boost::shared_ptr<KnobI> > KnobHelper::getMaster(int dimension) const
{
    assert(dimension >= 0 && dimension < (int)_imp->masters.size());
    QReadLocker l(&_imp->mastersMutex);
    
    return _imp->masters[dimension];
}

void
KnobHelper::resetMaster(int dimension)
{
    assert(dimension >= 0);
    _imp->masters[dimension].second.reset();
    _imp->masters[dimension].first = -1;
    _imp->ignoreMasterPersistence = false;
}

bool
KnobHelper::isSlave(int dimension) const
{
    assert(dimension >= 0);
    QReadLocker l(&_imp->mastersMutex);
    
    return bool(_imp->masters[dimension].second);
}

std::vector< std::pair<int,boost::shared_ptr<KnobI> > > KnobHelper::getMasters_mt_safe() const
{
    QReadLocker l(&_imp->mastersMutex);
    
    return _imp->masters;
}

void
KnobHelper::checkAnimationLevel(int dimension)
{
    AnimationLevelEnum level = Natron::eAnimationLevelNone;

    if ( canAnimate() && isAnimated(dimension) && getHolder() && getHolder()->getApp() ) {

        boost::shared_ptr<Curve> c = getCurve(dimension);
        SequenceTime time = getHolder()->getApp()->getTimeLine()->currentFrame();
        if (c->getKeyFramesCount() > 0) {
            KeyFrame kf;
            bool found = c->getKeyFrameWithTime(time, &kf);;
            if (found) {
                level = Natron::eAnimationLevelOnKeyframe;
            } else {
                level = Natron::eAnimationLevelInterpolatedValue;
            }
        } else {
            level = Natron::eAnimationLevelNone;
        }
    }
    setAnimationLevel(dimension,level);
}

void
KnobHelper::setAnimationLevel(int dimension,
                              Natron::AnimationLevelEnum level)
{
    {
        QMutexLocker l(&_imp->animationLevelMutex);
        assert( dimension < (int)_imp->animationLevel.size() );
        _imp->animationLevel[dimension] = level;
    }
    if ( _signalSlotHandler && _imp->gui && !_imp->gui->isGuiFrozenForPlayback() ) {
        if (getExpression(dimension).empty()) {
            _signalSlotHandler->s_animationLevelChanged( dimension,(int)level );
        }
    }
}

Natron::AnimationLevelEnum
KnobHelper::getAnimationLevel(int dimension) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    
    if (master.second) {
        return master.second->getAnimationLevel(master.first);
    }
    
    QMutexLocker l(&_imp->animationLevelMutex);
    if ( dimension > (int)_imp->animationLevel.size() ) {
        throw std::invalid_argument("Knob::getAnimationLevel(): Dimension out of range");
    }
    
    return _imp->animationLevel[dimension];
}

void
KnobHelper::deleteAnimationConditional(int time,int dimension,Natron::ValueChangedReasonEnum reason,bool before)
{
    if (!_imp->curves[dimension]) {
        return;
    }
    assert( 0 <= dimension && dimension < getDimension() );
    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui;
    
    if (!useGuiCurve) {
        if (holder) {
            holder->abortAnyEvaluation();
        }
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(dimension);
        setGuiCurveHasChanged(dimension,true);
    }
    
    std::list<int> keysRemoved;
    if (before) {
        curve->removeKeyFramesBeforeTime(time, &keysRemoved);
    } else {
        curve->removeKeyFramesAfterTime(time, &keysRemoved);
    }
    
    if (!useGuiCurve) {
        
        if (_signalSlotHandler) {
            _signalSlotHandler->s_updateDependencies(dimension);
        }
        checkAnimationLevel(dimension);
        guiCurveCloneInternalCurve(dimension, reason);
        evaluateValueChange(dimension,reason);
    }
    
    if (holder && holder->getApp()) {
        holder->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(keysRemoved, true);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_animationRemoved(dimension);
    }

}

void
KnobHelper::deleteAnimationBeforeTime(int time,
                                      int dimension,
                                      Natron::ValueChangedReasonEnum reason)
{
    deleteAnimationConditional(time, dimension, reason, true);
}

void
KnobHelper::deleteAnimationAfterTime(int time,
                                     int dimension,
                                     Natron::ValueChangedReasonEnum reason)
{
    deleteAnimationConditional(time, dimension, reason, false);
}

bool
KnobHelper::getKeyFrameTime(int index,
                            int dimension,
                            double* time) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !isAnimated(dimension) ) {
        return false;
    }
    boost::shared_ptr<Curve> curve = getCurve(dimension); //< getCurve will return the master's curve if any
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(index, &kf);
    if (ret) {
        *time = kf.getTime();
    }
    
    return ret;
}

bool
KnobHelper::getLastKeyFrameTime(int dimension,
                                double* time) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !canAnimate() || !isAnimated(dimension) ) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    *time = curve->getMaximumTimeCovered();
    
    return true;
}

bool
KnobHelper::getFirstKeyFrameTime(int dimension,
                                 double* time) const
{
    return getKeyFrameTime(0, dimension, time);
}

int
KnobHelper::getKeyFramesCount(int dimension) const
{
    if (!canAnimate() || !isAnimated(dimension)) {
        return 0;
    }

    boost::shared_ptr<Curve> curve = getCurve(dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    return curve->getKeyFramesCount();   //< getCurve will return the master's curve if any
}

bool
KnobHelper::getNearestKeyFrameTime(int dimension,
                                   double time,
                                   double* nearestTime) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !canAnimate() || !isAnimated(dimension) ) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getNearestKeyFrameWithTime(time, &kf);
    if (ret) {
        *nearestTime = kf.getTime();
    }
    
    return ret;
}

int
KnobHelper::getKeyFrameIndex(int dimension,
                             double time) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !canAnimate() || !isAnimated(dimension) ) {
        return -1;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    
    return curve->keyFrameIndex(time);
}

void
KnobHelper::onMasterChanged(KnobI* master,
                            int masterDimension)
{
    ///Map to the good dimension
    assert(QThread::currentThread() == qApp->thread());
    MastersMap masters;
    {
        QReadLocker l(&_imp->mastersMutex);
        masters = _imp->masters;
    }
    for (U32 i = 0; i < masters.size(); ++i) {
        if (masters[i].second.get() == master && masters[i].first == masterDimension) {
            
            if (getExpression(i).empty()) {
                ///We still want to clone the master's dimension because otherwise we couldn't edit the curve e.g in the curve editor
                ///For example we use it for roto knobs where selected beziers have their knobs slaved to the gui knobs
                clone(master,i);
                
                evaluateValueChange(i, Natron::eValueChangedReasonSlaveRefresh);
            }
            
            return;
        }
    }

}


void
KnobHelper::onExprDependencyChanged(KnobI* knob,int /*dimension*/)
{
    std::set<int> dimensionsToEvaluate;
    {
        QMutexLocker k(&_imp->expressionMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            std::list<KnobI*>::iterator found = std::find(_imp->expressions[i].dependencies.begin(),_imp->expressions[i].dependencies.end(),knob);
            if (found != _imp->expressions[i].dependencies.end()) {
                dimensionsToEvaluate.insert(i);
            }
        }
    }
    
    KnobHolder* holder = getHolder();
    for (std::set<int>::const_iterator it = dimensionsToEvaluate.begin(); it != dimensionsToEvaluate.end(); ++it) {
        if (holder && !holder->isSetValueCurrentlyPossible()) {
            holder->abortAnyEvaluation();
            QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
            _imp->mustClearExprResults[*it] = true;
        } else {
            clearExpressionsResults(*it);
            evaluateValueChange(*it, Natron::eValueChangedReasonSlaveRefresh);
        }
    }
}

void
KnobHelper::cloneExpressions(KnobI* other,int dimension)
{
    assert((int)_imp->expressions.size() == getDimension());
    try {
        int dims = std::min(getDimension(),other->getDimension());
        for (int i = 0; i < dims; ++i) {
            if (i == dimension || dimension == -1) {
                std::string expr = other->getExpression(i);
                bool hasRet = other->isExpressionUsingRetVariable(i);
                if (!expr.empty()) {
                    setExpression(i, expr,hasRet);
                    cloneExpressionsResults(other,i);
                }
            }
        }
    } catch(...) {
        ///ignore errors
    }
}

bool
KnobHelper::cloneExpressionsAndCheckIfChanged(KnobI* other,int dimension)
{
    assert((int)_imp->expressions.size() == getDimension());
    bool ret = false;
    try {
        int dims = std::min(getDimension(),other->getDimension());
        for (int i = 0; i < dims; ++i) {
            if (i == dimension || dimension == -1) {
                std::string expr = other->getExpression(i);
                bool hasRet = other->isExpressionUsingRetVariable(i);
                if (!expr.empty() && (expr != _imp->expressions[i].originalExpression || hasRet != _imp->expressions[i].hasRet)) {
                    setExpression(i, expr,hasRet);
                    cloneExpressionsResults(other,i);
                    ret = true;
                }
            }
        }
    } catch(...) {
        ///ignore errors
    }
    return ret;
}

void
KnobHelper::addListener(bool isExpression,int fromExprDimension,const boost::shared_ptr<KnobI>& knob)
{
    assert(fromExprDimension != -1);
    KnobHelper* slave = dynamic_cast<KnobHelper*>(knob.get());
    assert(slave);

    if ( slave && slave->getHolder() && slave->getSignalSlotHandler() && getSignalSlotHandler() ) {
        slave->getHolder()->onKnobSlaved(slave, this,fromExprDimension,true );
    }
    
    if (slave && slave->_signalSlotHandler && _signalSlotHandler) {
        if (!isExpression) {
            QObject::connect(_signalSlotHandler.get() , SIGNAL( updateSlaves(int) ),slave->_signalSlotHandler.get() , SLOT( onMasterChanged(int) ) );
        } else {
            QObject::connect(_signalSlotHandler.get() , SIGNAL( updateDependencies(int) ),slave->_signalSlotHandler.get() , SLOT( onExprDependencyChanged(int) ) );
            
            QMutexLocker k(&slave->_imp->expressionMutex);
            slave->_imp->expressions[fromExprDimension].dependencies.push_back(this);
        }
    }
    if (knob.get() != this) {
        QWriteLocker l(&_imp->mastersMutex);
        
        _imp->listeners.push_back(knob);
        
    }
    
    
}

void
KnobHelper::removeListener(KnobI* knob)
{
    KnobHelper* other = dynamic_cast<KnobHelper*>(knob);
    assert(other);
    if (other && other->_signalSlotHandler && _signalSlotHandler) {
        QObject::disconnect( other->_signalSlotHandler.get(), SIGNAL( updateSlaves(int) ), _signalSlotHandler.get(), SLOT( onMasterChanged(int) ) );
    }
    
    QWriteLocker l(&_imp->mastersMutex);
    for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        if (it->lock().get() == knob) {
            _imp->listeners.erase(it);
            break;
        }
    }
}


void
KnobHelper::getListeners(std::list<boost::shared_ptr<KnobI> > & listeners) const
{
    QReadLocker l(&_imp->mastersMutex);
    for (std::list<boost::weak_ptr<KnobI> >::const_iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->lock();
        if (knob) {
            listeners.push_back(knob);
        }
    }
}

double
KnobHelper::getCurrentTime() const
{
    KnobHolder* holder = getHolder();
    return holder && holder->getApp() ? holder->getCurrentTime() : 0;
}

int
KnobHelper::getCurrentView() const
{
    KnobHolder* holder = getHolder();
    return holder && holder->getApp() ? holder->getCurrentView() : 0;
}



double
KnobHelper::random(double time, unsigned int seed) const
{
    randomSeed(time, seed);
    return random();
}

double
KnobHelper::random(double min,double max) const
{
    QMutexLocker k(&_imp->lastRandomHashMutex);
    _imp->lastRandomHash = hashFunction(_imp->lastRandomHash);
    return ((double)_imp->lastRandomHash / (double)0x100000000LL) * (max - min)  + min;
}

int
KnobHelper::randomInt(double time,unsigned int seed) const
{
    randomSeed(time, seed);
    return randomInt();
}

int
KnobHelper::randomInt(int min,int max) const
{
    return (int)random((double)min,(double)max);
}

struct alias_cast_float
{
    alias_cast_float()
    : raw(0)
    {
    };                          // initialize to 0 in case sizeof(T) < 8
    
    union
    {
        U32 raw;
        float data;
    };
};

void
KnobHelper::randomSeed(double time, unsigned int seed) const
{
    
    U64 hash = 0;
    KnobHolder* holder = getHolder();
    if (holder) {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        if (effect) {
            hash = effect->getHash();
        }
    }
    U32 hash32 = (U32)hash;
    hash32 += seed;
    
    alias_cast_float ac;
    ac.data = (float)time;
    hash32 += ac.raw;
    
    QMutexLocker k(&_imp->lastRandomHashMutex);
    _imp->lastRandomHash = hash32;
}

bool
KnobHelper::hasModifications() const
{
    QMutexLocker k(&_imp->hasModificationsMutex);
    for (int i = 0; i < _imp->dimension; ++i) {
        if (_imp->hasModifications[i]) {
            return true;
        }
    }
    return false;
}

bool
KnobHelper::hasModifications(int dimension) const
{
    if (dimension < 0 || dimension >= _imp->dimension) {
        throw std::invalid_argument("KnobHelper::hasModifications: Dimension out of range");
    }
    QMutexLocker k(&_imp->hasModificationsMutex);
    return _imp->hasModifications[dimension];
}

bool
KnobHelper::setHasModifications(int dimension,bool value,bool lock)
{
    assert(dimension >= 0 && dimension < _imp->dimension);
    bool ret;
    if (lock) {
        QMutexLocker k(&_imp->hasModificationsMutex);
        ret = _imp->hasModifications[dimension] != value;
        _imp->hasModifications[dimension] = value;
    } else {
        assert(!_imp->hasModificationsMutex.tryLock());
        ret = _imp->hasModifications[dimension] != value;
        _imp->hasModifications[dimension] = value;
    }
    return ret;
}

/***************************KNOB HOLDER******************************************/

struct KnobHolder::KnobHolderPrivate
{
    AppInstance* app;
    
    QMutex knobsMutex;
    std::vector< boost::shared_ptr<KnobI> > knobs;
    bool knobsInitialized;
    bool isSlave;
    
    ///Use to count the recursion in the function calls
    /* The image effect actions which may trigger a recursive action call on a single instance are...
     
     kOfxActionBeginInstanceChanged
     kOfxActionInstanceChanged
     kOfxActionEndInstanceChanged
     The interact actions which may trigger a recursive action to be called on the associated plugin instance are...
     
     kOfxInteractActionGainFocus
     kOfxInteractActionKeyDown
     kOfxInteractActionKeyRepeat
     kOfxInteractActionKeyUp
     kOfxInteractActionLoseFocus
     kOfxInteractActionPenDown
     kOfxInteractActionPenMotion
     kOfxInteractActionPenUp
     
     The image effect actions which may be called recursively are...
     
     kOfxActionBeginInstanceChanged
     kOfxActionInstanceChanged
     kOfxActionEndInstanceChanged
     kOfxImageEffectActionGetClipPreferences
     The interact actions which may be called recursively are...
     
     kOfxInteractActionDraw
     
     */
    ThreadStorage<int> actionsRecursionLevel;

    ///Count how many times an overlay needs to be redrawn for the instanceChanged/penMotion/penDown etc... actions
    ///to just redraw it once when the recursion level is back to 0
    QMutex overlayRedrawStackMutex;
    int overlayRedrawStack;

    mutable QMutex paramsEditLevelMutex;
    KnobHolder::MultipleParamsEditEnum paramsEditLevel;
    int paramsEditRecursionLevel;
    
    mutable QMutex evaluationBlockedMutex;
    int evaluationBlocked;
    
    //Set in the begin/endChanges block
    bool canCurrentlySetValue;
    ChangesList knobChanged;
    
    bool changeSignificant;

    QMutex knobsFrozenMutex;
    bool knobsFrozen;
    
    mutable QMutex hasAnimationMutex;
    bool hasAnimation;
    
    DockablePanelI* settingsPanel;
    
    KnobHolderPrivate(AppInstance* appInstance_)
    : app(appInstance_)
    , knobsMutex()
    , knobs()
    , knobsInitialized(false)
    , isSlave(false)
    , actionsRecursionLevel()
    , overlayRedrawStackMutex()
    , overlayRedrawStack(0)
    , paramsEditLevel(eMultipleParamsEditOff)
    , paramsEditRecursionLevel(0)
    , evaluationBlockedMutex(QMutex::Recursive)
    , evaluationBlocked(0)
    , canCurrentlySetValue(true)
    , knobChanged()
    , changeSignificant(false)
    , knobsFrozenMutex()
    , knobsFrozen(false)
    , hasAnimationMutex()
    , hasAnimation(false)
    , settingsPanel(0)

    {
        // Initialize local data on the main-thread
        ///Don't remove the if condition otherwise this will crash because QApp is not initialized yet for Natron settings.
        if (appInstance_) {
            actionsRecursionLevel.localData() = 0;
        }
    }
};

KnobHolder::KnobHolder(AppInstance* appInstance)
: QObject()
, _imp( new KnobHolderPrivate(appInstance) )
{
    QObject::connect(this, SIGNAL(doEndChangesOnMainThread()), this, SLOT(onDoEndChangesOnMainThreadTriggered()));
    QObject::connect(this, SIGNAL(doEvaluateOnMainThread(KnobI*, bool, int)), this,
                     SLOT(onDoEvaluateOnMainThread(KnobI*, bool, int)));
    QObject::connect(this, SIGNAL(doValueChangeOnMainThread(KnobI*,int,int,bool)), this,
                     SLOT(onDoValueChangeOnMainThread(KnobI*,int,int,bool)));
}

KnobHolder::~KnobHolder()
{
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        KnobHelper* helper = dynamic_cast<KnobHelper*>( _imp->knobs[i].get() );
        assert(helper);
        if (helper) {
            helper->_imp->holder = 0;
        }
    }
}

void
KnobHolder::addKnob(boost::shared_ptr<KnobI> k)
{
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker kk(&_imp->knobsMutex);
    _imp->knobs.push_back(k);
}

void
KnobHolder::insertKnob(int index, const boost::shared_ptr<KnobI>& k)
{
    if (index < 0) {
        return;
    }
    QMutexLocker kk(&_imp->knobsMutex);
    
    if (index >= (int)_imp->knobs.size()) {
        _imp->knobs.push_back(k);
    } else {
        std::vector<boost::shared_ptr<KnobI> >::iterator it = _imp->knobs.begin();
        std::advance(it, index);
        _imp->knobs.insert(it, k);
    }
}

void
KnobHolder::setPanelPointer(DockablePanelI* gui)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->settingsPanel = gui;
}

void
KnobHolder::discardPanelPointer()
{
     assert(QThread::currentThread() == qApp->thread());
    _imp->settingsPanel = 0;
}

void
KnobHolder::refreshKnobs()
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->settingsPanel) {
        _imp->settingsPanel->scanForNewKnobs();
        Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
        if (isEffect) {
            isEffect->getNode()->declarePythonFields();
        }
    }
}



void
KnobHolder::removeDynamicKnob(KnobI* knob)
{
    std::vector<boost::shared_ptr<KnobI> > knobs;
    {
        QMutexLocker k(&_imp->knobsMutex);
        knobs = _imp->knobs;
    }
    for (std::vector<boost::shared_ptr<KnobI> >::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (it->get() == knob && (*it)->isDynamicallyCreated()) {
            (*it)->deleteKnob();
            break;
        }
    }
    
    {
        QMutexLocker k(&_imp->knobsMutex);
        for (std::vector<boost::shared_ptr<KnobI> >::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if (it2->get() == knob && (*it2)->isDynamicallyCreated()) {
                _imp->knobs.erase(it2);
                return;
            }
        }
    }
    
}

void
KnobHolder::moveKnobOneStepUp(KnobI* knob)
{
    if (!knob->isUserKnob()) {
        return;
    }
    boost::shared_ptr<KnobI> parent = knob->getParentKnob();
    Group_Knob* parentIsGrp = dynamic_cast<Group_Knob*>(parent.get());
    Page_Knob* parentIsPage = dynamic_cast<Page_Knob*>(parent.get());
    
    //the knob belongs to a group/page , change its index within the group instead
    if (parentIsGrp) {
        parentIsGrp->moveOneStepUp(knob);
    } else if (parentIsPage) {
        parentIsPage->moveOneStepUp(knob);
    }
    
    QMutexLocker k(&_imp->knobsMutex);
    int prevInPage = -1;
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i].get() == knob) {
            if (prevInPage != -1) {
                boost::shared_ptr<KnobI> tmp = _imp->knobs[prevInPage];
                _imp->knobs[prevInPage] = _imp->knobs[i];
                _imp->knobs[i] = tmp;
            }
            break;
        } else {
            if (_imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == knob->getParentKnob())) {
                prevInPage = i;
            }
        }
    }
    
}

void
KnobHolder::moveKnobOneStepDown(KnobI* knob)
{
    if (!knob->isUserKnob()) {
        return;
    }
    boost::shared_ptr<KnobI> parent = knob->getParentKnob();
    Group_Knob* parentIsGrp = dynamic_cast<Group_Knob*>(parent.get());
    Page_Knob* parentIsPage = dynamic_cast<Page_Knob*>(parent.get());
    
    //the knob belongs to a group/page , change its index within the group instead
    if (parentIsGrp) {
        parentIsGrp->moveOneStepDown(knob);
    } else if (parentIsPage) {
        parentIsPage->moveOneStepDown(knob);
    }
    
    
    QMutexLocker k(&_imp->knobsMutex);
    int foundIndex = - 1;
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i].get() == knob) {
            foundIndex = i;
            break;
        }
    }
    assert(foundIndex != -1);
    for (int i = foundIndex + 1; i < (int)_imp->knobs.size(); ++i) {
        if (_imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == knob->getParentKnob())) {
            boost::shared_ptr<KnobI> tmp = _imp->knobs[foundIndex];
            _imp->knobs[foundIndex] = _imp->knobs[i];
            _imp->knobs[i] = tmp;
            break;
        }
    }
    
}

boost::shared_ptr<Page_Knob>
KnobHolder::getOrCreateUserPageKnob() 
{
    {
        QMutexLocker k(&_imp->knobsMutex);
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            if ((*it)->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                return boost::dynamic_pointer_cast<Page_Knob>(*it);
            }
        }
    }
    boost::shared_ptr<Page_Knob> ret = Natron::createKnob<Page_Knob>(this,NATRON_USER_MANAGED_KNOBS_PAGE_LABEL,1,false);
    ret->setAsUserKnob();
    ret->setName(NATRON_USER_MANAGED_KNOBS_PAGE);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<Int_Knob>
KnobHolder::createIntKnob(const std::string& name, const std::string& label,int dimension)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Int_Knob>(existingKnob);
    }
    boost::shared_ptr<Int_Knob> ret = Natron::createKnob<Int_Knob>(this,label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<Double_Knob>
KnobHolder::createDoubleKnob(const std::string& name, const std::string& label,int dimension)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Double_Knob>(existingKnob);
    }
    boost::shared_ptr<Double_Knob> ret = Natron::createKnob<Double_Knob>(this,label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<Color_Knob>
KnobHolder::createColorKnob(const std::string& name, const std::string& label,int dimension)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Color_Knob>(existingKnob);
    }
    boost::shared_ptr<Color_Knob> ret = Natron::createKnob<Color_Knob>(this,label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<Bool_Knob>
KnobHolder::createBoolKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Bool_Knob>(existingKnob);
    }
    boost::shared_ptr<Bool_Knob> ret = Natron::createKnob<Bool_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<Choice_Knob>
KnobHolder::createChoiceKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Choice_Knob>(existingKnob);
    }
    boost::shared_ptr<Choice_Knob> ret = Natron::createKnob<Choice_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<Button_Knob>
KnobHolder::createButtonKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Button_Knob>(existingKnob);
    }
    boost::shared_ptr<Button_Knob> ret = Natron::createKnob<Button_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

//Type corresponds to the Type enum defined in StringParamBase in ParameterWrapper.h
boost::shared_ptr<String_Knob>
KnobHolder::createStringKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<String_Knob>(existingKnob);
    }
    boost::shared_ptr<String_Knob> ret = Natron::createKnob<String_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<File_Knob>
KnobHolder::createFileKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<File_Knob>(existingKnob);
    }
    boost::shared_ptr<File_Knob> ret = Natron::createKnob<File_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<OutputFile_Knob>
KnobHolder::createOuptutFileKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<OutputFile_Knob>(existingKnob);
    }
    boost::shared_ptr<OutputFile_Knob> ret = Natron::createKnob<OutputFile_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<Path_Knob>
KnobHolder::createPathKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Path_Knob>(existingKnob);
    }
    boost::shared_ptr<Path_Knob> ret = Natron::createKnob<Path_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<Group_Knob>
KnobHolder::createGroupKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Group_Knob>(existingKnob);
    }
    boost::shared_ptr<Group_Knob> ret = Natron::createKnob<Group_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<Page_Knob>
KnobHolder::createPageKnob(const std::string& name, const std::string& label)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Page_Knob>(existingKnob);
    }
    boost::shared_ptr<Page_Knob> ret = Natron::createKnob<Page_Knob>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<Parametric_Knob>
KnobHolder::createParametricKnob(const std::string& name, const std::string& label,int nbCurves)
{
    boost::shared_ptr<KnobI> existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<Parametric_Knob>(existingKnob);
    }
    boost::shared_ptr<Parametric_Knob> ret = Natron::createKnob<Parametric_Knob>(this,label, nbCurves, false);
    ret->setName(name);
    ret->setAsUserKnob();
    boost::shared_ptr<Page_Knob> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);
    Natron::EffectInstance* isEffect = dynamic_cast<Natron::EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

void
KnobHolder::onDoEvaluateOnMainThread(KnobI* knob,bool significant,int reason)
{
    assert(QThread::currentThread() == qApp->thread());
    evaluate_public(knob, significant, (Natron::ValueChangedReasonEnum)reason);
}

void
KnobHolder::onDoEndChangesOnMainThreadTriggered()
{
    assert(QThread::currentThread() == qApp->thread());
    endChanges();
}

void
KnobHolder::endChanges(bool discardEverything)
{
    bool isMT = QThread::currentThread() == qApp->thread();
    if (!isMT && !canHandleEvaluateOnChangeInOtherThread()) {
        Q_EMIT doEndChangesOnMainThread();
        return;
    }
    
    bool evaluate = false;
    ChangesList knobChanged;
    bool significant = false;
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);
        
       // std::cout <<"DECR: " << _imp->evaluationBlocked << std::endl;
        
        evaluate = _imp->evaluationBlocked == 1;
        knobChanged = _imp->knobChanged;
        if (evaluate) {
            _imp->knobChanged.clear();
            significant = _imp->changeSignificant;
            _imp->changeSignificant = false;
        }
    }

    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);
        
        if (_imp->evaluationBlocked > 0) {
            --_imp->evaluationBlocked;
        }
    }
    
    
    
    if (!knobChanged.empty() && !discardEverything && evaluate && significant) {
        ChangesList::iterator first = knobChanged.begin();
        Natron::ValueChangedReasonEnum reason = first->reason;
        KnobI* knob = first->knob;
        if (!isMT) {
            Q_EMIT doEvaluateOnMainThread(knob, significant, reason);
        } else {
            evaluate_public(knob, significant, reason);
        }
    }
}

void
KnobHolder::onDoValueChangeOnMainThread(KnobI* knob, int reason, int time, bool originatedFromMT)
{
    assert(QThread::currentThread() == qApp->thread());
    onKnobValueChanged_public(knob, (Natron::ValueChangedReasonEnum)reason, time, originatedFromMT);
}

void
KnobHolder::appendValueChange(KnobI* knob,Natron::ValueChangedReasonEnum reason)
{
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);

        KnobChange k;
        k.reason = reason;
        k.originatedFromMainThread = QThread::currentThread() == qApp->thread();
        k.knob = knob;
        
        if (knob && !knob->isValueChangesBlocked()) {
            if (!k.originatedFromMainThread && !canHandleEvaluateOnChangeInOtherThread()) {
                Q_EMIT doValueChangeOnMainThread(knob, reason, getCurrentTime(), k.originatedFromMainThread);
            } else {
                onKnobValueChanged_public(knob, reason, getCurrentTime(), k.originatedFromMainThread);
            }
        }

        _imp->knobChanged.push_back(k);
        if (knob) {
            _imp->changeSignificant |= knob->getEvaluateOnChange();
        }
    }
}

void
KnobHolder::beginChanges()
{
    /*
     * Start a begin/end block, actually blocking all evaluations (renders) but not value changed callback.
     */
    bool canSet = canSetValue();
    QMutexLocker l(&_imp->evaluationBlockedMutex);
    ++_imp->evaluationBlocked;
    if (_imp->evaluationBlocked == 1) {
        _imp->canCurrentlySetValue = canSet;
    }
    //std::cout <<"INCR: " << _imp->evaluationBlocked << std::endl;
}

bool
KnobHolder::isEvaluationBlocked() const
{
    QMutexLocker l(&_imp->evaluationBlockedMutex);
    
    return _imp->evaluationBlocked > 0;
}

bool
KnobHolder::isSetValueCurrentlyPossible() const
{
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);
        if (_imp->evaluationBlocked > 0) {
            return _imp->canCurrentlySetValue;
        }
    }
    return canSetValue();
}

KnobHolder::MultipleParamsEditEnum
KnobHolder::getMultipleParamsEditLevel() const
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);
    
    return _imp->paramsEditLevel;
}


void
KnobHolder::setMultipleParamsEditLevel(KnobHolder::MultipleParamsEditEnum level)
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);

    if (level == KnobHolder::eMultipleParamsEditOff) {
        if (_imp->paramsEditRecursionLevel > 0) {
            --_imp->paramsEditRecursionLevel;
        }
        if (_imp->paramsEditRecursionLevel == 0) {
            _imp->paramsEditLevel = KnobHolder::eMultipleParamsEditOff;
        }
        endChanges();

    } else if (level == KnobHolder::eMultipleParamsEditOn) {
        _imp->paramsEditLevel = level;
    } else {
        assert(level == KnobHolder::eMultipleParamsEditOnCreateNewCommand);
        beginChanges();
        if (_imp->paramsEditLevel == KnobHolder::eMultipleParamsEditOff) {
            _imp->paramsEditLevel = KnobHolder::eMultipleParamsEditOnCreateNewCommand;
        }
        ++_imp->paramsEditRecursionLevel;
    }
}

AppInstance*
KnobHolder::getApp() const
{
    return _imp->app;
}

void
KnobHolder::initializeKnobsPublic()
{
    initializeKnobs();
    _imp->knobsInitialized = true;
}

void
KnobHolder::onGuiFrozenChange(bool frozen)
{
    ///The issue with this is if the user toggles off the global frozen mode
    ///and the knobs are already frozen because for instance they are already rendering something
    ///that would unfrozen them, though this is very unlikely that the user does it.
    setKnobsFrozen(frozen);
}

void
KnobHolder::refreshAfterTimeChange(SequenceTime time)
{
    assert(QThread::currentThread() == qApp->thread());
    if (!getApp() || getApp()->isGuiFrozen()) {
        return;
    }
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->onTimeChanged(time);
    }
}

void
KnobHolder::refreshInstanceSpecificKnobsOnly(SequenceTime time)
{
    assert(QThread::currentThread() == qApp->thread());
    if (!getApp() || getApp()->isGuiFrozen()) {
        return;
    }
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if ( _imp->knobs[i]->isInstanceSpecific() ) {
            _imp->knobs[i]->onTimeChanged(time);
        }
    }
}

boost::shared_ptr<KnobI> KnobHolder::getKnobByName(const std::string & name) const
{
    QMutexLocker k(&_imp->knobsMutex);
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }
    
    return boost::shared_ptr<KnobI>();
}

// Same as getKnobByName expect that if we find the caller, we skip it
boost::shared_ptr<KnobI>
KnobHolder::getOtherKnobByName(const std::string & name,const KnobI* caller) const
{
    QMutexLocker k(&_imp->knobsMutex);
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i].get() == caller) {
            continue;
        }
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }
    
    return boost::shared_ptr<KnobI>();
 
}

const std::vector< boost::shared_ptr<KnobI> > &
KnobHolder::getKnobs() const
{
    
    assert(QThread::currentThread() == qApp->thread());
    return _imp->knobs;
}

std::vector< boost::shared_ptr<KnobI> >
KnobHolder::getKnobs_mt_safe() const
{
    QMutexLocker k(&_imp->knobsMutex);
    return _imp->knobs;
}

void
KnobHolder::slaveAllKnobs(KnobHolder* other,bool restore)
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->isSlave) {
        return;
    }
    ///Call it prior to slaveTo: it will set the master pointer as pointing to other
    onAllKnobsSlaved(true,other);

    ///When loading a project, we don't need to slave all knobs here because the serialization of each knob separatly
    ///will reslave it correctly if needed
    if (!restore) {
        beginChanges();
        
        const std::vector<boost::shared_ptr<KnobI> > & otherKnobs = other->getKnobs();
        const std::vector<boost::shared_ptr<KnobI> > & thisKnobs = getKnobs();
        for (U32 i = 0; i < otherKnobs.size(); ++i) {
            
            if (otherKnobs[i]->isDeclaredByPlugin() || otherKnobs[i]->isUserKnob()) {
                boost::shared_ptr<KnobI> foundKnob;
                for (U32 j = 0; j < thisKnobs.size(); ++j) {
                    if ( thisKnobs[j]->getName() == otherKnobs[i]->getName() ) {
                        foundKnob = thisKnobs[j];
                        break;
                    }
                }
                assert(foundKnob);
                int dims = foundKnob->getDimension();
                for (int j = 0; j < dims; ++j) {
                    foundKnob->slaveTo(j, otherKnobs[i], j);
                }
            }
        }
        endChanges();
    }
    _imp->isSlave = true;
}

bool
KnobHolder::isSlave() const
{
    return _imp->isSlave;
}

void
KnobHolder::unslaveAllKnobs()
{
    if (!_imp->isSlave) {
        return;
    }
    const std::vector<boost::shared_ptr<KnobI> > & thisKnobs = getKnobs();
    beginChanges();
    for (U32 i = 0; i < thisKnobs.size(); ++i) {
        int dims = thisKnobs[i]->getDimension();
        for (int j = 0; j < dims; ++j) {
            if ( thisKnobs[i]->isSlave(j) ) {
                thisKnobs[i]->unSlave(j,true);
            }
        }
    }
    endChanges();
    _imp->isSlave = false;
    onAllKnobsSlaved(false,(KnobHolder*)NULL);
}

void
KnobHolder::beginKnobsValuesChanged_public(Natron::ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    
    RECURSIVE_ACTION();
    beginKnobsValuesChanged(reason);
}

void
KnobHolder::endKnobsValuesChanged_public(Natron::ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    
    RECURSIVE_ACTION();
    endKnobsValuesChanged(reason);
}

void
KnobHolder::onKnobValueChanged_public(KnobI* k,
                                      Natron::ValueChangedReasonEnum reason,
                                      SequenceTime time,
                                      bool originatedFromMainThread)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->knobsInitialized) {
        return;
    }
    RECURSIVE_ACTION();
    onKnobValueChanged(k, reason,time, originatedFromMainThread);
}

void
KnobHolder::evaluate_public(KnobI* knob,
                            bool isSignificant,
                            Natron::ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    
    evaluate(knob, isSignificant,reason);
    
    if ( isSignificant && getApp() ) {
        ///Don't trigger autosaves for buttons
        Button_Knob* isButton = dynamic_cast<Button_Knob*>(knob);
        if (!isButton) {
            getApp()->triggerAutoSave();
        }
    }
    
}

void
KnobHolder::checkIfRenderNeeded()
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if ( (getRecursionLevel() == 0)) {
        endChanges();
    }
}

void
KnobHolder::assertActionIsNotRecursive() const
{
# ifdef DEBUG
    
    ///Only check recursions which are on a render threads, because we do authorize recursions in getRegionOfDefinition and such which
    ///always happen in the main thread.
    if (QThread::currentThread() != qApp->thread()) {
        int recursionLvl = getRecursionLevel();
        
        if ( getApp() && getApp()->isShowingDialog() ) {
            return;
        }
        if (recursionLvl != 0) {
            qDebug() << "A non-recursive action has been called recursively.";
        }
    }
# endif // DEBUG
}

void
KnobHolder::incrementRedrawNeededCounter()
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        ++_imp->overlayRedrawStack;
    }
}

bool
KnobHolder::checkIfOverlayRedrawNeeded()
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        bool ret = _imp->overlayRedrawStack > 0;
        _imp->overlayRedrawStack = 0;
        return ret;
    }
}

void
KnobHolder::incrementRecursionLevel()
{
    if ( !_imp->actionsRecursionLevel.hasLocalData() ) {
        _imp->actionsRecursionLevel.localData() = 1;
    } else {
        _imp->actionsRecursionLevel.localData() += 1;
    }
    
    /*NamedKnobHolder* named = dynamic_cast<NamedKnobHolder*>(this);
     if (named) {
     std::cout << named->getScriptName_mt_safe() <<  " INCR: " << _imp->actionsRecursionLevel.localData() <<  " ( "<<
     QThread::currentThread() <<
     " ) main-thread = " << (QThread::currentThread() == qApp->thread()) << std::endl;
     }
     */
}

void
KnobHolder::decrementRecursionLevel()
{
    assert( _imp->actionsRecursionLevel.hasLocalData() );
    _imp->actionsRecursionLevel.localData() -= 1;
    /*NamedKnobHolder* named = dynamic_cast<NamedKnobHolder*>(this);
     if (named) {
     std::cout << named->getScriptName_mt_safe() << " DECR: "<< _imp->actionsRecursionLevel.localData() <<  " ( "<<QThread::currentThread() <<
     " ) main-thread = " << (QThread::currentThread() == qApp->thread()) << std::endl;
     }
     */
}

int
KnobHolder::getRecursionLevel() const
{
    
    if ( _imp->actionsRecursionLevel.hasLocalData() ) {
        /* const NamedKnobHolder* named = dynamic_cast<const NamedKnobHolder*>(this);
         if (named) {
         std::cout << named->getScriptName_mt_safe() << " GET: "<< _imp->actionsRecursionLevel.localData() <<  " ( "<<
         QThread::currentThread() <<
         " ) main-thread = " << (QThread::currentThread() == qApp->thread()) << std::endl;
         }*/
        return _imp->actionsRecursionLevel.localData();
    } else {
        /*const NamedKnobHolder* named = dynamic_cast<const NamedKnobHolder*>(this);
         if (named) {
         std::cout << named->getScriptName_mt_safe() << "GET: "<< 0 <<  "( "<< QThread::currentThread() <<
         " ) main-thread = " << (QThread::currentThread() == qApp->thread()) << std::endl;
         }
         */
        return 0;
    }
}

void
KnobHolder::restoreDefaultValues()
{
    assert(QThread::currentThread() == qApp->thread());
    
    aboutToRestoreDefaultValues();
    
    beginChanges();

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        Button_Knob* isBtn = dynamic_cast<Button_Knob*>( _imp->knobs[i].get() );
        Page_Knob* isPage = dynamic_cast<Page_Knob*>( _imp->knobs[i].get() );
        Group_Knob* isGroup = dynamic_cast<Group_Knob*>( _imp->knobs[i].get() );
        Separator_Knob* isSeparator = dynamic_cast<Separator_Knob*>( _imp->knobs[i].get() );
        
        ///Don't restore buttons and the node label
        if ( !isBtn && !isPage && !isGroup && !isSeparator && (_imp->knobs[i]->getName() != kUserLabelKnobName) ) {
            for (int d = 0; d < _imp->knobs[i]->getDimension(); ++d) {
                _imp->knobs[i]->resetToDefaultValue(d);
            }
        }
    }
    endChanges();
}

void
KnobHolder::setKnobsFrozen(bool frozen)
{
    {
        QMutexLocker l(&_imp->knobsFrozenMutex);
        if (frozen == _imp->knobsFrozen) {
            return;
        }
        _imp->knobsFrozen = frozen;
    }
    std::vector<boost::shared_ptr<KnobI> >  knobs = getKnobs_mt_safe();
    
    for (U32 i = 0; i < knobs.size(); ++i) {
        knobs[i]->setIsFrozen(frozen);
    }
}

bool
KnobHolder::areKnobsFrozen() const
{
    QMutexLocker l(&_imp->knobsFrozenMutex);
    return _imp->knobsFrozen;
}

void
KnobHolder::dequeueValuesSet()
{
    assert(QThread::currentThread() == qApp->thread());
    
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->dequeueValuesSet(false);
    }
}

double
KnobHolder::getCurrentTime() const
{
    return getApp() ? getApp()->getTimeLine()->currentFrame() : 0;
}

void
KnobHolder::discardAppPointer()
{
    _imp->app = 0;
}


bool
KnobHolder::getHasAnimation() const
{
    QMutexLocker k(&_imp->hasAnimationMutex);
    return _imp->hasAnimation;
}


void
KnobHolder::setHasAnimation(bool hasAnimation)
{
    QMutexLocker k(&_imp->hasAnimationMutex);
    _imp->hasAnimation = hasAnimation;
}


void
KnobHolder::updateHasAnimation()
{
    
    bool hasAnimation = false;
    {
        QMutexLocker l(&_imp->knobsMutex);
        
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            if ((*it)->hasAnimation()) {
                hasAnimation = true;
                break;
            }
        }
    }
    QMutexLocker k(&_imp->hasAnimationMutex);
    _imp->hasAnimation = hasAnimation;
}

/***************************STRING ANIMATION******************************************/
void
AnimatingString_KnobHelper::cloneExtraData(KnobI* other,int /*dimension*/ )
{
    AnimatingString_KnobHelper* isAnimatedString = dynamic_cast<AnimatingString_KnobHelper*>(other);
    
    if (isAnimatedString) {
        _animation->clone( isAnimatedString->getAnimation() );
    }
}

bool
AnimatingString_KnobHelper::cloneExtraDataAndCheckIfChanged(KnobI* other,int /*dimension*/)
{
    AnimatingString_KnobHelper* isAnimatedString = dynamic_cast<AnimatingString_KnobHelper*>(other);
    
    if (isAnimatedString) {
       return  _animation->cloneAndCheckIfChanged( isAnimatedString->getAnimation() );
    }
    return false;
}

void
AnimatingString_KnobHelper::cloneExtraData(KnobI* other,
                                           SequenceTime offset,
                                           const RangeD* range,
                                           int /*dimension*/)
{
    AnimatingString_KnobHelper* isAnimatedString = dynamic_cast<AnimatingString_KnobHelper*>(other);
    
    if (isAnimatedString) {
        _animation->clone(isAnimatedString->getAnimation(), offset, range);
    }
}

AnimatingString_KnobHelper::AnimatingString_KnobHelper(KnobHolder* holder,
                                                       const std::string &description,
                                                       int dimension,
                                                       bool declaredByPlugin)
: Knob<std::string>(holder,description,dimension,declaredByPlugin)
, _animation( new StringAnimationManager(this) )
{
}

AnimatingString_KnobHelper::~AnimatingString_KnobHelper()
{
    delete _animation;
}

void
AnimatingString_KnobHelper::stringToKeyFrameValue(int time,
                                                  const std::string & v,
                                                  double* returnValue)
{
    _animation->insertKeyFrame(time, v, returnValue);
}

void
AnimatingString_KnobHelper::stringFromInterpolatedValue(double interpolated,
                                                        std::string* returnValue) const
{
    _animation->stringFromInterpolatedIndex(interpolated, returnValue);
}

void
AnimatingString_KnobHelper::animationRemoved_virtual(int /*dimension*/)
{
    _animation->clearKeyFrames();
}

void
AnimatingString_KnobHelper::keyframeRemoved_virtual(int /*dimension*/,
                                                    double time)
{
    _animation->removeKeyFrame(time);
}

std::string
AnimatingString_KnobHelper::getStringAtTime(double time,
                                            int dimension) const
{
    std::string ret;
    
    if ( _animation->hasCustomInterp() ) {
        bool succeeded = _animation->customInterpolation(time, &ret);
        if (!succeeded) {
            return getValue(dimension);
        } else {
            return ret;
        }
    }
    
    return ret;
}

void
AnimatingString_KnobHelper::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                                   void* ofxParamHandle)
{
    _animation->setCustomInterpolation(func, ofxParamHandle);
}

void
AnimatingString_KnobHelper::loadAnimation(const std::map<int,std::string> & keyframes)
{
    _animation->load(keyframes);
}

void
AnimatingString_KnobHelper::saveAnimation(std::map<int,std::string>* keyframes) const
{
    _animation->save(keyframes);
}

/***************************KNOB EXPLICIT TEMPLATE INSTANTIATION******************************************/


template class Knob<int>;
template class Knob<double>;
template class Knob<bool>;
template class Knob<std::string>;

