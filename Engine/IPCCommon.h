/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef NATRON_ENGINE_IPCCOMMON_H
#define NATRON_ENGINE_IPCCOMMON_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/weak_ptr.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/offset_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


typedef boost::interprocess::managed_external_buffer ExternalSegmentType;
typedef boost::interprocess::managed_shared_memory SharedMemorySegmentType;


typedef boost::interprocess::allocator<void, ExternalSegmentType::segment_manager> external_void_allocator;
typedef boost::interprocess::allocator<char, ExternalSegmentType::segment_manager> CharAllocator_ExternalSegment;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator_ExternalSegment> String_ExternalSegment;

typedef boost::interprocess::allocator<void, SharedMemorySegmentType::segment_manager> shm_void_allocator;
typedef boost::interprocess::allocator<char, SharedMemorySegmentType::segment_manager> CharAllocator_shm;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator_shm> String_Shm;

/**
 * @brief Function to serialize to the given memory segment the given object.
 * The object must not be a container (so it should be a pod or a struct encapsulating only pod's).
 * The only supported container is std::string
 **/
template <class T>
inline ExternalSegmentType::handle_t writeNamedSharedObject(const T& object, const std::string& objectName,  ExternalSegmentType* segment)
{
    T* sharedMemObject = segment->construct<T>(objectName.c_str())();
    if (sharedMemObject) {
        *sharedMemObject = object;
    } else {
        throw boost::interprocess::bad_alloc();
    }
    return segment->get_handle_from_address(sharedMemObject);
}

/**
 * @brief Function to serialize to the given memory segment the given object.
 * The object will be anonymous and its handle in the memory segment will be returned.
 * The object must not be a container (so it should be a pod or a struct encapsulating only pod's).
 * The only supported container is std::string
 **/
template <class T>
inline ExternalSegmentType::handle_t writeAnonymousSharedObject(const T& object,  ExternalSegmentType* segment)
{
    T* sharedMemObject = segment->construct<T>(boost::interprocess::anonymous_instance)();
    if (sharedMemObject) {
        *sharedMemObject = object;
    } else {
        throw boost::interprocess::bad_alloc();
    }
    return segment->get_handle_from_address(sharedMemObject);
}


/**
 * @brief Template specialization for std::string
 **/
template <>
inline ExternalSegmentType::handle_t writeNamedSharedObject(const std::string& object, const std::string& objectName,  ExternalSegmentType* segment)
{

    // Allocate a char array containing the string
    CharAllocator_ExternalSegment allocator(segment->get_segment_manager());
    String_ExternalSegment* sharedMemObject = segment->construct<String_ExternalSegment>(objectName.c_str())(allocator);
    if (sharedMemObject) {
        sharedMemObject->append(object.c_str());
    } else {
        throw boost::interprocess::bad_alloc();
    }
    return segment->get_handle_from_address(sharedMemObject);
}

template <>
inline ExternalSegmentType::handle_t writeAnonymousSharedObject(const std::string& object,  ExternalSegmentType* segment)
{

    // Allocate a char array containing the string
    CharAllocator_ExternalSegment allocator(segment->get_segment_manager());
    String_ExternalSegment* sharedMemObject = segment->construct<String_ExternalSegment>(boost::interprocess::anonymous_instance)(allocator);
    if (sharedMemObject) {
        sharedMemObject->append(object.c_str());
    } else {
        throw boost::interprocess::bad_alloc();
    }
    return segment->get_handle_from_address(sharedMemObject);
}

/**
 * @brief Same as writeNamedSharedObject but for arrays
 **/
template <class T>
inline ExternalSegmentType::handle_t writeNamedSharedObjectN(T *array, int count, const std::string& objectName,  ExternalSegmentType* segment)
{
    T* sharedMemObject = segment->construct<T>(objectName.c_str())[count]();
    if (!sharedMemObject) {
        throw boost::interprocess::bad_alloc();
    }
    memcpy(sharedMemObject, array, count * sizeof(T));
    return segment->get_handle_from_address(sharedMemObject);
}

template <class T>
inline ExternalSegmentType::handle_t writeAnonymousSharedObjectN(T *array, int count,  ExternalSegmentType* segment)
{
    T* sharedMemObject = segment->construct<T>(boost::interprocess::anonymous_instance)[count]();
    if (!sharedMemObject) {
        throw boost::interprocess::bad_alloc();
    }
    memcpy(sharedMemObject, array, count * sizeof(T));
    return segment->get_handle_from_address(sharedMemObject);
}

/**
 * @brief Template specialization for std::string
 **/
