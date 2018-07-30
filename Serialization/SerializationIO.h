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

#ifndef SERIALIZATIONIO_H
#define SERIALIZATIONIO_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <istream>
#include <ostream>
#include <stdexcept>
#include <locale>
#include "Global/Macros.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/emitter.h>
#include <yaml-cpp/node/impl.h>
#include <yaml-cpp/node/parse.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/SerializationFwd.h"
#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/ProjectSerialization.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/SerializationFwd.h"

SERIALIZATION_NAMESPACE_ENTER
/**
 * @brief Write any serialization object to a YAML encoded file.
 * @param header The given header string will be written on the first line of the file unless it is empty.
 **/
template <typename T>
void write(std::ostream& stream, const T& obj, const std::string& header)
{
    if (!header.empty()) {
        stream << header.c_str() << std::endl;
    }
    YAML::Emitter em;
    obj.encode(em);
    stream << em.c_str();
}

class InvalidSerializationFileException : public std::exception
{
    std::string _what;

public:

    InvalidSerializationFileException()
    : _what()
    {
    }

    virtual ~InvalidSerializationFileException() throw()
    {
    }

    virtual const char * what () const throw ()
    {
        return _what.c_str();
    }
};

/**
 * @brief Read any serialization object from a YAML encoded file. Upon failure an exception is thrown.
 * @param header The first line of the file is matched against the given header string.
 * If it does not match, this function throws a InvalidSerializationFileException exception
 * If header is empty, it does not check against the header.
 **/
template <typename T>
void read(const std::string& header, std::istream& stream, T* obj)
{
    if (!obj) {
        throw std::invalid_argument("Invalid serialization object");
    }
    {
        std::string firstLine;
        std::getline(stream, firstLine);
        if (!header.empty()) {
            // On Windows line ending is \r\n, so the getline will finish with a \r if the file was written on Windows
            if (firstLine.size() > 0 && firstLine[firstLine.size() - 1] == '\r') {
                firstLine.resize(firstLine.size() - 1);
            }
            if (firstLine != header) {
                throw InvalidSerializationFileException();
            }
        } else {
            // Check if the first-line contains a #, because it may contain a header which we should skip
            bool skipFirstLine = true;
            for (std::size_t i = 0; i < firstLine.size(); ++i) {
                if (std::isspace(firstLine[i])) {
                    continue;
                }
                if (firstLine[i] != '#') {
                    skipFirstLine = false;
                    break;
                } else {
                    break;
                }
            }
            // Since we called getline, we must reset the stream
            if (!skipFirstLine) {
                stream.seekg(0);
            }
        }
    }
    obj->decode(YAML::Load(stream));
}

SERIALIZATION_NAMESPACE_EXIT

#endif // SERIALIZATIONIO_H
