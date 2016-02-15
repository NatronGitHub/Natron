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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Knob.h"
#include "KnobImpl.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include <QtCore/QDataStream>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Curve.h"
#include "Engine/DockablePanelI.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/TLSHolder.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

using std::make_pair; using std::pair;

KnobSignalSlotHandler::KnobSignalSlotHandler(const KnobPtr& knob)
: QObject()
, k(knob)
{

}

void
KnobSignalSlotHandler::onAnimationRemoved(ViewSpec view,
                                          int dimension)
{
    getKnob()->onAnimationRemoved(view,dimension);
}


void
KnobSignalSlotHandler::onMasterKeyFrameSet(double time,
                                           ViewSpec view,
                                           int dimension,
                                           int reason,
                                           bool added)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    KnobPtr master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameSet(time, view, dimension, reason, added);
}

void
KnobSignalSlotHandler::onMasterKeyFrameRemoved(double time,
                                               ViewSpec view,
                                               int dimension,
                                               int reason)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    KnobPtr master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameRemoved(time, view, dimension, reason);
}

void
KnobSignalSlotHandler::onMasterKeyFrameMoved(ViewSpec view,
                                             int dimension,
                                             double oldTime,
                                             double newTime)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    KnobPtr master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameMoved(view, dimension, oldTime, newTime);
}

void
KnobSignalSlotHandler::onMasterAnimationRemoved(ViewSpec view,
                                                int dimension)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    assert(handler);
    KnobPtr master = handler->getKnob();
    
    getKnob()->clone(master.get(), dimension);
    Q_EMIT animationRemoved(view,dimension);
}


/***************** KNOBI**********************/

bool
KnobI::slaveTo(int dimension,
               const KnobPtr & other,
               int otherDimension,
               bool ignoreMasterPersistence)
{
    return slaveTo(dimension, other, otherDimension, eValueChangedReasonNatronInternalEdited, ignoreMasterPersistence);
}

void
KnobI::onKnobSlavedTo(int dimension,
                      const KnobPtr &  other,
                      int otherDimension)
{
    slaveTo(dimension, other, otherDimension, eValueChangedReasonUserEdited);
}

void
KnobI::unSlave(int dimension,
               bool copyState)
{
    unSlave(dimension, eValueChangedReasonNatronInternalEdited,copyState);
}

void
KnobI::onKnobUnSlaved(int dimension)
{
    unSlave(dimension, eValueChangedReasonUserEdited,true);
}

void
KnobI::removeAnimation(ViewSpec view,
                       int dimension)
{
    if (canAnimate()) {
        removeAnimation(view, dimension, eValueChangedReasonNatronInternalEdited);
    }
}


void
KnobI::onAnimationRemoved(ViewSpec view,
                          int dimension)
{
    if (canAnimate()) {
        removeAnimation(view, dimension, eValueChangedReasonUserEdited);
    }
}

boost::shared_ptr<KnobPage>
KnobI::getTopLevelPage()
{
    KnobPtr parentKnob = getParentKnob();
    KnobPtr parentKnobTmp = parentKnob;
    while (parentKnobTmp) {
        KnobPtr parent = parentKnobTmp->getParentKnob();
        if (!parent) {
            break;
        } else {
            parentKnobTmp = parent;
        }
    }

    ////find in which page the knob should be
    boost::shared_ptr<KnobPage> isTopLevelParentAPage = boost::dynamic_pointer_cast<KnobPage>(parentKnobTmp);
    return isTopLevelParentAPage;
}


/***********************************KNOB HELPER******************************************/

///for each dimension, the dimension of the master this dimension is linked to, and a pointer to the master
typedef std::vector< std::pair< int,KnobPtr > > MastersMap;

///a curve for each dimension
typedef std::vector< boost::shared_ptr<Curve> > CurvesMap;

struct Expr
{
    std::string expression; //< the one modified by Natron
    std::string originalExpression; //< the one input by the user
    
    bool hasRet;
    
    ///The list of pair<knob, dimension> dpendencies for an expression
    std::list< std::pair<KnobI*,int> > dependencies;
    
    //PyObject* code;
    
    Expr() : expression(), originalExpression(), hasRet(false) /*, code(0)*/{}
};


struct KnobHelperPrivate
{
    KnobHelper* publicInterface;

#ifdef DEBUG
#pragma message WARN("This should be a weak_ptr")
#endif
    KnobHolder* holder;
    std::string label; //< the text label that will be displayed  on the GUI
    bool labelVisible;
    std::string name; //< the knob can have a name different than the label displayed on GUI.
    std::string originalName; //< the original name passed to setName() by the user
    //By default this is the same as _description but can be set by calling setName().
    bool newLine;
    bool addSeparator;
    int itemSpacing;
    boost::weak_ptr<KnobI> parentKnob;
    
    mutable QMutex stateMutex; // protects IsSecret defaultIsSecret enabled
    bool IsSecret,defaultIsSecret;
    std::vector<bool> enabled,defaultEnabled;
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
    
    //Used when this knob is an alias of another knob. The other knob is set in "slaveForAlias"
    KnobPtr slaveForAlias;
    
    ///This is a list of all the knobs that have expressions/links to this knob.
    KnobI::ListenerDimsMap listeners;
    
    mutable QMutex animationLevelMutex;
    std::vector<AnimationLevelEnum> animationLevel; //< indicates for each dimension whether it is static/interpolated/onkeyframe
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
    boost::shared_ptr<TLSHolder<KnobHelper::KnobTLSData> > tlsData;
    
    mutable QMutex hasModificationsMutex;
    std::vector<bool> hasModifications;
    
    mutable QMutex valueChangedBlockedMutex;
    bool valueChangedBlocked;
    
    bool isClipPreferenceSlave;
    
    KnobHelperPrivate(KnobHelper* publicInterface_,
                      KnobHolder*  holder_,
                      int dimension_,
                      const std::string & label_,
                      bool declaredByPlugin_)
    : publicInterface(publicInterface_)
    , holder(holder_)
    , label(label_)
    , labelVisible(true)
    , name( label_.c_str() )
    , originalName(label_.c_str())
    , newLine(true)
    , addSeparator(false)
    , itemSpacing(0)
    , parentKnob()
    , stateMutex()
    , IsSecret(false)
    , defaultIsSecret(false)
    , enabled(dimension_)
    , defaultEnabled(dimension_)
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
    , slaveForAlias()
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
    , tlsData(new TLSHolder<KnobHelper::KnobTLSData>())
    , hasModificationsMutex()
    , hasModifications()
    , valueChangedBlockedMutex()
    , valueChangedBlocked(false)
    , isClipPreferenceSlave(false)
    {
        mustCloneGuiCurves.resize(dimension);
        mustCloneInternalCurves.resize(dimension);
        mustClearExprResults.resize(dimension);
        expressions.resize(dimension);
        hasModifications.resize(dimension);
        for (int i = 0; i < dimension_; ++i) {
            defaultEnabled[i] = enabled[i] = true;
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
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
: _signalSlotHandler()
, _imp( new KnobHelperPrivate(this, holder, dimension, label, declaredByPlugin) )
{
}

KnobHelper::~KnobHelper()
{
    
}

void
KnobHelper::incrementExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();
    assert(tls);
    ++tls->expressionRecursionLevel;
}

void
KnobHelper::decrementExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getTLSData();
    assert(tls);
    assert(tls->expressionRecursionLevel > 0);
    --tls->expressionRecursionLevel;
}

int
KnobHelper::getExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getTLSData();
    if (!tls) {
        return 0;
    }
    return tls->expressionRecursionLevel;
}

void
KnobHelper::deleteKnob()
{
    KnobI::ListenerDimsMap listenersCpy = _imp->listeners;
    for (ListenerDimsMap::iterator it = listenersCpy.begin(); it != listenersCpy.end(); ++it) {
        KnobPtr knob = it->first.lock();
        if (!knob) {
            continue;
        }
        KnobPtr aliasKnob = knob->getAliasMaster();
        if (aliasKnob.get() == this) {
            knob->setKnobAsAliasOfThis(aliasKnob, false);
        }
        for (int i = 0; i < knob->getDimension(); ++i) {
            knob->clearExpression(i,true);
            knob->unSlave(i, false);
        }
        
    }
    
    for (int i = 0; i < getDimension(); ++i) {
        clearExpression(i, true);
    }
    
    KnobPtr parent = _imp->parentKnob.lock();
    if (parent) {
        KnobGroup* isGrp =  dynamic_cast<KnobGroup*>(parent.get());
        KnobPage* isPage = dynamic_cast<KnobPage*>(parent.get());
        if (isGrp) {
            isGrp->removeKnob(this);
        } else if (isPage) {
            isPage->removeKnob(this);
        } else {
            assert(false);
        }
    }
    KnobGroup* isGrp =  dynamic_cast<KnobGroup*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    if (isGrp) {
        KnobsVec children = isGrp->getChildren();
        for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
            _imp->holder->removeDynamicKnob(it->get());
        }
    } else if (isPage) {
        KnobsVec children = isPage->getChildren();
        for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
            _imp->holder->removeDynamicKnob(it->get());
        }
    }
   
    KnobHolder* holder = getHolder();
    if (holder) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if (useHostOverlayHandle()) {
                effect->getNode()->removePositionHostOverlay(this);
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
KnobHelper::getAllDimensionVisible() const
{
    if (getKnobGuiPointer()) {
        return getKnobGuiPointer()->getAllDimensionsVisible();
    }
    return true;
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
    
    KnobPtr thisKnob = shared_from_this();
    boost::shared_ptr<KnobSignalSlotHandler> handler( new KnobSignalSlotHandler(thisKnob) );
    setSignalSlotHandler(handler);

    KnobColor* isColor = dynamic_cast<KnobColor*>(this);
    KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(this);
    if (isSep) {
        _imp->IsPersistant = false;
    }
    for (int i = 0; i < _imp->dimension; ++i) {
        _imp->enabled[i] = true;
        if (canAnimate()) {
            _imp->curves[i] = boost::shared_ptr<Curve>( new Curve(this,i) );
        }
        _imp->animationLevel[i] = eAnimationLevelNone;
        
        
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
KnobHelper::deleteValueAtTime(CurveChangeReason curveChangeReason,
                              double time,
                              ViewSpec view,
                              int dimension)
{
    if ( dimension > (int)_imp->curves.size() || dimension < 0) {
        throw std::invalid_argument("KnobHelper::deleteValueAtTime(): Dimension out of range");
    }

    if (!canAnimate() || !isAnimated(dimension, view)) {
        return;
    }
    
    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
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


    ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited;
    
    if (!useGuiCurve) {
        

        checkAnimationLevel(view, dimension);
        guiCurveCloneInternalCurve(curveChangeReason,view,dimension, reason);
        evaluateValueChange(dimension, time,view, reason);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_redrawGuiCurve(curveChangeReason, view, dimension);
        }
    }
    
    if (_signalSlotHandler/* && reason != eValueChangedReasonUserEdited*/) {
        _signalSlotHandler->s_keyFrameRemoved(time,view, dimension,(int)reason);
    }
    
}

void
KnobHelper::onKeyFrameRemoved(double time,
                              ViewSpec view,
                              int dimension)
{
    deleteValueAtTime(eCurveChangeReasonInternal, time,view,dimension);
}

bool
KnobHelper::moveValueAtTime(CurveChangeReason reason,
                            double time,
                            ViewSpec view,
                            int dimension,
                            double dt,
                            double dv,
                            KeyFrame* newKey)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());

    if (!canAnimate() || !isAnimated(dimension, view)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    /*
     We write on the "GUI" curve if the engine is either:
     - using it
     - is still marked as different from the "internal" curve
     */
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }
    assert(curve);
    
    if (!curve->moveKeyFrameValueAndTime(time, dt, dv, newKey)) {
        return false;
    }
    

    ///Make sure string animation follows up
    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>(this);
    std::string v;
    if (isString) {
        isString->stringFromInterpolatedValue(time, view, &v);
    }
    keyframeRemoved_virtual(dimension, time);
    if (isString) {
        double ret;
        isString->stringToKeyFrameValue(newKey->getTime(), view, v, &ret);
    }
    
    
    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameMoved(view, dimension, time,newKey->getTime());
    }
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, newKey->getTime(), view, eValueChangedReasonNatronInternalEdited);
        
        //We've set the internal curve, so synchronize the gui curve to the internal curve
        //the s_redrawGuiCurve signal will be emitted
        guiCurveCloneInternalCurve(reason, view, dimension, eValueChangedReasonNatronInternalEdited);
    } else {
        //notify that the gui curve has changed to redraw it
        if (_signalSlotHandler) {
            _signalSlotHandler->s_redrawGuiCurve(reason, view, dimension);
        }
    }
    return true;
    
}

