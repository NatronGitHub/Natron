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

#include "OfxImageEffectInstance.h"

#include <cassert>
#include <cstdarg>
#include <memory>
#include <string>
#include <map>
#include <locale>
#include <stdexcept>

#include <QDebug>

//ofx extension
#include <nuke/fnPublicOfxExtensions.h>

//for parametric params properties
#include <ofxParametricParam.h>

#include "ofxNatron.h"
#include <ofxhUtilities.h> // for StatStr

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFactory.h"
#include "Engine/OfxMemory.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Format.h"
#include "Engine/Node.h"
#include "Global/MemoryInfo.h"
#include "Engine/ViewerInstance.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_ENTER;

// see second answer of http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
static
std::string
string_format(const std::string fmt, ...)
{
    int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;
    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {  // Everything worked
            str.resize(n);
            return str;
        }
        if (n > -1) { // Needed size returned
            size = n + 1;   // For null char
        } else {
            size *= 2;      // Guess at a larger size (OS specific)
        }
    }
    return str;
}


OfxImageEffectInstance::OfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                               OFX::Host::ImageEffect::Descriptor & desc,
                                               const std::string & context,
                                               bool interactive)
    : OFX::Host::ImageEffect::Instance(plugin, desc, context, interactive)
      , _ofxEffectInstance(NULL)
      , _parentingMap()
{
}

OfxImageEffectInstance::~OfxImageEffectInstance()
{
}


class ThreadIsActionCaller_RAII
{
    OfxImageEffectInstance* _self;
    
public:
    
    ThreadIsActionCaller_RAII(OfxImageEffectInstance* self)
    : _self(self)
    {
        appPTR->setThreadAsActionCaller(_self,true);
    }
    
    ~ThreadIsActionCaller_RAII()
    {
        appPTR->setThreadAsActionCaller(_self,false);
    }
};

OfxStatus
OfxImageEffectInstance::mainEntry(const char *action,
                                  const void *handle,
                                  OFX::Host::Property::Set *inArgs,
                                  OFX::Host::Property::Set *outArgs)
{
    ThreadIsActionCaller_RAII t(this);
    return OFX::Host::ImageEffect::Instance::mainEntry(action, handle, inArgs, outArgs);
}

const std::string &
OfxImageEffectInstance::getDefaultOutputFielding() const
{
    static const std::string v(kOfxImageFieldNone);

    return v;
}

/// make a clip
OFX::Host::ImageEffect::ClipInstance*
OfxImageEffectInstance::newClipInstance(OFX::Host::ImageEffect::Instance* plugin,
                                        OFX::Host::ImageEffect::ClipDescriptor* descriptor,
                                        int index)
{
    Q_UNUSED(plugin);

    return new OfxClipInstance(getOfxEffectInstance(), this, index, descriptor);
}


OfxStatus
OfxImageEffectInstance::setPersistentMessage(const char* type,
                                             const char* /*id*/,
                                             const char* format,
                                             va_list args)
{
    assert(type);
    assert(format);
    std::string message = string_format(format, args);

    if (strcmp(type, kOfxMessageError) == 0) {
        _ofxEffectInstance->setPersistentMessage(eMessageTypeError, message);
    } else if ( strcmp(type, kOfxMessageWarning) == 0 ) {
        _ofxEffectInstance->setPersistentMessage(eMessageTypeWarning, message);
    } else if ( strcmp(type, kOfxMessageMessage) == 0 ) {
        _ofxEffectInstance->setPersistentMessage(eMessageTypeInfo, message);
    }

    return kOfxStatOK;
}

OfxStatus
OfxImageEffectInstance::clearPersistentMessage()
{
    _ofxEffectInstance->clearPersistentMessage(false);

    return kOfxStatOK;
}


OfxStatus
OfxImageEffectInstance::vmessage(const char* msgtype,
                                 const char* /*id*/,
                                 const char* format,
                                 va_list args)
{
    assert(msgtype);
    assert(format);
    std::string message = string_format(format, args);
    std::string type(msgtype);

    if (type == kOfxMessageLog) {
        appPTR->writeToOfxLog_mt_safe( message.c_str() );
    } else if ( (type == kOfxMessageFatal) || (type == kOfxMessageError) ) {
        _ofxEffectInstance->message(eMessageTypeError, message);
    } else if (type == kOfxMessageWarning) {
        _ofxEffectInstance->message(eMessageTypeWarning, message);
    } else if (type == kOfxMessageMessage) {
        _ofxEffectInstance->message(eMessageTypeInfo, message);
    } else if (type == kOfxMessageQuestion) {
        if ( _ofxEffectInstance->message(eMessageTypeQuestion, message) ) {
            return kOfxStatReplyYes;
        } else {
            return kOfxStatReplyNo;
        }
    }

    return kOfxStatReplyDefault;
}

