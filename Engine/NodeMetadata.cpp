/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeMetadata.h"
#include "Engine/RectI.h"
#include <cassert>
#include <vector>
#include <sstream>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/flat_map.hpp>
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Engine/ImagePlaneDesc.h"
#include "Engine/PropertiesHolder.h"

namespace bip = boost::interprocess;


NATRON_NAMESPACE_ENTER


class NodeMetadata::Implementation : public PropertiesHolder
{
public:


    Implementation()
    : PropertiesHolder()
    {
    }

    Implementation(const Implementation& other)
    : PropertiesHolder(other)
    {

    }

    virtual void initializeProperties() const OVERRIDE FINAL;

    void toMemorySegment(IPCPropertyMap* properties) const;

    void fromMemorySegment(const IPCPropertyMap& properties);

    virtual ~Implementation()
    {

    }

};

NodeMetadata::NodeMetadata()
    : _imp( new Implementation() )
{
}

NodeMetadata::~NodeMetadata()
{
}

void
NodeMetadata::Implementation::initializeProperties() const
{

}

void
NodeMetadata::setIntMetadata(const std::string& name, int value, int index, bool createIfNotExists)
{
    try {
        _imp->setProperty(name, value, index, !createIfNotExists);
    } catch (...) {

    }
}

void
NodeMetadata::setDoubleMetadata(const std::string& name, double value, int index, bool createIfNotExists)
{
    try {
        _imp->setProperty(name, value, index, !createIfNotExists);
    } catch (...) {

    }
}

void
NodeMetadata::setStringMetadata(const std::string& name, const std::string& value, int index, bool createIfNotExists)
{
    try {
        _imp->setProperty(name, value, index, !createIfNotExists);
    } catch (...) {

    }
}

void
NodeMetadata::setIntNMetadata(const std::string& name, const int value[], int count, bool createIfNotExists)
{
    try {
        std::vector<int> values(&value[0], (&value[count -1]) + 1);
        _imp->setPropertyN(name, values, !createIfNotExists);
    } catch (...) {

    }
}

void
NodeMetadata::setDoubleNMetadata(const std::string& name, const double value[], int count, bool createIfNotExists)
{
    try {
        std::vector<double> values(&value[0], (&value[count -1]) + 1);
        _imp->setPropertyN(name, values, !createIfNotExists);
    } catch (...) {

    }
}

void
NodeMetadata::setStringNMetadata(const std::string& name, const std::string value[], int count, bool createIfNotExists)
{
    try {
        std::vector<std::string> values(&value[0], (&value[count -1]) + 1);
        _imp->setPropertyN(name, values, !createIfNotExists);
    } catch (...) {

    }
}

bool
NodeMetadata::getIntMetadata(const std::string& name, int index, int *value) const
{
    return _imp->getPropertySafe<int>(name, index, value);
}

bool
NodeMetadata::getDoubleMetadata(const std::string& name, int index, double *value) const
{
    return _imp->getPropertySafe<double>(name, index, value);
}

bool
NodeMetadata::getStringMetadata(const std::string& name, int index, std::string *value) const
{
    return _imp->getPropertySafe<std::string>(name, index, value);

}

bool
NodeMetadata::getIntNMetadata(const std::string& name, int count, int *value) const
{
    std::vector<int> values;
    if (!_imp->getPropertyNSafe<int>(name, &values)) {
        return false;
    }
    int nVal = std::min(count, (int)values.size());
    memcpy(value, values.data(), sizeof(int) * nVal);
    return true;

}

bool
NodeMetadata::getDoubleNMetadata(const std::string& name, int count, double *value) const
{
    std::vector<double> values;
    if (!_imp->getPropertyNSafe<double>(name, &values)) {
        return false;
    }
    int nVal = std::min(count, (int)values.size());
    memcpy(value, values.data(), sizeof(double) * nVal);
    return true;

}

bool
NodeMetadata::getStringNMetadata(const std::string& name, int count, std::string *value) const
{
    std::vector<std::string> values;
    if (!_imp->getPropertyNSafe<std::string>(name, &values)) {
        return false;
    }

    int nVal = std::min(count, (int)values.size());
    for (int i = 0; i < nVal; ++i) {
        value[i] = values[i];
    }
    return true;

}