bool
KnobHelper::transformValueAtTime(CurveChangeReason curveChangeReason,
                                 double time,
                                 ViewSpec view,
                                 int dimension,
                                 const Transform::Matrix3x3& matrix,
                                 KeyFrame* newKey)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());
    
    if (!canAnimate() || !isAnimated(dimension, view)) {
        return false;
    }
    
    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
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
    
    
    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        p.y = std::floor(p.y + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        p.y = p.y < 0.5 ? 0 : 1;
    }
    
    ///Make sure string animation follows up
    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>(this);
    std::string v;
    if (isString) {
        isString->stringFromInterpolatedValue(k.getValue(), view, &v);
    }
    keyframeRemoved_virtual(dimension, time);
    if (isString) {
        double ret;
        isString->stringToKeyFrameValue(p.x, view, v, &ret);
    }
    
    
    try {
        *newKey = curve->setKeyFrameValueAndTime(p.x,p.y, keyindex, NULL);
    } catch (...) {
        return false;
    }
    
    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameMoved(view, dimension, time,p.x);
    }
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension,p.x, view,  eValueChangedReasonNatronInternalEdited);
        guiCurveCloneInternalCurve(curveChangeReason, view, dimension , eValueChangedReasonNatronInternalEdited);
    }
    return true;

}

void
KnobHelper::cloneCurve(ViewSpec view,
                       int dimension,
                       const Curve& curve)
{
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());
    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> thisCurve;
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    if (!useGuiCurve) {
        thisCurve = _imp->curves[dimension];
    } else {
        thisCurve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }
    assert(thisCurve);
    
    if (_signalSlotHandler) {
        _signalSlotHandler->s_animationAboutToBeRemoved(view, dimension);
        _signalSlotHandler->s_animationRemoved(view, dimension);
    }
    animationRemoved_virtual(dimension);
    thisCurve->clone(curve);
    if (!useGuiCurve) {
        evaluateValueChange(dimension, getCurrentTime(),view,  eValueChangedReasonNatronInternalEdited);
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, eValueChangedReasonNatronInternalEdited);
    }
    
    if (_signalSlotHandler) {
        std::list<double> keysList;
        KeyFrameSet keys = thisCurve->getKeyFrames_mt_safe();
        for (KeyFrameSet::iterator it = keys.begin(); it!=keys.end(); ++it) {
            keysList.push_back(it->getTime());
        }
        if (!keysList.empty()) {
            _signalSlotHandler->s_multipleKeyFramesSet(keysList, view, dimension, (int)eValueChangedReasonNatronInternalEdited);
        }
    }
}

bool
KnobHelper::setInterpolationAtTime(CurveChangeReason reason,
                                   ViewSpec view,
                                   int dimension,
                                   double time,
                                   KeyframeTypeEnum interpolation,
                                   KeyFrame* newKey)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());

    if (!canAnimate() || !isAnimated(dimension, view)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }
    assert(curve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }
    
    *newKey = curve->setKeyFrameInterpolation(interpolation, keyIndex);
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);
        guiCurveCloneInternalCurve(reason,view, dimension, eValueChangedReasonNatronInternalEdited);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_redrawGuiCurve(reason,view, dimension);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameInterpolationChanged(time,view, dimension);
    }
    return true;
}

bool
KnobHelper::moveDerivativesAtTime(CurveChangeReason reason,
                                  ViewSpec view,
                                  int dimension,
                                  double time,
                                  double left,
                                  double right)
{
    assert(QThread::currentThread() == qApp->thread());
    if ( dimension > (int)_imp->curves.size() || dimension < 0) {
        throw std::invalid_argument("KnobHelper::setInterpolationAtTime(): Dimension out of range");
    }
    
    if (!canAnimate() || !isAnimated(dimension, view)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }

    assert(curve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }
    
    curve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
    curve->setKeyFrameDerivatives(left, right, keyIndex);
    
    if (!useGuiCurve) {
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);
        guiCurveCloneInternalCurve(reason, view, dimension, eValueChangedReasonNatronInternalEdited);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_redrawGuiCurve(reason,view, dimension);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_derivativeMoved(time, view, dimension);
    }
    return true;
}

bool
KnobHelper::moveDerivativeAtTime(CurveChangeReason reason,
                                 ViewSpec view,
                                 int dimension,
                                 double time,
                                 double derivative,
                                 bool isLeft)
{
    assert(QThread::currentThread() == qApp->thread());
    if ( dimension > (int)_imp->curves.size() || dimension < 0) {
        throw std::invalid_argument("KnobHelper::setInterpolationAtTime(): Dimension out of range");
    }
    
    if (!canAnimate() || !isAnimated(dimension, view)) {
        return false;
    }

    KnobHolder* holder = getHolder();
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
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
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);
        guiCurveCloneInternalCurve(reason,view, dimension, eValueChangedReasonNatronInternalEdited);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_redrawGuiCurve(reason,view,dimension);
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_derivativeMoved(time,view, dimension);
    }
    return true;
}

void
KnobHelper::removeAnimation(ViewSpec view, int dimension,
                            ValueChangedReasonEnum reason)
{
    assert(QThread::currentThread() == qApp->thread());
    assert(0 <= dimension);
    if ( (dimension < 0) || ( (int)_imp->curves.size() <= dimension ) ) {
        throw std::invalid_argument("KnobHelper::removeAnimation(): Dimension out of range");
    }


    if (!canAnimate() || !isAnimated(dimension, view)) {
        return ;
    }

    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }
    
    if ( _signalSlotHandler && (reason != eValueChangedReasonUserEdited) ) {
        _signalSlotHandler->s_animationAboutToBeRemoved(view, dimension);
    }

    assert(curve);
    if (curve) {
        curve->clearKeyFrames();
    }
    
    if ( _signalSlotHandler && (reason != eValueChangedReasonUserEdited) ) {
        _signalSlotHandler->s_animationRemoved(view, dimension);
    }
    
    animationRemoved_virtual(dimension);

    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }
    
    
    if (!useGuiCurve) {
        //virtual portion
        evaluateValueChange(dimension, getCurrentTime(), view, reason);
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_redrawGuiCurve(eCurveChangeReasonInternal,view, dimension);
        }
    }
}

void
KnobHelper::clearExpressionsResultsIfNeeded(std::map<int,ValueChangedReasonEnum>& modifiedDimensions)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    for (int i = 0; i < getDimension(); ++i) {
        if (_imp->mustClearExprResults[i]) {
            clearExpressionsResults(i);
            _imp->mustClearExprResults[i] = false;
            modifiedDimensions.insert(std::make_pair(i, eValueChangedReasonNatronInternalEdited));
        }
    }
}

void
KnobHelper::cloneInternalCurvesIfNeeded(std::map<int,ValueChangedReasonEnum>& modifiedDimensions)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    for (int i = 0; i < getDimension(); ++i) {
        if (_imp->mustCloneInternalCurves[i]) {
            guiCurveCloneInternalCurve(eCurveChangeReasonInternal, ViewIdx(0), i, eValueChangedReasonNatronInternalEdited);
            _imp->mustCloneInternalCurves[i] = false;
            modifiedDimensions.insert(std::make_pair(i,eValueChangedReasonNatronInternalEdited));
        }
    }
}

void
KnobHelper::setInternalCurveHasChanged(ViewSpec /*view*/,
                                       int dimension,
                                       bool changed)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    _imp->mustCloneInternalCurves[dimension] = changed;
}

void
KnobHelper::cloneGuiCurvesIfNeeded(std::map<int,ValueChangedReasonEnum>& modifiedDimensions)
{
    if (!canAnimate()) {
        return;
    }

    bool hasChanged = false;
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    for (int i = 0; i < getDimension(); ++i) {
        if (_imp->mustCloneGuiCurves[i]) {
            hasChanged = true;
            boost::shared_ptr<Curve> curve = getCurve(ViewIdx(0), i);
            assert(curve);
            boost::shared_ptr<Curve> guicurve = _imp->gui->getCurve(ViewIdx(0),i);
            assert(guicurve);
            curve->clone(*guicurve);
            _imp->mustCloneGuiCurves[i] = false;
            
            modifiedDimensions.insert(std::make_pair(i,eValueChangedReasonUserEdited));
        }
    }
    if (hasChanged && _imp->holder) {
        _imp->holder->updateHasAnimation();
    }
}

void
KnobHelper::guiCurveCloneInternalCurve(CurveChangeReason curveChangeReason,
                                       ViewSpec view,
                                       int dimension,ValueChangedReasonEnum reason)
{
    if (!canAnimate()) {
        return;
    }

    if (_imp->gui) {
        boost::shared_ptr<Curve> guicurve = _imp->gui->getCurve(view, dimension);
        assert(guicurve);
        guicurve->clone(*(_imp->curves[dimension]));
        if (_signalSlotHandler && reason != eValueChangedReasonUserEdited) {
            _signalSlotHandler->s_redrawGuiCurve(curveChangeReason,view, dimension);
        }
    }
}