// The size of the current project in canonical coordinates.
// The size of a project is a sub set of the kOfxImageEffectPropProjectExtent. For example a
// project may be a PAL SD project, but only be a letter-box within that. The project size is
// the size of this sub window.
void
OfxImageEffectInstance::getProjectSize(double & xSize,
                                       double & ySize) const
{
    Format f;
    _ofxEffectInstance->getRenderFormat(&f);
    RectI pixelF;
    pixelF.x1 = f.x1;
    pixelF.x2 = f.x2;
    pixelF.y1 = f.y1;
    pixelF.y2 = f.y2;
    RectD canonicalF;
    pixelF.toCanonical_noClipping(0, f.getPixelAspectRatio(), &canonicalF);
    xSize = canonicalF.width();
    ySize = canonicalF.height();
}

// The offset of the current project in canonical coordinates.
// The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin
// of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
// project offset is the offset to the bottom left hand corner of the letter box. The project
// offset is in canonical coordinates.
void
OfxImageEffectInstance::getProjectOffset(double & xOffset,
                                         double & yOffset) const
{
    Format f;
    _ofxEffectInstance->getRenderFormat(&f);
    RectI pixelF;
    pixelF.x1 = f.x1;
    pixelF.x2 = f.x2;
    pixelF.y1 = f.y1;
    pixelF.y2 = f.y2;
    RectD canonicalF;
    pixelF.toCanonical_noClipping(0, f.getPixelAspectRatio(), &canonicalF);
    xOffset = canonicalF.left();
    yOffset = canonicalF.bottom();
}

// The extent of the current project in canonical coordinates.
// The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems
// for more infomation on the project extent. The extent is in canonical coordinates and only
// returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD
// project would have an extent of 768, 576.
void
OfxImageEffectInstance::getProjectExtent(double & xSize,
                                         double & ySize) const
{
    Format f;
    _ofxEffectInstance->getRenderFormat(&f);
    RectI pixelF;
    pixelF.x1 = f.x1;
    pixelF.x2 = f.x2;
    pixelF.y1 = f.y1;
    pixelF.y2 = f.y2;
    RectD canonicalF;
    pixelF.toCanonical_noClipping(0, f.getPixelAspectRatio(), &canonicalF);
    xSize = canonicalF.right();
    ySize = canonicalF.top();
}

// The pixel aspect ratio of the current project
double
OfxImageEffectInstance::getProjectPixelAspectRatio() const
{
    assert(_ofxEffectInstance);
    Format f;
    _ofxEffectInstance->getRenderFormat(&f);

    return f.getPixelAspectRatio();
}

// The duration of the effect
// This contains the duration of the plug-in effect, in frames.
double
OfxImageEffectInstance::getEffectDuration() const
{
    assert( getOfxEffectInstance() );
#pragma message WARN("getEffectDuration unimplemented, should we store the previous result to getTimeDomain ?")

    return 1.0;
}

// For an instance, this is the frame rate of the project the effect is in.
double
OfxImageEffectInstance::getFrameRate() const
{
    assert( getOfxEffectInstance() && getOfxEffectInstance()->getApp() );
    return getOfxEffectInstance()->getApp()->getProjectFrameRate();
}

/// This is called whenever a param is changed by the plugin so that
/// the recursive instanceChangedAction will be fed the correct frame
double
OfxImageEffectInstance::getFrameRecursive() const
{
    assert( getOfxEffectInstance() );
    return getOfxEffectInstance()->getCurrentTime();
}


/// This is called whenever a param is changed by the plugin so that
/// the recursive instanceChangedAction will be fed the correct
/// renderScale
void
OfxImageEffectInstance::getRenderScaleRecursive(double &x,
                                                double &y) const
{
    assert( getOfxEffectInstance() );
    std::list<ViewerInstance*> attachedViewers;
    getOfxEffectInstance()->getNode()->hasViewersConnected(&attachedViewers);
    ///get the render scale of the 1st viewer
    if ( !attachedViewers.empty() ) {
        ViewerInstance* first = attachedViewers.front();
        int mipMapLevel = first->getMipMapLevel();
        x = Image::getScaleFromMipMapLevel( (unsigned int)mipMapLevel );
        y = x;
    } else {
        x = 1.;
        y = 1.;
    }
}