int
NodeMetadata::getMetadataDimension(const std::string& name) const
{
    return _imp->getPropertyDimension(name, false /*throwIfFailed*/);
}

void
NodeMetadata::toMemorySegment(IPCPropertyMap* properties) const
{
    _imp->toMemorySegment(properties);
}

void
NodeMetadata::fromMemorySegment(const IPCPropertyMap& properties)
{
    _imp->fromMemorySegment(properties);
}

enum NodeMetadataDataTypeEnum
{
    eNodeMetadataDataTypeInt = 0,
    eNodeMetadataDataTypeDouble,
    eNodeMetadataDataTypeString
};

struct MetadataValueIPC
{
    String_ExternalSegment stringValue;
    double podValue;


    MetadataValueIPC(const external_void_allocator& alloc)
    : stringValue(alloc)
    , podValue(0)
    {

    }

    void operator=(const MetadataValueIPC& other)
    {
        stringValue = other.stringValue;
        podValue = other.podValue;
    }
};
// Typedef our interprocess types
typedef bip::allocator<MetadataValueIPC, ExternalSegmentType::segment_manager> MetadataValueIPC_Allocator_ExternalSegment;

// The unordered set of free tiles indices in a bucket
typedef bip::vector<MetadataValueIPC, MetadataValueIPC_Allocator_ExternalSegment> MetadataValueVector;

struct MetadataValuesIPC
{
    MetadataValueVector values;
    NodeMetadataDataTypeEnum type;


    MetadataValuesIPC(const external_void_allocator& alloc)
    : values(alloc)
    , type(eNodeMetadataDataTypeInt)
    {

    }

    void operator=(const MetadataValuesIPC& other)
    {
        values = other.values;
        type = other.type;
    }
};

void
NodeMetadata::Implementation::toMemorySegment(IPCPropertyMap* properties) const
{

    for (std::map<std::string, boost::shared_ptr<PropertiesHolder::PropertyBase> >::const_iterator it = _properties->begin(); it != _properties->end(); ++it) {


        PropertiesHolder::Property<int>* isInt = dynamic_cast<PropertiesHolder::Property<int>*>(it->second.get());
        PropertiesHolder::Property<double>* isDouble = dynamic_cast<PropertiesHolder::Property<double>*>(it->second.get());
        PropertiesHolder::Property<std::string>* isString = dynamic_cast<PropertiesHolder::Property<std::string>*>(it->second.get());
        assert(isInt || isDouble || isString);

        if (isInt) {
            properties->setIPCPropertyN<int>(it->first, isInt->value);
        } else if (isDouble) {
            properties->setIPCPropertyN<double>(it->first, isDouble->value);
        } else if (isString) {
            properties->setIPCPropertyN<std::string>(it->first, isString->value);
        }
    }
} // toMemorySegment

void
NodeMetadata::Implementation::fromMemorySegment(const IPCPropertyMap& properties)
{


    for (IPCPropertyMap::const_iterator it = properties.begin(); it != properties.end(); ++it) {

        std::string name(it->first.c_str());

        switch (it->second.getType()) {
            case eIPCVariantTypeInt:
            {
                boost::shared_ptr<Property<int> > prop = createPropertyInternal<int>(name);
                IPCPropertyMap::getIPCPropertyN(it->second, &prop->value);
            }   break;
            case eIPCVariantTypeDouble:
            {
                boost::shared_ptr<Property<double> > prop = createPropertyInternal<double>(name);
                IPCPropertyMap::getIPCPropertyN(it->second, &prop->value);
            }   break;
            case eIPCVariantTypeString:
            {
                boost::shared_ptr<Property<std::string> > prop = createPropertyInternal<std::string>(name);
                IPCPropertyMap::getIPCPropertyN(it->second, &prop->value);
            }   break;
            default:
                break;
        }


    }

} // fromMemorySegment


void
NodeMetadata::setOutputPremult(ImagePremultiplicationEnum premult)
{
    setIntMetadata(kNatronMetadataOutputPremult, (int)premult, 0, true);
}

ImagePremultiplicationEnum
NodeMetadata::getOutputPremult() const
{
    int ret;
    if (getIntMetadata(kNatronMetadataOutputPremult, 0, &ret)) {
        return (ImagePremultiplicationEnum)ret;
    }
    return eImagePremultiplicationPremultiplied;
}