boost::shared_ptr<Curve>
KnobHelper::getGuiCurve(ViewSpec view,
                        int dimension,
                        bool byPassMaster) const
{
    if (!canAnimate()) {
        return boost::shared_ptr<Curve>();
    }

    std::pair<int,KnobPtr > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return master.second->getGuiCurve(view, master.first);
    }

    
    if (_imp->gui) {
        return _imp->gui->getCurve(view, dimension);
    } else {
        return boost::shared_ptr<Curve>();
    }
}

void
KnobHelper::setGuiCurveHasChanged(ViewSpec /*view*/,
                                  int dimension,
                                  bool changed)
{
    assert(dimension < (int)_imp->mustCloneGuiCurves.size());
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    _imp->mustCloneGuiCurves[dimension] = changed;
}

bool
KnobHelper::hasGuiCurveChanged(ViewSpec /*view*/,
                               int dimension) const
{
    assert(dimension < (int)_imp->mustCloneGuiCurves.size());
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    return _imp->mustCloneGuiCurves[dimension];
}

boost::shared_ptr<Curve> KnobHelper::getCurve(ViewSpec view,
                                              int dimension,
                                              bool byPassMaster) const
{

    if (dimension < 0 || dimension >= (int)_imp->curves.size() ) {
        return boost::shared_ptr<Curve>();
    }

    std::pair<int,KnobPtr > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return master.second->getCurve(view, master.first);
    }
    
    return _imp->curves[dimension];
}

bool
KnobHelper::isAnimated(int dimension,
                       ViewSpec view) const
{
    if (!canAnimate()) {
        return false;
    }
    boost::shared_ptr<Curve> curve = getCurve(view, dimension);
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
KnobHelper::evaluateValueChangeInternal(int dimension,
                                 double time,
                                 ViewSpec view,
                                 ValueChangedReasonEnum reason,
                                 ValueChangedReasonEnum originalReason)
{
    AppInstance* app = 0;
    if (_imp->holder) {
        app = _imp->holder->getApp();
    }
    
    bool guiFrozen = app && _imp->gui && _imp->gui->isGuiFrozenForPlayback();
    
    /// For eValueChangedReasonTimeChanged we never call the instanceChangedAction and evaluate otherwise it would just throttle
    /// the application responsiveness
    if ((originalReason != eValueChangedReasonTimeChanged || evaluateValueChangeOnTimeChange()) && _imp->holder) {
        if (!app || !app->getProject()->isLoadingProject()) {
            
            if (_imp->holder->isEvaluationBlocked()) {
                _imp->holder->appendValueChange(this, time, view, originalReason);
            } else {
                _imp->holder->beginChanges();
                _imp->holder->appendValueChange(this, time, view, originalReason);
                _imp->holder->endChanges();
            }
            
        }
    }
    
    onInternalValueChanged(dimension, time, view);
    
    if (!guiFrozen  && _signalSlotHandler) {
        computeHasModifications();
        bool refreshWidget = !app || hasAnimation() || time == app->getTimeLine()->currentFrame();
        if (refreshWidget) {
            _signalSlotHandler->s_valueChanged(view, dimension,(int)reason);
        }
        //if (reason != eValueChangedReasonSlaveRefresh) {
            refreshListenersAfterValueChange(view, originalReason, dimension);
        //}
        if (dimension == -1) {
            for (int i = 0; i < _imp->dimension; ++i) {
                checkAnimationLevel(view, i);
            }
        } else {
            checkAnimationLevel(view, dimension);
        }
    }
}

void
KnobHelper::evaluateValueChange(int dimension,
                                double time,
                                ViewSpec view,
                                ValueChangedReasonEnum reason)
{
    evaluateValueChangeInternal(dimension, time, view, reason, reason);
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
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->enabled[dimension] = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void
KnobHelper::setDefaultEnabled(int dimension,bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->defaultEnabled[dimension] = b;
    }
    setEnabled(dimension, b);
}

void
KnobHelper::setAllDimensionsEnabled(bool b)
{
    bool changed = false;
    {
        QMutexLocker k(&_imp->stateMutex);
        for (U32 i = 0; i < _imp->enabled.size(); ++i) {
            if (b != _imp->enabled[i]) {
                _imp->enabled[i] = b;
                changed = true;
            }
        }
    }
    if (changed && _signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void
KnobHelper::setDefaultAllDimensionsEnabled(bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        for (U32 i = 0; i < _imp->enabled.size(); ++i) {
            _imp->defaultEnabled[i] = b;
        }
    }
    setAllDimensionsEnabled(b);
}

void
KnobHelper::setSecretByDefault(bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->defaultIsSecret = b;
    }
    setSecret(b);
}

void
KnobHelper::setSecret(bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        if (_imp->IsSecret == b) {
            return;
        }
        _imp->IsSecret = b;
    }
    
    ///the knob was revealed , refresh its gui to the current time
    if ( !b && _imp->holder && _imp->holder->getApp() ) {
        onTimeChanged( false, _imp->holder->getApp()->getTimeLine()->currentFrame() );
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_secretChanged();
    }
}

int
KnobHelper::determineHierarchySize() const
{
    int ret = 0;
    KnobPtr current = getParentKnob();
    
    while (current) {
        ++ret;
        current = current->getParentKnob();
    }
    
    return ret;
}

const std::string &
KnobHelper::getLabel() const
{
    return _imp->label;
}

void
KnobHelper::setLabel(const std::string& label)
{
    _imp->label = label;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_labelChanged();
    }
}

void
KnobHelper::hideLabel()
{
    _imp->labelVisible = false;
}

bool
KnobHelper::isLabelVisible() const
{
    return _imp->labelVisible;
}

