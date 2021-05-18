/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_OFXIMAGEEFFECTINSTANCE_H
#define NATRON_ENGINE_OFXIMAGEEFFECTINSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <cstdarg>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare)
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

class OfxImageEffectInstance
    : public OFX::Host::ImageEffect::Instance
{
    Q_DECLARE_TR_FUNCTIONS(OfxImageEffectInstance)

public:
    OfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                           OFX::Host::ImageEffect::Descriptor & desc,
                           const std::string & context,
                           bool interactive);

    OfxImageEffectInstance(const OfxImageEffectInstance& other);

    virtual ~OfxImageEffectInstance();

    void setOfxEffectInstance(const OfxEffectInstancePtr& node)
    {
        _ofxEffectInstance = node;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for ImageEffect::Instance

    /// call the effect entry point
    virtual OfxStatus mainEntry(const char *action,
                                const void *handle,
                                OFX::Host::Property::Set *inArgs,
                                OFX::Host::Property::Set *outArgs) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual OFX::Host::Memory::Instance* newMemoryInstance(size_t nBytes) OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// get default output fielding. This is passed into the clip prefs action
    /// and  might be mapped (if the host allows such a thing)
    virtual const std::string &getDefaultOutputFielding() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    /// make a clip
    virtual OFX::Host::ImageEffect::ClipInstance* newClipInstance(OFX::Host::ImageEffect::Instance* plugin,
                                                                  OFX::Host::ImageEffect::ClipDescriptor* descriptor,
                                                                  int index) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual OfxStatus vmessage(const char* type,
                               const char* id,
                               const char* format,
                               va_list args) OVERRIDE FINAL;
    virtual OfxStatus setPersistentMessage(const char* type,
                                           const char* id,
                                           const char* format,
                                           va_list args) OVERRIDE FINAL;
    virtual OfxStatus clearPersistentMessage() OVERRIDE FINAL;

    virtual int getDimension(const std::string &name) const OFX_EXCEPTION_SPEC OVERRIDE FINAL;

    //
    // live parameters
    //
    virtual const std::vector<std::string>& getUserCreatedPlanes() const OVERRIDE FINAL;

    // The size of the current project in canonical coordinates.
    // The size of a project is a sub set of the kOfxImageEffectPropProjectExtent. For example a
    // project may be a PAL SD project, but only be a letter-box within that. The project size is
    // the size of this sub window.
    virtual void getProjectSize(double & xSize, double & ySize) const OVERRIDE FINAL;

    // The offset of the current project in canonical coordinates.
    // The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin
    // of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
    // project offset is the offset to the bottom left hand corner of the letter box. The project
    // offset is in canonical coordinates.
    virtual void getProjectOffset(double & xOffset, double & yOffset) const OVERRIDE FINAL;

    // The extent of the current project in canonical coordinates.
    // The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems
    // for more information on the project extent. The extent is in canonical coordinates and only
    // returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD
    // project would have an extent of 768, 576.
    virtual void getProjectExtent(double & xSize, double & ySize) const OVERRIDE FINAL;

    // The pixel aspect ratio of the current project
    virtual double getProjectPixelAspectRatio() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    // The duration of the effect
    // This contains the duration of the plug-in effect, in frames.
    virtual double getEffectDuration() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    // For an instance, this is the frame rate of the project the effect is in.
    virtual double getFrameRate() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// This is called whenever a param is changed by the plugin so that
    /// the recursive instanceChangedAction will be fed the correct frame
    virtual double getFrameRecursive() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void paramChangedByPlugin(OFX::Host::Param::Instance *param) OVERRIDE FINAL;

    /// This is called whenever a param is changed by the plugin so that
    /// the recursive instanceChangedAction will be fed the correct
    /// renderScale
    virtual void getRenderScaleRecursive(double &x, double &y) const OVERRIDE FINAL;
    virtual OfxStatus getViewCount(int *nViews) const OVERRIDE FINAL;
    virtual OfxStatus getViewName(int viewIndex, const char** name) const OVERRIDE FINAL;

    void setupClipPreferencesArgsFromMetadata(NodeMetadata& metadata,
                                              OFX::Host::Property::Set &outArgs,
                                              std::map<OfxClipInstance*, int>& clipsMapping);

    /**
     * We add some output parameters to the function so that we can delay the actual setting of the clip preferences
     **/
    StatusEnum getClipPreferences_safe(NodeMetadata& defaultPrefs);

    virtual OfxStatus createInstanceAction() OVERRIDE FINAL;

    /**
     * @brief To be called once no action is currently being run after getClipPreferences_safe was called.
     * Caller maintains a lock around this call to prevent race conditions.
     **/
    bool updatePreferences_safe(double frameRate, const std::string& fielding, const std::string& premult,
                                bool continuous, bool frameVarying);
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for Param::SetInstance

    /// make a parameter instance
    ///
    /// Client host code needs to implement this
    virtual OFX::Host::Param::Instance* newParam(const std::string & name, OFX::Host::Param::Descriptor & Descriptor) OVERRIDE FINAL WARN_UNUSED_RETURN;


    /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditBegin
    ///
    /// Client host code needs to implement this
    virtual OfxStatus editBegin(const std::string & name) OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditEnd
    ///
    /// Client host code needs to implement this
    virtual OfxStatus editEnd() OVERRIDE FINAL;

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for Progress::ProgressI

    /// Start doing progress.
    virtual void progressStart(const std::string &message, const std::string &messageid) OVERRIDE FINAL;

    /// finish yer progress
    virtual void progressEnd() OVERRIDE FINAL;

    /// set the progress to some level of completion, returns
    /// false if you should abandon processing, true to continue
    virtual bool progressUpdate(double t) OVERRIDE FINAL WARN_UNUSED_RETURN;

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for TimeLine::TimeLineI

    /// get the current time on the timeline. This is not necessarily the same
    /// time as being passed to an action (eg render)
    virtual double timeLineGetTime() OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// set the timeline to a specific time
    virtual void timeLineGotoTime(double t) OVERRIDE FINAL;

    /// get the first and last times available on the effect's timeline
    virtual void timeLineGetBounds(double &t1, double &t2) OVERRIDE FINAL;
    virtual int abort() OVERRIDE FINAL;

    //
    // END of OFX::Host::ImageEffect::Instance methods
    //
    OfxEffectInstancePtr getOfxEffectInstance() const WARN_UNUSED_RETURN
    {
        return _ofxEffectInstance.lock();
    }

    /// to be called right away after populate() is called. It adds to their group all the params.
    /// This is done in a deferred manner as some params can sometimes not be defined in a good order.
    void addParamsToTheirParents();

    bool areAllNonOptionalClipsConnected() const;

#ifdef kOfxImageEffectPropInAnalysis // removed in OFX 1.4
    ///True if the effect is currently performing an analysis
    bool isInAnalysis() const;
#endif


    bool getInputsHoldingTransform(std::list<int>* inputs) const;

    const std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>& getClips() const;
    static bool ofxCursorToNatronCursor(const std::string& ofxCursor, CursorEnum* cursor);
    static const OFX::Host::Property::PropSpec* getOfxParamOverlayInteractDescProps();

private:
    OfxEffectInstanceWPtr _ofxEffectInstance; /* FIXME: OfxImageEffectInstance should be able to work without the node_ //
                                                              Not easy since every Knob need a valid pointer to a node when
                                                              AppManager::createKnob() is called. That's why we need to pass a pointer
                                                              to an OfxParamInstance. Without this pointer we would be unable
                                                              to track the knobs that have been created for 1 Node since OfxParamInstance
                                                              is totally dissociated from Node.*/
};

class OfxImageEffectDescriptor
    : public OFX::Host::ImageEffect::Descriptor
{
public:

    OfxImageEffectDescriptor(OFX::Host::Plugin *plug);

    OfxImageEffectDescriptor(const std::string &bundlePath,
                             OFX::Host::Plugin *plug);

    OfxImageEffectDescriptor(const OFX::Host::ImageEffect::Descriptor &rootContext,
                             OFX::Host::Plugin *plugin);

    virtual OFX::Host::Param::Descriptor * paramDefine(const char *paramType,
                                                       const char *name) OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_ENGINE_OFXIMAGEEFFECTINSTANCE_H