void
NodeMetadata::setOutputFrameRate(double fps)
{
    setDoubleMetadata(kNatronMetadataOutputFrameRate, fps, 0, true);
}

double
NodeMetadata::getOutputFrameRate() const
{
    double ret;
    if (getDoubleMetadata(kNatronMetadataOutputFrameRate, 0, &ret)) {
        return ret;
    }
    return 24.;
}

void
NodeMetadata::setOutputFielding(ImageFieldingOrderEnum fielding)
{
    setIntMetadata(kNatronMetadataOutputFielding, (int)fielding, 0, true);
}

ImageFieldingOrderEnum
NodeMetadata::getOutputFielding() const
{
    int ret;
    if (getIntMetadata(kNatronMetadataOutputFielding, 0, &ret)) {
        return (ImageFieldingOrderEnum)ret;
    }
    return eImageFieldingOrderNone;
}

void
NodeMetadata::setIsContinuous(bool continuous)
{
    setIntMetadata(kNatronMetadataIsContinuous, (int)continuous, 0, true);
}

bool
NodeMetadata::getIsContinuous() const
{
    int ret;
    if (getIntMetadata(kNatronMetadataIsContinuous, 0, &ret)) {
        return (bool)ret;
    }
    return false;
}

void
NodeMetadata::setIsFrameVarying(bool varying)
{
    setIntMetadata(kNatronMetadataIsFrameVarying, (int)varying, 0, true);
}

bool
NodeMetadata::getIsFrameVarying() const
{
    int ret;
    if (getIntMetadata(kNatronMetadataIsFrameVarying, 0, &ret)) {
        return (bool)ret;
    }
    return false;
}

void
NodeMetadata::setPixelAspectRatio(int inputNb, double par)
{
    std::stringstream ss;
    ss << kNatronMetadataPixelAspectRatio << inputNb;
    setDoubleMetadata(ss.str(), par, 0, true);
}

double
NodeMetadata::getPixelAspectRatio(int inputNb) const
{
    std::stringstream ss;
    ss << kNatronMetadataPixelAspectRatio << inputNb;
    double ret;
    if (getDoubleMetadata(ss.str(), 0, &ret)) {
        return ret;
    }
    return 1.;
}

void
NodeMetadata::setBitDepth(int inputNb, ImageBitDepthEnum depth)
{
    std::stringstream ss;
    ss << kNatronMetadataBitDepth << inputNb;
    setIntMetadata(ss.str(), (int)depth, 0, true);
}

ImageBitDepthEnum
NodeMetadata::getBitDepth(int inputNb) const
{
    std::stringstream ss;
    ss << kNatronMetadataBitDepth << inputNb;
    int ret;
    if (getIntMetadata(ss.str(), 0, &ret)) {
        return (ImageBitDepthEnum)ret;
    }
    return eImageBitDepthFloat;
}


void
NodeMetadata::setColorPlaneNComps(int inputNb, int nComps)
{
    std::stringstream ss;
    ss << kNatronMetadataColorPlaneNComps << inputNb;
    setIntMetadata(ss.str(), (int)nComps, 0, true);
}

int
NodeMetadata::getColorPlaneNComps(int inputNb) const
{
    std::stringstream ss;
    ss << kNatronMetadataColorPlaneNComps << inputNb;
    int ret;
    if (getIntMetadata(ss.str(), 0, &ret)) {
        return ret;
    }
    return 4;
}

void
NodeMetadata::setComponentsType(int inputNb, const std::string& componentsType)
{
    std::stringstream ss;
    ss << kNatronMetadataComponentsType << inputNb;
    setStringMetadata(ss.str(), componentsType, 0, true);
}

std::string
NodeMetadata::getComponentsType(int inputNb) const
{
    std::stringstream ss;
    ss << kNatronMetadataComponentsType << inputNb;
    std::string ret;
    if (getStringMetadata(ss.str(), 0, &ret)) {
        return ret;
    }
    return kNatronColorPlaneID;
}

void
NodeMetadata::setOutputFormat(const RectI& format)
{
    setIntNMetadata(kNatronMetadataOutputFormat, &format.x1, 4, true);
}

RectI
NodeMetadata::getOutputFormat() const
{
    RectI ret;
    getIntNMetadata(kNatronMetadataOutputFormat, 4, &ret.x1);
    return ret;
}

NATRON_NAMESPACE_EXIT
