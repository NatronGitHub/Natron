//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 03/09/13.
//
//

#ifndef POWITER_ENGINE_OFXIMAGEEFFECTINSTANCE_H_
#define POWITER_ENGINE_OFXIMAGEEFFECTINSTANCE_H_

#include <string>
#include <cstdarg>
#include <ofxhImageEffect.h>

#include "Global/Macros.h"

class OfxNode;

namespace Powiter {

class OfxImageEffectInstance : public OFX::Host::ImageEffect::Instance
{
public:
    OfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                           OFX::Host::ImageEffect::Descriptor& desc,
                           const std::string& context,
                           bool interactive);

    virtual ~OfxImageEffectInstance();

    void setOfxNodePointer(OfxNode* node){_node = node;}

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for ImageEffect::Instance

    /// get default output fielding. This is passed into the clip prefs action
    /// and  might be mapped (if the host allows such a thing)
    virtual const std::string &getDefaultOutputFielding() const OVERRIDE;
    /// make a clip
    virtual OFX::Host::ImageEffect::ClipInstance* newClipInstance(OFX::Host::ImageEffect::Instance* plugin,
                                                                  OFX::Host::ImageEffect::ClipDescriptor* descriptor,
                                                                  int index) OVERRIDE;

    virtual OfxStatus vmessage(const char* type,
                               const char* id,
                               const char* format,
                               va_list args) OVERRIDE;

    //
    // live parameters
    //

    // The size of the current project in canonical coordinates.
    // The size of a project is a sub set of the kOfxImageEffectPropProjectExtent. For example a
    // project may be a PAL SD project, but only be a letter-box within that. The project size is
    // the size of this sub window.
    virtual void getProjectSize(double& xSize, double& ySize) const OVERRIDE;

    // The offset of the current project in canonical coordinates.
    // The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin
    // of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
    // project offset is the offset to the bottom left hand corner of the letter box. The project
    // offset is in canonical coordinates.
    virtual void getProjectOffset(double& xOffset, double& yOffset) const OVERRIDE;
    
    // The extent of the current project in canonical coordinates.
    // The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems
    // for more infomation on the project extent. The extent is in canonical coordinates and only
    // returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD
    // project would have an extent of 768, 576.
    virtual void getProjectExtent(double& xSize, double& ySize) const OVERRIDE;
    
    // The pixel aspect ratio of the current project
    virtual double getProjectPixelAspectRatio() const OVERRIDE;

    // The duration of the effect
    // This contains the duration of the plug-in effect, in frames.
    virtual double getEffectDuration() const OVERRIDE;

    // For an instance, this is the frame rate of the project the effect is in.
    virtual double getFrameRate() const OVERRIDE;

    /// This is called whenever a param is changed by the plugin so that
    /// the recursive instanceChangedAction will be fed the correct frame
    virtual double getFrameRecursive() const OVERRIDE;

    /// This is called whenever a param is changed by the plugin so that
    /// the recursive instanceChangedAction will be fed the correct
    /// renderScale
    virtual void getRenderScaleRecursive(double &x, double &y) const OVERRIDE;

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for Param::SetInstance

    /// make a parameter instance
    ///
    /// Client host code needs to implement this
    virtual OFX::Host::Param::Instance* newParam(const std::string& name, OFX::Host::Param::Descriptor& Descriptor) OVERRIDE;


    /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditBegin
    ///
    /// Client host code needs to implement this
    virtual OfxStatus editBegin(const std::string& name) OVERRIDE;

    /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditEnd
    ///
    /// Client host code needs to implement this
    virtual OfxStatus editEnd() OVERRIDE;

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for Progress::ProgressI

    /// Start doing progress.
    virtual void progressStart(const std::string &message) OVERRIDE;

    /// finish yer progress
    virtual void progressEnd() OVERRIDE;

    /// set the progress to some level of completion, returns
    /// false if you should abandon processing, true to continue
    virtual bool progressUpdate(double t) OVERRIDE;

    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for TimeLine::TimeLineI

    /// get the current time on the timeline. This is not necessarily the same
    /// time as being passed to an action (eg render)
    virtual double timeLineGetTime() OVERRIDE;

    /// set the timeline to a specific time
    virtual void timeLineGotoTime(double t) OVERRIDE;

    /// get the first and last times available on the effect's timeline
    virtual void timeLineGetBounds(double &t1, double &t2) OVERRIDE;

    //
    // END of OFX::Host::ImageEffect::Instance methods
    //

    
    OfxNode* node() { return _node; }
    
    const std::map<std::string,OFX::Host::Param::Instance*>& getParams() const {return _parentingMap;}
    
private:
    const OfxNode* node() const { return _node; }

private:
    OfxNode* _node; /* FIXME: OfxImageEffectInstance should be able to work without the node_ //
                     Not easy since every Knob need a valid pointer to a node when 
                     KnobFactory::createKnob() is called. That's why we need to pass a pointer
                     to an OfxParamInstance. Without this pointer we would be unable
                     to track the knobs that have been created for 1 Node since OfxParamInstance
                     is totally dissociated from Node.*/
    
    /*Use this to re-create parenting between effect's params.
     The key is the name of a param and the Instance a pointer to the associated effect.
     This has nothing to do with the base class _params member! */
    std::map<std::string,OFX::Host::Param::Instance*> _parentingMap; 
};

} // namespace Powiter

#endif
