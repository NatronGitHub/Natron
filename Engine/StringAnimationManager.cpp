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


#include "StringAnimationManager.h"

#include <set>
#include <cmath>

#include "Engine/Knob.h"

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


struct StringAnimationManagerPrivate {
    StringAnimationManager::customParamInterpolationV1Entry_t customInterpolation;
    void* ofxParamHandle;
    Keyframes keyframes;
    const Knob* knob;
    
    StringAnimationManagerPrivate(const Knob* knob)
    : customInterpolation(NULL)
    , ofxParamHandle(NULL)
    , keyframes()
    , knob(knob)
    {
        
    }
};

StringAnimationManager::StringAnimationManager(const Knob* knob)
: _imp(new StringAnimationManagerPrivate(knob))
{
}

StringAnimationManager::~StringAnimationManager() {}


bool StringAnimationManager::hasCustomInterp() const {
    return _imp->customInterpolation != NULL;
}

void StringAnimationManager::setCustomInterpolation(customParamInterpolationV1Entry_t func,void* ofxParamHandle) {
    _imp->customInterpolation = func;
    _imp->ofxParamHandle = ofxParamHandle;
}

bool StringAnimationManager::customInterpolation(double time,QString* ret) const {
    assert(_imp->customInterpolation);
    ///if there's a single keyframe, return it
    
    if (_imp->keyframes.empty()) {
        return false;
    }
    
    if (_imp->keyframes.size() == 1) {
        *ret = _imp->keyframes.begin()->value;
        return true;
    }
    
    /// get the keyframes surrounding the time
    Keyframes::const_iterator upper = _imp->keyframes.end();
    Keyframes::const_iterator lower = _imp->keyframes.end();
    for (Keyframes::const_iterator it = _imp->keyframes.begin(); it!=_imp->keyframes.end(); ++it) {
        if (it->time > time) {
            upper = it;
            break;
        } else if(it->time == time) {
            ///if there's a keyframe exactly at this time, return its value
            *ret = it->value;
            return true;
        }
    }
    
    if (upper == _imp->keyframes.end()) {
        ///if the time is greater than the time of all keyframes return the last
        
        --upper;
        *ret = upper->value;
        return true;
    } else if(upper == _imp->keyframes.begin()) {
        ///if the time is lesser than the time of all keyframes, return the first
        * ret = upper->value;
    } else {
        ///general case, we're in-between 2 keyframes
        lower = upper;
        --lower;
    }
    
    OFX::Host::Property::PropSpec inArgsSpec[] = {
        { kOfxPropName,    OFX::Host::Property::eString, 1, true, "" },
        { kOfxPropTime,    OFX::Host::Property::eDouble, 1, true, "" },
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 2, true, ""},
        { kOfxParamPropInterpolationTime,    OFX::Host::Property::eDouble, 2, true, "" },
        { kOfxParamPropInterpolationAmount,    OFX::Host::Property::eDouble, 1, true, "" },
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set inArgs(inArgsSpec);
    inArgs.setStringProperty(kOfxPropName, _imp->knob->getName());
    inArgs.setDoubleProperty(kOfxPropTime, time);
    
    inArgs.setStringProperty(kOfxParamPropCustomValue, lower->value.toStdString(),0);
    inArgs.setStringProperty(kOfxParamPropCustomValue, upper->value.toStdString(),1);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, lower->time,0);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, upper->time,1);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationAmount, (time - lower->time) / (double)(upper->time - lower->time));
    
    
    
    OFX::Host::Property::PropSpec outArgsSpec[] = {
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 1, false, ""},
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set outArgs(outArgsSpec);
    
    _imp->customInterpolation(_imp->ofxParamHandle,inArgs.getHandle(),outArgs.getHandle());
    
    *ret = outArgs.getStringProperty(kOfxParamPropCustomValue,0).c_str();
    return true;
}

void StringAnimationManager::insertKeyFrame(int time,const QString& v,double* index) {
    StringKeyFrame k;
    k.time = time;
    k.value = v;
    std::pair<Keyframes::iterator,bool> ret = _imp->keyframes.insert(k);
    if(!ret.second){
        _imp->keyframes.erase(ret.first);
        ret = _imp->keyframes.insert(k);
        assert(ret.second);
    }
    *index = std::distance(_imp->keyframes.begin(), ret.first);

}

void StringAnimationManager::removeKeyFrame(int time) {
    for (Keyframes::iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (it->time == time) {
            _imp->keyframes.erase(it);
            return;
        }
    }

}

void StringAnimationManager::clearKeyFrames() {
    _imp->keyframes.clear();
}

void StringAnimationManager::stringFromInterpolatedIndex(double interpolated,QString* returnValue) const {
    int index = std::floor(interpolated + 0.5);
    int i = 0;
    for (Keyframes::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (i == index) {
            *returnValue = it->value;
            return;
        }
        ++i;
    }
    ///interpolated is wrong
    assert(false);

}

void StringAnimationManager::clone(const StringAnimationManager& other) {
    _imp->keyframes = other._imp->keyframes;
}

static const QString stringSeparatorTag = QString("__SEP__");
static const QString keyframeSepTag = QString("__,__");

void StringAnimationManager::load(const QString& str) {
    if (str.isEmpty()) {
        return;
    }
    int sepIndex = str.indexOf(stringSeparatorTag);
    
    int i = 0;
    while (sepIndex != -1) {
        
        int keyFrameSepIndex = str.indexOf(keyframeSepTag,i);
        assert(keyFrameSepIndex != -1);
        
        QString keyframeTime;
        while (i < keyFrameSepIndex) {
            keyframeTime.push_back(str.at(i));
            ++i;
        }
        
        i+= keyframeSepTag.size();
        
        QString keyframevalue;
        while (i < sepIndex) {
            keyframevalue.push_back(str.at(i));
            ++i;
        }
        
        StringKeyFrame k;
        k.time = keyframeTime.toInt();
        k.value = keyframevalue;
        _imp->keyframes.insert(k);
        
        i+= stringSeparatorTag.size();
        sepIndex = str.indexOf(stringSeparatorTag,sepIndex + 1);
    }

}

QString StringAnimationManager::save() const {
    if (_imp->keyframes.empty()) {
        return "";
    }
    QString ret;
    
    for (Keyframes::const_iterator it = _imp->keyframes.begin();it!=_imp->keyframes.end();++it) {
        ret.push_back(QString::number(it->time));
        ret.push_back(keyframeSepTag);
        ret.push_back(it->value);
        ret.push_back(stringSeparatorTag);
    }
    return ret;

}