template <>
inline ExternalSegmentType::handle_t writeNamedSharedObjectN(std::string *array, int count, const std::string& objectName,  ExternalSegmentType* segment)
{
    // Allocate a char array containing the string
    CharAllocator_ExternalSegment allocator(segment->get_segment_manager());

    String_ExternalSegment* sharedMemObject = segment->construct<String_ExternalSegment>(objectName.c_str())[count](allocator);
    if (!sharedMemObject) {
        throw boost::interprocess::bad_alloc();
    }
    for (int i = 0; i < count; ++i) {
        sharedMemObject[i].append(array[i].c_str());
    }
    return segment->get_handle_from_address(sharedMemObject);
}

template <>
inline ExternalSegmentType::handle_t writeAnonymousSharedObjectN(std::string *array, int count,  ExternalSegmentType* segment)
{
    // Allocate a char array containing the string
    CharAllocator_ExternalSegment allocator(segment->get_segment_manager());

    String_ExternalSegment* sharedMemObject = segment->construct<String_ExternalSegment>(boost::interprocess::anonymous_instance)[count](allocator);
    if (!sharedMemObject) {
        throw boost::interprocess::bad_alloc();
    }
    for (int i = 0; i < count; ++i) {
        sharedMemObject[i].append(array[i].c_str());
    }
    return segment->get_handle_from_address(sharedMemObject);
}

/**
 * @brief Function to read from given memory segment the given object.
 * The object must not be a container (so it should be a pod or a struct encapsulating only pod's).
 * The only supported container is std::string
 **/
template <class T>
inline void readNamedSharedObject(const std::string& objectName, ExternalSegmentType* segment, T* object)
{
    std::pair<T*, ExternalSegmentType::size_type> found = segment->find<T>(objectName.c_str());
    if (found.first) {
        *object = *found.first;
    } else {
        throw boost::interprocess::bad_alloc();
    }
}

template <class T>
inline void readAnonymousSharedObject(ExternalSegmentType::handle_t handle, ExternalSegmentType* segment, T* object)
{
    *object = *(T*)segment->get_address_from_handle(handle);
}


/**
 * @brief Template specialization for std::string
 **/
template <>
inline void readNamedSharedObject(const std::string& objectName, ExternalSegmentType* segment, std::string* object)
{
    std::pair<String_ExternalSegment*, ExternalSegmentType::size_type> found = segment->find<String_ExternalSegment>(objectName.c_str());
    if (found.first) {
        *object = std::string(found.first->c_str());
    } else {
        throw boost::interprocess::bad_alloc();
    }
}

template <>
inline void readAnonymousSharedObject(ExternalSegmentType::handle_t handle, ExternalSegmentType* segment, std::string* object)
{
    String_ExternalSegment* found = (String_ExternalSegment*)segment->get_address_from_handle(handle);
    *object = std::string(found->c_str());
}

/**
 * @brief All types serialized to the cache are flattened to a IPCVariant
 **/
enum IPCVariantTypeEnum
{
    eIPCVariantTypeBool,
    eIPCVariantTypeInt,
    eIPCVariantTypeDouble,
    eIPCVariantTypeULong,
    eIPCVariantTypeULongLong,
    eIPCVariantTypeString
};

struct IPCVariant
{
    U64 scalar;
    boost::interprocess::offset_ptr<String_ExternalSegment> string;

    IPCVariant();

    IPCVariant(const IPCVariant& other);

    void operator=(const IPCVariant& other);

    ~IPCVariant();

};

typedef boost::interprocess::allocator<IPCVariant, ExternalSegmentType::segment_manager> ExternalSegmentTypeIPCVariantAllocator;
typedef boost::interprocess::vector<IPCVariant, ExternalSegmentTypeIPCVariantAllocator> IPCVariantVector;


class IPCProperty
{

    IPCVariantTypeEnum type;
    IPCVariantVector data;

public:

    IPCProperty(IPCVariantTypeEnum type, const external_void_allocator& alloc)
    : type(type)
    , data(alloc)
    {

    }

    IPCProperty(const IPCProperty& other)
    : type(other.type)
    , data(other.data)
    {

    }

    void operator=(const IPCProperty& other)
    {
        type = other.type;
        data = other.data;
    }

    IPCVariantVector* getData()
    {
        return &data;
    }

    const IPCVariantVector& getData() const
    {
        return data;
    }

    IPCVariantTypeEnum getType() const
    {
        return type;
    }

    std::size_t getNumDimensions() const;

    void clear();

    void resize(std::size_t nDims);

    static void getStringValue(const IPCVariantVector& vec, int index, std::string* value);
    static void setStringValue(int index, const std::string& value, IPCVariantVector* vec);

    static void getBoolValue(const IPCVariantVector& vec, int index, bool* value);
    static void setBoolValue(int index, const bool& value, IPCVariantVector* vec);

    static void getIntValue(const IPCVariantVector& vec, int index, int* value);
    static void setIntValue(int index, const int& value, IPCVariantVector* vec);

