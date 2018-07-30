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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "IPCCommon.h"
#include "Engine/Hash64.h"

NATRON_NAMESPACE_ENTER

IPCVariant::IPCVariant()
: scalar(0)
, string(0)
{

}

IPCVariant::IPCVariant(const IPCVariant& other)
{
    *this = other;
}

void
IPCVariant::operator=(const IPCVariant& other)
{
    if (other.string) {
        ExternalSegmentType::segment_manager* manager = other.string->get_allocator().get_segment_manager();
        CharAllocator_ExternalSegment charAlloc(manager);
        string = manager->construct<String_ExternalSegment>(boost::interprocess::anonymous_instance)(charAlloc);
    } else {
        scalar = other.scalar;
    }
}

IPCVariant::~IPCVariant()
{
    if (string) {
        ExternalSegmentType::segment_manager* manager = string->get_allocator().get_segment_manager();
        manager->destroy_ptr<String_ExternalSegment>(string.get());
        string = 0;
    }
}

void
IPCProperty::clear()
{
    data.clear();
}

template <typename T>
void
IPCProperty::getTypeInfos(TypeFunctions<T>* /*functions*/, IPCVariantTypeEnum* /*type*/)
{
    assert(false);
    throw std::invalid_argument("Unknown type");
}

template <>
void
IPCProperty::getTypeInfos(TypeFunctions<std::string>* functions, IPCVariantTypeEnum* type)
{
    *type = eIPCVariantTypeString;
    functions->getter = &IPCProperty::getStringValue;
    functions->setter = &IPCProperty::setStringValue;
}

template <>
void
IPCProperty::getTypeInfos(TypeFunctions<bool>* functions, IPCVariantTypeEnum* type)
{
    *type = eIPCVariantTypeBool;
    functions->getter = &IPCProperty::getBoolValue;
    functions->setter = &IPCProperty::setBoolValue;
}

template <>
void
IPCProperty::getTypeInfos(TypeFunctions<int>* functions, IPCVariantTypeEnum* type)
{
    *type = eIPCVariantTypeInt;
    functions->getter = &IPCProperty::getIntValue;
    functions->setter = &IPCProperty::setIntValue;
}

template <>
void
IPCProperty::getTypeInfos(TypeFunctions<U32>* functions, IPCVariantTypeEnum* type)
{
    *type = eIPCVariantTypeULong;
    functions->getter = &IPCProperty::getULongValue;
    functions->setter = &IPCProperty::setULongValue;
}


template <>
void
IPCProperty::getTypeInfos(TypeFunctions<U64>* functions, IPCVariantTypeEnum* type)
{
    *type = eIPCVariantTypeULongLong;
    functions->getter = &IPCProperty::getULongLongValue;
    functions->setter = &IPCProperty::setULongLongValue;
}

template <>
void
IPCProperty::getTypeInfos(TypeFunctions<double>* functions, IPCVariantTypeEnum* type)
{
    *type = eIPCVariantTypeDouble;
    functions->getter = &IPCProperty::getDoubleValue;
    functions->setter = &IPCProperty::setDoubleValue;
}


std::size_t
IPCProperty::getNumDimensions() const
{
    return data.size();
}

void
IPCProperty::resize(std::size_t nDims)
{
    if (data.size() != nDims) {
        data.resize(nDims);
    }
}

void
IPCProperty::getStringValue(const IPCVariantVector& vec, int index, std::string* value) 
{
    assert(index >= 0 && index < (int)vec.size());
    *value = std::string(vec[index].string->c_str());
}

void
IPCProperty::setStringValue(int index, const std::string& value, IPCVariantVector* vec)
{
    assert(index >= 0 && index < (int)vec->size());
    ExternalSegmentType::segment_manager* manager = vec->get_allocator().get_segment_manager();
    CharAllocator_ExternalSegment charAlloc(manager);
    if ((*vec)[index].string) {
        (*vec)[index].string->clear();
    } else {
        (*vec)[index].string = manager->construct<String_ExternalSegment>(boost::interprocess::anonymous_instance)(charAlloc);
    }
    (*vec)[index].string->append(value.c_str());
}

void
IPCProperty::getBoolValue(const IPCVariantVector& vec, int index, bool* value)
{
    assert(index >= 0 && index < (int)vec.size());
    *value = Hash64::fromU64<bool>(vec[index].scalar);
}

