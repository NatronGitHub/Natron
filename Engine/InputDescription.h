/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef INPUTDESCRIPTION_H
#define INPUTDESCRIPTION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <bitset>
#include "Global/Enums.h"
#include "Engine/PropertiesHolder.h"

/**
 * @brief x1 std::string property indicating the plug-in unique internal script-name of the input
 * Default value - Empty
 * If empty, defaults to the input number.
 **/
#define kInputDescPropScriptName "InputDescPropScriptName"


/**
 * @brief x1 std::string property indicating the label of the input as seen by the user in the NodeGraph GUI
 * Default value - Empty
 * If empty, defaults to the script-name.
 **/
#define kInputDescPropLabel "InputDescPropLabel"

/**
 * @brief x1 std::string property indicating the role of the input as help for the user. This is found in the documentation for
 * the plug-in.
 * Default value - Empty
 **/
#define kInputDescPropHint "InputDescPropHint"

/**
 * @brief x1 std::bitset<4> property indicating the accepted number of components in input.
 * This is a bitset: each bit tells whether the plug-in supports N comps
 * (e.g: if the 4th bit is set to 1 it means theinput supports RGBA)
 * Default value - 0000, no images can be fetched on the input.
 * Must be specified
 **/
#define kInputDescPropSupportedComponents "InputDescPropSupportedComponents"

/**
 * @brief x1 ImageFieldExtractionEnum property
 * This controls how a plug-in wishes to fetch images from a fielded input, so it can tune it behaviour when it renders fielded footage.
 * Note that if it fetches eImageFieldExtractionSingle and the host stores images natively as both fields interlaced, 
 * it can return a single image by doubling rowbytes and tweaking the starting address of the image data. This saves on a buffer copy.
 * Default value - eImageFieldExtractionDouble
 **/
#define kInputDescPropFieldingSupport "InputDescPropFieldingSupport"

/**
 * @brief [DEPRECATED] x1 bool property indicating whether the input may receive 3x3 transform from upstream to apply
 * This property is to be replaced with the newer kInputDescPropCanReceiveDistortion property, but left for compat with older
 * plug-ins which use it.
 * Default value - false
 **/
#define kInputDescPropCanReceiveTransform3x3 "InputDescPropCanReceiveTransform3x3"
/**
 * @brief x1 bool property indicating whether the input may receive a generic distortion function from upstream to apply
 * Default value - false
 **/
#define kInputDescPropCanReceiveDistortion "InputDescPropCanReceiveDistortion"

/**
 * @brief x1 bool property indicating if the input should be visible to the user or not
 * Default value - true
 **/
#define kInputDescPropIsVisible "InputDescPropIsVisible"

/**
 * @brief x1 bool property indicating if the input should be considered optional or not.
 * If optional, the plug-in should render successfully without having this input connected.
 * Default value - false
 **/
#define kInputDescPropIsOptional "InputDescPropIsOptional"

/**
 * @brief x1 bool property indicating whether the input is considered as a mask. This also
 * sets kInputDescPropIsOptional to true.
 * Default value - false
 **/
#define kInputDescPropIsMask "InputDescPropIsMask"

/**
 * @brief x1 bool property indicating whether the input is described by the host and unknown by the plug-in
 * Default value - false
 **/
#define kInputDescPropIsHostInput "InputDescPropIsHostInput"

/**
 * @brief x1 bool property indicating whether the input may receive tiled images;
 * This is only relevant for plug-ins which supports tiles, otherwise this is disregarded and
 * untiled images are received.
 * Default value - true
 **/
#define kInputDescPropSupportsTiles "InputDescPropSupportsTiles"

/**
 * @brief x1 bool property indicating whether the input may fetch images at a different time that the one
 * passed to the render action
 * Default value - false
 **/
#define kInputDescPropSupportsTemporal "InputDescPropSupportsTemporal"

NATRON_NAMESPACE_ENTER

class InputDescription : public PropertiesHolder
{
public:

    InputDescription();

    InputDescription(const InputDescription& other);

    virtual ~InputDescription();

    static InputDescriptionPtr create(const std::string& name, const std::string& label, const std::string& hint, bool optional, bool mask, std::bitset<4> supportedComponents);

private:

    virtual void initializeProperties() const OVERRIDE FINAL;
};
NATRON_NAMESPACE_EXIT

#endif // INPUTDESCRIPTION_H
