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


#ifndef STRINGANIMATIONMANAGER_H
#define STRINGANIMATIONMANAGER_H

#include <boost/scoped_ptr.hpp>
#include <string>
#include "Global/GlobalDefines.h"

class KnobI;
struct StringAnimationManagerPrivate;


///not thread-safe
class StringAnimationManager
{
public:
    StringAnimationManager(const KnobI* knob);
    
    ~StringAnimationManager();
    
    ///for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle   inArgsRaw,
                                                           OfxPropertySetHandle   outArgsRaw);
    
    bool hasCustomInterp() const;
    
    void setCustomInterpolation(customParamInterpolationV1Entry_t func,void* ofxParamHandle);
    
    bool customInterpolation(double time,std::string* ret) const;
    
    void insertKeyFrame(int time,const std::string& v,double* index);
    
    void removeKeyFrame(int time);
    
    void clearKeyFrames();
    
    void stringFromInterpolatedIndex(double interpolated,std::string* returnValue) const;
    
    void clone(const StringAnimationManager& other);
    
    void load(const std::map<int,std::string>& keyframes);
    
    void save(std::map<int,std::string>* keyframes) const;

    
private:
    
    boost::scoped_ptr<StringAnimationManagerPrivate> _imp;
};

#endif // STRINGANIMATIONMANAGER_H