OfxStatus
OfxImageEffectInstance::getViewCount(int *nViews) const
{
    *nViews = getOfxEffectInstance()->getApp()->getProject()->getProjectViewsCount();
    return kOfxStatOK;
}

OfxStatus
OfxImageEffectInstance::getViewName(int viewIndex, const char** name) const
{
    const std::vector<std::string>& views = getOfxEffectInstance()->getApp()->getProject()->getProjectViewNames();
    if (viewIndex >= 0 && viewIndex < (int)views.size()) {
        *name = views[viewIndex].data();
        return kOfxStatOK;
    }
    return kOfxStatErrBadIndex;
}

///These props are properties of the PARAMETER descriptor but the describe function of the INTERACT descriptor
///expects those properties to exist, so we add them to the INTERACT descriptor.
static const OFX::Host::Property::PropSpec interactDescProps[] = {
    { kOfxParamPropInteractSize,        OFX::Host::Property::eInt,  2, true, "0" },
    { kOfxParamPropInteractSizeAspect,  OFX::Host::Property::eDouble,  1, false, "1" },
    { kOfxParamPropInteractMinimumSize, OFX::Host::Property::eDouble,  2, false, "10" },
    { kOfxParamPropInteractPreferedSize,OFX::Host::Property::eInt,     2, false, "10" },
    OFX::Host::Property::propSpecEnd
};