bool
KnobHelper::hasAnimation() const
{
    
    for (int i = 0; i < getDimension(); ++i) {
        if (getKeyFramesCount(ViewIdx(0),i) > 0) {
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

static void extractParameters(std::size_t startParenthesis, std::size_t endParenthesis, const std::string& str, std::vector<std::string>* params)
{
    std::size_t i = startParenthesis + 1;
    int insideParenthesis = 0;
    while (i < endParenthesis || insideParenthesis < 0) {
        std::string curParam;
        if (str.at(i) == '(') {
            ++insideParenthesis;
        } else if (str.at(i) == ')') {
            --insideParenthesis;
        }
        while (i < str.size() && (str.at(i) != ',' || insideParenthesis > 0)) {
            curParam.push_back(str.at(i));
            ++i;
            if (str.at(i) == '(') {
                ++insideParenthesis;
            } else if (str.at(i) == ')') {
                if (insideParenthesis > 0) {
                    --insideParenthesis;
                } else {
                    break;
                }
            }
        }
        params->push_back(curParam);
    }
}

static bool parseTokenFrom(int fromDim,
                           int dimensionParamPos,
                           bool returnsTuple,
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
    
    std::vector<std::string> params;
    ///If the function returns a tuple like get()[dimension], do not extract parameters
    if (!returnsTuple) {
        extractParameters(pos, endingParenthesis, str, &params);
    } else {
        //try to find the tuple
        std::size_t it = endingParenthesis + 1;
        while (it < str.size() && str.at(it) == ' ') {
            ++it;
        }
        if (it < str.size() && str.at(it) == '[') {
            ///we found a tuple
            std::size_t endingBracket = getMatchingParenthesisPosition(it, '[', ']',  str);
            if (endingBracket == std::string::npos) {
                throw std::invalid_argument("Invalid expr");
            }
            params.push_back(str.substr(it + 1, endingBracket - it - 1));
        }
    }
    
    
    
    //The get() function does not always returns a tuple
    if (params.empty()) {
        params.push_back("-1");
    }
    if (dimensionParamPos == -1) {
        ++dimensionParamPos;
    }
    
    
    if (dimensionParamPos < 0 || (int)params.size() <= dimensionParamPos) {
        throw std::invalid_argument("Invalid expr");
    }
    
    std::stringstream ss;
    /*
     When replacing the getValue() (or similar function) call by addAsDepdendencyOf
     the parameter prefixing the addAsDepdendencyOf will register itself its dimension params[dimensionParamPos] as a dependency of the expression
     at the "fromDim" dimension of thisParam
     */
    ss << ".addAsDependencyOf(" << fromDim << ",thisParam," <<  params[dimensionParamPos] << ")\n";
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

static bool extractAllOcurrences(const std::string& str,const std::string& token,bool returnsTuple,int dimensionParamPos,int fromDim,std::string *outputScript)
{
    
    std::size_t tokenStart;
    bool couldFindToken;
    try {
        couldFindToken = parseTokenFrom(fromDim, dimensionParamPos, returnsTuple, str, token, 0, &tokenStart, outputScript);
    } catch (...) {
        return false;
    }
    
    while (couldFindToken) {
        try {
            couldFindToken = parseTokenFrom(fromDim, dimensionParamPos, returnsTuple, str, token, tokenStart + 1, &tokenStart, outputScript);
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
    
    
    //Define all nodes reachable through expressions in the scope
    
    
    
    //Define all nodes in the same group reachable by their bare script-name
    NodesList siblings = collection->getNodes();
    for (NodesList::iterator it = siblings.begin(); it != siblings.end(); ++it) {
        if ((*it)->isActivated() && !(*it)->getParentMultiInstance()) {
            std::string scriptName = (*it)->getScriptName_mt_safe();
            std::string fullName = (*it)->getFullyQualifiedName();
            ss << tabStr << scriptName << " = " << appID << "." << fullName << "\n";
        }
    }
    
    if (isParentGrp) {
        ss << tabStr << "thisGroup = " << appID << "." << isParentGrp->getNode()->getFullyQualifiedName() << "\n";
    } else {
        ss << tabStr << "thisGroup = " << appID << "\n";
    }
    ss << tabStr << "thisNode = " << node->getScriptName_mt_safe() <<  "\n";
    
    ///Now define the variables in the scope
    ss << tabStr << "thisParam = thisNode." << name << "\n";
    ss << tabStr << "random = thisParam.random\n";
    ss << tabStr << "randomInt = thisParam.randomInt\n";
    ss << tabStr << "curve = thisParam.curve\n";
    if (dimension != -1) {
        ss << tabStr << "dimension = " << dim << "\n";
    }
    
    //If this node is a group, also define all nodes inside the group, though they will be referencable via
    //thisNode.childname but also with <NodeName.childname>
    NodeGroup* isHolderGrp = dynamic_cast<NodeGroup*>(effect);
    if (isHolderGrp) {
        NodesList children = isHolderGrp->getNodes();
        for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
            if ((*it)->isActivated() && !(*it)->getParentMultiInstance()) {
                std::string scriptName = (*it)->getScriptName_mt_safe();
                std::string fullName = (*it)->getFullyQualifiedName();
                ss << tabStr << node->getScriptName_mt_safe() << "." << scriptName << " = " << appID << "." << fullName << "\n";
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
    if  (!extractAllOcurrences(expressionCopy, "getValue", false, 0, dimension,&script)) {
        return ;
    }
    
    if (!extractAllOcurrences(expressionCopy, "getValueAtTime", false, 1,  dimension,&script)) {
        return;
    }
    
    if (!extractAllOcurrences(expressionCopy, "getDerivativeAtTime", false, 1,  dimension,&script)) {
        return;
    }
    
    if (!extractAllOcurrences(expressionCopy, "getIntegrateFromTimeToTime", false, 2, dimension,&script)) {
        return;
    }
    
    if (!extractAllOcurrences(expressionCopy, "get", true, -1, dimension,&script)) {
        return;
    }
    
    std::string declarations = declarePythonVariables(false, dimension);
    script = declarations + "\n" + script;
    ///This will register the listeners
    std::string error;
    bool ok = Python::interpretPythonScript(script, &error,NULL);
    if (!error.empty()) {
        qDebug() << error.c_str();
    }
    assert(ok);
    if (!ok) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): interpretPythonScript("+script+") failed!");
    }
}



std::string
KnobHelper::validateExpression(const std::string& expression,int dimension,bool hasRetVariable,std::string* resultAsString)
{
    PythonGILLocker pgl;
    
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
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string exprFuncPrefix = nodeFullName + "." + getName() + ".";
    std::string exprFuncName;
    {
        std::stringstream tmpSs;
        tmpSs << "expression" << dimension;
        exprFuncName = tmpSs.str();
    }
    
    exprCpy.append("\n    return ret\n");
    
    ///Now define the thisNode variable

    std::stringstream ss;
    ss << "def "  << exprFuncName << "(frame, view):\n";
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
        
        if (!Python::interpretPythonScript(script, &error, 0)) {
            throw std::runtime_error(error);
        }
        
        std::stringstream ss;
        ss << funcExecScript <<'(' << getCurrentTime()<<", " <<  getCurrentView() << ")\n";
        if (!Python::interpretPythonScript(ss.str(), &error, 0)) {
            throw std::runtime_error(error);
        }
        
        PyObject *ret = PyObject_GetAttrString(Python::getMainModule(),"ret"); //get our ret variable created above
        
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
    
    PythonGILLocker pgl;
    
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
        //Python::compilePyScript(exprCpy, &_imp->expressions[dimension].code);
    }
  

    //Parse listeners of the expression, to keep track of dependencies to indicate them to the user.
    if (getHolder()) {
        {
            EXPR_RECURSION_LEVEL();
            _imp->parseListenersFromExpression(dimension);
        }
    }
    
    //Notify the expr. has changed
    expressionChanged(dimension);
}


void
KnobHelper::replaceNodeNameInExpression(int dimension,
                                         const std::string& oldName,
                                        const std::string& newName)
{

    assert(dimension >= 0 && dimension < _imp->dimension);
    KnobHolder* holder = getHolder();
    if (!holder) {
        return;
    }
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    if (!isEffect) {
        return;
    }
    
    isEffect->beginChanges();
    std::string hasExpr = getExpression(dimension);
    if (hasExpr.empty()) {
        return;
    }
    bool hasRetVar = isExpressionUsingRetVariable(dimension);
    try {
        //Change in expressions the script-name
        QString estr(hasExpr.c_str());
        estr.replace(oldName.c_str(), newName.c_str());
        hasExpr = estr.toStdString();
        setExpression(dimension, hasExpr, hasRetVar);
    } catch (...) {
        
    }
    
    isEffect->endChanges(true);
    
}

bool
KnobHelper::isExpressionUsingRetVariable(int dimension) const
{
    QMutexLocker k(&_imp->expressionMutex);
    return _imp->expressions[dimension].hasRet;
}

bool
KnobHelper::getExpressionDependencies(int dimension, std::list<std::pair<KnobI*,int> >& dependencies) const
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
    PythonGILLocker pgl;
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
        std::list<std::pair<KnobI*,int> > dependencies;
        {
            QWriteLocker kk(&_imp->mastersMutex);
            dependencies = _imp->expressions[dimension].dependencies;
            _imp->expressions[dimension].dependencies.clear();
        }
        for (std::list<std::pair<KnobI*,int> >::iterator it = dependencies.begin();
             it != dependencies.end(); ++it) {
            
            KnobHelper* other = dynamic_cast<KnobHelper*>(it->first);
            assert(other);
            
            ListenerDimsMap otherListeners;
            {
                QReadLocker otherMastersLocker(&other->_imp->mastersMutex);
                otherListeners = other->_imp->listeners;
            }
            
            for (ListenerDimsMap::iterator it = otherListeners.begin(); it != otherListeners.end(); ++it) {
                KnobPtr knob = it->first.lock();
                if (knob.get() == this) {
                    
                    //erase from the dimensions vector
                    assert(dimension < (int)it->second.size());
                    it->second[dimension].isListening = false;
                    
                    //if it has no longer has a reference to this knob, erase it
                    bool hasReference = false;
                    for (std::size_t d = 0; d < it->second.size(); ++d) {
                        if (it->second[d].isListening) {
                            hasReference = true;
                            break;
                        }
                    }
                    if (!hasReference) {
                        otherListeners.erase(it);
                    }
                    
                    break;
                }
            }

            if (getHolder()) {
                getHolder()->onKnobSlaved(this, other,dimension,false );
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
KnobHelper::executeExpression(double time,
                              ViewSpec view,
                              int dimension) const
{
    
    std::string expr;
    {
        QMutexLocker k(&_imp->expressionMutex);
        expr = _imp->expressions[dimension].expression;
    }
    
    //returns a new ref, this function's documentation is not clear onto what it returns...
    //https://docs.python.org/2/c-api/veryhigh.html
    PyObject* mainModule = Python::getMainModule();
    PyObject* globalDict = PyModule_GetDict(mainModule);
    
    std::stringstream ss;
    ss << expr << '(' << time << ", " <<  view << ")\n";
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
                error = Python::PY3String_asString(errorObj);
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
KnobHelper::setName(const std::string & name,bool throwExceptions)
{
    _imp->originalName = name;
    _imp->name = Python::makeNameScriptFriendly(name);
    
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
    
    
    EffectInstance* effect = dynamic_cast<EffectInstance*>(_imp->holder);
    if (effect) {
        NodePtr node = effect->getNode();
        std::string effectScriptName = node->getScriptName_mt_safe();
        if (!effectScriptName.empty()) {
            std::string newPotentialQualifiedName = node->getApp()->getAppIDString() +  node->getFullyQualifiedName();
            newPotentialQualifiedName += '.';
            newPotentialQualifiedName += finalName;
            
            bool isAttrDefined = false;
            (void)Python::getAttrRecursive(newPotentialQualifiedName, appPTR->getMainModule(), &isAttrDefined);
            if (isAttrDefined) {
                std::stringstream ss;
                ss << "A Python attribute with the same name (" << newPotentialQualifiedName << ") already exists.";
                if (throwExceptions) {
                    throw std::runtime_error(ss.str());
                } else {
                    std::string err = ss.str();
                    appPTR->writeToOfxLog_mt_safe(err.c_str());
                    std::cerr << err << std::endl;
                    return;
                }
            }
            
        }
    }
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
KnobHelper::setParentKnob(KnobPtr knob)
{
    _imp->parentKnob = knob;
}

KnobPtr KnobHelper::getParentKnob() const
{
    return _imp->parentKnob.lock();
}

bool
KnobHelper::getIsSecret() const
{
    QMutexLocker k(&_imp->stateMutex);
    return _imp->IsSecret;
}

bool
KnobHelper::getIsSecretRecursive() const
{
    if (getIsSecret()) {
        return true;
    }
    KnobPtr parent = getParentKnob();
    if (parent) {
        return parent->getIsSecretRecursive();
    }
    return false;
}

bool
KnobHelper::getDefaultIsSecret() const
{
    QMutexLocker k(&_imp->stateMutex);
    return _imp->defaultIsSecret;
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
    
    QMutexLocker k(&_imp->stateMutex);
    return _imp->enabled[dimension];
}

bool
KnobHelper::isDefaultEnabled(int dimension) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    
    QMutexLocker k(&_imp->stateMutex);
    return _imp->defaultEnabled[dimension];
}

void
KnobHelper::setDirty(bool d)
{
    _signalSlotHandler->s_setDirty(d);
}

void
KnobHelper::setEvaluateOnChange(bool b)
{
    {
        QMutexLocker k(&_imp->evaluateOnChangeMutex);
        _imp->evaluateOnChange = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_evaluateOnChangeChanged(b);
    }
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


void
KnobHelper::setIsClipPreferencesSlave(bool slave)
{
    _imp->isClipPreferenceSlave = slave;
}

bool
KnobHelper::getIsClipPreferencesSlave() const
{
    return _imp->isClipPreferenceSlave;
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
        _imp->gui->copyAnimationToClipboard(-1);
    }
}

bool
KnobHelper::slaveTo(int dimension,
                    const KnobPtr & other,
                    int otherDimension,
                    ValueChangedReasonEnum reason,
                    bool ignoreMasterPersistence)
{
    assert(other.get() != this);
    assert( 0 <= dimension && dimension < (int)_imp->masters.size() );

    if (other->getMaster(otherDimension).second.get() == this) {
        //avoid recursion
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
    
    KnobHelper* masterKnob = dynamic_cast<KnobHelper*>( other.get() );
    assert(masterKnob);
    if (!masterKnob) {
        return false;
    }

    if (masterKnob->_signalSlotHandler && _signalSlotHandler) {

        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL( keyFrameSet(double, ViewIdx,int,int,bool) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameSet(double, ViewIdx,int,int,bool) ), Qt::UniqueConnection );
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL( keyFrameRemoved(double, ViewIdx,int,int) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameRemoved(double, ViewIdx,int,int)),Qt::UniqueConnection );
        
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL( keyFrameMoved(ViewIdx,int,double,double) ),
                         _signalSlotHandler.get(), SLOT( onMasterKeyFrameMoved(ViewIdx,int,double,double) ),Qt::UniqueConnection );
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL(animationRemoved(ViewIdx,int) ),
                         _signalSlotHandler.get(), SLOT(onMasterAnimationRemoved(ViewIdx,int)),Qt::UniqueConnection );
        
    }
    
    bool hasChanged = cloneAndCheckIfChanged(other.get(),dimension);
    
    //Do not disable buttons when they are slaved
    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    if (!isBtn) {
        setEnabled(dimension, false);
    }
    
    if (_signalSlotHandler) {
        ///Notify we want to refresh
        if (reason == eValueChangedReasonNatronInternalEdited) {
            _signalSlotHandler->s_knobSlaved(dimension,true);
        }
    }
    
    if (hasChanged) {
        evaluateValueChange(dimension, getCurrentTime(), ViewIdx(0), reason);
    } else if (isBtn) {
        //For buttons, don't evaluate or the instanceChanged action of the button will be called,
        //just refresh the hasModifications flag so it gets serialized
        isBtn->computeHasModifications();
        //force the aliased parameter to be persistant if it's not, otherwise it will not be saved
        isBtn->setIsPersistant(true);
    }

    ///Register this as a listener of the master
    if (masterKnob) {
        masterKnob->addListener(false,dimension, otherDimension, shared_from_this());
    }
    
    return true;
}

std::pair<int,KnobPtr > KnobHelper::getMaster(int dimension) const
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

std::vector< std::pair<int,KnobPtr > > KnobHelper::getMasters_mt_safe() const
{
    QReadLocker l(&_imp->mastersMutex);
    
    return _imp->masters;
}

void
KnobHelper::checkAnimationLevel(ViewSpec view,
                                int dimension)
{
    AnimationLevelEnum level = eAnimationLevelNone;

    if ( canAnimate() && isAnimated(dimension, view) && getHolder() && getHolder()->getApp() ) {

        boost::shared_ptr<Curve> c = getCurve(view, dimension);
        double time = getHolder()->getApp()->getTimeLine()->currentFrame();
        if (c->getKeyFramesCount() > 0) {
            KeyFrame kf;
            int nKeys = c->getNKeyFramesInRange(time, time +1);
            if (nKeys > 0) {
                level = eAnimationLevelOnKeyframe;
            } else {
                level = eAnimationLevelInterpolatedValue;
            }
        } else {
            level = eAnimationLevelNone;
        }
    }
    setAnimationLevel(view, dimension,level);
}

void
KnobHelper::setAnimationLevel(ViewSpec view,
                              int dimension,
                              AnimationLevelEnum level)
{
    bool changed = false;
    {
        QMutexLocker l(&_imp->animationLevelMutex);
        assert( dimension < (int)_imp->animationLevel.size() );
        if (_imp->animationLevel[dimension] != level) {
            changed = true;
            _imp->animationLevel[dimension] = level;
        }
    }
    if ( changed && _signalSlotHandler && _imp->gui && !_imp->gui->isGuiFrozenForPlayback() ) {
        if (getExpression(dimension).empty()) {
            _signalSlotHandler->s_animationLevelChanged(view, dimension,(int)level );
        }
    }
}

AnimationLevelEnum
KnobHelper::getAnimationLevel(int dimension) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,KnobPtr > master = getMaster(dimension);
    
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
KnobHelper::deleteAnimationConditional(double time,
                                       ViewSpec view,
                                       int dimension,
                                       ValueChangedReasonEnum reason,
                                       bool before)
{
    if (!_imp->curves[dimension]) {
        return;
    }
    assert( 0 <= dimension && dimension < getDimension() );
    KnobHolder* holder = getHolder();
    
    boost::shared_ptr<Curve> curve;
    
    bool useGuiCurve = (!holder || !holder->isSetValueCurrentlyPossible()) && _imp->gui && !hasGuiCurveChanged(view, dimension);
    
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = _imp->gui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension,true);
    }
    
    std::list<int> keysRemoved;
    if (before) {
        curve->removeKeyFramesBeforeTime(time, &keysRemoved);
    } else {
        curve->removeKeyFramesAfterTime(time, &keysRemoved);
    }
    
    if (!useGuiCurve) {
        
        checkAnimationLevel(view, dimension);
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal,view, dimension, reason);
        evaluateValueChange(dimension, time, view, reason);
    }
    
    if (holder && holder->getApp()) {
        holder->getApp()->removeMultipleKeyframeIndicator(keysRemoved, true);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_animationRemoved(view, dimension);
    }

}

void
KnobHelper::deleteAnimationBeforeTime(double time,
                                      ViewSpec view,
                                      int dimension,
                                      ValueChangedReasonEnum reason)
{
    deleteAnimationConditional(time, view, dimension, reason, true);
}

void
KnobHelper::deleteAnimationAfterTime(double time,
                                     ViewSpec view,
                                     int dimension,
                                     ValueChangedReasonEnum reason)
{
    deleteAnimationConditional(time, view, dimension, reason, false);
}

bool
KnobHelper::getKeyFrameTime(ViewSpec view,
                            int index,
                            int dimension,
                            double* time) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !isAnimated(dimension, view) ) {
        return false;
    }
    boost::shared_ptr<Curve> curve = getCurve(view, dimension); //< getCurve will return the master's curve if any
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(index, &kf);
    if (ret) {
        *time = kf.getTime();
    }
    
    return ret;
}

