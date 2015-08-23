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
#include <vector>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_OFF(unused-parameter)
CLANG_DIAG_OFF(unused-local-typedefs) //-Wunused-local-typedefs
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
GCC_DIAG_ON(unused-parameter)
CLANG_DIAG_ON(unused-local-typedefs) //-Wunused-local-typedefs
#endif

#define kNatronColorPlaneName "Color"
#define kNatronBackwardMotionVectorsPlaneName "Backward"
#define kNatronForwardMotionVectorsPlaneName "Forward"
#define kNatronDisparityLeftPlaneName "DisparityLeft"
#define kNatronDisparityRightPlaneName "DisparityRight"

#define kNatronRGBAComponentsName "RGBA"
#define kNatronRGBComponentsName "RGB"
#define kNatronAlphaComponentsName "Alpha"

#define kNatronDisparityComponentsName "Disparity"
#define kNatronMotionComponentsName "Motion"


namespace Natron {

class ImageComponents
{
public:
    
    ImageComponents();
    
    ImageComponents(const std::string& layerName,
                    const std::string& globalCompName,
                    const std::vector<std::string>& componentsName);
    
    ImageComponents(const std::string& layerName,
                    const std::string& globalCompName,
                    const char** componentsName,
                    int count);
    
    ~ImageComponents();
    
    // Is it Alpha, RGB or RGBA
    bool isColorPlane() const;
    
    // Only color plane (isColorPlane()) can be convertible
    bool isConvertibleTo(const ImageComponents& other) const;
    
    int getNumComponents() const;
    
    ///E.g color
    const std::string& getLayerName() const;
    
    ///E.g ["r","g","b","a"]
    const std::vector<std::string>& getComponentsNames() const;
    
    ///E.g "rgba"
    const std::string& getComponentsGlobalName() const;
    
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
    static const ImageComponents& getXYComponents();
    
    friend class boost::serialization::access;
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/);
    
private:
    
    
    
    std::string _layerName;
    std::vector<std::string> _componentNames;
    std::string _globalComponentsName;
};

}

#endif // IMAGECOMPONENTS_H