// make a parameter instance
OFX::Host::Param::Instance *
OfxImageEffectInstance::newParam(const std::string &paramName,
                                 OFX::Host::Param::Descriptor &descriptor)
{
    // note: the order for parameter types is the same as in ofxParam.h
    OFX::Host::Param::Instance* instance = NULL;
    boost::shared_ptr<KnobI> knob;
    bool paramShouldBePersistant = true;

    if (descriptor.getType() == kOfxParamTypeInteger) {
        OfxIntegerInstance *ret = new OfxIntegerInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeDouble) {
        OfxDoubleInstance  *ret = new OfxDoubleInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeBoolean) {
        OfxBooleanInstance *ret = new OfxBooleanInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeChoice) {
        OfxChoiceInstance *ret = new OfxChoiceInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeRGBA) {
        OfxRGBAInstance *ret = new OfxRGBAInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeRGB) {
        OfxRGBInstance *ret = new OfxRGBInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeDouble2D) {
        OfxDouble2DInstance *ret = new OfxDouble2DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeInteger2D) {
        OfxInteger2DInstance *ret = new OfxInteger2DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeDouble3D) {
        OfxDouble3DInstance *ret = new OfxDouble3DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeInteger3D) {
        OfxInteger3DInstance *ret = new OfxInteger3DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeString) {
        OfxStringInstance *ret = new OfxStringInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(),
                             SIGNAL( animationLevelChanged(int,int) ),
                             ret,
                             SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeCustom) {
        /*
           http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamTypeCustom

           Custom parameters contain null terminated char * C strings, and may animate. They are designed to provide plugins with a way of storing data that is too complicated or impossible to store in a set of ordinary parameters.

           If a custom parameter animates, it must set its kOfxParamPropCustomInterpCallbackV1 property, which points to a OfxCustomParamInterpFuncV1 function. This function is used to interpolate keyframes in custom params.

           Custom parameters have no interface by default. However,

         * if they animate, the host's animation sheet/editor should present a keyframe/curve representation to allow positioning of keys and control of interpolation. The 'normal' (ie: paged or hierarchical) interface should not show any gui.
         * if the custom param sets its kOfxParamPropInteractV1 property, this should be used by the host in any normal (ie: paged or hierarchical) interface for the parameter.

           Custom parameters are mandatory, as they are simply ASCII C strings. However, animation of custom parameters an support for an in editor interact is optional.
         */
        //throw std::runtime_error(std::string("Parameter ") + paramName + " has unsupported OFX type " + descriptor.getType());
        OfxCustomInstance *ret = new OfxCustomInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        assert(knob);
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(), SIGNAL( animationLevelChanged(int,int) ),ret,SLOT( onKnobAnimationLevelChanged(int,int) ) );
        }
        instance = ret;
    } else if (descriptor.getType() == kOfxParamTypeGroup) {
        OfxGroupInstance *ret = new OfxGroupInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
        paramShouldBePersistant = false;
    } else if (descriptor.getType() == kOfxParamTypePage) {
        OfxPageInstance* ret = new OfxPageInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
#ifdef DEBUG_PAGE
        qDebug() << "Page " << descriptor.getName().c_str() << " has children:";
        int nChildren = ret->getProperties().getDimension(kOfxParamPropPageChild);
        for(int i = 0; i < nChildren; ++i) {
            qDebug() << "- " << ret->getProperties().getStringProperty(kOfxParamPropPageChild,i).c_str();
        }
#endif
        instance = ret;
        paramShouldBePersistant = false;
    } else if (descriptor.getType() == kOfxParamTypePushButton) {
        OfxPushButtonInstance *ret = new OfxPushButtonInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
        //paramShouldBePersistant = false;
    } else if (descriptor.getType() == kOfxParamTypeParametric) {
        OfxParametricInstance* ret = new OfxParametricInstance(getOfxEffectInstance(), descriptor);
        OfxStatus stat = ret->defaultInitializeAllCurves(descriptor);
        
        if (stat == kOfxStatFailed) {
            throw std::runtime_error("The parameter failed to create curves from their default\n"
                                     "initialized by the plugin.");
        }
        ret->onCurvesDefaultInitialized();

        knob = ret->getKnob();
        instance = ret;
    }


    if (!instance) {
        throw std::runtime_error( std::string("Parameter ") + paramName + " has unknown OFX type " + descriptor.getType() );
    }

    std::string parent = instance->getParentName();
    if ( !parent.empty() ) {
        _parentingMap.insert( make_pair(instance,parent) );
    }
   
    knob->setName(paramName);
    knob->setEvaluateOnChange( descriptor.getEvaluateOnChange() );

    bool persistant = descriptor.getIsPersistant();
    if (!paramShouldBePersistant) {
        persistant = false;
    }
    knob->setIsClipPreferencesSlave(isClipPreferencesSlaveParam(paramName));
    knob->setIsPersistant(persistant);
    knob->setAnimationEnabled( descriptor.getCanAnimate() );
    knob->setSecretByDefault( descriptor.getSecret() );
    knob->setDefaultAllDimensionsEnabled( descriptor.getEnabled() );
    knob->setHintToolTip( descriptor.getHint() );
    knob->setCanUndo( descriptor.getCanUndo() );
    knob->setSpacingBetweenItems( descriptor.getProperties().getIntProperty(kOfxParamPropLayoutPadWidth) );
    
    int layoutHint = descriptor.getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if (layoutHint == kOfxParamPropLayoutHintNoNewLine) {
        knob->setAddNewLine(false);
    } else if (layoutHint == kOfxParamPropLayoutHintDivider) {
        knob->setAddSeparator(true);
    }
    knob->setOfxParamHandle( (void*)instance->getHandle() );
    
    bool isInstanceSpecific = descriptor.getProperties().getIntProperty(kNatronOfxParamPropIsInstanceSpecific) != 0;
    if (isInstanceSpecific) {
        knob->setAsInstanceSpecific();
    }
    
    OfxParamToKnob* ptk = dynamic_cast<OfxParamToKnob*>(instance);
    assert(ptk);
    if (!ptk) {
        throw std::logic_error("");
    }
    ptk->connectDynamicProperties();
    
    OfxPluginEntryPoint* interact =
        (OfxPluginEntryPoint*)descriptor.getProperties().getPointerProperty(kOfxParamPropInteractV1);
    if (interact) {
        OFX::Host::Interact::Descriptor & interactDesc = ptk->getInteractDesc();
        interactDesc.getProperties().addProperties(interactDescProps);
        interactDesc.setEntryPoint(interact);
        interactDesc.describe(8, false);
    }
    
    return instance;
} // newParam

