//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef IMAGECOMPONENTS_H
#define IMAGECOMPONENTS_H

#include <string>


#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
#endif

#define kNatronColorPlaneName "rgba"
#define kNatronBackwardMotionVectorsPlaneName "backward"
#define kNatronForwardMotionVectorsPlaneName "forward"
#define kNatronDisparityLeftPlaneName "disparityL"
#define kNatronDisparityRightPlaneName "disparityR"

#define kNatronRGBAComponentsName "rgba"
#define kNatronRGBComponentsName "rgb"
#define kNatronAlphaComponentsName "alpha"

#define kNatronDisparityComponentsName "disparity"
#define kNatronMotionComponentsName "motion"


namespace Natron {

class ImageComponents
{
public:
    
    ImageComponents();
    
    ImageComponents(const std::string& layerName,const std::string& componentsName,int count);
    
    ~ImageComponents();
    
    bool isColorPlane() const;
    
    bool isConvertibleTo(const ImageComponents& other) const;
    
    int getNumComponents() const;
    
    const std::string& getLayerName() const;
    
    const std::string& getComponentsName() const;
    
    bool operator==(const ImageComponents& other) const;
    
    bool operator!=(const ImageComponents& other) const {
        return !(*this == other);
    }
    
    //For std::map
    bool operator<(const ImageComponents& other) const;
    
    /*
     * These are default presets image components
     */
    static const ImageComponents& getNoneComponents();
    static const ImageComponents& getRGBAComponents();
    static const ImageComponents& getRGBComponents();
    static const ImageComponents& getAlphaComponents();
    static const ImageComponents& getBackwardMotionComponents();
    static const ImageComponents& getForwardMotionComponents();
    static const ImageComponents& getDisparityLeftComponents();
    static const ImageComponents& getDisparityRightComponents();
    
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/);
    
private:
    
    
    
    std::string _layerName,_componentsName;
    int _count;
};

}

#endif // IMAGECOMPONENTS_H