    static void getULongValue(const IPCVariantVector& vec, int index, U32* value);
    static void setULongValue(int index, const U32& value, IPCVariantVector* vec);


    static void getULongLongValue(const IPCVariantVector& vec, int index, U64* value);
    static void setULongLongValue(int index, const U64& value, IPCVariantVector* vec);

    static void getDoubleValue(const IPCVariantVector& vec, int index, double* value);
    static void setDoubleValue(int index, const double& value, IPCVariantVector* vec);

    template <typename T>
    struct TypeFunctions
    {
        typedef void (*PFN_GET)(const IPCVariantVector& vec, int index, T* value);
        typedef void (*PFN_SET)(int index, const T& value, IPCVariantVector* vec);

        PFN_GET getter;
        PFN_SET setter;
    };

    template <typename T>
    static void getTypeInfos(TypeFunctions<T>* functions, IPCVariantTypeEnum* type);

};


class IPCPropertyMap
{
public:

    typedef std::pair<const String_ExternalSegment, IPCProperty > IPCVariantMapValueType;
    typedef boost::interprocess::allocator<IPCVariantMapValueType, ExternalSegmentType::segment_manager> IPCVariantMapValueType_Allocator_ExternalSegment;
    typedef boost::interprocess::map<String_ExternalSegment, IPCProperty, std::less<String_ExternalSegment>, IPCVariantMapValueType_Allocator_ExternalSegment> IPCVariantMap;

private:

    IPCVariantMap _properties;

public:

    typedef IPCVariantMap::iterator iterator;
    typedef IPCVariantMap::const_iterator const_iterator;

    IPCPropertyMap(const external_void_allocator& alloc);

    ~IPCPropertyMap();

    const_iterator begin() const;

    const_iterator end() const;

    ExternalSegmentType::segment_manager* getSegmentManager() const;

    void operator=(const IPCPropertyMap& other);

    void clear();

    // May throw an exception
    IPCProperty* getOrCreateIPCProperty(const std::string& name, IPCVariantTypeEnum type);

    // May return NULL but does not throw
    const IPCProperty* getIPCProperty(const std::string& name) const;

    template <typename T>
    void setIPCPropertyN(const std::string& name, const std::vector<T>& values)
    {

        IPCProperty::TypeFunctions<T> functions;
        IPCVariantTypeEnum type;
        IPCProperty::getTypeInfos<T>(&functions, &type);

        IPCProperty* prop = getOrCreateIPCProperty(name, type);
        prop->resize(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            functions.setter(i, values[i], prop->getData());
        }
    }

    template <typename T>
    void setIPCProperty(const std::string& name,  const T& value)
    {

        IPCProperty::TypeFunctions<T> functions;
        IPCVariantTypeEnum type;
        IPCProperty::getTypeInfos<T>(&functions, &type);

        IPCProperty* prop = getOrCreateIPCProperty(name, type);
        prop->resize(1);
        functions.setter(0, value, prop->getData());
    }

    template <typename T>
    static bool getIPCPropertyN(const IPCProperty& prop, std::vector<T>* values)
    {

        IPCProperty::TypeFunctions<T> functions;
        IPCVariantTypeEnum type;
        IPCProperty::getTypeInfos<T>(&functions, &type);
        if (prop.getType() != type) {
            return false;
        }
        values->resize(prop.getNumDimensions());
        for (std::size_t i = 0; i < values->size(); ++i) {
            functions.getter(prop.getData(), i, &(*values)[i]);
        }
        return true;
    }

    template <typename T>
    bool getIPCPropertyN(const std::string& name, std::vector<T>* values) const
    {
        const IPCProperty* prop = getIPCProperty(name);
        if (!prop) {
            return false;
        }
        return getIPCPropertyN(*prop, values);
    }

    template <typename T>
    static bool getIPCProperty(const IPCProperty& prop, int index, T* value)
    {

        IPCProperty::TypeFunctions<T> functions;
        IPCVariantTypeEnum type;
        IPCProperty::getTypeInfos<T>(&functions, &type);

        if (prop.getType() != type || (int)prop.getNumDimensions() <= index) {
            return false;
        }
        functions.getter(prop.getData(), index, value);
        return true;
    }


    template <typename T>
    bool getIPCProperty(const std::string& name, int index, T* value) const
    {

        IPCProperty::TypeFunctions<T> functions;
        IPCVariantTypeEnum type;
        IPCProperty::getTypeInfos<T>(&functions, &type);

        const IPCProperty* prop = getIPCProperty(name);
        if (!prop) {
            return false;
        }
        return getIPCProperty(*prop, index, value);
    }


};




NATRON_NAMESPACE_EXIT


#endif // NATRON_ENGINE_IPCCOMMON_H
