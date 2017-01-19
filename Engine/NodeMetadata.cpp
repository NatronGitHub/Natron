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

#include "Engine/PropertiesHolder.h"


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

    void toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers) const;

    void fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix);

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
    try {
        *value = _imp->getProperty<int>(name, index);
        return true;
    } catch (...) {
        return false;
    }
}

bool
NodeMetadata::getDoubleMetadata(const std::string& name, int index, double *value) const
{
    try {
        *value = _imp->getProperty<double>(name, index);
        return true;
    } catch (...) {
        return false;
    }
}

bool
NodeMetadata::getStringMetadata(const std::string& name, int index, std::string *value) const
{
    try {
        *value = _imp->getProperty<std::string>(name, index);
        return true;
    } catch (...) {
        return false;
    }
}

bool
NodeMetadata::getIntNMetadata(const std::string& name, int count, int *value) const
{
    try {
        const std::vector<int>& values = _imp->getPropertyN<int>(name);
        int nVal = std::min(count, (int)values.size());
        memcpy(value, values.data(), sizeof(int) * nVal);
        return true;
    } catch (...) {
        return false;
    }
}

bool
NodeMetadata::getDoubleNMetadata(const std::string& name, int count, double *value) const
{
    try {
        const std::vector<double>& values = _imp->getPropertyN<double>(name);
        int nVal = std::min(count, (int)values.size());
        memcpy(value, values.data(), sizeof(double) * nVal);
        return true;
    } catch (...) {
        return false;
    }
}

bool
NodeMetadata::getStringNMetadata(const std::string& name, int count, std::string *value) const
{
    try {
        const std::vector<std::string>& values = _imp->getPropertyN<std::string>(name);
        int nVal = std::min(count, (int)values.size());
        for (int i = 0; i < nVal; ++i) {
            value[i] = values[i];
        }
        return true;
    } catch (...) {
        return false;
    }

}

int
NodeMetadata::getMetadataDimension(const std::string& name) const
{
    return _imp->getPropertyDimension(name, false /*throwIfFailed*/);
}

void
NodeMetadata::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers) const
{
    _imp->toMemorySegment(segment, objectNamesPrefix, objectPointers);
}

void
NodeMetadata::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix)
{
    _imp->fromMemorySegment(segment, objectNamesPrefix);
}

enum NodeMetadataDataTypeEnum
{
    eNodeMetadataDataTypeInt = 0,
    eNodeMetadataDataTypeDouble,
    eNodeMetadataDataTypeString
};

void
NodeMetadata::Implementation::toMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix, ExternalSegmentTypeHandleList* objectPointers) const
{
    // Add a prefix to the meta-data name in the memory segment to ensure that the meta-data name is not the same as
    // another item we serialized to the segment.
    const std::string prefix = objectNamesPrefix + "NodeMetadata";
    int nElements = _properties.size();
    objectPointers->push_back(writeNamedSharedObject(nElements, prefix + "NElements", segment));

    int i = 0;
    for (std::map<std::string, boost::shared_ptr<PropertiesHolder::PropertyBase> >::const_iterator it = _properties.begin(); it != _properties.end(); ++it, ++i) {
        std::stringstream ss;
        ss << prefix << i;
        std::string metadataPrefix = ss.str();

        {
            std::string metadataName = metadataPrefix + "Name";
            objectPointers->push_back(writeNamedSharedObject(it->first, metadataName, segment));
        }

        int nDims = it->second->getNDimensions();
        objectPointers->push_back(writeNamedSharedObject(nDims, metadataPrefix + "NDims", segment));

        PropertiesHolder::Property<int>* isInt = dynamic_cast<PropertiesHolder::Property<int>*>(it->second.get());
        PropertiesHolder::Property<double>* isDouble = dynamic_cast<PropertiesHolder::Property<double>*>(it->second.get());
        PropertiesHolder::Property<std::string>* isString = dynamic_cast<PropertiesHolder::Property<std::string>*>(it->second.get());

        std::string metadataData = metadataPrefix + "Data";
        std::string metadataType = metadataPrefix + "Type";
        assert(isInt || isDouble || isString);
        if (isInt) {
            NodeMetadataDataTypeEnum type = eNodeMetadataDataTypeInt;
            objectPointers->push_back(writeNamedSharedObject((int)type, metadataType, segment));
            objectPointers->push_back(writeNamedSharedObjectN(isInt->value.data(), nDims, metadataData, segment));
        } else if (isDouble) {
            NodeMetadataDataTypeEnum type = eNodeMetadataDataTypeDouble;
            objectPointers->push_back(writeNamedSharedObject((int)type, metadataType, segment));
            objectPointers->push_back(writeNamedSharedObjectN(isDouble->value.data(), nDims, metadataData, segment));
        } else if (isString) {
            NodeMetadataDataTypeEnum type = eNodeMetadataDataTypeString;
            objectPointers->push_back(writeNamedSharedObject((int)type, metadataType, segment));
            objectPointers->push_back(writeNamedSharedObjectN(isString->value.data(), nDims, metadataData, segment));
        }
        
    }
} // toMemorySegment

void
NodeMetadata::Implementation::fromMemorySegment(ExternalSegmentType* segment, const std::string& objectNamesPrefix)
{
    const std::string prefix = objectNamesPrefix + "NodeMetadata";
    int nElements;
    readNamedSharedObject(prefix + "NElements", segment, &nElements);

    for (int i = 0; i < nElements; ++i) {
        std::stringstream ss;
        ss << prefix << i;
        std::string metadataPrefix = ss.str();

        std::string metadataName;
        readNamedSharedObject(metadataPrefix + "Name", segment, &metadataName);

        int nDims;
        readNamedSharedObject(metadataPrefix + "NDims", segment, &nDims);
        int type_i;
        readNamedSharedObject(metadataPrefix + "Type", segment, &type_i);

        std::string metadataData = metadataPrefix + "Data";
        switch ((NodeMetadataDataTypeEnum)type_i) {
            case eNodeMetadataDataTypeInt: {
                boost::shared_ptr<Property<int> > prop = createPropertyInternal<int>(metadataName);
                prop->value.resize(nDims);
                readNamedSharedObjectN<int>(metadataData, segment, nDims, prop->value.data());
            }   break;
            case eNodeMetadataDataTypeDouble: {
                boost::shared_ptr<Property<double> > prop = createPropertyInternal<double>(metadataName);
                prop->value.resize(nDims);
                readNamedSharedObjectN<double>(metadataData, segment, nDims, prop->value.data());
            }   break;

            case eNodeMetadataDataTypeString: {
                boost::shared_ptr<Property<std::string> > prop = createPropertyInternal<std::string>(metadataName);
                prop->value.resize(nDims);
                readNamedSharedObjectN<std::string>(metadataData, segment, nDims, prop->value.data());
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
