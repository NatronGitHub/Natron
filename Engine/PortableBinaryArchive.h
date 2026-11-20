/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// This file is derived from Boost.Serialization's example
// portable_binary_iarchive.hpp and portable_binary_oarchive.hpp:
//   (C) Copyright 2002-7 Robert Ramey - http://www.rrsd.com .
//   Distributed under the Boost Software License, Version 1.0.
//   (See http://www.boost.org/LICENSE_1_0.txt)
// The Boost Software License is compatible with the GPL.
//
// Changes from the Boost example, so that a file written on any platform reads
// on any other one (Windows/macOS/Linux, x86-64/arm64) and in later versions:
// - integers are always stored little-endian, as a signed byte count followed by
//   the magnitude, whatever their C++ type (long is 32-bit on Windows, 64-bit elsewhere).
//   The count is a signed char: the example used char, which is unsigned on arm64 Linux.
// - float and double are stored as their IEEE 754 bits, little-endian (the example
//   wrote native bytes).
// - std::wstring is not supported (wchar_t is 16-bit on Windows, 32-bit elsewhere).
// - the library version of the file is passed to Boost (set_library_version() was
//   only called with Metrowerks in the example), and, like xml_iarchive and unlike
//   binary_iarchive, a file written by a more recent Boost is accepted.

#ifndef NATRON_ENGINE_PORTABLEBINARYARCHIVE_H
#define NATRON_ENGINE_PORTABLEBINARYARCHIVE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <climits>
#include <istream>
#include <ostream>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format off
GCC_DIAG_OFF(unused-parameter)
#include <boost/cstdint.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/archive/basic_binary_iprimitive.hpp>
#include <boost/archive/basic_binary_oprimitive.hpp>
#include <boost/archive/detail/common_iarchive.hpp>
#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/archive/detail/register_archive.hpp>
GCC_DIAG_ON(unused-parameter)
// clang-format on
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#if CHAR_BIT != 8
#error This code assumes an eight-bit byte.
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

// The first bytes of a portable binary archive: the length of the Boost archive
// signature (one byte count, then 22), followed by the signature itself.
#define NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC "\x01\x16serialization::archive"
#define NATRON_PORTABLE_BINARY_ARCHIVE_MAGIC_SIZE 24

class PortableBinaryArchiveException
    : public boost::archive::archive_exception
{
public:
    enum ExceptionCodeEnum
    {
        eExceptionCodeIntegerTooLarge,
        eExceptionCodeUnsupportedEncoding,
    };

    explicit PortableBinaryArchiveException(ExceptionCodeEnum c)
        : boost::archive::archive_exception(boost::archive::archive_exception::other_exception)
        , _code(c)
    {
    }

    virtual const char * what() const throw() OVERRIDE
    {
        switch (_code) {
        case eExceptionCodeIntegerTooLarge:

            return "integer cannot be represented";
        case eExceptionCodeUnsupportedEncoding:

            return "unsupported binary encoding";
        }

        return "unknown portable binary archive error";
    }

private:
    ExceptionCodeEnum _code;
};

