//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */


#include "ofxnode.h"
#include "Core/ofxparaminstance.h"

#include "Core/ofxclipinstance.h"


OfxNode::OfxNode(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                 OFX::Host::ImageEffect::Descriptor         &other,
                 const std::string  &context):
Node(),
OFX::Host::ImageEffect::Instance(plugin,other,context,false)
{
}

OFX::Host::ImageEffect::ClipInstance* OfxNode::newClipInstance(OFX::Host::ImageEffect::Instance* plugin,
                                                      OFX::Host::ImageEffect::ClipDescriptor* descriptor,
                                                      int index){
    return new OfxClipInstance(this,descriptor);
}


// make a parameter instance
OFX::Host::Param::Instance* OfxNode::newParam(const std::string& name, OFX::Host::Param::Descriptor& descriptor)
{
    if(descriptor.getType()==kOfxParamTypeInteger)
        return new OfxIntegerInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeDouble)
        return new OfxDoubleInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeBoolean)
        return new OfxBooleanInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeChoice)
        return new OfxChoiceInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeRGBA)
        return new OfxRGBAInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeRGB)
        return new OfxRGBInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeDouble2D)
        return new OfxDouble2DInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeInteger2D)
        return new OfxInteger2DInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypePushButton)
        return new OfxPushButtonInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeGroup)
        return new OFX::Host::Param::GroupInstance(descriptor,this);
    else if(descriptor.getType()==kOfxParamTypePage)
        return new OFX::Host::Param::PageInstance(descriptor,this);
    else
        return 0;
}


OfxStatus OfxNode::editBegin(const std::string& name)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus OfxNode::editEnd(){
    return kOfxStatErrMissingHostFeature;
}

/// Start doing progress.
void  OfxNode::progressStart(const std::string &message)
{
}

/// finish yer progress
void  OfxNode::progressEnd()
{
}

/// set the progress to some level of completion, returns
/// false if you should abandon processing, true to continue
bool  OfxNode::progressUpdate(double t)
{
    return true;
}


/// get the current time on the timeline. This is not necessarily the same
/// time as being passed to an action (eg render)
double  OfxNode::timeLineGetTime()
{
    return 0;
}

/// set the timeline to a specific time
void  OfxNode::timeLineGotoTime(double t)
{
}

/// get the first and last times available on the effect's timeline
void  OfxNode::timeLineGetBounds(double &t1, double &t2)
{
    t1 = 0;
    t2 = 25;
}
