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

#include <cassert>
#include <vector>

#include "NodeMetadata.h"

NATRON_NAMESPACE_ENTER;

struct PerInputData
{
    //Either 1 or 2 components in the case of MotionVectors/Disparity
    double pixelAspectRatio;
    ImageComponents components;
    ImageBitDepthEnum bitdepth;
    
    PerInputData()
    : pixelAspectRatio(1.)
    , components()
    , bitdepth(eImageBitDepthFloat)
    {
        
    }
};

struct NodeMetadataPrivate
{
    
    
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
    
    
    NodeMetadataPrivate()
    : outputPremult(eImagePremultiplicationPremultiplied)
    , outputFielding(eImageFieldingOrderNone)
    , frameRate(24.)
    , outputData()
    , inputsData()
    , canRenderAtNonframes(true)
    , isFrameVarying(false)
    {
        
    }
    
    NodeMetadataPrivate(const NodeMetadataPrivate& other)
    {
        *this = other;
    }
    
    void operator=(const NodeMetadataPrivate& other)
    {
        outputPremult = other.outputPremult;
        outputFielding = other.outputFielding;
        frameRate = other.frameRate;
        outputData = other.outputData;
        inputsData = other.inputsData;
        canRenderAtNonframes = other.canRenderAtNonframes;
        isFrameVarying = other.isFrameVarying;
    }
};

NodeMetadata::NodeMetadata()
: _imp(new NodeMetadataPrivate())
{
    
}

NodeMetadata::NodeMetadata(const NodeMetadata& other)
: _imp(new NodeMetadataPrivate(*other._imp))
{
    
}

NodeMetadata::~NodeMetadata()
{
    
}

void
NodeMetadata::clearAndResize(int inputCount)
{
    _imp->inputsData.resize(inputCount);
}

void
NodeMetadata::operator=(const NodeMetadata& other)
{
    *_imp = *other._imp;
}

bool
NodeMetadata::operator==(const NodeMetadata& other) const
{
    if (_imp->outputPremult != other._imp->outputPremult) {
        return false;
    }
    if (_imp->outputFielding != other._imp->outputFielding) {
        return false;
    }
    if (_imp->frameRate != other._imp->frameRate) {
        return false;
    }
    if (_imp->canRenderAtNonframes != other._imp->canRenderAtNonframes) {
        return false;
    }
    if (_imp->isFrameVarying != other._imp->isFrameVarying) {
        return false;
    }
    if (_imp->outputData.bitdepth != other._imp->outputData.bitdepth) {
        return false;
    }
    if (_imp->outputData.pixelAspectRatio != other._imp->outputData.pixelAspectRatio) {
        return false;
    }
    if (_imp->outputData.components != other._imp->outputData.components) {
        return false;
    }
    if (_imp->inputsData.size() != other._imp->inputsData.size()) {
        return false;
    }
    for (std::size_t i = 0; i < _imp->inputsData.size(); ++i) {
        if (_imp->inputsData[i].pixelAspectRatio != other._imp->inputsData[i].pixelAspectRatio) {
            return false;
        }
        if (_imp->inputsData[i].bitdepth != other._imp->inputsData[i].bitdepth) {
            return false;
        }
        if (_imp->inputsData[i].components != other._imp->inputsData[i].components) {
            return false;
        }
    }
    return true;
}


void
NodeMetadata::setOutputPremult(ImagePremultiplicationEnum premult)
{
    _imp->outputPremult = premult;
}

ImagePremultiplicationEnum
NodeMetadata::getOutputPremult() const
{
    return _imp->outputPremult;
}

void
NodeMetadata::setOutputFrameRate(double fps)
{
    _imp->frameRate = fps;
}

double
NodeMetadata::getOutputFrameRate() const
{
    return _imp->frameRate;
}

void
NodeMetadata::setOutputFielding(ImageFieldingOrderEnum fielding)
{
    _imp->outputFielding = fielding;
}

ImageFieldingOrderEnum
NodeMetadata::getOutputFielding() const
{
    return _imp->outputFielding;
}

void
NodeMetadata::setIsContinuous(bool continuous)
{
    _imp->canRenderAtNonframes = continuous;
}

bool
NodeMetadata::getIsContinuous() const
{
    return _imp->canRenderAtNonframes;
}

void
NodeMetadata::setIsFrameVarying(bool varying)
{
    _imp->isFrameVarying = varying;
}

bool
NodeMetadata::getIsFrameVarying() const
{
    return _imp->isFrameVarying;
}

void
NodeMetadata::setPixelAspectRatio(int inputNb, double par)
{
    if (inputNb == -1) {
        _imp->outputData.pixelAspectRatio = par;
    } else {
        assert(inputNb < (int)_imp->inputsData.size());
        _imp->inputsData[inputNb].pixelAspectRatio = par;
    }
}

double
NodeMetadata::getPixelAspectRatio(int inputNb) const
{
    if (inputNb == -1) {
        return _imp->outputData.pixelAspectRatio;
    } else {
        assert(inputNb < (int)_imp->inputsData.size());
        return _imp->inputsData[inputNb].pixelAspectRatio;
    }
}

void
NodeMetadata::setBitDepth(int inputNb, ImageBitDepthEnum depth)
{
    if (inputNb == -1) {
        _imp->outputData.bitdepth = depth;
    } else {
        assert(inputNb < (int)_imp->inputsData.size());
        _imp->inputsData[inputNb].bitdepth = depth;
    }
}

ImageBitDepthEnum
NodeMetadata::getBitDepth(int inputNb) const
{
    if (inputNb == -1) {
        return _imp->outputData.bitdepth;
    } else {
        assert(inputNb < (int)_imp->inputsData.size());
        return _imp->inputsData[inputNb].bitdepth;
    }
}

void
NodeMetadata::setImageComponents(int inputNb, const ImageComponents& components)
{
    if (inputNb == -1) {
        _imp->outputData.components = components;
    } else {
        assert(inputNb < (int)_imp->inputsData.size());
        _imp->inputsData[inputNb].components = components;
    }
}

const ImageComponents&
NodeMetadata::getImageComponents(int inputNb) const
{
    if (inputNb == -1) {
        return _imp->outputData.components;
    } else {
        assert(inputNb < (int)_imp->inputsData.size());
        return _imp->inputsData[inputNb].components;
    }
}

NATRON_NAMESPACE_EXIT;