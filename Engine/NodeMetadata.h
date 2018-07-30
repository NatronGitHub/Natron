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

#ifndef Engine_NodeMetadata_h
#define Engine_NodeMetadata_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/IPCCommon.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

// Common meta-datas definitions
// Each meta-data is encoded as a property.
// A meta-data property can have the following types: int, double, std::string
// A meta-data property may be multi-dimensional hence can store more complex data-types.
// We do not allow more data-types to be compliant with OpenFX properties.
//
// Note: a meta-data of type std::string can store binary data as long as the std::string
// is handled correctly, i.e:
//
// Creating the string:
//
// void* binData = ...;
// std::size_t binDataSize = ...;
// std::string myString(binData, binDataSize);
//
// Another variant:
//
// std::string myString;
// myString.push_back('a');
// myString.push_back('\0');
// assert(s.length() == 2);
//
// And to access the data in the std::string:
//
// void* binData = myString.data();
//
// Do not use the c_str() function which will null terminate the buffer.

// int x1 property
// Valid values: corresponds to the ImagePremultiplicationEnum
// Default value: eImagePremultiplicationPremultiplied
// The alpha premultiplication state in output of the effect.
#define kNatronMetadataOutputPremult "NatronMetadataOutputPremult"

// double x1 property
// Valid values: any positive double value
// Default value: 24.
// The frame rate in output of the effect
#define kNatronMetadataOutputFrameRate "NatronMetadataOutputFrameRate"

// int x1 property
// Valid values: corresponds to the ImageFieldingOrderEnum
// Default value: eImageFieldingOrderNone
// The image fielding in output of the effect
#define kNatronMetadataOutputFielding "NatronMetadataOutputFielding"

// int x1 property
// Valid values: 0 or 1
// Default value: 0
// Can the effect be sampled at non integer frames?
#define kNatronMetadataIsContinuous "NatronMetadataIsContinuous"

// int x1 property
// Valid values: 0 or 1
// Default value: 0
// Does this effect produce a different result at different times, even if its parameters are not animated
#define kNatronMetadataIsFrameVarying "NatronMetadataIsFrameVarying"

// double x1 property
// Valid values: Any positive double value
// Default value: 1.
// The pixel aspect ratio on the different inputs and in output of the effect.
// The index of the input (or -1 for the output) must be appended to the property
// name.
#define kNatronMetadataPixelAspectRatio "NatronMetadataPixelAspectRatio_"

// int x1 property
// Valid values: corresponds to the ImageBitDepthEnum
// Default value: eImageBitDepthFloat
// The bitdepth on the different inputs and in output of the effect.
// The index of the input (or -1 for the output) must be appended to the property
// name.
#define kNatronMetadataBitDepth "NatronMetadataBitDepth_"

// int x1 property
// Valid values: 1, 2, 3, 4
// Default value: 4
// The effect desired number of components for images coming from an input or in output
// of the effect.
// For a multi-planar effect, only images for the color plane will be remapped to this
// number of components.
// The index of the input (or -1 for the output) must be appended to the property
// name.
#define kNatronMetadataColorPlaneNComps "NatronMetadataColorPlaneNComps_"

// std::string x1 property
// Valid values:
// - kNatronColorPlaneName indicating the number of components specified with kNatronMetadataColorPlaneNComps should be mapped to the color-plane
//  Special 2-channel images are mapped to XY images to enable non-multiplanar effects to support 2-channel images.
// - kNatronDisparityComponentsLabel indicating that the plug-in expects to receive 2-channel images and 2 planes (Disparity Left/Right)
// - kNatronMotionComponentsLabel indicating that the plug-in expects to receive 2-channel images and 2 planes (Motion Backward/Forward)
// Default value: kNatronColorPlaneID
//
// The effect desired components type for images coming from an input or in output
// of the effect.
// For multi-planar effects, this is completly disregarded
// The index of the input (or -1 for the output) must be appended to the property
// name.
#define kNatronMetadataComponentsType "NatronMetadataComponentsType_"

