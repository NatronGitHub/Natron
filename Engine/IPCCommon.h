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

#include "Engine/EngineFwd.h"

#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/weak_ptr.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#endif

NATRON_NAMESPACE_ENTER;


// We share the cache across processed with a memory mapped file so that it is also persistent
typedef boost::interprocess::managed_mapped_file MemorySegmentType;

typedef boost::interprocess::managed_external_buffer ExternalSegmentType;

typedef boost::interprocess::allocator<void, MemorySegmentType::segment_manager>  MM_allocator_void;



// A pointer (in process memory) of a memory mapped file.
typedef boost::shared_ptr<boost::interprocess::file_mapping> FileMappingPtr;

/**
 * @brief Function to serialize to the given memory segment the given object.
 * The object must not be a container (so it should be a pod or a struct encapsulating only pod's).
 * The only supported container is std::string
 **/
template <class T>
inline void writeMMObject(const T& object, const std::string& objectName,  ExternalSegmentType* segment)
{
    T* sharedMemObject = segment->construct<T>(objectName.c_str())();
    if (sharedMemObject) {
        *sharedMemObject = object;
    } else {
        throw std::bad_alloc();
    }
}

/**
 * @brief Template specialization for std::string
 **/
template <>
inline void writeMMObject(const std::string& object, const std::string& objectName,  ExternalSegmentType* segment)
{
    std::size_t strSizeWithoutNullCharacter = object.size();

    // Allocate a char array containing the string
    char* sharedMemObject = segment->construct<char>(objectName.c_str())[strSizeWithoutNullCharacter + 1]();
    if (sharedMemObject) {
        strncpy(sharedMemObject, object.c_str(), strSizeWithoutNullCharacter);
        sharedMemObject[strSizeWithoutNullCharacter] = 1;
    } else {
        throw std::bad_alloc();
    }
}


/**
 * @brief Function to read from given memory segment the given object.
 * The object must not be a container (so it should be a pod or a struct encapsulating only pod's).
 * The only supported container is std::string
 **/
template <class T>
inline void readMMObject(const std::string& objectName, const ExternalSegmentType& segment, T* object)
{
    std::pair<T*, MemorySegmentType::size_type> found = segment.find<T>(objectName.c_str());
    if (found.first) {
        *object = *found.first;
    } else {
        throw std::bad_alloc();
    }
}


/**
 * @brief Template specialization for std::string
 **/
template <>
inline void readMMObject(const std::string& objectName, const ExternalSegmentType& segment, std::string* object)
{
    std::pair<char*, MemorySegmentType::size_type> found = segment.find<char>(objectName.c_str());
    if (found.first) {
        *object = std::string(found.first);
    } else {
        throw std::bad_alloc();
    }
}

NATRON_NAMESPACE_EXIT;


#endif // NATRON_ENGINE_IPCCOMMON_H