void
OfxImageEffectInstance::addParamsToTheirParents()
{
    const std::list<OFX::Host::Param::Instance*> & params = getParamList();
    const std::map<std::string, OFX::Host::Param::Instance*> & paramsMap = getParams();
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _ofxEffectInstance->getKnobs();
    
    //for each params find their parents if any and add to the parent this param's knob
    std::list<OfxPageInstance*> pages;
    for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it != params.end(); ++it) {
        OfxPageInstance* isPage = dynamic_cast<OfxPageInstance*>(*it);
        if (isPage) {
            pages.push_back(isPage);
        } else {
            std::map<OFX::Host::Param::Instance*,std::string>::const_iterator found = _parentingMap.find(*it);

            //the param has no parent
            if ( found == _parentingMap.end() ) {
                continue;
            }

            assert( !found->second.empty() );

            //find the parent by name
            std::map<std::string, OFX::Host::Param::Instance*>::const_iterator foundParent = paramsMap.find(found->second);

            //the parent must exist!
            assert( foundParent != paramsMap.end() );

            //add the param's knob to the parent
            OfxParamToKnob* knobHolder = dynamic_cast<OfxParamToKnob*>(found->first);
            assert(knobHolder);
            OfxGroupInstance* grp = (( foundParent != paramsMap.end() ) ?
                                     dynamic_cast<OfxGroupInstance*>(foundParent->second) :
                                     0);
            assert(grp);
            if (grp && knobHolder) {
                grp->addKnob( knobHolder->getKnob() );
            } else {
                // coverity[dead_error_line]
                std::cerr << "Warning: attempting to set a parent which is not a group to parameter " << (*it)->getName() << std::endl;;
                continue;
            }
            
            int layoutHint = (*it)->getProperties().getIntProperty(kOfxParamPropLayoutHint);
            if (layoutHint == kOfxParamPropLayoutHintDivider) {
                
                boost::shared_ptr<KnobSeparator> sep = AppManager::createKnob<KnobSeparator>( getOfxEffectInstance(),"");
                sep->setName((*it)->getName() + "_separator");
                if (grp) {
                    grp->addKnob(sep);
                }
            }
        }
    }
    
    ///Add parameters to pages if they do not have a parent
    for (std::list<OfxPageInstance*>::const_iterator it = pages.begin(); it != pages.end(); ++it) {
        int nChildren = (*it)->getProperties().getDimension(kOfxParamPropPageChild);
        for (int i = 0; i < nChildren; ++i) {
            std::string childName = (*it)->getProperties().getStringProperty(kOfxParamPropPageChild,i);
            if (childName == kOfxParamPageSkipRow ||
                childName == kOfxParamPageSkipColumn) {
                // Pseudo parameter names used to skip a row/column in a page layout.
                continue;
            }

            boost::shared_ptr<KnobI> child;
            for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
                if ((*it2)->getOriginalName() == childName) {
                    child = *it2;
                    break;
                }
            }
            if (!child) {
                std::cerr << "Warning: " << childName << " is in the children list of " << (*it)->getName() << " but does not seem to be a valid parameter." << std::endl;
                continue;
            }
            if (child && !child->getParentKnob()) {
                OfxParamToKnob* knobHolder = dynamic_cast<OfxParamToKnob*>(*it);
                assert(knobHolder);
                if (knobHolder) {
                    boost::shared_ptr<KnobI> knob_i = knobHolder->getKnob();
                    assert(knob_i);
                    if (knob_i) {
                        KnobPage* pageKnob = dynamic_cast<KnobPage*>(knob_i.get());
                        assert(pageKnob);
                        if (pageKnob) {
                            pageKnob->addKnob(child);
                        
                            if (child->isSeparatorActivated()) {
                    
                                boost::shared_ptr<KnobSeparator> sep = AppManager::createKnob<KnobSeparator>( getOfxEffectInstance(),"");
                                sep->setName(child->getName() + "_separator");
                                pageKnob->addKnob(sep);
                            }
                        }
                    }
                }
            }
        }

    }
}

/** @brief Used to group any parameter changes for undo/redo purposes

   \arg paramSet   the parameter set in which this is happening
   \arg name       label to attach to any undo/redo string UTF8

   If a plugin calls paramSetValue/paramSetValueAtTime on one or more parameters, either from custom GUI interaction
   or some analysis of imagery etc.. this is used to indicate the start of a set of a parameter
   changes that should be considered part of a single undo/redo block.

   See also OfxParameterSuiteV1::paramEditEnd

   \return
   - ::kOfxStatOK       - all was OK
   - ::kOfxStatErrBadHandle  - if the instance handle was invalid

 */
OfxStatus
OfxImageEffectInstance::editBegin(const std::string & /*name*/)
{
    ///Don't push undo/redo actions while creating a group
    if (!_ofxEffectInstance->getApp()->isCreatingPythonGroup()) {
        _ofxEffectInstance->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
    }

    return kOfxStatOK;
}

/// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditEnd
///
/// Client host code needs to implement this
OfxStatus
OfxImageEffectInstance::editEnd()
{
    ///Don't push undo/redo actions while creating a group
    if (!_ofxEffectInstance->getApp()->isCreatingPythonGroup()) {
        _ofxEffectInstance->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
    }

    return kOfxStatOK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// overridden for Progress::ProgressI

/// Start doing progress.
void
OfxImageEffectInstance::progressStart(const std::string & message, const std::string &messageid)
{
    _ofxEffectInstance->getApp()->progressStart(_ofxEffectInstance, message, messageid);
}

/// finish yer progress
void
OfxImageEffectInstance::progressEnd()
{
    _ofxEffectInstance->getApp()->progressEnd(_ofxEffectInstance);
}

/** @brief Indicate how much of the processing task has been completed and reports on any abort status.

   \arg \e effectInstance - the instance of the plugin this progress bar is
   associated with. It cannot be NULL.
   \arg \e progress - a number between 0.0 and 1.0 indicating what proportion of the current task has been processed.
   \returns false if you should abandon processing, true to continue
 */
bool
OfxImageEffectInstance::progressUpdate(double t)
{
    return _ofxEffectInstance->getApp()->progressUpdate(_ofxEffectInstance, t);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// overridden for TimeLine::TimeLineI

/// get the current time on the timeline. This is not necessarily the same
/// time as being passed to an action (eg render)
double
OfxImageEffectInstance::timeLineGetTime()
{
    return _ofxEffectInstance->getApp()->getTimeLine()->currentFrame();
}

/// set the timeline to a specific time
void
OfxImageEffectInstance::timeLineGotoTime(double t)
{
    _ofxEffectInstance->updateThreadLocalRenderTime( (int)t );
    
    ///Calling seek will force a re-render of the frame T so we wipe the overlay redraw needed counter
    bool redrawNeeded = _ofxEffectInstance->checkIfOverlayRedrawNeeded();
    Q_UNUSED(redrawNeeded);
    
    _ofxEffectInstance->getApp()->getTimeLine()->seekFrame( (int)t, false, 0, eTimelineChangeReasonOtherSeek);
}

/// get the first and last times available on the effect's timeline
void
OfxImageEffectInstance::timeLineGetBounds(double &t1,
                                          double &t2)
{
    double first,last;
    _ofxEffectInstance->getApp()->getFrameRange(&first, &last);
    t1 = first;
    t2 = last;
}

// override this to make processing abort, return 1 to abort processing
int
OfxImageEffectInstance::abort()
{
    return (int)getOfxEffectInstance()->aborted();
}

OFX::Host::Memory::Instance*
OfxImageEffectInstance::newMemoryInstance(size_t nBytes)
{
    OfxMemory* ret = new OfxMemory(_ofxEffectInstance);
    bool allocated = ret->alloc(nBytes);

    if ((nBytes != 0 && !ret->getPtr()) || !allocated) {
        Dialogs::errorDialog(QObject::tr("Out of memory").toStdString(), getOfxEffectInstance()->getNode()->getLabel_mt_safe() + QObject::tr(" failed to allocate memory (").toStdString() + printAsRAM(nBytes).toStdString() + ").");
    }

    return ret;
}



bool
OfxImageEffectInstance::areAllNonOptionalClipsConnected() const
{
    for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = _clips.begin(); it != _clips.end(); ++it) {
        if ( !it->second->isOutput() && !it->second->isOptional() && !it->second->getConnected() ) {
            return false;
        }
    }

    return true;
}

bool
OfxImageEffectInstance::getClipPreferences_safe(std::map<OfxClipInstance*, ClipPrefs>& clipPrefs,EffectPrefs& effectPrefs)
{
    /// create the out args with the stuff that does not depend on individual clips
    OFX::Host::Property::Set outArgs;
    
    double inputPar = 1.;
    bool inputParSet = false;
    bool mustWarnPar = false;
    bool outputFrameRateSet = false;
    double outputFrameRate = _outputFrameRate;
    bool mustWarnFPS = false;
    for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it2 = _clips.begin(); it2 != _clips.end(); ++it2) {
        if (!it2->second->isOutput() && it2->second->getConnected()) {
            if (!inputParSet) {
                inputPar = it2->second->getAspectRatio();
                inputParSet = true;
            } else if (inputPar != it2->second->getAspectRatio()) {
                // We have several inputs with different aspect ratio, which should be forbidden by the host.
                mustWarnPar = true;
            }
            
            if (!outputFrameRateSet) {
                outputFrameRate = it2->second->getFrameRate();
                outputFrameRateSet = true;
            } else if (std::abs(outputFrameRate - it2->second->getFrameRate()) > 0.01) {
                // We have several inputs with different frame rates
                mustWarnFPS = true;
            }

        }
    }

    
    try {
        setupClipPreferencesArgs(outArgs);
    } catch (OFX::Host::Property::Exception) {
        outArgs.setDoubleProperty("OfxImageClipPropPAR_Output" , inputPar);
        outArgs.setDoubleProperty(kOfxImageEffectPropFrameRate, getFrameRate());
    }
    if (mustWarnPar) {
        qDebug()
        << "WARNING: getClipPreferences() for "
        << _ofxEffectInstance->getScriptName_mt_safe().c_str()
        << ": This node has several input clips with different pixel aspect ratio but it does "
        "not support multiple input clips PAR. Your script or the GUI should have handled this "
        "earlier (before connecting the node @see Node::canConnectInput) .";
        outArgs.setDoubleProperty("OfxImageClipPropPAR_Output" , inputPar);

    }
    
    if (mustWarnFPS) {
        qDebug()
        << "WARNING: getClipPreferences() for "
        << _ofxEffectInstance->getScriptName_mt_safe().c_str()
        << ": This node has several input clips with different frame rates but it does "
        "not support it. Your script or the GUI should have handled this "
        "earlier (before connecting the node @see Node::canConnectInput) .";
        outArgs.setDoubleProperty(kOfxImageEffectPropFrameRate, getFrameRate());
        std::string name = _ofxEffectInstance->getScriptName_mt_safe();
        _ofxEffectInstance->setPersistentMessage(eMessageTypeWarning, "Several input clips with different pixel aspect ratio or different frame rates but it cannot handle it.");
    }

    if (mustWarnPar && !mustWarnFPS) {
        std::string name = _ofxEffectInstance->getNode()->getLabel_mt_safe();
        _ofxEffectInstance->setPersistentMessage(eMessageTypeWarning, "Several input clips with different pixel aspect ratio but it cannot handle it.");
    } else if (!mustWarnPar && mustWarnFPS) {
        std::string name = _ofxEffectInstance->getNode()->getLabel_mt_safe();
        _ofxEffectInstance->setPersistentMessage(eMessageTypeWarning, "Several input clips with different frame rates but it cannot handle it.");
    } else if (mustWarnPar && mustWarnFPS) {
        std::string name = _ofxEffectInstance->getNode()->getLabel_mt_safe();
        _ofxEffectInstance->setPersistentMessage(eMessageTypeWarning, "Several input clips with different pixel aspect ratio and different frame rates but it cannot handle it.");
    } else {
        if (_ofxEffectInstance->getNode()->hasPersistentMessage()) {
            _ofxEffectInstance->clearPersistentMessage(false);
        }
    }
    

#       ifdef OFX_DEBUG_ACTIONS
    std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetClipPreferences<<"()"<<std::endl;
#       endif
    OfxStatus st = mainEntry(kOfxImageEffectActionGetClipPreferences,
                             this->getHandle(),
                             0,
                             &outArgs);
#       ifdef OFX_DEBUG_ACTIONS
    std::cout << "OFX: "<<(void*)this<<"->"<<kOfxImageEffectActionGetClipPreferences<<"()->"<<OFX::StatStr(st);
#       endif
    
    if(st!=kOfxStatOK && st!=kOfxStatReplyDefault) {
#       ifdef OFX_DEBUG_ACTIONS
        std::cout << std::endl;
#       endif
        /// ouch
        return false;
    }
    
#       ifdef OFX_DEBUG_ACTIONS
    std::cout << ": ";
#       endif
    /// OK, go pump the components/depths back into the clips themselves
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it=_clips.begin();
        it!=_clips.end();
        ++it) {
        ClipPrefs prefs;
        
        std::string componentParamName = "OfxImageClipPropComponents_"+it->first;
        std::string depthParamName = "OfxImageClipPropDepth_"+it->first;
        std::string parParamName = "OfxImageClipPropPAR_"+it->first;
        
#       ifdef OFX_DEBUG_ACTIONS
        std::cout << it->first<<"->"<<outArgs.getStringProperty(depthParamName)<<","<<outArgs.getStringProperty(componentParamName)<<","<<outArgs.getDoubleProperty(parParamName)<<" ";
#       endif
        prefs.bitdepth = outArgs.getStringProperty(depthParamName);
        prefs.components = outArgs.getStringProperty(componentParamName);
        prefs.par = outArgs.getDoubleProperty(parParamName);
        OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
        assert(clip);
        if (clip) {
            clipPrefs.insert(std::make_pair(clip, prefs));
        }
    }
    
    
    effectPrefs.frameRate         = outArgs.getDoubleProperty(kOfxImageEffectPropFrameRate);
    effectPrefs.fielding          = outArgs.getStringProperty(kOfxImageClipPropFieldOrder);
    effectPrefs.premult           = outArgs.getStringProperty(kOfxImageEffectPropPreMultiplication);
    effectPrefs.continuous        = outArgs.getIntProperty(kOfxImageClipPropContinuousSamples) != 0;
    effectPrefs.frameVarying      = outArgs.getIntProperty(kOfxImageEffectFrameVarying) != 0;
    
