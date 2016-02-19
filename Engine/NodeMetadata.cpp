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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeMetadata.h"

NATRON_NAMESPACE_ENTER;

NodeMetadata::NodeMetadata()
: outputPremult(eImagePremultiplicationPremultiplied)
, outputFielding(eImageFieldingOrderNone)
, frameRate(24.)
, outputData()
, inputsData()
, canRenderAtNonframes(true)
, isFrameVarying(false)
{
    
}

NodeMetadata::~NodeMetadata()
{
    
}

bool
NodeMetadata::operator==(const NodeMetadata& other) const
{
    if (outputPremult != other.outputPremult) {
        return false;
    }
    if (outputFielding != other.outputFielding) {
        return false;
    }
    if (frameRate != other.frameRate) {
        return false;
    }
    if (canRenderAtNonframes != other.canRenderAtNonframes) {
        return false;
    }
    if (isFrameVarying != other.isFrameVarying) {
        return false;
    }
    if (outputData.bitdepth != other.outputData.bitdepth) {
        return false;
    }
    if (outputData.pixelAspectRatio != other.outputData.pixelAspectRatio) {
        return false;
    }
    if (outputData.components.size() != other.outputData.components.size()) {
        return false;
    }
    std::list<ImageComponents>::const_iterator ito = other.outputData.components.begin();
    for (std::list<ImageComponents>::const_iterator it = outputData.components.begin(); it!=outputData.components.end(); ++it,++ito) {
        if (*it != *ito) {
            return false;
        }
    }
    if (inputsData.size() != other.inputsData.size()) {
        return false;
    }
    for (std::size_t i = 0; i < inputsData.size(); ++i) {
        if (inputsData[i].pixelAspectRatio != other.inputsData[i].pixelAspectRatio) {
            return false;
        }
        if (inputsData[i].bitdepth != other.inputsData[i].bitdepth) {
            return false;
        }
        std::list<ImageComponents>::const_iterator ito = other.inputsData[i].components.begin();
        for (std::list<ImageComponents>::const_iterator it = inputsData[i].components.begin(); it!=inputsData[i].components.end(); ++it,++ito) {
            if (*it != *ito) {
                return false;
            }
        }
    }
    return true;
}

NATRON_NAMESPACE_EXIT;