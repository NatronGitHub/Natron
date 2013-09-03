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

#ifndef POWITER_ENGINE_OFXNODE_H_
#define POWITER_ENGINE_OFXNODE_H_

#include <map>
#include <string>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QObject>
//ofx
#include "ofxhImageEffect.h"

#include "Engine/Node.h"

//ours
class Tab_Knob;
class QHBoxLayout;

class QImage;

// FIXME: OFX::Host::ImageEffect::Instance should be a member
class OfxNode : public QObject,public Node,public OFX::Host::ImageEffect::Instance
{
    Q_OBJECT
    
    
    QMutex _lock; // lock used in engine(...) function
    std::map<std::string,OFX::Host::Param::Instance*> _params; // used to re-create parenting between params
    Tab_Knob* _tabKnob; // for nuke tab extension: it creates all Group param as a tab
    QHBoxLayout* _lastKnobLayoutWithNoNewLine; // for nuke layout hint extension
    bool _firstTime; //used in engine(...) to operate once per frame
    bool _isOutput;
    int _currentFrame; // valid only when _isOutput is true
    std::pair<int,int> _frameRange; // valid only when _isOutput is true
    QImage* _preview;
    bool _canHavePreview;
    
public:
    OfxNode(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
            OFX::Host::ImageEffect::Descriptor         &other,
            const std::string  &context);
    
    virtual ~OfxNode(){}
    


    ///////////////////////////
    // Powiter::Node methods //
    ///////////////////////////

    virtual bool isInputNode() OVERRIDE;
    
    virtual bool isOutputNode() OVERRIDE;
    
    /*Returns the clips count minus the output clip*/
    virtual int maximumInputs() OVERRIDE;
    
    virtual int minimumInputs() OVERRIDE;
    
    virtual bool cacheData() OVERRIDE {return false;}
    
    virtual std::string className() OVERRIDE;
    
    virtual std::string description() OVERRIDE {return "";}
    
    virtual std::string setInputLabel (int inputNb) OVERRIDE;
    
    virtual bool isOpenFXNode() const OVERRIDE {return true;}
    
    static ChannelSet ofxComponentsToPowiterChannels(const std::string& comp);
    
    virtual ChannelSet supportedComponents() OVERRIDE;
    
    virtual bool _validate(bool) OVERRIDE;
    
    virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out) OVERRIDE;


    
    //////////////////////////////////////////////
    // OFX::Host::ImageEffect::Instance methods //
    //////////////////////////////////////////////

    /// get default output fielding. This is passed into the clip prefs action
    /// and  might be mapped (if the host allows such a thing)
    virtual const std::string &getDefaultOutputFielding() const OVERRIDE {
        static const std::string v(kOfxImageFieldNone);
        return v;
    }
    
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
    virtual void getProjectSize(double& xSize, double& ySize) const{
        xSize = _info->getDisplayWindow().w();
        ySize = _info->getDisplayWindow().h();
    }
    
    // The offset of the current project in canonical coordinates.
    // The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin
    // of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
    // project offset is the offset to the bottom left hand corner of the letter box. The project
    // offset is in canonical coordinates.
    virtual void getProjectOffset(double& xOffset, double& yOffset) const{
        xOffset = _info->x();
        yOffset = _info->y();
    }
    
    // The extent of the current project in canonical coordinates.
    // The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems
    // for more infomation on the project extent. The extent is in canonical coordinates and only
    // returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD
    // project would have an extent of 768, 576.
    virtual void getProjectExtent(double& xSize, double& ySize) const {
        xSize = _info->w();
        ySize = _info->h();
    }
    
    // The pixel aspect ratio of the current project
    virtual double getProjectPixelAspectRatio() const {
        return _info->getDisplayWindow().pixel_aspect();
    }
    
    // The duration of the effect
    // This contains the duration of the plug-in effect, in frames.
    virtual double getEffectDuration() const {return 1.0;}
    
    // For an instance, this is the frame rate of the project the effect is in.
    virtual double getFrameRate() const {return 25.0;}
    
    /// This is called whenever a param is changed by the plugin so that
    /// the recursive instanceChangedAction will be fed the correct frame
    virtual double getFrameRecursive() const {return 0.0;}
    
    /// This is called whenever a param is changed by the plugin so that
    /// the recursive instanceChangedAction will be fed the correct
    /// renderScale
    virtual void getRenderScaleRecursive(double &x, double &y) const{
        x = y = 1.0;
    }

    virtual OFX::Host::Param::Instance* newParam(const std::string& name, OFX::Host::Param::Descriptor& Descriptor);
    
    
    /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditBegin
    ///
    /// Client host code needs to implement this
    virtual OfxStatus editBegin(const std::string& name);
    
    /// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditEnd
    ///
    /// Client host code needs to implement this
    virtual OfxStatus editEnd();
    
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for Progress::ProgressI
    
    /// Start doing progress.
    virtual void progressStart(const std::string &message);
    
    /// finish yer progress
    virtual void progressEnd();
    
    /// set the progress to some level of completion, returns
    /// false if you should abandon processing, true to continue
    virtual bool progressUpdate(double t);
    
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    // overridden for TimeLine::TimeLineI
    
    /// get the current time on the timeline. This is not necessarily the same
    /// time as being passed to an action (eg render)
    virtual double timeLineGetTime();
    
    /// set the timeline to a specific time
    virtual void timeLineGotoTime(double t);
    
    /// get the first and last times available on the effect's timeline
    virtual void timeLineGetBounds(double &t1, double &t2);

    //
    // END of OFX::Host::ImageEffect::Instance methods
    //



    

    bool isInputOptional(int inpubNb);

    void setAsOutputNode() {_isOutput = true;}

    bool hasPreviewImage() const {return _preview!=NULL;}

    bool canHavePreviewImage() const {return _canHavePreview;}

    void setCanHavePreviewImage() {_canHavePreview = true;}

    QImage* getPreview() const {return _preview;}

    int firstFrame(){return _frameRange.first;}

    int lastFrame(){return _frameRange.second;}

    void incrementCurrentFrame() {++_currentFrame;}

    void setCurrentFrameToStart(){_currentFrame = _frameRange.first;}

    void setCurrentFrame(int f){
        if(f >= _frameRange.first && f <= _frameRange.second)
            _currentFrame = f;
    }

    /*Useful only when _isOutput is true*/
    int currentFrame() const {return _currentFrame;}

    void setTabKnob(Tab_Knob* k){_tabKnob = k;}
    
    Tab_Knob* getTabKnob() const {return _tabKnob;}
    
    void setLastKnobLayoutWithNoNewLine(QHBoxLayout* layout){_lastKnobLayoutWithNoNewLine = layout;}
    
    QHBoxLayout* getLastKnobLayoutWithNoNewLine() const {return _lastKnobLayoutWithNoNewLine;}

    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;

    
    MappedInputV inputClipsCopyWithoutOutput();

    void computePreviewImage();

    /*group is a string as such:
     Toto/Superplugins/blabla
     This functions extracts the all parts of such a grouping, e.g in this case
     it would return [Toto,Superplugins,blabla].*/
    static std::vector<std::string> extractAllPartsOfGrouping(const std::string& group);


public slots:
    void onInstanceChangedAction(const QString&);
    
    
};

#endif // POWITER_ENGINE_OFXNODE_H_
