/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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


NATRON_NAMESPACE_ENTER;


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

    void toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const;

    void fromMemorySegment(ExternalSegmentType* segment,
                           ExternalSegmentTypeHandleList::const_iterator *start,
                           ExternalSegmentTypeHandleList::const_iterator end);

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
NodeMetadata::toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const
{
    _imp->toMemorySegment(segment, objectPointers);
}

void
NodeMetadata::fromMemorySegment(ExternalSegmentType* segment,
                                ExternalSegmentTypeHandleList::const_iterator *start,
                                ExternalSegmentTypeHandleList::const_iterator end)
{
    _imp->fromMemorySegment(segment, start, end);
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


    MetadataValueIPC(const void_allocator& alloc)
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


    MetadataValuesIPC(const void_allocator& alloc)
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

typedef std::pair<String_ExternalSegment, MetadataValuesIPC> MetadataMapIPCValue;

typedef bip::allocator<MetadataMapIPCValue, ExternalSegmentType::segment_manager> MetadataMapIPCValue_Allocator_ExternalSegment;

typedef bip::flat_map<String_ExternalSegment, MetadataValuesIPC, std::less<String_ExternalSegment>, MetadataMapIPCValue_Allocator_ExternalSegment> MetadataMapIPC;

void
NodeMetadata::Implementation::toMemorySegment(ExternalSegmentType* segment, ExternalSegmentTypeHandleList* objectPointers) const
{
    // Add a prefix to the meta-data name in the memory segment to ensure that the meta-data name is not the same as
    // another item we serialized to the segment.
    void_allocator allocator(segment->get_segment_manager());
    MetadataMapIPC* ipcMap = segment->construct<MetadataMapIPC>(bip::anonymous_instance)(allocator);
    if (!ipcMap) {
        throw std::bad_alloc();
    }
    objectPointers->push_back(segment->get_handle_from_address(ipcMap));

    for (std::map<std::string, boost::shared_ptr<PropertiesHolder::PropertyBase> >::const_iterator it = _properties.begin(); it != _properties.end(); ++it) {

        String_ExternalSegment name(allocator);
        MetadataValuesIPC values(allocator);

        MetadataMapIPCValue pair = std::make_pair(name, values);

        pair.first.append(it->first.c_str());


        PropertiesHolder::Property<int>* isInt = dynamic_cast<PropertiesHolder::Property<int>*>(it->second.get());
        PropertiesHolder::Property<double>* isDouble = dynamic_cast<PropertiesHolder::Property<double>*>(it->second.get());
        PropertiesHolder::Property<std::string>* isString = dynamic_cast<PropertiesHolder::Property<std::string>*>(it->second.get());
        assert(isInt || isDouble || isString);


        int nDims = it->second->getNDimensions();
        for (int i = 0; i < nDims; ++i) {

            MetadataValueIPC data(allocator);

            if (isInt) {
                values.type = eNodeMetadataDataTypeInt;
                data.podValue = (double)isInt->value[i];
            } else if (isDouble) {
                values.type = eNodeMetadataDataTypeDouble;
                data.podValue = (double)isDouble->value[i];
            } else if (isString) {
                values.type = eNodeMetadataDataTypeString;
                data.stringValue.append(isString->value[i].c_str());
            }
            pair.second.values.push_back(data);
        }

        ipcMap->insert(boost::move(pair));
    }
} // toMemorySegment

void
NodeMetadata::Implementation::fromMemorySegment(ExternalSegmentType* segment,
                                                ExternalSegmentTypeHandleList::const_iterator *start,
                                                ExternalSegmentTypeHandleList::const_iterator /*end*/)
{

    MetadataMapIPC* ipcMap = (MetadataMapIPC*)segment->get_address_from_handle(**start);
    ++(*start);

    for (MetadataMapIPC::const_iterator it = ipcMap->begin(); it != ipcMap->end(); ++it) {

        std::string name(it->first.c_str());
        assert(!it->second.values.empty());

        switch (it->second.type) {
            case eNodeMetadataDataTypeInt:
            {
                boost::shared_ptr<Property<int> > prop = createPropertyInternal<int>(name);
                prop->value.resize(it->second.values.size());
                for (std::size_t i = 0; i < it->second.values.size(); ++i) {
                    prop->value[i] = (int)it->second.values[i].podValue;
                }
            }   break;
            case eNodeMetadataDataTypeDouble:
            {
                boost::shared_ptr<Property<double> > prop = createPropertyInternal<double>(name);
                prop->value.resize(it->second.values.size());
                for (std::size_t i = 0; i < it->second.values.size(); ++i) {
                    prop->value[i] = it->second.values[i].podValue;
                }
            }   break;
            case eNodeMetadataDataTypeString:
            {
                boost::shared_ptr<Property<std::string> > prop = createPropertyInternal<std::string>(name);
                prop->value.resize(it->second.values.size());
                for (std::size_t i = 0; i < it->second.values.size(); ++i) {
                    prop->value[i].append(it->second.values[i].stringValue.c_str());
                }
            }   break;
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
<<<<<<< HEAD
NodeMetadata::setPixelAspectRatio(int inputNb, double par)
{
    std::stringstream ss;
    ss << kNatronMetadataPixelAspectRatio << inputNb;
    setDoubleMetadata(ss.str(), par, 0, true);
=======
NodeMetadata::setPixelAspectRatio(int inputNb,
                                  double par)
{
    if (inputNb == -1) {
        _imp->outputData.pixelAspectRatio = par;
    } else {
        if ( inputNb < (int)_imp->inputsData.size() ) {
            _imp->inputsData[inputNb].pixelAspectRatio = par;
        }
    }
>>>>>>> RB-2.2
}

double
NodeMetadata::getPixelAspectRatio(int inputNb) const
{
<<<<<<< HEAD
    std::stringstream ss;
    ss << kNatronMetadataPixelAspectRatio << inputNb;
    double ret;
    if (getDoubleMetadata(ss.str(), 0, &ret)) {
        return ret;
=======
    if (inputNb == -1) {
        return _imp->outputData.pixelAspectRatio;
    } else {
        if ( inputNb < (int)_imp->inputsData.size() ) {
            return _imp->inputsData[inputNb].pixelAspectRatio;
        } else {
            return 1.;
        }
>>>>>>> RB-2.2
    }
    return 1.;
}

void
<<<<<<< HEAD
NodeMetadata::setBitDepth(int inputNb, ImageBitDepthEnum depth)
{
    std::stringstream ss;
    ss << kNatronMetadataBitDepth << inputNb;
    setIntMetadata(ss.str(), (int)depth, 0, true);
=======
NodeMetadata::setBitDepth(int inputNb,
                          ImageBitDepthEnum depth)
{
    if (inputNb == -1) {
        _imp->outputData.bitdepth = depth;
    } else {
        if ( inputNb < (int)_imp->inputsData.size() ) {
            _imp->inputsData[inputNb].bitdepth = depth;
        }
        
    }
>>>>>>> RB-2.2
}

ImageBitDepthEnum
NodeMetadata::getBitDepth(int inputNb) const
{
<<<<<<< HEAD
    std::stringstream ss;
    ss << kNatronMetadataBitDepth << inputNb;
    int ret;
    if (getIntMetadata(ss.str(), 0, &ret)) {
        return (ImageBitDepthEnum)ret;
=======
    if (inputNb == -1) {
        return _imp->outputData.bitdepth;
    } else {
        if ( inputNb < (int)_imp->inputsData.size() ) {
            return _imp->inputsData[inputNb].bitdepth;
        } else {
            return eImageBitDepthFloat;
        }
>>>>>>> RB-2.2
    }
    return eImageBitDepthFloat;
}


void
<<<<<<< HEAD
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
=======
NodeMetadata::setImageComponents(int inputNb,
                                 const ImageComponents& components)
{
    if (inputNb == -1) {
        _imp->outputData.components = components;
    } else {
        if ( inputNb < (int)_imp->inputsData.size() ) {
            _imp->inputsData[inputNb].components = components;
        }
>>>>>>> RB-2.2
    }
    return 4;
}

void
NodeMetadata::setComponentsType(int inputNb, const std::string& componentsType)
{
<<<<<<< HEAD
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
        return ret;    }
    return kNatronColorPlaneID;
=======
    if (inputNb == -1) {
        return _imp->outputData.components;
    } else {
        if ( inputNb < (int)_imp->inputsData.size() ) {
            return _imp->inputsData[inputNb].components;
        } else {
            return ImageComponents::getRGBAComponents();
        }
    }
>>>>>>> RB-2.2
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

NATRON_NAMESPACE_EXIT;