void
IPCProperty::setBoolValue(int index, const bool& value, IPCVariantVector* vec)
{
    assert(index >= 0 && index < (int)vec->size());
    (*vec)[index].scalar = Hash64::toU64<bool>(value);
}

void
IPCProperty::getIntValue(const IPCVariantVector& vec, int index, int* value)
{
    assert(index >= 0 && index < (int)vec.size());
    *value = Hash64::fromU64<int>(vec[index].scalar);
}

void
IPCProperty::setIntValue(int index, const int& value, IPCVariantVector* vec)
{
    assert(index >= 0 && index < (int)vec->size());
    (*vec)[index].scalar = Hash64::toU64<int>(value);
}

void
IPCProperty::getULongValue(const IPCVariantVector& vec, int index, U32* value)
{
    assert(index >= 0 && index < (int)vec.size());
    *value = vec[index].scalar;
}

void
IPCProperty::setULongValue(int index, const U32& value, IPCVariantVector* vec)
{
    assert(index >= 0 && index < (int)vec->size());
    (*vec)[index].scalar = value;
}

void
IPCProperty::getULongLongValue(const IPCVariantVector& vec, int index, U64* value)
{
    assert(index >= 0 && index < (int)vec.size());
    *value = vec[index].scalar;
}

void
IPCProperty::setULongLongValue(int index, const U64& value, IPCVariantVector* vec)
{
    assert(index >= 0 && index < (int)vec->size());
    (*vec)[index].scalar = value;
}

void
IPCProperty::getDoubleValue(const IPCVariantVector& vec, int index, double* value)
{
    assert(index >= 0 && index < (int)vec.size());
    *value = Hash64::fromU64<double>(vec[index].scalar);
}

void
IPCProperty::setDoubleValue(int index, const double& value, IPCVariantVector* vec)
{
    assert(index >= 0 && index < (int)vec->size());
    (*vec)[index].scalar = Hash64::toU64<double>(value);
}



IPCProperty*
IPCPropertyMap::getOrCreateIPCProperty(const std::string& name, IPCVariantTypeEnum type)
{
    ExternalSegmentType::segment_manager* manager = _properties.get_allocator().get_segment_manager();

    CharAllocator_ExternalSegment charAlloc(manager);
    String_ExternalSegment nameKey(charAlloc);
    nameKey.append(name.c_str());

    IPCPropertyMap::IPCVariantMap::iterator found = _properties.find(nameKey);
    if (found != _properties.end()) {
        if (found->second.getType() != type) {
            assert(false);
            throw std::invalid_argument("A property with the name " + name + " already exists but with a different type");
        }
        return &found->second;
    }


    ExternalSegmentTypeIPCVariantAllocator vecAlloc(manager);
    IPCProperty prop(type, vecAlloc);

    // We must create the pair before or else this does not compile.
    IPCPropertyMap::IPCVariantMapValueType valPair = std::make_pair(nameKey, prop);
    std::pair<IPCPropertyMap::IPCVariantMap::iterator,bool> ret = _properties.insert(valPair);
    assert(ret.second);
    return &ret.first->second;
} // getOrCreateIPCProperty

const IPCProperty*
IPCPropertyMap::getIPCProperty(const std::string& name) const
{
    ExternalSegmentType::segment_manager* manager = _properties.get_allocator().get_segment_manager();

    CharAllocator_ExternalSegment charAlloc(manager);
    String_ExternalSegment nameKey(charAlloc);
    nameKey.append(name.c_str());

    IPCPropertyMap::IPCVariantMap::const_iterator found = _properties.find(nameKey);
    if (found != _properties.end()) {
        return &found->second;
    }
    return (const IPCProperty*)NULL;

} // getOrCreateIPCProperty

IPCPropertyMap::IPCPropertyMap(const external_void_allocator& alloc)
: _properties(alloc)
{

}

IPCPropertyMap::~IPCPropertyMap()
{

}

IPCPropertyMap::const_iterator
IPCPropertyMap::begin() const
{
    return _properties.begin();
}

IPCPropertyMap::const_iterator
IPCPropertyMap::end() const
{
    return _properties.end();
}

ExternalSegmentType::segment_manager*
IPCPropertyMap::getSegmentManager() const
{
    return _properties.get_allocator().get_segment_manager();
}

void
IPCPropertyMap::operator=(const IPCPropertyMap& other)
{
    _properties = other._properties;
}

void
IPCPropertyMap::clear()
{
    _properties.clear();
}



NATRON_NAMESPACE_EXIT
