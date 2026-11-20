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

// Derived from Boost.Serialization's example portable_binary_iarchive.cpp and
// portable_binary_oarchive.cpp:
//   (C) Copyright 2002-7 Robert Ramey - http://www.rrsd.com .
//   Distributed under the Boost Software License, Version 1.0.
//   (See http://www.boost.org/LICENSE_1_0.txt)
// See PortableBinaryArchive.h for the changes.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PortableBinaryArchive.h"

#include <cstring>
#include <limits>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format off
GCC_DIAG_OFF(unused-parameter)
#include <boost/serialization/throw_exception.hpp>
#include <boost/archive/impl/archive_serializer_map.ipp>
#include <boost/archive/impl/basic_binary_iprimitive.ipp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>
GCC_DIAG_ON(unused-parameter)
// clang-format on
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

static_assert(std::numeric_limits<float>::is_iec559 && sizeof(float) == 4, "float must be IEEE 754 single precision");
static_assert(std::numeric_limits<double>::is_iec559 && sizeof(double) == 8, "double must be IEEE 754 double precision");
static_assert(sizeof(boost::intmax_t) == 8, "intmax_t must be 64-bit");

// The only encoding written: little-endian (the endian_little flag of the Boost example).
#define kPortableBinaryFlagLittleEndian 0x80

NATRON_NAMESPACE_ENTER

///////////////////////////////////////////////////////////////////////////////
// PortableBinaryIArchive

void
PortableBinaryIArchive::loadInteger(boost::intmax_t & l,
                                    int maxsize)
{
    signed char size;

    l = 0;
    this->primitive_base_t::load(size);
    if (size == 0) {
        return;
    }
    const bool negative = (size < 0);
    const int n = negative ? -int(size) : int(size);
    if ( (n > maxsize) || ( n > (int)sizeof(boost::intmax_t) ) ) {
        boost::serialization::throw_exception( PortableBinaryArchiveException(PortableBinaryArchiveException::eExceptionCodeIntegerTooLarge) );
    }
    unsigned char bytes[sizeof(boost::intmax_t)];
    this->primitive_base_t::load_binary(bytes, n);
    boost::uintmax_t magnitude = 0;
    for (int i = 0; i < n; ++i) {
        magnitude |= boost::uintmax_t(bytes[i]) << (CHAR_BIT * i);
    }
    // modular conversion: a 64-bit unsigned value larger than INTMAX_MAX was written as negative
    l = static_cast<boost::intmax_t>(negative ? (0 - magnitude) : magnitude);
}

void
PortableBinaryIArchive::loadIEEE754(boost::uint64_t* bits,
                                    int nbytes)
{
    unsigned char bytes[8];

    this->primitive_base_t::load_binary(bytes, nbytes);
    *bits = 0;
    for (int i = 0; i < nbytes; ++i) {
        *bits |= boost::uint64_t(bytes[i]) << (CHAR_BIT * i);
    }
}

// Sizes up to this are not checked: allocating them is harmless, and checking every size would seek in the stream.
#define kPortableBinaryUncheckedSize 65536

void
PortableBinaryIArchive::checkSize(boost::uintmax_t size)
{
    if (size <= kPortableBinaryUncheckedSize) {
        return;
    }
    std::streambuf& sb = this->m_sb;
    const std::streampos cur = sb.pubseekoff(0, std::ios_base::cur, std::ios_base::in);
    const std::streampos end = sb.pubseekoff(0, std::ios_base::end, std::ios_base::in);
    if ( (cur == std::streampos(-1)) || (end == std::streampos(-1)) ) {
        // not seekable: can't check
        return;
    }
    sb.pubseekpos(cur, std::ios_base::in);
    if ( size > boost::uintmax_t(end - cur) ) {
        boost::serialization::throw_exception( PortableBinaryArchiveException(PortableBinaryArchiveException::eExceptionCodeSizeTooLarge) );
    }
}

void
PortableBinaryIArchive::load(std::string & t)
{
    std::size_t l;

    load(l);
    checkSize(l);
    t.resize(l);
    if (l > 0) {
        this->primitive_base_t::load_binary(&t[0], l);
    }
}

void
PortableBinaryIArchive::load(boost::serialization::collection_size_type & t)
{
    std::size_t l;

    load(l);
    // each element takes at least one byte
    checkSize(l);
    t = boost::serialization::collection_size_type(l);
}

void
PortableBinaryIArchive::load(float & t)
{
    boost::uint64_t bits;

    loadIEEE754(&bits, 4);
    boost::uint32_t bits32 = static_cast<boost::uint32_t>(bits);
    std::memcpy( &t, &bits32, sizeof(t) );
}

