/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef IMAGECOMPONENTS_H
#define IMAGECOMPONENTS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include <vector>

#include "Global/Macros.h"
#include "Engine/EngineFwd.h"

#define kNatronColorPlaneName "Color"
#define kNatronBackwardMotionVectorsPlaneName "Backward.Motion"
#define kNatronForwardMotionVectorsPlaneName "Forward.Motion"
#define kNatronDisparityLeftPlaneName "DisparityLeft.Disparity"
#define kNatronDisparityRightPlaneName "DisparityRight.Disparity"

#define kNatronRGBAComponentsName "RGBA"
#define kNatronRGBComponentsName "RGB"
#define kNatronAlphaComponentsName "Alpha"

#define kNatronDisparityComponentsName "XY"
#define kNatronMotionComponentsName "UV"

#define kNatronAlphaPlaneUserName kNatronColorPlaneName "." kNatronAlphaComponentsName
#define kNatronRGBAPlaneUserName kNatronColorPlaneName "." kNatronRGBAComponentsName
#define kNatronRGBPlaneUserName kNatronColorPlaneName "." kNatronRGBComponentsName
#define kNatronDisparityLeftPlaneUserName kNatronDisparityLeftPlaneName
#define kNatronDisparityRightPlaneUserName kNatronDisparityRightPlaneName
#define kNatronBackwardMotionVectorsPlaneUserName kNatronBackwardMotionVectorsPlaneName
#define kNatronForwardMotionVectorsPlaneUserName kNatronForwardMotionVectorsPlaneName

NATRON_NAMESPACE_ENTER;

class ImageComponents
{
public:
    
    /**
     * @brief The default components registered in Natron, you can iterate until {0,0}
     **/
    static const char* defaultComponents[][2];
    
    static std::string mapUserFriendlyPlaneNameToNatronInternalPlaneName(const std::string& userfriendlyPlaneName);
    static std::string mapNatronInternalPlaneNameToUserFriendlyPlaneName(const std::string& planeName);
    
    static const ImageComponents& getDefaultComponent(const std::string& planeName);
    
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
    
    static bool isColorPlane(const std::string& layerName);
    
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
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    
private:
    
    
    
    std::string _layerName;
    std::vector<std::string> _componentNames;
    std::string _globalComponentsName;
};

NATRON_NAMESPACE_EXIT;

#endif // IMAGECOMPONENTS_H