class PortableBinaryIArchive
    : public boost::archive::basic_binary_iprimitive<PortableBinaryIArchive, std::istream::char_type, std::istream::traits_type>
    , public boost::archive::detail::common_iarchive<PortableBinaryIArchive>
{
    typedef boost::archive::basic_binary_iprimitive<PortableBinaryIArchive, std::istream::char_type, std::istream::traits_type> primitive_base_t;
    typedef boost::archive::detail::common_iarchive<PortableBinaryIArchive> archive_base_t;

public:
    // These are public, as in the Boost example: the Boost archive machinery calls them.

    void loadInteger(boost::intmax_t & l, int maxsize);
    void loadIEEE754(boost::uint64_t* bits, int nbytes);

    // default for all integer types (and bool)
    template<class T>
    void load(T & t)
    {
        boost::intmax_t l;

        loadInteger(l, sizeof(T));
        t = T(l);
    }

    void load(boost::serialization::item_version_type & t)
    {
        boost::intmax_t l;

        loadInteger(l, sizeof(boost::serialization::item_version_type));
        t = boost::serialization::item_version_type(l);
    }

    void load(boost::archive::version_type & t)
    {
        boost::intmax_t l;

        loadInteger(l, sizeof(boost::archive::version_type));
        t = boost::archive::version_type(l);
    }

    void load(boost::archive::class_id_type & t)
    {
        boost::intmax_t l;

        loadInteger(l, sizeof(boost::archive::class_id_type));
        t = boost::archive::class_id_type( static_cast<int>(l) );
    }

    void load(std::string & t)
    {
        this->primitive_base_t::load(t);
    }

    void load(float & t);
    void load(double & t);

    void load(char & t)
    {
        this->primitive_base_t::load(t);
    }

    void load(signed char & t)
    {
        this->primitive_base_t::load(t);
    }

    void load(unsigned char & t)
    {
        this->primitive_base_t::load(t);
    }

    template<class T>
    void load_override(T & t)
    {
        this->archive_base_t::load_override(t);
    }

    void load_override(boost::archive::class_name_type & t);

    // binary files don't include the optional information
    void load_override(boost::archive::class_id_optional_type &)
    {
    }

    explicit PortableBinaryIArchive(std::istream & is,
                                    unsigned int flags = 0)
        : primitive_base_t(*is.rdbuf(), true /*no_codecvt*/)
        , archive_base_t(flags)
    {
        init(flags);
    }

private:
    void init(unsigned int flags);
};

class PortableBinaryOArchive
    : public boost::archive::basic_binary_oprimitive<PortableBinaryOArchive, std::ostream::char_type, std::ostream::traits_type>
    , public boost::archive::detail::common_oarchive<PortableBinaryOArchive>
{
    typedef boost::archive::basic_binary_oprimitive<PortableBinaryOArchive, std::ostream::char_type, std::ostream::traits_type> primitive_base_t;
    typedef boost::archive::detail::common_oarchive<PortableBinaryOArchive> archive_base_t;

public:
    void saveInteger(boost::intmax_t l);
    void saveIEEE754(boost::uint64_t bits, int nbytes);

    // default for all integer types (and bool)
    template<class T>
    void save(const T & t)
    {
        saveInteger( static_cast<boost::intmax_t>(t) );
    }

    void save(const std::string & t)
    {
        this->primitive_base_t::save(t);
    }

    void save(const float & t);
    void save(const double & t);

    void save(const char & t)
    {
        this->primitive_base_t::save(t);
    }

    void save(const signed char & t)
    {
        this->primitive_base_t::save(t);
    }

    void save(const unsigned char & t)
    {
        this->primitive_base_t::save(t);
    }

    template<class T>
    void save_override(T & t)
    {
        this->archive_base_t::save_override(t);
    }

    // explicitly convert to char * to avoid compile ambiguities
    void save_override(const boost::archive::class_name_type & t)
    {
        const std::string s(t);

        *this << s;
    }

    // binary files don't include the optional information
    void save_override(const boost::archive::class_id_optional_type & /* t */)
    {
    }

    explicit PortableBinaryOArchive(std::ostream & os,
                                    unsigned int flags = 0)
        : primitive_base_t(*os.rdbuf(), true /*no_codecvt*/)
        , archive_base_t(flags)
    {
        init(flags);
    }

private:
    void init(unsigned int flags);
};

NATRON_NAMESPACE_EXIT

// required by BOOST_CLASS_EXPORT
BOOST_SERIALIZATION_REGISTER_ARCHIVE(NATRON_NAMESPACE::PortableBinaryIArchive)
BOOST_SERIALIZATION_REGISTER_ARCHIVE(NATRON_NAMESPACE::PortableBinaryOArchive)

#endif // NATRON_ENGINE_PORTABLEBINARYARCHIVE_H
