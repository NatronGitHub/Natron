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

#include "IPCCommon.h"
#include "Engine/Hash64.h"

NATRON_NAMESPACE_ENTER;

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

/**
 * @brief Append a string to a property
 **/
inline void appendIPCPropString(const std::string& val, IPCVariantVector* prop)
{
    ExternalSegmentType::segment_manager* manager = prop->get_allocator().get_segment_manager();
    CharAllocator_ExternalSegment charAlloc(manager);
    IPCVariant variant;
    variant.string = manager->construct<String_ExternalSegment>(boost::interprocess::anonymous_instance)(charAlloc);
    variant.string->append(val.c_str());
    prop->push_back(variant);
}

/**
 * @brief Append a string to a property
 **/
template <typename T>
inline void appendIPCPropScalar(T val, IPCVariantVector* prop)
{
    IPCVariant variant;
    variant.scalar = Hash64::toU64<T>(val);
    prop->push_back(variant);
}


static IPCProperty* getOrCreateIPCProperty(const std::string& name, IPCPropertyMap::IPCVariantMap* propsMap)
{
    ExternalSegmentType::segment_manager* manager = propsMap->get_allocator().get_segment_manager();

    CharAllocator_ExternalSegment charAlloc(manager);
    String_ExternalSegment nameKey(charAlloc);
    nameKey.append(name.c_str());

    IPCPropertyMap::IPCVariantMap::iterator found = propsMap->find(nameKey);
    if (found != propsMap->end()) {
        return &found->second;
    }

    ExternalSegmentTypeIPCVariantAllocator vecAlloc(manager);
    IPCVariantVector vec(vecAlloc);

    // We must create the pair before or else this does not compile.
    IPCPropertyMap::IPCVariantMapValueType valPair = std::make_pair(nameKey, vec);
    std::pair<IPCPropertyMap::IPCVariantMap::iterator,bool> ret = propsMap->insert(valPair);
    assert(ret.second);
    return &ret.first->second;
} // getOrCreateIPCProperty

static const IPCProperty* getIPCPropertyVector(const std::string& name, const IPCPropertyMap::IPCVariantMap& propsMap)
{
    ExternalSegmentType::segment_manager* manager = propsMap.get_allocator().get_segment_manager();

    CharAllocator_ExternalSegment charAlloc(manager);
    String_ExternalSegment nameKey(charAlloc);
    nameKey.append(name.c_str());

    IPCPropertyMap::IPCVariantMap::const_iterator found = propsMap.find(nameKey);
    if (found != propsMap.end()) {
        return &found->second;
    }
    return (const IPCProperty*)NULL;

} // getOrCreateIPCProperty

IPCPropertyMap::IPCPropertyMap(const void_allocator& alloc)
: _properties(alloc)
{

}

IPCPropertyMap::~IPCPropertyMap()
{

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

void
IPCPropertyMap::setIPCStringPropertyN(const std::string& name,
                                  const std::vector<std::string>& values)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeString;
    for (std::size_t i = 0; i < values.size(); ++i) {
        appendIPCPropString(values[i], &prop->data);
    }
}

void
IPCPropertyMap::setIPCBoolPropertyN(const std::string& name,
                                const std::vector<bool>& values)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeBool;
    for (std::size_t i = 0; i < values.size(); ++i) {
        appendIPCPropScalar<bool>(values[i], &prop->data);
    }
}

void
IPCPropertyMap::setIPCIntPropertyN(const std::string& name,
                               const std::vector<int>& values)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeInt;
    for (std::size_t i = 0; i < values.size(); ++i) {
        appendIPCPropScalar<int>(values[i], &prop->data);
    }
}

void
IPCPropertyMap::setIPCDoublePropertyN(const std::string& name,
                                  const std::vector<double>& values)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeDouble;
    for (std::size_t i = 0; i < values.size(); ++i) {
        appendIPCPropScalar<double>(values[i], &prop->data);
    }
}

void
IPCPropertyMap::setIPCULongLongPropertyN(const std::string& name,
                              const std::vector<U64>& values)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeDouble;
    for (std::size_t i = 0; i < values.size(); ++i) {
        IPCVariant variant;
        variant.scalar = values[i];
        prop->data.push_back(variant);
    }

}

void
IPCPropertyMap::setIPCStringProperty(const std::string& name, const std::string& value)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeString;
    appendIPCPropString(value, &prop->data);
}