// int x4 property
// Valid values: Any
//
// Used to indicate the format size (in pixel coordinates) of the stream that should be displayed in output of this plug-in.
// It is not necessarily the same as the region of definition.
// Example: User has a project size 2K.
// The user draws a rectangle which does not cover the full project size,
// the region of definition is thus smaller than the project size.
// Think now of a Reformat plug-in appended after the rectangle to perform a scale down to 1K:
// the region of definition in output of the plug-in should be half the region of the original
// rectangle and the format should become 1K.
// Without this property, the Reformat plug-in would be forced to have
// a region of definition of 1K and render many un-needed pixels.
#define kNatronMetadataOutputFormat "NatronMetadataOutputFormat"

/**
 * @brief These meta-data are represents what's flowing through a node.
 **/

class NodeMetadata : public boost::noncopyable
{
public:

    NodeMetadata();

    ~NodeMetadata();

    /**
     * @brief Set a meta-data to the given value.
     * @param name The unique name identifying the meta-data
     * @param value The value to set the meta-data
     * @param index If the meta-data is multi-dimensional, this is the index
     * @param createIfNotExists If this property does not exist, create it.
     *
     * Note that if the index is greater than the dimension of the meta-data, the meta-data dimension
     * will be expanded so it includes at least the index.
     **/
    void setIntMetadata(const std::string& name, int value, int index, bool createIfNotExists);
    void setDoubleMetadata(const std::string& name, double value, int index, bool createIfNotExists);
    void setStringMetadata(const std::string& name, const std::string& value, int index, bool createIfNotExists);

    /**
     * @brief Same as the set function above but set all dimensions of the property at once.
     **/
    void setIntNMetadata(const std::string& name, const int value[], int count, bool createIfNotExists);
    void setDoubleNMetadata(const std::string& name, const double value[], int count, bool createIfNotExists);
    void setStringNMetadata(const std::string& name, const std::string value[], int count, bool createIfNotExists);

    /**
     * @brief Get a meta-data value.
     * @param name The unique name identifying the meta-data
     * @param index If the meta-data is multi-dimensional, this is the index of the value to retrieve.
     * @returns True if a property exists with a matching name and type and the index is valid, false otherwise.
     **/
    bool getIntMetadata(const std::string& name, int index, int *value) const;
    bool getDoubleMetadata(const std::string& name, int index, double *value) const;
    bool getStringMetadata(const std::string& name, int index, std::string *value) const;

    /**
     * @brief Same as the get function above but read all dimensions of the property at once.
     **/
    bool getIntNMetadata(const std::string& name, int count, int *value) const;
    bool getDoubleNMetadata(const std::string& name, int count, double *value) const;
    bool getStringNMetadata(const std::string& name, int count, std::string *value) const;

    /**
     * @brief Get the number of dimensions in the meta-data
     * @returns A value > 0 if a matching meta-data was found, 0 otherwise.
     **/
    int getMetadataDimension(const std::string& name) const;

    /**
     * @brief Serializes the meta-data to a memory segment
     **/
    void toMemorySegment(IPCPropertyMap* properties) const;

    /**
     * @brief Serializes the meta-data from a memory segment
     **/
    void fromMemorySegment(const IPCPropertyMap& properties);

    // Helper function for built-in meta-datas

    void setOutputPremult(ImagePremultiplicationEnum premult);

    ImagePremultiplicationEnum getOutputPremult() const;

    void setOutputFrameRate(double fps);

    double getOutputFrameRate() const;

    void setOutputFielding(ImageFieldingOrderEnum fielding);

    ImageFieldingOrderEnum getOutputFielding() const;

    void setIsContinuous(bool continuous);

    bool getIsContinuous() const;

    void setIsFrameVarying(bool varying);

    bool getIsFrameVarying() const;

    void setPixelAspectRatio(int inputNb, double par);

    double getPixelAspectRatio(int inputNb) const;

    void setBitDepth(int inputNb, ImageBitDepthEnum depth);

    ImageBitDepthEnum getBitDepth(int inputNb) const;

    void setColorPlaneNComps(int inputNb, int nComps);

    int getColorPlaneNComps(int inputNb) const;

    void setComponentsType(int inputNb, const std::string& componentsType);

    std::string getComponentsType(int inputNb) const;

    void setOutputFormat(const RectI& format);
    
    RectI getOutputFormat() const;

private:

    class Implementation;
    boost::scoped_ptr<Implementation> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_NodeMetadata_h
