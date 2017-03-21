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

NATRON_NAMESPACE_ENTER;


typedef boost::interprocess::managed_external_buffer ExternalSegmentType;
typedef boost::interprocess::allocator<void, ExternalSegmentType::segment_manager> void_allocator;
typedef boost::interprocess::allocator<char, ExternalSegmentType::segment_manager> CharAllocator_ExternalSegment;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator_ExternalSegment> String_ExternalSegment;


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
public:

    IPCVariantTypeEnum type;
    IPCVariantVector data;

    IPCProperty(const void_allocator& alloc)
    : type(eIPCVariantTypeBool)
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

    IPCPropertyMap(const void_allocator& alloc);

    ~IPCPropertyMap();

    ExternalSegmentType::segment_manager* getSegmentManager() const;

    void operator=(const IPCPropertyMap& other);

    void clear();

    void setIPCStringPropertyN(const std::string& name,
                               const std::vector<std::string>& values);

    void setIPCBoolPropertyN(const std::string& name,
                             const std::vector<bool>& values);

    void setIPCIntPropertyN(const std::string& name,
                           const std::vector<int>& values);

    void setIPCDoublePropertyN(const std::string& name,
                               const std::vector<double>& values);

    void setIPCStringProperty(const std::string& name,
                               const std::string& values);

    void setIPCULongLongPropertyN(const std::string& name,
                                  const std::vector<U64>& values);

    void setIPCBoolProperty(const std::string& name,
                            bool values);

    void setIPCIntProperty(const std::string& name,
                            int values);

    void setIPCDoubleProperty(const std::string& name,
                            double values);

    void setIPCULongLongProperty(const std::string& name,
                                  U64 value);


    bool getIPCStringPropertyN(const std::string& name,
                               std::vector<std::string>* values) const;

    bool getIPCBoolPropertyN(const std::string& name,
                               std::vector<bool>* values) const;

    bool getIPCIntPropertyN(const std::string& name,
                               std::vector<int>* values) const;

    bool getIPCDoublePropertyN(const std::string& name,
                               std::vector<double>* values) const;

    bool getIPCULongLongPropertyN(const std::string& name,
                               std::vector<U64>* values) const;

    bool getIPCStringProperty(const std::string& name,
                               int index,
                               std::string* value) const;

    bool getIPCBoolProperty(const std::string& name,
                             int index,
                             bool* value) const;

    bool getIPCIntProperty(const std::string& name,
                            int index,
                            int* value) const;

    bool getIPCDoubleProperty(const std::string& name,
                               int index,
                               double* value) const;

    bool getIPCULongLongProperty(const std::string& name,
                              int index,
                              U64* value) const;

};




NATRON_NAMESPACE_EXIT;


#endif // NATRON_ENGINE_IPCCOMMON_H