void
PortableBinaryIArchive::load(double & t)
{
    boost::uint64_t bits;

    loadIEEE754(&bits, 8);
    std::memcpy( &t, &bits, sizeof(t) );
}

void
PortableBinaryIArchive::load_override(boost::archive::class_name_type & t)
{
    std::string cn;

    cn.reserve(BOOST_SERIALIZATION_MAX_KEY_SIZE);
    load_override(cn);
    if ( cn.size() > (BOOST_SERIALIZATION_MAX_KEY_SIZE - 1) ) {
        boost::serialization::throw_exception( boost::archive::archive_exception(boost::archive::archive_exception::invalid_class_name) );
    }
    std::memcpy( t, cn.data(), cn.size() );
    t.t[cn.size()] = '\0';
}

void
PortableBinaryIArchive::init(unsigned int flags)
{
    if ( 0 == (flags & boost::archive::no_header) ) {
        // read signature in an archive version independent manner
        std::string file_signature;
        *this >> file_signature;
        if ( file_signature != boost::archive::BOOST_ARCHIVE_SIGNATURE() ) {
            boost::serialization::throw_exception( boost::archive::archive_exception(boost::archive::archive_exception::invalid_signature) );
        }
        // The version of the Boost library that wrote the file. Unlike binary_iarchive, and
        // like xml_iarchive, accept a more recent version: the same Natron version is built
        // with different Boost versions on different platforms, and the changes between Boost
        // archive versions 19 (Boost 1.76) and 21 only changed the size of some integers,
        // which this archive doesn't store.
        boost::archive::library_version_type input_library_version;
        *this >> input_library_version;
        this->set_library_version(input_library_version);
    }
    unsigned char x;
    load(x);
    if (x != kPortableBinaryFlagLittleEndian) {
        boost::serialization::throw_exception( PortableBinaryArchiveException(PortableBinaryArchiveException::eExceptionCodeUnsupportedEncoding) );
    }
}

///////////////////////////////////////////////////////////////////////////////
// PortableBinaryOArchive

void
PortableBinaryOArchive::saveInteger(boost::intmax_t l)
{
    if (l == 0) {
        this->primitive_base_t::save( static_cast<signed char>(0) );

        return;
    }
    const bool negative = (l < 0);
    boost::uintmax_t magnitude = static_cast<boost::uintmax_t>(l);
    if (negative) {
        magnitude = 0 - magnitude;
    }
    unsigned char bytes[sizeof(boost::intmax_t)];
    signed char size = 0;
    do {
        bytes[(int)size] = static_cast<unsigned char>(magnitude & 0xff);
        magnitude >>= CHAR_BIT;
        ++size;
    } while (magnitude != 0);
    this->primitive_base_t::save( static_cast<signed char>(negative ? -size : size) );
    this->primitive_base_t::save_binary(bytes, size);
}

void
PortableBinaryOArchive::saveIEEE754(boost::uint64_t bits,
                                    int nbytes)
{
    unsigned char bytes[8];

    for (int i = 0; i < nbytes; ++i) {
        bytes[i] = static_cast<unsigned char>( (bits >> (CHAR_BIT * i)) & 0xff );
    }
    this->primitive_base_t::save_binary(bytes, nbytes);
}

void
PortableBinaryOArchive::save(const float & t)
{
    boost::uint32_t bits32;

    std::memcpy( &bits32, &t, sizeof(t) );
    saveIEEE754(bits32, 4);
}

void
PortableBinaryOArchive::save(const double & t)
{
    boost::uint64_t bits;

    std::memcpy( &bits, &t, sizeof(t) );
    saveIEEE754(bits, 8);
}

void
PortableBinaryOArchive::init(unsigned int flags)
{
    if ( 0 == (flags & boost::archive::no_header) ) {
        // write signature in an archive version independent manner
        const std::string file_signature( boost::archive::BOOST_ARCHIVE_SIGNATURE() );
        *this << file_signature;
        // write library version
        const boost::archive::library_version_type v( boost::archive::BOOST_ARCHIVE_VERSION() );
        *this << v;
    }
    save( static_cast<unsigned char>(kPortableBinaryFlagLittleEndian) );
}

NATRON_NAMESPACE_EXIT

namespace boost {
namespace archive {
namespace detail {
template class archive_serializer_map<NATRON_NAMESPACE::PortableBinaryIArchive>;
template class archive_serializer_map<NATRON_NAMESPACE::PortableBinaryOArchive>;
}

template class basic_binary_iprimitive<NATRON_NAMESPACE::PortableBinaryIArchive, std::istream::char_type, std::istream::traits_type>;
template class basic_binary_oprimitive<NATRON_NAMESPACE::PortableBinaryOArchive, std::ostream::char_type, std::ostream::traits_type>;
} // namespace archive
} // namespace boost