bool
KnobHelper::getLastKeyFrameTime(ViewSpec view,
                                int dimension,
                                double* time) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    *time = curve->getMaximumTimeCovered();
    
    return true;
}

bool
KnobHelper::getFirstKeyFrameTime(ViewSpec view,
                                 int dimension,
                                 double* time) const
{
    return getKeyFrameTime(view, 0, dimension, time);
}

int
KnobHelper::getKeyFramesCount(ViewSpec view,
                              int dimension) const
{
    if (!canAnimate() || !isAnimated(dimension, view)) {
        return 0;
    }

    boost::shared_ptr<Curve> curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    return curve->getKeyFramesCount();   //< getCurve will return the master's curve if any
}

bool
KnobHelper::getNearestKeyFrameTime(ViewSpec view,
                                   int dimension,
                                   double time,
                                   double* nearestTime) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getNearestKeyFrameWithTime(time, &kf);
    if (ret) {
        *nearestTime = kf.getTime();
    }
    
    return ret;
}

int
KnobHelper::getKeyFrameIndex(ViewSpec view,
                             int dimension,
                             double time) const
{
    assert( 0 <= dimension && dimension < getDimension() );
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return -1;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    
    return curve->keyFrameIndex(time);
}


void
KnobHelper::refreshListenersAfterValueChange(ViewSpec view,
                                             ValueChangedReasonEnum reason,
                                             int dimension)
{
    ListenerDimsMap listeners;
    getListeners(listeners);
    
    if (listeners.empty()) {
        return;
    }

    double time = getCurrentTime();
    for (ListenerDimsMap::iterator it = listeners.begin(); it!=listeners.end(); ++it) {
        
        KnobHelper* slaveKnob = dynamic_cast<KnobHelper*>(it->first.lock().get());
        if (!slaveKnob) {
            continue;
        }
        
        
        std::set<int> dimensionsToEvaluate;
        bool mustClone = false;
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            if (it->second[i].isListening && (it->second[i].targetDim == dimension || it->second[i].targetDim == -1)) {
                dimensionsToEvaluate.insert(i);
                if (!it->second[i].isExpr) {
                    mustClone = true;
                }
            }
        }
        
        if (dimensionsToEvaluate.empty()) {
            continue;
        }
        
        int dimChanged;
        if (dimensionsToEvaluate.size() > 1) {
            dimChanged = -1;
        } else {
            dimChanged = *dimensionsToEvaluate.begin();
        }
        
        if (mustClone) {
            ///We still want to clone the master's dimension because otherwise we couldn't edit the curve e.g in the curve editor
            ///For example we use it for roto knobs where selected beziers have their knobs slaved to the gui knobs
            slaveKnob->clone(this,dimChanged);
        }
        
        slaveKnob->evaluateValueChangeInternal(dimChanged, time, view, eValueChangedReasonSlaveRefresh, reason);
        
        //call recursively
        slaveKnob->refreshListenersAfterValueChange(view, reason, dimChanged);
        
    } // for all listeners
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

//The knob in parameter will "listen" to this knob. Hence this knob is a dependency of the knob in parameter.
void
KnobHelper::addListener(const bool isExpression,
                        const int listenerDimension,
                        const int listenedToDimension,
                        const KnobPtr& listener)
{
    assert(listenedToDimension == -1 || (listenedToDimension >= 0 && listenedToDimension < _imp->dimension));
    KnobHelper* listenerIsHelper = dynamic_cast<KnobHelper*>(listener.get());
    assert(listenerIsHelper);
    if (!listenerIsHelper) {
        return;
    }
    
    if (listenerIsHelper->_signalSlotHandler && _signalSlotHandler) {
        
        
        //Notify the holder one of its knob is now slaved
        if (listenerIsHelper->getHolder()) {
            listenerIsHelper->getHolder()->onKnobSlaved(listenerIsHelper, this,listenerDimension,true );
        }
        
    }
    
    // If this knob is already a dependency of the knob, add it to its dimension vector
    {
        QWriteLocker l(&_imp->mastersMutex);
        ListenerDimsMap::iterator foundListening = _imp->listeners.find(listener);
        if (foundListening != _imp->listeners.end()) {
            foundListening->second[listenerDimension].isListening = true;
            foundListening->second[listenerDimension].isExpr = isExpression;
            foundListening->second[listenerDimension].targetDim = listenedToDimension;
        } else {
            std::vector<ListenerDim>& dims = _imp->listeners[listener];
            dims.resize(listener->getDimension());
            dims[listenerDimension].isListening = true;
            dims[listenerDimension].isExpr = isExpression;
            dims[listenerDimension].targetDim = listenedToDimension;
        }
    }
    
    if (isExpression) {
        QMutexLocker k(&listenerIsHelper->_imp->expressionMutex);
        assert(listenerDimension >= 0 && listenerDimension < listenerIsHelper->_imp->dimension);
        listenerIsHelper->_imp->expressions[listenerDimension].dependencies.push_back(std::make_pair(this,listenedToDimension));
    }
    
    
}

void
KnobHelper::removeListener(KnobI* listener, int listenerDimension)
{
    KnobHelper* listenerHelper = dynamic_cast<KnobHelper*>(listener);
    assert(listenerHelper);
    
    QWriteLocker l(&_imp->mastersMutex);
    for (ListenerDimsMap::iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        if (it->first.lock().get() == listener) {
            it->second[listenerDimension].isListening = false;
            
            bool hasDimensionListening = false;
            for (std::size_t i = 0; i < it->second.size(); ++i) {
                if (it->second[listenerDimension].isListening) {
                    hasDimensionListening = true;
                    break;
                }
            }
            if (!hasDimensionListening) {
                _imp->listeners.erase(it);
            }
            break;
        }
    }
}


void
KnobHelper::getListeners(KnobI::ListenerDimsMap & listeners) const
{
    QReadLocker l(&_imp->mastersMutex);
    listeners = _imp->listeners;
}

double
KnobHelper::getCurrentTime() const
{
    KnobHolder* holder = getHolder();
    return holder && holder->getApp() ? holder->getCurrentTime() : 0;
}

