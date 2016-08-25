/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_CacheSerialization_h
#define Engine_CacheSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN) && defined(NATRON_BOOST_SERIALIZATION_COMPAT)
#include <boost/shared_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/split_member.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Serialization/ImageSerialization.h"
#include "Serialization/ImageParamsSerialization.h"
#include "Serialization/FrameEntrySerialization.h"
#include "Serialization/FrameParamsSerialization.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define SERIALIZED_ENTRY_INTRODUCES_SIZE 2
#define SERIALIZED_ENTRY_VERSION SERIALIZED_ENTRY_INTRODUCES_SIZE
#endif


///When defined, number of opened files, memory size and disk size of the cache are printed whenever there's activity.
//#define NATRON_DEBUG_CACHE

NATRON_NAMESPACE_ENTER;


template<typename EntryType>
class SerializedEntry
{
public:

    typedef typename EntryType::hash_type hash_type;
    typedef typename EntryType::key_t key_t;
    typedef typename EntryType::param_t param_t;
    typedef boost::shared_ptr<param_t> ParamsTypePtr;


    hash_type hash;
    key_t key;
    ParamsTypePtr params;
    std::size_t size; //< the data size in bytes
    std::string filePath; //< we need to serialize it as several entries can have the same hash, hence we index them
    std::size_t dataOffsetInFile;

    SerializedEntry()
        : hash(0)
          , key()
          , params()
          , size(0)
          , filePath()
          , dataOffsetInFile(0)
    {
    }

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("Hash", hash);
        ar & ::boost::serialization::make_nvp("Key", key);
        ar & ::boost::serialization::make_nvp("Params", params);
        ar & ::boost::serialization::make_nvp("Size", size);
        ar & ::boost::serialization::make_nvp("Filename", filePath);
        ar & ::boost::serialization::make_nvp("Offset", dataOffsetInFile);
    }
#endif
};


#pragma message WARN("Missing CacheSerialization class")
NATRON_NAMESPACE_EXIT;


#endif // Engine_CacheSerialization_h