void
IPCPropertyMap::setIPCBoolProperty(const std::string& name, bool value)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeBool;
    appendIPCPropScalar<bool>(value, &prop->data);
}

void
IPCPropertyMap::setIPCIntProperty(const std::string& name, int value)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeInt;
    appendIPCPropScalar<int>(value, &prop->data);
}


void
IPCPropertyMap::setIPCDoubleProperty(const std::string& name, double value)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeDouble;
    appendIPCPropScalar<double>(value, &prop->data);
}

void
IPCPropertyMap::setIPCULongLongProperty(const std::string& name,
                             U64 value)
{
    IPCProperty* prop = getOrCreateIPCProperty(name, &_properties);
    prop->data.clear();
    prop->type = eIPCVariantTypeULongLong;
    IPCVariant variant;
    variant.scalar = value;
    prop->data.push_back(variant);
}


bool
IPCPropertyMap::getIPCStringPropertyN(const std::string& name,
                                      std::vector<std::string>* values) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeString) {
        return false;
    }
    values->resize(prop->data.size());
    for (std::size_t i = 0; i < prop->data.size(); ++i) {
        (*values)[i] = std::string(prop->data[i].string->c_str());
    }
    return true;
}


bool
IPCPropertyMap::getIPCBoolPropertyN(const std::string& name,
                                      std::vector<bool>* values) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeBool) {
        return false;
    }
    values->resize(prop->data.size());
    for (std::size_t i = 0; i < prop->data.size(); ++i) {
        (*values)[i] = Hash64::fromU64<bool>(prop->data[i].scalar);
    }
    return true;
}

bool
IPCPropertyMap::getIPCIntPropertyN(const std::string& name,
                                    std::vector<int>* values) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeInt) {
        return false;
    }
    values->resize(prop->data.size());
    for (std::size_t i = 0; i < prop->data.size(); ++i) {
        (*values)[i] = Hash64::fromU64<int>(prop->data[i].scalar);
    }
    return true;
}

bool
IPCPropertyMap::getIPCDoublePropertyN(const std::string& name,
                                    std::vector<double>* values) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeDouble) {
        return false;
    }
    values->resize(prop->data.size());
    for (std::size_t i = 0; i < prop->data.size(); ++i) {
        (*values)[i] = Hash64::fromU64<double>(prop->data[i].scalar);
    }
    return true;
}

bool
IPCPropertyMap::getIPCULongLongPropertyN(const std::string& name,
                              std::vector<U64>* values) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeULongLong) {
        return false;
    }
    values->resize(prop->data.size());
    for (std::size_t i = 0; i < prop->data.size(); ++i) {
        (*values)[i] = prop->data[i].scalar;
    }
    return true;
}

bool
IPCPropertyMap::getIPCStringProperty(const std::string& name,
                          int index,
                          std::string* value) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeString) {
        return false;
    }
    if (index < 0 || index >= (int)prop->data.size()) {
        return false;
    }
    *value = std::string(prop->data[index].string->c_str());
    return true;
}

bool
IPCPropertyMap::getIPCBoolProperty(const std::string& name,
                                    int index,
                                    bool* value) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeBool) {
        return false;
    }
    if (index < 0 || index >= (int)prop->data.size()) {
        return false;
    }
    *value = Hash64::fromU64<bool>(prop->data[index].scalar);
    return true;
}

bool
IPCPropertyMap::getIPCIntProperty(const std::string& name,
                                   int index,
                                   int* value) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeInt) {
        return false;
    }
    if (index < 0 || index >= (int)prop->data.size()) {
        return false;
    }
    *value = Hash64::fromU64<int>(prop->data[index].scalar);
    return true;
}

bool
IPCPropertyMap::getIPCDoubleProperty(const std::string& name,
                                   int index,
                                   double* value) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeDouble) {
        return false;
    }
    if (index < 0 || index >= (int)prop->data.size()) {
        return false;
    }
    *value = Hash64::fromU64<double>(prop->data[index].scalar);
    return true;
}

bool
IPCPropertyMap::getIPCULongLongProperty(const std::string& name,
                             int index,
                             U64* value) const
{
    const IPCProperty* prop = getIPCPropertyVector(name, _properties);
    if (!prop) {
        return false;
    }
    if (prop->type != eIPCVariantTypeULongLong) {
        return false;
    }
    if (index < 0 || index >= (int)prop->data.size()) {
        return false;
    }
    *value = prop->data[index].scalar;
    return true;
}

NATRON_NAMESPACE_EXIT;