ViewIdx
KnobHelper::getCurrentView() const
{
    KnobHolder* holder = getHolder();
    return (holder && holder->getApp()) ? holder->getCurrentView() : ViewIdx(0);
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
        EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
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
KnobHelper::hasModificationsForSerialization() const
{
    bool enabledChanged = false;
    for (int i = 0; i < getDimension(); ++i) {
        if (isEnabled(i) != isDefaultEnabled(i)) {
            enabledChanged = true;
        }
    }
    return hasModifications() ||
    getIsSecret() != getDefaultIsSecret() || enabledChanged;
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

KnobPtr
KnobHelper::createDuplicateOnNode(EffectInstance* effect,
                                  const boost::shared_ptr<KnobPage>& page,
                                  const boost::shared_ptr<KnobGroup>& group,
                                  int indexInParent,
                                  bool makeAlias,
                                  const std::string& newScriptName,
                                  const std::string& newLabel,
                                  const std::string& newToolTip,
                                  bool refreshParams)
{
    ///find-out to which node that master knob belongs to
    if (!getHolder()->getApp()) {
        return KnobPtr();
    }
    
    KnobHolder* holder = getHolder();
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    assert(isEffect);
    if (!isEffect) {
        return KnobPtr();
    }
    
    KnobBool* isBool = dynamic_cast<KnobBool*>(this);
    KnobInt* isInt = dynamic_cast<KnobInt*>(this);
    KnobDouble* isDbl = dynamic_cast<KnobDouble*>(this);
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
    KnobColor* isColor = dynamic_cast<KnobColor*>(this);
    KnobString* isString = dynamic_cast<KnobString*>(this);
    KnobFile* isFile = dynamic_cast<KnobFile*>(this);
    KnobOutputFile* isOutputFile = dynamic_cast<KnobOutputFile*>(this);
    KnobPath* isPath = dynamic_cast<KnobPath*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);

    
    //Ensure the group user page is created

    boost::shared_ptr<KnobPage> destPage;
    if (page) {
        destPage = page;
    } else {
        destPage = effect->getOrCreateUserPageKnob();
    }
    
    KnobPtr output;
    if (isBool) {
        boost::shared_ptr<KnobBool> newKnob = effect->createBoolKnob(newScriptName, newLabel);
        output = newKnob;
    } else if (isInt) {
        boost::shared_ptr<KnobInt> newKnob = effect->createIntKnob(newScriptName, newLabel,getDimension());
        newKnob->setMinimumsAndMaximums(isInt->getMinimums(), isInt->getMaximums());
        newKnob->setDisplayMinimumsAndMaximums(isInt->getDisplayMinimums(),isInt->getDisplayMaximums());
        output = newKnob;
    } else if (isDbl) {
        boost::shared_ptr<KnobDouble> newKnob = effect->createDoubleKnob(newScriptName, newLabel,getDimension());
        newKnob->setMinimumsAndMaximums(isDbl->getMinimums(), isDbl->getMaximums());
        newKnob->setDisplayMinimumsAndMaximums(isDbl->getDisplayMinimums(),isDbl->getDisplayMaximums());
        output = newKnob;
    } else if (isChoice) {
        boost::shared_ptr<KnobChoice> newKnob = effect->createChoiceKnob(newScriptName, newLabel);
        if (!makeAlias) {
            newKnob->populateChoices(isChoice->getEntries_mt_safe(),isChoice->getEntriesHelp_mt_safe());
        }
        output = newKnob;
    } else if (isColor) {
        boost::shared_ptr<KnobColor> newKnob = effect->createColorKnob(newScriptName, newLabel,getDimension());
        newKnob->setMinimumsAndMaximums(isColor->getMinimums(), isColor->getMaximums());
        newKnob->setDisplayMinimumsAndMaximums(isColor->getDisplayMinimums(),isColor->getDisplayMaximums());
        output = newKnob;
    } else if (isString) {
        boost::shared_ptr<KnobString> newKnob = effect->createStringKnob(newScriptName, newLabel);
        if (isString->isLabel()) {
            newKnob->setAsLabel();
        }
        if (isString->isCustomKnob()) {
            newKnob->setAsCustom();
        }
        if (isString->isMultiLine()) {
            newKnob->setAsMultiLine();
        }
        if (isString->usesRichText()) {
            newKnob->setUsesRichText(true);
        }
        output = newKnob;
    } else if (isFile) {
        boost::shared_ptr<KnobFile> newKnob = effect->createFileKnob(newScriptName, newLabel);
        if (isFile->isInputImageFile()) {
            newKnob->setAsInputImage();
        }
        output = newKnob;
    } else if (isOutputFile) {
        boost::shared_ptr<KnobOutputFile> newKnob = effect->createOuptutFileKnob(newScriptName, newLabel);
        if (isOutputFile->isOutputImageFile()) {
            newKnob->setAsOutputImageFile();
        }
        output = newKnob;
    } else if (isPath) {
        boost::shared_ptr<KnobPath> newKnob = effect->createPathKnob(newScriptName, newLabel);
        if (isPath->isMultiPath()) {
            newKnob->setMultiPath(true);
        }
        output = newKnob;
        
    } else if (isGrp) {
        boost::shared_ptr<KnobGroup> newKnob = effect->createGroupKnob(newScriptName, newLabel);
        if (isGrp->isTab()) {
            newKnob->setAsTab();
        }
        output = newKnob;
        
    } else if (isPage) {
        boost::shared_ptr<KnobPage> newKnob = effect->createPageKnob(newScriptName, newLabel);
        output = newKnob;
        
    } else if (isBtn) {
        boost::shared_ptr<KnobButton> newKnob = effect->createButtonKnob(newScriptName, newLabel);
        output = newKnob;
        
    } else if (isParametric) {
        boost::shared_ptr<KnobParametric> newKnob = effect->createParametricKnob(newScriptName, newLabel, isParametric->getDimension());
        output = newKnob;
    }
    if (!output) {
        return KnobPtr();
    }
    
    output->setName(newScriptName, true);
    output->setAsUserKnob();
    output->cloneDefaultValues(this);
    output->clone(this);
    if (canAnimate()) {
        output->setAnimationEnabled(isAnimationEnabled());
    }
    output->setEvaluateOnChange(getEvaluateOnChange());
    output->setHintToolTip(newToolTip);
    output->setAddNewLine(true);
    if (group) {
        if (indexInParent == -1) {
            group->addKnob(output);
        } else {
            group->insertKnob(indexInParent, output);
        }
    } else {
        assert(destPage);
        if (indexInParent == -1) {
            destPage->addKnob(output);
        } else {
            destPage->insertKnob(indexInParent, output);
        }
    }
    effect->getNode()->declarePythonFields();
    if (!makeAlias) {
        
        boost::shared_ptr<NodeCollection> collec;
        collec = isEffect->getNode()->getGroup();
        
        NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
        
        std::stringstream ss;
        if (isCollecGroup) {
            ss << "thisGroup." << newScriptName;
        } else {
            ss << "app." << effect->getNode()->getFullyQualifiedName() << "." << newScriptName;
        }
        if (output->getDimension() > 1) {
            ss << ".get()[dimension]";
        } else {
            ss << ".get()";
        }
        
        try {
            std::string script = ss.str();
            for (int i = 0; i < getDimension(); ++i) {
                clearExpression(i, true);
                setExpression(i, script, false);
            }
        } catch (...) {
            
        }
    } else {
        setKnobAsAliasOfThis(output, true);
    }
    if (refreshParams) {
        effect->refreshKnobs();
    }
    return output;
}

bool
KnobHelper::setKnobAsAliasOfThis(const KnobPtr& master, bool doAlias)
{
    //Sanity check
    if (!master || master->getDimension() != getDimension() ||
        master->typeName() != typeName()) {
        return false;
    }
    
    /*
     For choices, copy exactly the menu entries because they have to be the same
     */
    if (doAlias) {
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(master.get());
        if (isChoice) {
            KnobChoice* thisChoice = dynamic_cast<KnobChoice*>(this);
            assert(thisChoice);
            isChoice->populateChoices(thisChoice->getEntries_mt_safe(),thisChoice->getEntriesHelp_mt_safe());
        }
    }
    beginChanges();
    for (int i = 0; i < getDimension(); ++i) {
        
        if (isSlave(i)) {
            unSlave(i, false);
        }
        if (doAlias) {
            bool ok = slaveTo(i, master, i, eValueChangedReasonNatronInternalEdited, false);
            assert(ok);
        }
        handleSignalSlotsForAliasLink(master,doAlias);
    }
    endChanges();
        
    QWriteLocker k(&_imp->mastersMutex);
    if (doAlias) {
        _imp->slaveForAlias = master;
    } else {
        _imp->slaveForAlias.reset();
    }
    return true;
}

KnobPtr
KnobHelper::getAliasMaster()  const
{
    QReadLocker k(&_imp->mastersMutex);
    return _imp->slaveForAlias;
}

void
KnobHelper::getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const
{
    std::set<KnobI*> deps;
    {
        QMutexLocker k(&_imp->expressionMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            for (std::list< std::pair<KnobI*,int> >::const_iterator it = _imp->expressions[i].dependencies.begin();
                 it != _imp->expressions[i].dependencies.end(); ++it) {
                
                deps.insert(it->first);
               
            }
        }
    }
    {
        QReadLocker k(&_imp->mastersMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            if (_imp->masters[i].second) {
                if (std::find(deps.begin(), deps.end(), _imp->masters[i].second.get()) == deps.end()) {
                    deps.insert(_imp->masters[i].second.get());
                }
            }
        }
    }
    
    std::list<KnobI*> knobsToInspectRecursive;
    for (std::set<KnobI*>::iterator it = deps.begin(); it!=deps.end(); ++it) {
        EffectInstance* effect  = dynamic_cast<EffectInstance*>((*it)->getHolder());
        if (effect) {
            NodePtr node = effect->getNode();

            std::pair<std::set<NodePtr>::iterator,bool> ok = nodes.insert(node);
            if (ok.second) {
                knobsToInspectRecursive.push_back(*it);
            }
        }
    }
    
    
    for (std::list<KnobI*>::iterator it = knobsToInspectRecursive.begin(); it!=knobsToInspectRecursive.end(); ++it) {
        (*it)->getAllExpressionDependenciesRecursive(nodes);
    }
}

/***************************KNOB HOLDER******************************************/

struct KnobHolder::KnobHolderPrivate
{
    AppInstance* app;
    
    QMutex knobsMutex;
    std::vector< KnobPtr > knobs;
    bool knobsInitialized;
    bool isInitializingKnobs;
    bool isSlave;
    

    ///Count how many times an overlay needs to be redrawn for the instanceChanged/penMotion/penDown etc... actions
    ///to just redraw it once when the recursion level is back to 0
    QMutex overlayRedrawStackMutex;
    int overlayRedrawStack;
    bool isDequeingValuesSet;

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
    , isInitializingKnobs(false)
    , isSlave(false)
    , overlayRedrawStackMutex()
    , overlayRedrawStack(0)
    , isDequeingValuesSet(false)
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

    }
};