#       ifdef OFX_DEBUG_ACTIONS
    std::cout << _outputFrameRate<<","<<_outputFielding<<","<<_outputPreMultiplication<<","<<_continuousSamples<<","<<_frameVarying<<std::endl;
#       endif
    
    
    _clipPrefsDirty  = false;
    
    return true;

}

bool
OfxImageEffectInstance::updatePreferences_safe(double frameRate,const std::string& fielding,const std::string& premult,
                            bool continuous,bool frameVarying)
{
    bool changed = false;
    if (_outputFrameRate != frameRate) {
        _outputFrameRate = frameRate;
        changed = true;
    }
    if (_outputFielding != fielding) {
        _outputFielding = fielding;
        changed = true;
    }
    if (_outputPreMultiplication != premult) {
        _outputPreMultiplication = premult;
        changed = true;
    }
    if (_continuousSamples != continuous) {
        _continuousSamples = continuous;
        changed = true;
    }
    if (_frameVarying != frameVarying) {
        _frameVarying = frameVarying;
        changed = true;
    }
    return changed;
}

const
std::map<std::string,OFX::Host::ImageEffect::ClipInstance*>&
OfxImageEffectInstance::getClips() const
{
    return _clips;
}

bool
OfxImageEffectInstance::getInputsHoldingTransform(std::list<int>* inputs) const
{
    if (!inputs) {
        return false;
    }
    for (std::map<std::string,OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = _clips.begin(); it != _clips.end(); ++it) {
        if (it->second && it->second->canTransform()) {
            
            ///Output clip should not have the property set.
            assert(!it->second->isOutput());
            if (it->second->isOutput()) {
                return false;
            }
            
            
            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            inputs->push_back(clip->getInputNb());
        }
    }
    return !inputs->empty();
}

