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
#include <cstring> // for std::memcpy, std::memset, std::strcmp

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

//ofx extension
#include <nuke/fnPublicOfxExtensions.h>

//for parametric params properties
#include <ofxParametricParam.h>

#include <ofxNatron.h>
#include <ofxhUtilities.h> // for StatStr
#include <ofxhPluginCache.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Format.h"
#include "Engine/Knob.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobTypes.h"
#include "Engine/MemoryInfo.h" // printAsRAM
#include "Engine/Node.h"
#include "Engine/NodeMetadata.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxMemory.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER

// see second answer of http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
static
std::string
string_format(const std::string fmt,
              ...)
{
    int size = ( (int)fmt.size() ) * 2 + 50;   // Use a rubric appropriate for your code
    std::string str;
    va_list ap;

    while (1) {     // Maximum two passes on a POSIX system...
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf( (char *)str.data(), size, fmt.c_str(), ap );
        va_end(ap);
        if ( (n > -1) && (n < size) ) {  // Everything worked
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
    , _ofxEffectInstance()
{
    getProps().setGetHook(kNatronOfxExtraCreatedPlanes, (OFX::Host::Property::GetHook*)this);

}

OfxImageEffectInstance::OfxImageEffectInstance(const OfxImageEffectInstance& other)
    : OFX::Host::ImageEffect::Instance(other)
    , _ofxEffectInstance()
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
        appPTR->setThreadAsActionCaller(_self, true);
    }

    ~ThreadIsActionCaller_RAII()
    {
        appPTR->setThreadAsActionCaller(_self, false);
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

OfxStatus
OfxImageEffectInstance::createInstanceAction()
{
    ///Overriden because the call to setDefaultClipPreferences is done in Natron

#       ifdef OFX_DEBUG_ACTIONS
    std::cout << "OFX: " << (void*)this << "->" << kOfxActionCreateInstance << "()" << std::endl;
#       endif
    // now tell the plug-in to create instance
    OfxStatus st = mainEntry(kOfxActionCreateInstance, this->getHandle(), 0, 0);
#       ifdef OFX_DEBUG_ACTIONS
    std::cout << "OFX: " << (void*)this << "->" << kOfxActionCreateInstance << "()->" << StatStr(st) << std::endl;
#       endif

    if (st == kOfxStatOK) {
        _created = true;
    }

    return st;
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
    boost::shared_ptr<OfxEffectInstance> effect = _ofxEffectInstance.lock();
    assert(effect);

    if (effect) {
        if (std::strcmp(type, kOfxMessageError) == 0) {
            effect->setPersistentMessage(eMessageTypeError, message);
        } else if (std::strcmp(type, kOfxMessageWarning) == 0) {
            effect->setPersistentMessage(eMessageTypeWarning, message);
        } else if (std::strcmp(type, kOfxMessageMessage) == 0) {
            effect->setPersistentMessage(eMessageTypeInfo, message);
        }
    }

    return kOfxStatOK;
}

OfxStatus
OfxImageEffectInstance::clearPersistentMessage()
{
    boost::shared_ptr<OfxEffectInstance> effect = _ofxEffectInstance.lock();
    if (!effect) {
        return kOfxStatFailed;
    }
    effect->clearPersistentMessage(false);

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
    boost::shared_ptr<OfxEffectInstance> effect = _ofxEffectInstance.lock();
    if (!effect) {
        return kOfxStatFailed;
    }
    if (type == kOfxMessageLog) {
        LogEntry::LogEntryColor c;
        if (effect->getNode()->getColor(&c.r, &c.g, &c.b)) {
            c.colorSet = true;
        }
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( effect->getNode()->getLabel().c_str() ), QDateTime::currentDateTime(),
                                        QString::fromUtf8( message.c_str() ), false, c);
    } else if ( (type == kOfxMessageFatal) || (type == kOfxMessageError) ) {
        effect->message(eMessageTypeError, message);
    } else if (type == kOfxMessageWarning) {
        effect->message(eMessageTypeWarning, message);
    } else if (type == kOfxMessageMessage) {
        effect->message(eMessageTypeInfo, message);
    } else if (type == kOfxMessageQuestion) {
        if ( effect->message(eMessageTypeQuestion, message) ) {
            return kOfxStatReplyYes;
        } else {
            return kOfxStatReplyNo;
        }
    }

    return kOfxStatReplyDefault;
}

const std::vector<std::string>&
OfxImageEffectInstance::getUserCreatedPlanes() const
{
    boost::shared_ptr<OfxEffectInstance> effect = _ofxEffectInstance.lock();
    const std::vector<std::string>& planes = effect->getUserPlanes();
    return planes;
}

int
OfxImageEffectInstance::getDimension(const std::string &name) const OFX_EXCEPTION_SPEC
{
    boost::shared_ptr<OfxEffectInstance> effect = _ofxEffectInstance.lock();
    if (!effect) {
        return 0;
    }
    if (name != kNatronOfxExtraCreatedPlanes) {
        return OFX::Host::ImageEffect::Instance::getDimension(name);
    }
    try {
        const std::vector<std::string>& planes = effect->getUserPlanes();
        return (int)planes.size();
    } catch (...) {
        throw OFX::Host::Property::Exception(kOfxStatErrUnknown);
    }
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

    _ofxEffectInstance.lock()->getApp()->getProject()->getProjectDefaultFormat(&f);
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

    _ofxEffectInstance.lock()->getApp()->getProject()->getProjectDefaultFormat(&f);
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

    _ofxEffectInstance.lock()->getApp()->getProject()->getProjectDefaultFormat(&f);
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
    assert( _ofxEffectInstance.lock() );
    Format f;
    _ofxEffectInstance.lock()->getApp()->getProject()->getProjectDefaultFormat(&f);

    return f.getPixelAspectRatio();
}

// The duration of the effect
// This contains the duration of the plug-in effect, in frames.
double
OfxImageEffectInstance::getEffectDuration() const
{
    assert( getOfxEffectInstance() );
    NodePtr node = getOfxEffectInstance()->getNode();
    if (!node) {
        return 0;
    }
    int firstFrame, lastFrame;
    bool lifetimeEnabled = node->isLifetimeActivated(&firstFrame, &lastFrame);
    if (lifetimeEnabled) {
        return std::max(double(lastFrame - firstFrame) + 1., 1.);
    } else {
        // return the project duration if the effect has no lifetime
        double projFirstFrame, projLastFrame;
        node->getApp()->getProject()->getFrameRange(&projFirstFrame, &projLastFrame);

        return std::max(projLastFrame - projFirstFrame + 1., 1.);
    }
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

// overridden from OFX::Host::ImageEffect::Instance
OfxStatus
OfxImageEffectInstance::getViewName(int viewIndex,
                                    const char** name) const
{
    const std::vector<std::string>& views = getOfxEffectInstance()->getApp()->getProject()->getProjectViewNames();

    if ( (viewIndex >= 0) && ( viewIndex < (int)views.size() ) ) {
        *name = views[viewIndex].data();

        return kOfxStatOK;
    }
    static const std::string emptyViewName;
    *name = emptyViewName.data();
    return kOfxStatErrBadIndex;
}

const OFX::Host::Property::PropSpec*
OfxImageEffectInstance::getOfxParamOverlayInteractDescProps()
{
    ///These props are properties of the PARAMETER descriptor but the describe function of the INTERACT descriptor
    ///expects those properties to exist, so we add them to the INTERACT descriptor.
    static const OFX::Host::Property::PropSpec interactDescProps[] = {
        { kOfxParamPropInteractSize,        OFX::Host::Property::eInt,  2, true, "0" },
        { kOfxParamPropInteractSizeAspect,  OFX::Host::Property::eDouble,  1, false, "1" },
        { kOfxParamPropInteractMinimumSize, OFX::Host::Property::eDouble,  2, false, "10" },
        { kOfxParamPropInteractPreferedSize, OFX::Host::Property::eInt,     2, false, "10" },
        OFX::Host::Property::propSpecEnd
    };

    return interactDescProps;
}

// make a parameter instance
OFX::Host::Param::Instance *
OfxImageEffectInstance::newParam(const std::string &paramName,
                                 OFX::Host::Param::Descriptor &descriptor)
{
    // note: the order for parameter types is the same as in ofxParam.h
    OFX::Host::Param::Instance* instance = NULL;
    KnobPtr knob;
    bool paramShouldBePersistent = true;
    bool secretByDefault = descriptor.getSecret();
    bool enabledByDefault = descriptor.getEnabled();

    std::string paramType = descriptor.getType();
    bool isToggableButton = false;
    if (paramType == kOfxParamTypeBoolean) {
        isToggableButton = (bool)descriptor.getProperties().getIntProperty(kNatronOfxBooleanParamPropIsToggableButton);
        if (isToggableButton) {
            paramType = kOfxParamTypePushButton;
        }
    }

    if (paramType == kOfxParamTypeInteger) {
        OfxIntegerInstance *ret = new OfxIntegerInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeDouble) {
        OfxDoubleInstance  *ret = new OfxDoubleInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeBoolean) {
        OfxBooleanInstance *ret = new OfxBooleanInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeChoice) {
        OfxChoiceInstance *ret = new OfxChoiceInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeRGBA) {
        OfxRGBAInstance *ret = new OfxRGBAInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeRGB) {
        OfxRGBInstance *ret = new OfxRGBInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeDouble2D) {
        OfxDouble2DInstance *ret = new OfxDouble2DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeInteger2D) {
        OfxInteger2DInstance *ret = new OfxInteger2DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeDouble3D) {
        OfxDouble3DInstance *ret = new OfxDouble3DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeInteger3D) {
        OfxInteger3DInstance *ret = new OfxInteger3DInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeString) {
        OfxStringInstance *ret = new OfxStringInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeCustom) {
        /*
           http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamTypeCustom

           Custom parameters contain null terminated char * C strings, and may animate. They are designed to provide plugins with a way of storing data that is too complicated or impossible to store in a set of ordinary parameters.

           If a custom parameter animates, it must set its kOfxParamPropCustomInterpCallbackV1 property, which points to a OfxCustomParamInterpFuncV1 function. This function is used to interpolate keyframes in custom params.

           Custom parameters have no interface by default. However,

         * if they animate, the host's animation sheet/editor should present a keyframe/curve representation to allow positioning of keys and control of interpolation. The 'normal' (ie: paged or hierarchical) interface should not show any gui.
         * if the custom param sets its kOfxParamPropInteractV1 property, this should be used by the host in any normal (ie: paged or hierarchical) interface for the parameter.

           Custom parameters are mandatory, as they are simply ASCII C strings. However, animation of custom parameters an support for an in editor interact is optional.
         */
        //throw std::runtime_error(std::string("Parameter ") + paramName + " has unsupported OFX type " + paramType);
        secretByDefault = true;
        enabledByDefault = false;
        OfxCustomInstance *ret = new OfxCustomInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        instance = ret;
    } else if (paramType == kOfxParamTypeGroup) {
        OfxGroupInstance *ret = new OfxGroupInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        KnobGroup* isGroup = dynamic_cast<KnobGroup*>(knob.get());
        assert(isGroup);
        if (isGroup) {
            bool haveShortcut = (bool)descriptor.getProperties().getIntProperty(kNatronOfxParamPropInViewerContextCanHaveShortcut);
            isGroup->setInViewerContextCanHaveShortcut(haveShortcut);
            bool isInToolbar = (bool)descriptor.getProperties().getIntProperty(kNatronOfxParamPropInViewerContextIsInToolbar);
            if (isInToolbar) {
                isGroup->setAsToolButton(true);
            } else {
                bool isDialog = (bool)descriptor.getProperties().getIntProperty(kNatronOfxGroupParamPropIsDialog);
                if (isDialog) {
                    isGroup->setAsDialog(true);
                }
            }
        }
        instance = ret;
        paramShouldBePersistent = false;
    } else if (paramType == kOfxParamTypePage) {
        OfxPageInstance* ret = new OfxPageInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
#ifdef DEBUG_PAGE
        qDebug() << "Page " << descriptor.getName().c_str() << " has children:";
        int nChildren = ret->getProperties().getDimension(kOfxParamPropPageChild);
        for (int i = 0; i < nChildren; ++i) {
            qDebug() << "- " << ret->getProperties().getStringProperty(kOfxParamPropPageChild, i).c_str();
        }
#endif
        KnobPage* isPage = dynamic_cast<KnobPage*>(knob.get());
        assert(isPage);
        if (isPage) {
            bool isInToolbar = (bool)descriptor.getProperties().getIntProperty(kNatronOfxParamPropInViewerContextIsInToolbar);
            if (isInToolbar) {
                isPage->setAsToolBar(true);
            }
        }
        instance = ret;
        paramShouldBePersistent = false;
    } else if (paramType == kOfxParamTypePushButton) {
        OfxPushButtonInstance *ret = new OfxPushButtonInstance(getOfxEffectInstance(), descriptor);
        knob = ret->getKnob();
        if (isToggableButton) {
            KnobButton* isBtn = dynamic_cast<KnobButton*>(knob.get());
            assert(isBtn);
            if (isBtn) {
                isBtn->setCheckable(true);
                int def = descriptor.getProperties().getIntProperty(kOfxParamPropDefault);
                isBtn->setValue((bool)def);

                bool haveShortcut = (bool)descriptor.getProperties().getIntProperty(kNatronOfxParamPropInViewerContextCanHaveShortcut);
                isBtn->setInViewerContextCanHaveShortcut(haveShortcut);
            }
        }
        instance = ret;
        //paramShouldBePersistent = false;
    } else if (paramType == kOfxParamTypeParametric) {
        OfxParametricInstance* ret = new OfxParametricInstance(getOfxEffectInstance(), descriptor);
        OfxStatus stat = ret->defaultInitializeAllCurves(descriptor);

        if (stat == kOfxStatFailed) {
            throw std::runtime_error("The parameter failed to create curves from their default\n"
                                     "initialized by the plug-in.");
        }
        ret->onCurvesDefaultInitialized();

        knob = ret->getKnob();
        instance = ret;
    }

    assert(knob);
    if (!instance) {
        throw std::runtime_error( std::string("Parameter ") + paramName + " has unknown OFX type " + paramType );
    }

#ifdef NATRON_ENABLE_IO_META_NODES
    /**
     * For readers/writers embedded in a ReadNode or WriteNode, the holder will be the ReadNode and WriteNode
     * but to ensure that all functions such as getKnobByName actually work, we add them to the knob vector so that
     * interacting with the Reader or the container is actually the same.
     **/
    if ( knob->getHolder() != getOfxEffectInstance().get() ) {
        getOfxEffectInstance()->addKnob(knob);
    }
#endif

    OfxParamToKnob* ptk = dynamic_cast<OfxParamToKnob*>(instance);
    assert(ptk);
    if (!ptk) {
        throw std::logic_error("");
    }

    //knob->setName(paramName);
    knob->setEvaluateOnChange( descriptor.getEvaluateOnChange() );

    bool persistent = descriptor.getIsPersistent();
    if (!paramShouldBePersistent) {
        persistent = false;
    }

    const std::string & iconFilePath = descriptor.getProperties().getStringProperty(kOfxPropIcon, 1);
    if ( !iconFilePath.empty() ) {
        knob->setIconLabel(iconFilePath);
    }

    bool isMarkdown = descriptor.getProperties().getIntProperty(kNatronOfxPropDescriptionIsMarkdown);
    knob->setHintIsMarkdown(isMarkdown);

    knob->setIsMetadataSlave( isClipPreferencesSlaveParam(paramName) );
    knob->setIsPersistent(persistent);
    knob->setAnimationEnabled( descriptor.getCanAnimate() );
    knob->setSecretByDefault(secretByDefault);
    knob->setDefaultAllDimensionsEnabled(enabledByDefault);
    knob->setHintToolTip( descriptor.getHint() );
    knob->setCanUndo( descriptor.getCanUndo() );
    knob->setSpacingBetweenItems( descriptor.getProperties().getIntProperty(kOfxParamPropLayoutPadWidth) );

    if ( knob->isAnimationEnabled() ) {
        boost::shared_ptr<KnobSignalSlotHandler> handler = knob->getSignalSlotHandler();
        if (handler) {
            QObject::connect( handler.get(), SIGNAL(animationLevelChanged(ViewSpec,int)), ptk,
                              SLOT(onKnobAnimationLevelChanged(ViewSpec,int)) );
        }
    }

    int layoutHint = descriptor.getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if (layoutHint == kOfxParamPropLayoutHintNoNewLine) {
        knob->setAddNewLine(false);
    } else if (layoutHint == kOfxParamPropLayoutHintDivider) {
        knob->setAddSeparator(true);
    }


    knob->setInViewerContextItemSpacing( descriptor.getProperties().getIntProperty(kNatronOfxParamPropInViewerContextLayoutPadWidth) );

    int viewportLayoutHint = descriptor.getProperties().getIntProperty(kNatronOfxParamPropInViewerContextLayoutHint);
    if (viewportLayoutHint == kNatronOfxParamPropInViewerContextLayoutHintAddNewLine) {
        knob->setInViewerContextNewLineActivated(true);
    } else if (viewportLayoutHint == kNatronOfxParamPropInViewerContextLayoutHintNormalDivider) {
        knob->setInViewerContextAddSeparator(true);
    }

    bool viewportSecret = (bool)descriptor.getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret);
    if (viewportSecret) {
        knob->setInViewerContextSecret(viewportSecret);
    }

    const std::string& viewportLabel = descriptor.getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel);
    if (!viewportLabel.empty()) {
        knob->setInViewerContextLabel(QString::fromUtf8(viewportLabel.c_str()));
    }


    knob->setOfxParamHandle( (void*)instance->getHandle() );

    bool isInstanceSpecific = descriptor.getProperties().getIntProperty(kNatronOfxParamPropIsInstanceSpecific) != 0;
    if (isInstanceSpecific) {
        knob->setAsInstanceSpecific();
    }

    ptk->connectDynamicProperties();

    return instance;
} // newParam

struct PageOrdered
{
    boost::shared_ptr<KnobPage> page;
    std::list<OfxParamToKnob*> paramsOrdered;
};

typedef std::list<boost::shared_ptr<PageOrdered> > PagesOrdered;

void
OfxImageEffectInstance::addParamsToTheirParents()
{
    //All parameters in their order of declaration by the plug-in
    const std::list<OFX::Host::Param::Instance*> & params = getParamList();
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    //Extract pages and their children and add knobs to groups
    PagesOrdered finalPages;

    for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it != params.end(); ++it) {
        OfxParamToKnob* isKnownKnob = dynamic_cast<OfxParamToKnob*>(*it);
        assert(isKnownKnob);
        if (!isKnownKnob) {
            continue;
        }
        KnobPtr associatedKnob = isKnownKnob->getKnob();
        if (!associatedKnob) {
            continue;
        }
        OfxPageInstance* isPage = dynamic_cast<OfxPageInstance*>(*it);
        if (isPage) {
            const std::map<int, OFX::Host::Param::Instance*>& children = isPage->getChildren();
            boost::shared_ptr<PageOrdered> pageData = boost::make_shared<PageOrdered>();
            pageData->page = boost::dynamic_pointer_cast<KnobPage>(associatedKnob);
            assert(pageData->page);
            std::map<OfxParamToKnob*, int> childrenList;
            for (std::map<int, OFX::Host::Param::Instance*>::const_iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                OfxParamToKnob* isParamToKnob = dynamic_cast<OfxParamToKnob*>(it2->second);
                assert(isParamToKnob);
                if (!isParamToKnob) {
                    continue;
                }
                pageData->paramsOrdered.push_back(isParamToKnob);
            }
            finalPages.push_back(pageData);
        } else {
            OFX::Host::Param::Instance* hasParent = (*it)->getParentInstance();
            if (hasParent) {
                OfxGroupInstance* parentIsGroup = dynamic_cast<OfxGroupInstance*>(hasParent);
                if (!parentIsGroup) {
                    std::cerr << getDescriptor().getPlugin()->getIdentifier() << ": Warning: attempting to set a parent which is not a group. (" << (*it)->getName() << ")" << std::endl;
                } else {
                    parentIsGroup->addKnob(associatedKnob);

                    ///Add a separator in the group if needed
                    if ( associatedKnob->isSeparatorActivated() ) {
                        std::string separatorName = (*it)->getName() + "_separator";
                        KnobHolder* knobHolder = associatedKnob->getHolder();
                        boost::shared_ptr<KnobSeparator> sep = knobHolder->getKnobByNameAndType<KnobSeparator>(separatorName);
                        if (sep) {
                            sep->resetParent();
                        } else {
                            sep = AppManager::createKnob<KnobSeparator>( knobHolder, std::string() );
                            assert(sep);
                            sep->setName(separatorName);
#ifdef NATRON_ENABLE_IO_META_NODES
                            /**
                             * For readers/writers embedded in a ReadNode or WriteNode, the holder will be the ReadNode and WriteNode
                             * but to ensure that all functions such as getKnobByName actually work, we add them to the knob vector so that
                             * interacting with the Reader or the container is actually the same.
                             **/
                            if ( knobHolder != getOfxEffectInstance().get() ) {
                                getOfxEffectInstance()->addKnob(sep);
                            }
#endif
                        }
                        parentIsGroup->addKnob(sep);
                    }
                }
            }
        }
    }

    //Extract the "Main" page, i.e: the first page declared, if no page were created, create one
    PagesOrdered::iterator mainPage = finalPages.end();
    if ( !finalPages.empty() ) {
        mainPage = finalPages.begin();
    } else {
        boost::shared_ptr<KnobPage> page = AppManager::createKnob<KnobPage>( effect.get(), tr("Settings") );
        boost::shared_ptr<PageOrdered> pageData = boost::make_shared<PageOrdered>();
        pageData->page = page;
        finalPages.push_back(pageData);
        mainPage = finalPages.begin();
    }
    assert( mainPage != finalPages.end() );
    if ( mainPage == finalPages.end() ) {
        throw std::logic_error("");
    }
    // In this pass we check that all parameters belong to a page.
    // For parameters that do not belong to a page, we add them "on the fly" to the page

    std::list<OfxParamToKnob*> &mainPageParamsOrdered = (*mainPage)->paramsOrdered;
    std::list<OfxParamToKnob*>::iterator lastParamInsertedInMainPage = mainPageParamsOrdered.end();

    for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it != params.end(); ++it) {
        OfxParamToKnob* isKnownKnob = dynamic_cast<OfxParamToKnob*>(*it);
        assert(isKnownKnob);
        if (!isKnownKnob) {
            continue;
        }

        KnobPtr knob = isKnownKnob->getKnob();
        assert(knob);
        if (!knob) {
            continue;
        }

        KnobPage* isPage = dynamic_cast<KnobPage*>( knob.get() );
        if (isPage) {
            continue;
        }


        bool foundPage = false;
        for (std::list<OfxParamToKnob*>::iterator itParam = mainPageParamsOrdered.begin(); itParam != mainPageParamsOrdered.end(); ++itParam) {
            if (isKnownKnob == *itParam) {
                foundPage = true;
                lastParamInsertedInMainPage = itParam;
                break;
            }
        }
        if (!foundPage) {
            // param not found in main page, try in other pages
            PagesOrdered::iterator itPage = mainPage;
            ++itPage;
            for (; itPage != finalPages.end(); ++itPage) {
                for (std::list<OfxParamToKnob*>::iterator itParam = (*itPage)->paramsOrdered.begin(); itParam != (*itPage)->paramsOrdered.end(); ++itParam) {
                    if (isKnownKnob == *itParam) {
                        foundPage = true;
                        break;
                    }
                }
                if (foundPage) {
                    break;
                }
            }
        }

        if (!foundPage) {
            //The parameter does not belong to a page, put it in the main page
            if ( lastParamInsertedInMainPage != mainPageParamsOrdered.end() ) {
                ++lastParamInsertedInMainPage;
            }
            lastParamInsertedInMainPage = mainPageParamsOrdered.insert(lastParamInsertedInMainPage, isKnownKnob);
        }
    } // for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it != params.end(); ++it)


    // For all pages, append their knobs in order
    for (PagesOrdered::iterator itPage = finalPages.begin(); itPage != finalPages.end(); ++itPage) {
        boost::shared_ptr<KnobPage> pageKnob = (*itPage)->page;

        for (std::list<OfxParamToKnob*>::iterator itParam = (*itPage)->paramsOrdered.begin(); itParam != (*itPage)->paramsOrdered.end(); ++itParam) {
            OfxParamToKnob* isKnownKnob = *itParam;
            assert(isKnownKnob);
            if (!isKnownKnob) {
                continue;
            }

            KnobPtr child = isKnownKnob->getKnob();
            assert(child);
            if ( !child->getParentKnob() ) {
                pageKnob->addKnob(child);

                if ( child->isSeparatorActivated() ) {
                    std::string separatorName = child->getName() + "_separator";
                    KnobHolder* knobHolder = child->getHolder();
                    boost::shared_ptr<KnobSeparator> sep = knobHolder->getKnobByNameAndType<KnobSeparator>(separatorName);
                    if (sep) {
                        sep->resetParent();
                    } else {
                        sep = AppManager::createKnob<KnobSeparator>( knobHolder, std::string() );
                        assert(sep);
                        sep->setName(separatorName);
#ifdef NATRON_ENABLE_IO_META_NODES
                        /**
                         * For readers/writers embedded in a ReadNode or WriteNode, the holder will be the ReadNode and WriteNode
                         * but to ensure that all functions such as getKnobByName actually work, we add them to the knob vector so that
                         * interacting with the Reader or the container is actually the same.
                         **/
                        if ( knobHolder != getOfxEffectInstance().get() ) {
                            getOfxEffectInstance()->addKnob(sep);
                        }
#endif
                    }
                    pageKnob->addKnob(sep);
                }
            } // if (!knob->getParentKnob()) {
        }
    }


    // Add the parameters to the viewport in order
    int nDims = getProps().getDimension(kNatronOfxImageEffectPropInViewerContextParamsOrder);
    for (int i = 0; i < nDims; ++i) {
        const std::string& paramName = getProps().getStringProperty(kNatronOfxImageEffectPropInViewerContextParamsOrder);
        OFX::Host::Param::Instance* param = getParam(paramName);
        if (!param) {
            continue;
        }
        OfxParamToKnob* isKnownKnob = dynamic_cast<OfxParamToKnob*>(param);
        assert(isKnownKnob);
        if (isKnownKnob) {
            KnobPtr knob = isKnownKnob->getKnob();
            assert(knob);
            effect->addKnobToViewerUI(knob);
        }

    }
} // OfxImageEffectInstance::addParamsToTheirParents

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
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    if ( !effect->getApp()->isCreatingPythonGroup() ) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOnCreateNewCommand);
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
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    if ( !effect->getApp()->isCreatingPythonGroup() ) {
        effect->setMultipleParamsEditLevel(KnobHolder::eMultipleParamsEditOff);
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
OfxImageEffectInstance::progressStart(const std::string & message,
                                      const std::string &messageid)
{
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    effect->getApp()->progressStart(effect->getNode(), message, messageid);
}

/// finish yer progress
void
OfxImageEffectInstance::progressEnd()
{
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    effect->getApp()->progressEnd( effect->getNode() );
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
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    return effect->getApp()->progressUpdate(effect->getNode(), t);
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
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    return effect->getApp()->getTimeLine()->currentFrame();
}

/// set the timeline to a specific time
void
OfxImageEffectInstance::timeLineGotoTime(double t)
{
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();

    effect->updateThreadLocalRenderTime(t);

    ///Calling seek will force a re-render of the frame T so we wipe the overlay redraw needed counter
    bool redrawNeeded = effect->checkIfOverlayRedrawNeeded();
    Q_UNUSED(redrawNeeded);

    effect->getApp()->getTimeLine()->seekFrame( (int)t, false, 0, eTimelineChangeReasonOtherSeek );
}

/// get the first and last times available on the effect's timeline
void
OfxImageEffectInstance::timeLineGetBounds(double &t1,
                                          double &t2)
{
    double first, last;

    _ofxEffectInstance.lock()->getApp()->getFrameRange(&first, &last);
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
    boost::shared_ptr<OfxEffectInstance> effect = getOfxEffectInstance();
    OfxMemory* ret = new OfxMemory(effect);
    bool allocated = ret->alloc(nBytes);

    if ( ( (nBytes != 0) && !ret->getPtr() ) || !allocated ) {
        Dialogs::errorDialog( tr("Out of memory").toStdString(),
                              tr("%1 failed to allocate memory (%2).")
                              .arg( QString::fromUtf8( getOfxEffectInstance()->getNode()->getLabel_mt_safe().c_str() ) )
                              .arg( printAsRAM(nBytes) ).toStdString() );
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

void
OfxImageEffectInstance::setupClipPreferencesArgsFromMetadata(NodeMetadata& metadata,
                                                             OFX::Host::Property::Set &outArgs,
                                                             std::map<OfxClipInstance*, int>& clipsMapping)
{
    static const OFX::Host::Property::PropSpec clipPrefsStuffs [] =
    {
        { kOfxImageEffectPropFrameRate,          OFX::Host::Property::eDouble,  1, false,  "1" },
        { kOfxImageEffectPropPreMultiplication,  OFX::Host::Property::eString,  1, false,  "" },
        { kOfxImageClipPropFieldOrder,           OFX::Host::Property::eString,  1, false,  "" },
        { kOfxImageClipPropContinuousSamples,    OFX::Host::Property::eInt,     1, false,  "0" },
        { kOfxImageEffectFrameVarying,           OFX::Host::Property::eInt,     1, false,  "0" },
        { kOfxImageClipPropFormat,             OFX::Host::Property::eInt,     4, false,  "0" },
        OFX::Host::Property::propSpecEnd
    };

    outArgs.addProperties(clipPrefsStuffs);

    /// set the default for those

    /// is there multiple bit depth support? Depends on host, plugin and context
    bool multiBitDepth = canCurrentlyHandleMultipleClipDepths();

    //Set the output frame rate according to what input clips have. Several inputs with different frame rates should be
    //forbidden by the host.
    double outputFrameRate = metadata.getOutputFrameRate();
    outArgs.setDoubleProperty(kOfxImageEffectPropFrameRate, outputFrameRate);
    outArgs.setStringProperty( kOfxImageEffectPropPreMultiplication, OfxClipInstance::natronsPremultToOfxPremult( metadata.getOutputPremult() ) );
    outArgs.setStringProperty( kOfxImageClipPropFieldOrder, OfxClipInstance::natronsFieldingToOfxFielding( metadata.getOutputFielding() ) );
    outArgs.setIntProperty( kOfxImageClipPropContinuousSamples, metadata.getIsContinuous() );
    outArgs.setIntProperty( kOfxImageEffectFrameVarying, metadata.getIsFrameVarying() );

    const RectI& outputFormat = metadata.getOutputFormat();
    outArgs.setIntPropertyN( kOfxImageClipPropFormat, (const int*)&outputFormat.x1, 4);

    /// now add the clip gubbins to the out args
    for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin();
         it != _clips.end();
         ++it) {
        OfxClipInstance *clip = dynamic_cast<OfxClipInstance*>(it->second);
        assert(clip);
        if (!clip) {
            continue;
        }

        int inputNb = clip->getInputNb();

        clipsMapping[clip] = inputNb;

        std::string componentParamName = "OfxImageClipPropComponents_" + it->first;
        std::string depthParamName     = "OfxImageClipPropDepth_" + it->first;
        std::string parParamName       = "OfxImageClipPropPAR_" + it->first;
        OFX::Host::Property::PropSpec specComp = {componentParamName.c_str(),  OFX::Host::Property::eString, 0, false,          ""}; // note the support for multi-planar clips
        outArgs.createProperty(specComp);

        std::string ofxClipComponentStr;
        std::string componentsType = metadata.getComponentsType(inputNb);
        int nComps = metadata.getNComps(inputNb);
        ImagePlaneDesc natronPlane = ImagePlaneDesc::mapNCompsToColorPlane(nComps);
        if (componentsType == kNatronColorPlaneID) {
            ofxClipComponentStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(natronPlane);
        } else if (componentsType == kNatronDisparityComponentsLabel) {
            ofxClipComponentStr = kFnOfxImageComponentStereoDisparity;
        } else if (componentsType == kNatronMotionComponentsLabel) {
            ofxClipComponentStr = kFnOfxImageComponentMotionVectors;
        } else {
            ofxClipComponentStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(natronPlane);
        }

        outArgs.setStringProperty( componentParamName.c_str(), ofxClipComponentStr.c_str() ); // as it is variable dimension, there is no default value, so we have to set it explicitly

        const std::string& bitDepthStr = OfxClipInstance::natronsDepthToOfxDepth( metadata.getBitDepth(inputNb) );
        OFX::Host::Property::PropSpec specDep = {depthParamName.c_str(),       OFX::Host::Property::eString, 1, !multiBitDepth, bitDepthStr.c_str()};
        outArgs.createProperty(specDep);

        OFX::Host::Property::PropSpec specPAR = {parParamName.c_str(),         OFX::Host::Property::eDouble, 1, false,          "1"};
        outArgs.createProperty(specPAR);
        outArgs.setDoubleProperty( parParamName, metadata.getPixelAspectRatio(inputNb) );
    }
} // OfxImageEffectInstance::setupClipPreferencesArgsFromMetadata

StatusEnum
OfxImageEffectInstance::getClipPreferences_safe(NodeMetadata& defaultPrefs)
{
    /// create the out args with the stuff that does not depend on individual clips
    OFX::Host::Property::Set outArgs;
    boost::shared_ptr<OfxEffectInstance> effect = _ofxEffectInstance.lock();
    std::map<OfxClipInstance*, int> clipInputs;

    setupClipPreferencesArgsFromMetadata(defaultPrefs, outArgs, clipInputs);


#       ifdef OFX_DEBUG_ACTIONS
    std::cout << "OFX: " << (void*)this << "->" << kOfxImageEffectActionGetClipPreferences << "()" << std::endl;
#       endif
    OfxStatus st = mainEntry(kOfxImageEffectActionGetClipPreferences,
                             this->getHandle(),
                             0,
                             &outArgs);
#       ifdef OFX_DEBUG_ACTIONS
    std::cout << "OFX: " << (void*)this << "->" << kOfxImageEffectActionGetClipPreferences << "()->" << OFX::StatStr(st);
#       endif

#       ifdef OFX_DEBUG_ACTIONS
    std::cout << ": ";
#       endif

    if (st == kOfxStatReplyDefault) {
#       ifdef OFX_DEBUG_ACTIONS
        std::cout << "default" << std::endl;
#       endif

        return eStatusReplyDefault;
    }

    /// Only copy the meta-data if they actually changed
    if (st != kOfxStatOK) {
#       ifdef OFX_DEBUG_ACTIONS
        std::cout << "error" << std::endl;
#       endif

        /// ouch
        return eStatusFailed;
    } else {
        /// OK, go pump the components/depths back into the clips themselves
        for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin();
             it != _clips.end();
             ++it) {
            OfxClipInstance *clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            if (!clip) {
                continue;
            }

            std::string componentParamName = "OfxImageClipPropComponents_" + it->first;
            std::string depthParamName = "OfxImageClipPropDepth_" + it->first;
            std::string parParamName = "OfxImageClipPropPAR_" + it->first;
            std::map<OfxClipInstance*, int>::iterator foundClip = clipInputs.find(clip);
            assert( foundClip != clipInputs.end() );
            if ( foundClip == clipInputs.end() ) {
                continue;
            }
            int inputNb = foundClip->second;

#       ifdef OFX_DEBUG_ACTIONS
            std::cout << it->first << "->" << outArgs.getStringProperty(depthParamName) << "," << outArgs.getStringProperty(componentParamName) << "," << outArgs.getDoubleProperty(parParamName) << " ";
#       endif

            defaultPrefs.setBitDepth( inputNb, OfxClipInstance::ofxDepthToNatronDepth( outArgs.getStringProperty(depthParamName) ) );
            ImagePlaneDesc plane, pairedPlane;
            std::string ofxComponentsType = outArgs.getStringProperty(componentParamName);
            ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(ofxComponentsType, &plane, &pairedPlane);
            defaultPrefs.setNComps( inputNb, plane.getNumComponents() );

            if (plane.isColorPlane()) {
                defaultPrefs.setComponentsType( inputNb, kNatronColorPlaneID);
            } else if (plane.getChannelsLabel() == kNatronMotionComponentsLabel) {
                defaultPrefs.setComponentsType( inputNb, kNatronMotionComponentsLabel);
            } else if (plane.getChannelsLabel() == kNatronDisparityComponentsLabel) {
                defaultPrefs.setComponentsType( inputNb, kNatronDisparityComponentsLabel);
            } else {
                defaultPrefs.setComponentsType( inputNb, ofxComponentsType);
            }

            defaultPrefs.setPixelAspectRatio( inputNb, outArgs.getDoubleProperty(parParamName) );
        }


        defaultPrefs.setOutputFrameRate( outArgs.getDoubleProperty(kOfxImageEffectPropFrameRate) );
        defaultPrefs.setOutputFielding( OfxClipInstance::ofxFieldingToNatronFielding( outArgs.getStringProperty(kOfxImageClipPropFieldOrder) ) );
        defaultPrefs.setOutputPremult( OfxClipInstance::ofxPremultToNatronPremult( outArgs.getStringProperty(kOfxImageEffectPropPreMultiplication) ) );
        defaultPrefs.setIsContinuous(outArgs.getIntProperty(kOfxImageClipPropContinuousSamples) != 0);
        defaultPrefs.setIsFrameVarying(outArgs.getIntProperty(kOfxImageEffectFrameVarying) != 0);
        int formatV[4];
        outArgs.getIntPropertyN(kOfxImageClipPropFormat, formatV, 4);
        RectI format;
        format.x1 = formatV[0];
        format.y1 = formatV[1];
        format.x2 = formatV[2];
        format.y2 = formatV[3];
        defaultPrefs.setOutputFormat(format);

#       ifdef OFX_DEBUG_ACTIONS
        std::cout << outArgs.getDoubleProperty(kOfxImageEffectPropFrameRate) << ","
                  << outArgs.getStringProperty(kOfxImageClipPropFieldOrder) << ","
                  << outArgs.getStringProperty(kOfxImageEffectPropPreMultiplication) << ","
                  << outArgs.getIntProperty(kOfxImageClipPropContinuousSamples) << ","
                  << outArgs.getIntProperty(kOfxImageEffectFrameVarying) << std::endl;
#       endif

        return eStatusOK;
    } //if (st == kOfxStatOK)
} // OfxImageEffectInstance::getClipPreferences_safe

bool
OfxImageEffectInstance::updatePreferences_safe(double frameRate,
                                               const std::string& fielding,
                                               const std::string& premult,
                                               bool continuous,
                                               bool frameVarying)
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
std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>&
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
    for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = _clips.begin(); it != _clips.end(); ++it) {
        if ( it->second && it->second->canTransform() ) {
            ///Output clip should not have the property set.
            assert( !it->second->isOutput() );
            if ( it->second->isOutput() ) {
                return false;
            }


            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            if (clip) {
                inputs->push_back( clip->getInputNb() );
            }
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

OfxImageEffectDescriptor::OfxImageEffectDescriptor(const std::string &bundlePath,
                                                   OFX::Host::Plugin *plug)
    : OFX::Host::ImageEffect::Descriptor(bundlePath, plug)
{
}

OfxImageEffectDescriptor::OfxImageEffectDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                                                   OFX::Host::Plugin *plugin)
    : OFX::Host::ImageEffect::Descriptor(rootContext, plugin)
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

    if (std::strcmp(paramType, kOfxParamTypeDouble2D) == 0) {
        const std::string& type = ret->getDoubleType();
        // only kOfxParamDoubleTypeXYAbsolute and kOfxParamDoubleTypeNormalisedXYAbsolute are be supported
        if (  //type == kOfxParamDoubleTypePlain ||
                                                   ( type == kOfxParamDoubleTypeNormalisedXYAbsolute) ||
                                                   //type == kOfxParamDoubleTypeNormalisedXY ||
                                                   //type == kOfxParamDoubleTypeXY ||
                                                   ( type == kOfxParamDoubleTypeXYAbsolute) ) {
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

bool
OfxImageEffectInstance::ofxCursorToNatronCursor(const std::string &ofxCursor,
                                                CursorEnum* cursor)
{
    bool ret = true;

    if (ofxCursor == kNatronOfxDefaultCursor) {
        *cursor = eCursorDefault;
    } else if (ofxCursor == kNatronOfxBlankCursor) {
        *cursor =  eCursorBlank;
    } else if (ofxCursor == kNatronOfxArrowCursor) {
        *cursor =  eCursorArrow;
    } else if (ofxCursor == kNatronOfxUpArrowCursor) {
        *cursor =  eCursorUpArrow;
    } else if (ofxCursor == kNatronOfxCrossCursor) {
        *cursor =  eCursorCross;
    } else if (ofxCursor == kNatronOfxIBeamCursor) {
        *cursor =  eCursorIBeam;
    } else if (ofxCursor == kNatronOfxWaitCursor) {
        *cursor =  eCursorWait;
    } else if (ofxCursor == kNatronOfxBusyCursor) {
        *cursor =  eCursorBusy;
    } else if (ofxCursor == kNatronOfxForbiddenCursor) {
        *cursor =  eCursorForbidden;
    } else if (ofxCursor == kNatronOfxPointingHandCursor) {
        *cursor =  eCursorPointingHand;
    } else if (ofxCursor == kNatronOfxWhatsThisCursor) {
        *cursor =  eCursorWhatsThis;
    } else if (ofxCursor == kNatronOfxSizeVerCursor) {
        *cursor =  eCursorSizeVer;
    } else if (ofxCursor == kNatronOfxSizeHorCursor) {
        *cursor =  eCursorSizeHor;
    } else if (ofxCursor == kNatronOfxSizeBDiagCursor) {
        *cursor =  eCursorBDiag;
    } else if (ofxCursor == kNatronOfxSizeFDiagCursor) {
        *cursor =  eCursorFDiag;
    } else if (ofxCursor == kNatronOfxSizeAllCursor) {
        *cursor =  eCursorSizeAll;
    } else if (ofxCursor == kNatronOfxSplitVCursor) {
        *cursor =  eCursorSplitV;
    } else if (ofxCursor == kNatronOfxSplitHCursor) {
        *cursor =  eCursorSplitH;
    } else if (ofxCursor == kNatronOfxOpenHandCursor) {
        *cursor =  eCursorOpenHand;
    } else if (ofxCursor == kNatronOfxClosedHandCursor) {
        *cursor =  eCursorClosedHand;
    } else {
        ret = false;
    }

    return ret;
} // OfxImageEffectInstance::ofxCursorToNatronCursor

NATRON_NAMESPACE_EXIT