KnobHolder::KnobHolder(AppInstance* appInstance)
: QObject()
, _imp( new KnobHolderPrivate(appInstance) )
{
    QObject::connect(this, SIGNAL(doEndChangesOnMainThread()), this, SLOT(onDoEndChangesOnMainThreadTriggered()));
    QObject::connect(this, SIGNAL(doEvaluateOnMainThread(KnobI*, bool, int)), this,
                     SLOT(onDoEvaluateOnMainThread(KnobI*, bool, int)));
    QObject::connect(this, SIGNAL(doValueChangeOnMainThread(KnobI*, int, double, ViewSpec, bool)), this,
                     SLOT(onDoValueChangeOnMainThread(KnobI*, int, double, ViewSpec, bool)));
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
KnobHolder::setIsInitializingKnobs(bool b)
{
    QMutexLocker k(&_imp->knobsMutex);
    _imp->isInitializingKnobs = b;
}

bool
KnobHolder::isInitializingKnobs() const
{
    QMutexLocker k(&_imp->knobsMutex);
    return _imp->isInitializingKnobs;
}

void
KnobHolder::addKnob(KnobPtr k)
{
    assert(QThread::currentThread() == qApp->thread());
    QMutexLocker kk(&_imp->knobsMutex);
    _imp->knobs.push_back(k);
}

void
KnobHolder::insertKnob(int index, const KnobPtr& k)
{
    if (index < 0) {
        return;
    }
    QMutexLocker kk(&_imp->knobsMutex);
    
    if (index >= (int)_imp->knobs.size()) {
        _imp->knobs.push_back(k);
    } else {
        KnobsVec::iterator it = _imp->knobs.begin();
        std::advance(it, index);
        _imp->knobs.insert(it, k);
    }
}

void
KnobHolder::removeKnobFromList(const KnobI* knob)
{
    QMutexLocker kk(&_imp->knobsMutex);

    for (KnobsVec::iterator it = _imp->knobs.begin(); it!=_imp->knobs.end(); ++it) {
        if (it->get() == knob) {
            _imp->knobs.erase(it);
            return;
        }
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
KnobHolder::refreshKnobs(bool keepCurPageIndex)
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->settingsPanel) {
        _imp->settingsPanel->scanForNewKnobs(keepCurPageIndex);
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
        if (isEffect) {
            isEffect->getNode()->declarePythonFields();
        }
    }
}



void
KnobHolder::removeDynamicKnob(KnobI* knob)
{
    KnobsVec knobs;
    {
        QMutexLocker k(&_imp->knobsMutex);
        knobs = _imp->knobs;
    }
    
    KnobPtr sharedKnob;
    for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (it->get() == knob && (*it)->isDynamicallyCreated()) {
            (*it)->deleteKnob();
            sharedKnob = *it;
            break;
        }
    }
    
    {
        QMutexLocker k(&_imp->knobsMutex);
        for (KnobsVec::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if (it2->get() == knob && (*it2)->isDynamicallyCreated()) {
                _imp->knobs.erase(it2);
                break;
            }
        }
    }
    
    if (_imp->settingsPanel) {
        _imp->settingsPanel->deleteKnobGui(sharedKnob);
    }
    
    
}

bool
KnobHolder::moveKnobOneStepUp(KnobI* knob)
{
    if (!knob->isUserKnob() && !dynamic_cast<KnobPage*>(knob)) {
        return false;
    }
    KnobPtr parent = knob->getParentKnob();
    KnobGroup* parentIsGrp = dynamic_cast<KnobGroup*>(parent.get());
    KnobPage* parentIsPage = dynamic_cast<KnobPage*>(parent.get());
    
    //the knob belongs to a group/page , change its index within the group instead
    bool moveOk = false;
    if (!parent) {
        moveOk = true;
    }
    try {
        if (parentIsGrp) {
            moveOk = parentIsGrp->moveOneStepUp(knob);
        } else if (parentIsPage) {
            moveOk = parentIsPage->moveOneStepUp(knob);
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        assert(false);
        return false;
    }
    
    if (moveOk) {
        QMutexLocker k(&_imp->knobsMutex);
        int prevInPage = -1;
        if (parent) {
            for (U32 i = 0; i < _imp->knobs.size(); ++i) {
                if (_imp->knobs[i].get() == knob) {
                    if (prevInPage != -1) {
                        KnobPtr tmp = _imp->knobs[prevInPage];
                        _imp->knobs[prevInPage] = _imp->knobs[i];
                        _imp->knobs[i] = tmp;
                    }
                    break;
                } else {
                    if (_imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == parent)) {
                        prevInPage = i;
                    }
                }
            }
        } else {
            bool foundPrevPage = false;
            for (U32 i = 0; i < _imp->knobs.size(); ++i) {
                if (_imp->knobs[i].get() == knob) {
                    if (prevInPage != -1) {
                        KnobPtr tmp = _imp->knobs[prevInPage];
                        _imp->knobs[prevInPage] = _imp->knobs[i];
                        _imp->knobs[i] = tmp;
                        foundPrevPage = true;
                    }
                    break;
                } else {
                    if (!_imp->knobs[i]->getParentKnob()) {
                        prevInPage = i;
                    }
                }
            }
            if (!foundPrevPage) {
                moveOk = false;
            }
        }
    }
    return moveOk;
}



bool
KnobHolder::moveKnobOneStepDown(KnobI* knob)
{
    if (!knob->isUserKnob() && !dynamic_cast<KnobPage*>(knob)) {
        return false;
    }
    KnobPtr parent = knob->getParentKnob();
    KnobGroup* parentIsGrp = dynamic_cast<KnobGroup*>(parent.get());
    KnobPage* parentIsPage = dynamic_cast<KnobPage*>(parent.get());
    
    //the knob belongs to a group/page , change its index within the group instead
    bool moveOk = false;
    if (!parent) {
        moveOk = true;
    }
    try {
        if (parentIsGrp) {
            moveOk = parentIsGrp->moveOneStepDown(knob);
        } else if (parentIsPage) {
            moveOk = parentIsPage->moveOneStepDown(knob);
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        assert(false);
        return false;
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
    if (foundIndex < 0) {
        return false;
    }
    if (moveOk) {
        //The knob (or page) could be moved inside the group/page, just move it down
        if (parent) {
            for (int i = foundIndex + 1; i < (int)_imp->knobs.size(); ++i) {
                if (_imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == parent)) {
                    KnobPtr tmp = _imp->knobs[foundIndex];
                    _imp->knobs[foundIndex] = _imp->knobs[i];
                    _imp->knobs[i] = tmp;
                    break;
                }
            }
        } else {
            bool foundNextPage = false;
            for (int i = foundIndex + 1; i < (int)_imp->knobs.size(); ++i) {
                if (!_imp->knobs[i]->getParentKnob()) {
                    KnobPtr tmp = _imp->knobs[foundIndex];
                    _imp->knobs[foundIndex] = _imp->knobs[i];
                    _imp->knobs[i] = tmp;
                    foundNextPage = true;
                    break;
                }
            }
            
            if (!foundNextPage) {
                moveOk = false;
            }
        }
    }
    return moveOk;
}

boost::shared_ptr<KnobPage>
KnobHolder::getUserPageKnob() const
{
    {
        QMutexLocker k(&_imp->knobsMutex);
        for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            if ((*it)->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                return boost::dynamic_pointer_cast<KnobPage>(*it);
            }
        }
    }
    return boost::shared_ptr<KnobPage>();
}

boost::shared_ptr<KnobPage>
KnobHolder::getOrCreateUserPageKnob() 
{
    
    boost::shared_ptr<KnobPage> ret = getUserPageKnob();
    if (ret) {
        return ret;
    }
    ret = AppManager::createKnob<KnobPage>(this,NATRON_USER_MANAGED_KNOBS_PAGE_LABEL,1,false);
    ret->setAsUserKnob();
    ret->setName(NATRON_USER_MANAGED_KNOBS_PAGE);
    
    
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobInt>
KnobHolder::createIntKnob(const std::string& name, const std::string& label,int dimension)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobInt>(existingKnob);
    }
    boost::shared_ptr<KnobInt> ret = AppManager::createKnob<KnobInt>(this,label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobDouble>
KnobHolder::createDoubleKnob(const std::string& name, const std::string& label,int dimension)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobDouble>(existingKnob);
    }
    boost::shared_ptr<KnobDouble> ret = AppManager::createKnob<KnobDouble>(this,label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobColor>
KnobHolder::createColorKnob(const std::string& name, const std::string& label,int dimension)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobColor>(existingKnob);
    }
    boost::shared_ptr<KnobColor> ret = AppManager::createKnob<KnobColor>(this,label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobBool>
KnobHolder::createBoolKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobBool>(existingKnob);
    }
    boost::shared_ptr<KnobBool> ret = AppManager::createKnob<KnobBool>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobChoice>
KnobHolder::createChoiceKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobChoice>(existingKnob);
    }
    boost::shared_ptr<KnobChoice> ret = AppManager::createKnob<KnobChoice>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobButton>
KnobHolder::createButtonKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobButton>(existingKnob);
    }
    boost::shared_ptr<KnobButton> ret = AppManager::createKnob<KnobButton>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobSeparator>
KnobHolder::createSeparatorKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobSeparator>(existingKnob);
    }
    boost::shared_ptr<KnobSeparator> ret = AppManager::createKnob<KnobSeparator>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
     Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

//Type corresponds to the Type enum defined in StringParamBase in Parameter.h
boost::shared_ptr<KnobString>
KnobHolder::createStringKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobString>(existingKnob);
    }
    boost::shared_ptr<KnobString> ret = AppManager::createKnob<KnobString>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<KnobFile>
KnobHolder::createFileKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobFile>(existingKnob);
    }
    boost::shared_ptr<KnobFile> ret = AppManager::createKnob<KnobFile>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;
}

boost::shared_ptr<KnobOutputFile>
KnobHolder::createOuptutFileKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobOutputFile>(existingKnob);
    }
    boost::shared_ptr<KnobOutputFile> ret = AppManager::createKnob<KnobOutputFile>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<KnobPath>
KnobHolder::createPathKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobPath>(existingKnob);
    }
    boost::shared_ptr<KnobPath> ret = AppManager::createKnob<KnobPath>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<KnobGroup>
KnobHolder::createGroupKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobGroup>(existingKnob);
    }
    boost::shared_ptr<KnobGroup> ret = AppManager::createKnob<KnobGroup>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<KnobPage>