#ifdef kOfxImageEffectPropInAnalysis // removed in OFX 1.4
bool
OfxImageEffectInstance::isInAnalysis() const
{
    return _properties.getIntProperty(kOfxImageEffectPropInAnalysis) == 1;
}
#endif

OfxImageEffectDescriptor::OfxImageEffectDescriptor(OFX::Host::Plugin *plug)
: OFX::Host::ImageEffect::Descriptor(plug)
{
    
}

OfxImageEffectDescriptor::OfxImageEffectDescriptor(const std::string &bundlePath, OFX::Host::Plugin *plug)
: OFX::Host::ImageEffect::Descriptor(bundlePath,plug)
{
    
}


OfxImageEffectDescriptor::OfxImageEffectDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                         OFX::Host::Plugin *plugin)
: OFX::Host::ImageEffect::Descriptor(rootContext,plugin)
{
    
}

OFX::Host::Param::Descriptor *
OfxImageEffectDescriptor::paramDefine(const char *paramType,
                                        const char *name)
{
    static const OFX::Host::Property::PropSpec hostOverlaysProps[] = {
        { kOfxParamPropHasHostOverlayHandle,  OFX::Host::Property::eInt,    1,    true,    "0" },
        { kOfxParamPropUseHostOverlayHandle,  OFX::Host::Property::eInt,    1,    false,    "0" },
        OFX::Host::Property::propSpecEnd
    };
    
    OFX::Host::Param::Descriptor *ret = OFX::Host::Param::SetDescriptor::paramDefine(paramType, name);
    OFX::Host::Property::Set& props = ret->getProperties();
    props.addProperties(hostOverlaysProps);
    
    if (strcmp(paramType, kOfxParamTypeDouble2D) == 0) {
        
        const std::string& type = ret->getDoubleType() ;
        // only kOfxParamDoubleTypeXYAbsolute and kOfxParamDoubleTypeNormalisedXYAbsolute are be supported
        if (//type == kOfxParamDoubleTypePlain ||
            type == kOfxParamDoubleTypeNormalisedXYAbsolute ||
            //type == kOfxParamDoubleTypeNormalisedXY ||
            //type == kOfxParamDoubleTypeXY ||
            type == kOfxParamDoubleTypeXYAbsolute) {
            props.setIntProperty(kOfxParamPropHasHostOverlayHandle, 1);
        }
    }
    return ret;
}


void
OfxImageEffectInstance::paramChangedByPlugin(OFX::Host::Param::Instance */*param*/)
{
    /*
     Do nothing: this is handled by Natron internally already in OfxEffectInstance::knobChanged.
     The reason for that is that the plug-in instanceChanged action may be called from a render thread if
     e.g a plug-in decides to violate the spec and set a parameter value in the render action.
     To prevent that, Natron already checks the current thread and calls the instanceChanged action in the appropriate thread.
     */
}

NATRON_NAMESPACE_EXIT;
