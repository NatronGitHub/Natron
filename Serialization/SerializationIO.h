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

#include "Serialization/SerializationFwd.h"
#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/ProjectSerialization.h"
#include "Serialization/NodeSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

/**
 * @brief Write any serialization object to a YAML encoded file.
 **/
template <typename T>
void write(std::ostream& stream, const T& obj)
{
    YAML_NAMESPACE::Emitter em;
    em.SetStringFormat(YAML_NAMESPACE::DoubleQuoted);
    obj.encode(em);
    stream << em.c_str();
}

/**
 * @brief Read any serialization object from a YAML encoded file. Upon failure an exception is thrown.
 **/
template <typename T>
void read(std::istream& stream, T* obj)
{
    if (!obj) {
        throw std::invalid_argument("Invalid serialization object");
    }
    YAML_NAMESPACE::Node node = YAML_NAMESPACE::Load(stream);
    obj->decode(node);
}

SERIALIZATION_NAMESPACE_EXIT

#endif // SERIALIZATIONIO_H