KnobHolder::createPageKnob(const std::string& name, const std::string& label)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobPage>(existingKnob);
    }
    boost::shared_ptr<KnobPage> ret = AppManager::createKnob<KnobPage>(this,label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

boost::shared_ptr<KnobParametric>
KnobHolder::createParametricKnob(const std::string& name, const std::string& label,int nbCurves)
{
    KnobPtr existingKnob = getKnobByName(name);
    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobParametric>(existingKnob);
    }
    boost::shared_ptr<KnobParametric> ret = AppManager::createKnob<KnobParametric>(this,label, nbCurves, false);
    ret->setName(name);
    ret->setAsUserKnob();
    /*boost::shared_ptr<KnobPage> pageknob = getOrCreateUserPageKnob();
    Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
    return ret;

}

void
KnobHolder::onDoEvaluateOnMainThread(KnobI* knob,bool significant,int reason)
{
    assert(QThread::currentThread() == qApp->thread());
    evaluate_public(knob, significant, (ValueChangedReasonEnum)reason);
}

void
KnobHolder::onDoEndChangesOnMainThreadTriggered()
{
    assert(QThread::currentThread() == qApp->thread());
    endChanges();
}

ChangesList
KnobHolder::getKnobChanges() const
{
    QMutexLocker l(&_imp->evaluationBlockedMutex);
    return _imp->knobChanged;
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
        ValueChangedReasonEnum reason = first->reason;
        KnobI* knob = first->knob;
        if (!isMT) {
            Q_EMIT doEvaluateOnMainThread(knob, significant, reason);
        } else {
            evaluate_public(knob, significant, reason);
        }
    }
}

void
KnobHolder::onDoValueChangeOnMainThread(KnobI* knob,
                                        int reason,
                                        double time,
                                        ViewSpec view,
                                        bool originatedFromMT)
{
    assert(QThread::currentThread() == qApp->thread());
    onKnobValueChanged_public(knob, (ValueChangedReasonEnum)reason, time, view, originatedFromMT);
}

void
KnobHolder::appendValueChange(KnobI* knob,
                              double time,
                              ViewSpec view,
                              ValueChangedReasonEnum reason)
{
    if (isInitializingKnobs()) {
        return;
    }
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);

        KnobChange k;
        k.reason = reason;
        k.originatedFromMainThread = QThread::currentThread() == qApp->thread();
        k.knob = knob;
        
        if (knob && !knob->isValueChangesBlocked()) {
            if (!k.originatedFromMainThread && !canHandleEvaluateOnChangeInOtherThread()) {
                Q_EMIT doValueChangeOnMainThread(knob, reason, time, view, k.originatedFromMainThread);
            } else {
                onKnobValueChanged_public(knob, reason, time, view, k.originatedFromMainThread);
            }
        }
        
        if (reason == eValueChangedReasonTimeChanged) {
            return;
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


void
KnobHolder::getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const
{
    QMutexLocker k(&_imp->knobsMutex);
    for (KnobsVec::const_iterator it = _imp->knobs.begin(); it!=_imp->knobs.end(); ++it) {
        (*it)->getAllExpressionDependenciesRecursive(nodes);
    }
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
    if (appPTR->isBackground()) {
        _imp->paramsEditLevel = KnobHolder::eMultipleParamsEditOff;
    } else {
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
}

AppInstance*
KnobHolder::getApp() const
{
    return _imp->app;
}

void
KnobHolder::initializeKnobsPublic()
{
    {
        InitializeKnobsFlag_RAII __isInitializingKnobsFlag__(this);
        initializeKnobs();
    }
    _imp->knobsInitialized = true;
}

void
KnobHolder::refreshAfterTimeChange(bool isPlayback, double time)
{
    assert(QThread::currentThread() == qApp->thread());
    AppInstance* app = getApp();
    if (!app || app->isGuiFrozen()) {
        return;
    }
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->onTimeChanged(isPlayback, time);
    }
    refreshExtraStateAfterTimeChanged(time);
}

void
KnobHolder::refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(double time)
{
    assert(QThread::currentThread() == qApp->thread());
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i]->evaluateValueChangeOnTimeChange()) {
            _imp->knobs[i]->onTimeChanged(false, time);
        }
    }

}

void
KnobHolder::refreshInstanceSpecificKnobsOnly(bool isPlayback, double time)
{
    assert(QThread::currentThread() == qApp->thread());
    if (!getApp() || getApp()->isGuiFrozen()) {
        return;
    }
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if ( _imp->knobs[i]->isInstanceSpecific() ) {
            _imp->knobs[i]->onTimeChanged(isPlayback, time);
        }
    }
}

KnobPtr KnobHolder::getKnobByName(const std::string & name) const
{
    QMutexLocker k(&_imp->knobsMutex);
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }
    
    return KnobPtr();
}

// Same as getKnobByName expect that if we find the caller, we skip it
KnobPtr
KnobHolder::getOtherKnobByName(const std::string & name,
                               const KnobI* caller) const
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
    
    return KnobPtr();
 
}

const std::vector< KnobPtr > &
KnobHolder::getKnobs() const
{
    
    assert(QThread::currentThread() == qApp->thread());
    return _imp->knobs;
}

std::vector< KnobPtr >
KnobHolder::getKnobs_mt_safe() const
{
    QMutexLocker k(&_imp->knobsMutex);
    return _imp->knobs;
}

void
KnobHolder::slaveAllKnobs(KnobHolder* other,
                          bool restore)
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
        
        const KnobsVec & otherKnobs = other->getKnobs();
        const KnobsVec & thisKnobs = getKnobs();
        for (U32 i = 0; i < otherKnobs.size(); ++i) {
            
            if (otherKnobs[i]->isDeclaredByPlugin() || otherKnobs[i]->isUserKnob()) {
                KnobPtr foundKnob;
                for (U32 j = 0; j < thisKnobs.size(); ++j) {
                    if ( thisKnobs[j]->getName() == otherKnobs[i]->getName() ) {
                        foundKnob = thisKnobs[j];
                        break;
                    }
                }
                assert(foundKnob);
                if (!foundKnob) {
                    continue;
                }
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
    const KnobsVec & thisKnobs = getKnobs();
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
KnobHolder::beginKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    
    RECURSIVE_ACTION();
    beginKnobsValuesChanged(reason);
}

void
KnobHolder::endKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    
    RECURSIVE_ACTION();
    endKnobsValuesChanged(reason);
}

void
KnobHolder::onKnobValueChanged_public(KnobI* k,
                                      ValueChangedReasonEnum reason,
                                      double time,
                                      ViewSpec view,
                                      bool originatedFromMainThread)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->knobsInitialized) {
        return;
    }
    RECURSIVE_ACTION();
    onKnobValueChanged(k, reason, time, view, originatedFromMainThread);
}

void
KnobHolder::evaluate_public(KnobI* knob,
                            bool isSignificant,
                            ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    
    evaluate(knob, isSignificant,reason);
    
    if ( isSignificant && getApp() ) {
        ///Don't trigger autosaves for buttons
        KnobButton* isButton = dynamic_cast<KnobButton*>(knob);
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
KnobHolder::restoreDefaultValues()
{
    assert(QThread::currentThread() == qApp->thread());
    
    aboutToRestoreDefaultValues();
    
    beginChanges();

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        KnobButton* isBtn = dynamic_cast<KnobButton*>( _imp->knobs[i].get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( _imp->knobs[i].get() );
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>( _imp->knobs[i].get() );
        KnobSeparator* isSeparator = dynamic_cast<KnobSeparator*>( _imp->knobs[i].get() );
        
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
    KnobsVec  knobs = getKnobs_mt_safe();
    
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

bool
KnobHolder::isDequeueingValuesSet() const
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        return _imp->isDequeingValuesSet;
    }
}

bool
KnobHolder::dequeueValuesSet()
{
    assert(QThread::currentThread() == qApp->thread());
    beginChanges();
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        _imp->isDequeingValuesSet = true;
    }
    bool ret = false;
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        ret |= _imp->knobs[i]->dequeueValuesSet(false);
    }
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        _imp->isDequeingValuesSet = false;
    }
    endChanges();
    return ret;
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

int
KnobHolder::getPageIndex(const KnobPage* page) const
{
    QMutexLocker k(&_imp->knobsMutex);
    int pageIndex = 0;
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        KnobPage* ispage = dynamic_cast<KnobPage*>(_imp->knobs[i].get());
        if (ispage) {
            if (page == ispage) {
                return pageIndex;
            } else {
                ++pageIndex;
            }
        }
    }
    return -1;
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
        
        for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
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
AnimatingKnobStringHelper::cloneExtraData(KnobI* other,int /*dimension*/ )
{
    AnimatingKnobStringHelper* isAnimatedString = dynamic_cast<AnimatingKnobStringHelper*>(other);
    
    if (isAnimatedString) {
        _animation->clone( isAnimatedString->getAnimation() );
    }
}

bool
AnimatingKnobStringHelper::cloneExtraDataAndCheckIfChanged(KnobI* other,int /*dimension*/)
{
    AnimatingKnobStringHelper* isAnimatedString = dynamic_cast<AnimatingKnobStringHelper*>(other);
    
    if (isAnimatedString) {
       return  _animation->cloneAndCheckIfChanged( isAnimatedString->getAnimation() );
    }
    return false;
}

void
AnimatingKnobStringHelper::cloneExtraData(KnobI* other,
                                           double offset,
                                           const RangeD* range,
                                           int /*dimension*/)
{
    AnimatingKnobStringHelper* isAnimatedString = dynamic_cast<AnimatingKnobStringHelper*>(other);
    
    if (isAnimatedString) {
        _animation->clone(isAnimatedString->getAnimation(), offset, range);
    }
}

AnimatingKnobStringHelper::AnimatingKnobStringHelper(KnobHolder* holder,
                                                       const std::string &description,
                                                       int dimension,
                                                       bool declaredByPlugin)
: Knob<std::string>(holder,description,dimension,declaredByPlugin)
, _animation( new StringAnimationManager(this) )
{
}

AnimatingKnobStringHelper::~AnimatingKnobStringHelper()
{
    delete _animation;
}

void
AnimatingKnobStringHelper::stringToKeyFrameValue(double time,
                                                 ViewSpec /*view*/,
                                                 const std::string & v,
                                                 double* returnValue)
{
    _animation->insertKeyFrame(time, v, returnValue);
}

void
AnimatingKnobStringHelper::stringFromInterpolatedValue(double interpolated,
                                                       ViewSpec view,
                                                       std::string* returnValue) const
{
    assert(!view.isAll());
    assert(!view.isCurrent()); // not yet implemented
    Q_UNUSED(view);
    _animation->stringFromInterpolatedIndex(interpolated, returnValue);
}

void
AnimatingKnobStringHelper::animationRemoved_virtual(int /*dimension*/)
{
    _animation->clearKeyFrames();
}

void
AnimatingKnobStringHelper::keyframeRemoved_virtual(int /*dimension*/,
                                                   double time)
{
    _animation->removeKeyFrame(time);
}

std::string
AnimatingKnobStringHelper::getStringAtTime(double time,
                                           ViewSpec view,
                                           int dimension) const
{
    std::string ret;
    assert(!view.isAll());
    assert(!view.isCurrent()); // not yet implemented
    if ( _animation->hasCustomInterp() ) {
        bool succeeded = false;
        try {
            succeeded = _animation->customInterpolation(time, &ret);
        } catch (...) {
            
        }
        if (!succeeded) {
            return getValue(dimension, view);
        } else {
            return ret;
        }
    }
    
    return ret;
}

void
AnimatingKnobStringHelper::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                                   void* ofxParamHandle)
{
    _animation->setCustomInterpolation(func, ofxParamHandle);
}

void
AnimatingKnobStringHelper::loadAnimation(const std::map<int,std::string> & keyframes)
{
    _animation->load(keyframes);
}

void
AnimatingKnobStringHelper::saveAnimation(std::map<int,std::string>* keyframes) const
{
    _animation->save(keyframes);
}

/***************************KNOB EXPLICIT TEMPLATE INSTANTIATION******************************************/


template class Knob<int>;
template class Knob<double>;
template class Knob<bool>;
template class Knob<std::string>;

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Knob.cpp"
