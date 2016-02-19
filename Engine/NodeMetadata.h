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

#ifndef Engine_NodeMetaData_h
#define Engine_NodeMetaData_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <list>

#include "Global/GlobalDefines.h"
#include "Engine/ImageComponents.h"


NATRON_NAMESPACE_ENTER;

/**
 * @brief These meta-data are represents what's flowing through a node. They change
 * everytimes refreshMetaDatas() is called.
 * The bitdepth and inputs components must have been validated against the plug-ins supported components/bitdepth
 *
 * The members are public because this class is only used in the implementation of getPreferredMetaDatas()
 **/
class NodeMetadata
{
    
public:
    
    struct PerInputData
    {
        //Either 1 or 2 components in the case of MotionVectors/Disparity
        double pixelAspectRatio;
        std::list<ImageComponents> components;
        ImageBitDepthEnum bitdepth;
        
        PerInputData()
        : pixelAspectRatio(1.)
        , components()
        , bitdepth(eImageBitDepthFloat)
        {
            
        }
    };
    
    //The premult in output of the node
    ImagePremultiplicationEnum outputPremult;
    
    //The image fielding in output
    ImageFieldingOrderEnum outputFielding;
        
    //The fps in output
    double frameRate;
    
    
    PerInputData outputData;
    
    //For each input specific datas
    std::vector<PerInputData> inputsData;
    
    //True if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
    //False if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
    bool canRenderAtNonframes;

    //True if the effect changes throughout the time
    bool isFrameVarying;
    
    NodeMetadata();
    
    ~NodeMetadata();
    
    bool operator==(const NodeMetadata& other) const;
    bool operator!=(const NodeMetadata& other) const
    {
        return !(*this == other);
    }
};

NATRON_NAMESPACE_EXIT

#endif // Engine_NodeMetaData_h
