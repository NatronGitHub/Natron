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

#include <QString>

#include "Global/GlobalDefines.h"

class Knob;
struct StringAnimationManagerPrivate;


///not thread-safe
class StringAnimationManager
{
public:
    StringAnimationManager(const Knob* knob);
    
    ~StringAnimationManager();
    
    ///for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle   inArgsRaw,
                                                           OfxPropertySetHandle   outArgsRaw);
    
    bool hasCustomInterp() const;
    
    void setCustomInterpolation(customParamInterpolationV1Entry_t func,void* ofxParamHandle);
    
    bool customInterpolation(double time,QString* ret) const;
    
    void insertKeyFrame(int time,const QString& v,double* index);
    
    void removeKeyFrame(int time);
    
    void clearKeyFrames();
    
    void stringFromInterpolatedIndex(double interpolated,QString* returnValue) const;
    
    void clone(const StringAnimationManager& other);
    
    void load(const QString& str);
    
    QString save() const;

    
private:
    
    boost::scoped_ptr<StringAnimationManagerPrivate> _imp;
};

#endif // STRINGANIMATIONMANAGER_H
