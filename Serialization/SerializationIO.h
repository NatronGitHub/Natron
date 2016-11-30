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

#ifndef SERIALIZATIONIO_H
#define SERIALIZATIONIO_H

#include <istream>
#include <ostream>
#include <stdexcept>

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
        stream << header.c_str() << "\n";
    }
    YAML::Emitter em;
    obj.encode(em);
    stream << em.c_str();
}


/**
 * @brief Read any serialization object from a YAML encoded file. Upon failure an exception is thrown.
 * @param header The first line of the file is matched against the given header string.
 * If it does not match, this function return false, otherwise it returns true.
 * If header is empty, it does not check against the header.
 **/
template <typename T>
bool read(const std::string& header, std::istream& stream, T* obj)
{
    if (!obj) {
        throw std::invalid_argument("Invalid serialization object");
    }

    std::string firstLine;
    std::getline(stream, firstLine);
    if (!header.empty()) {
        if (firstLine != header) {
            return false;
        }
    }
    YAML::Node node = YAML::Load(stream);
    obj->decode(node);
    return true;
}

SERIALIZATION_NAMESPACE_EXIT

#endif // SERIALIZATIONIO_H
