/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#include "InputDescription.h"

NATRON_NAMESPACE_ENTER

InputDescription::InputDescription()
: PropertiesHolder()
{

}

InputDescription::InputDescription(const InputDescription& other)
: PropertiesHolder(other)
{

}

InputDescription::~InputDescription()
{

}

InputDescriptionPtr
InputDescription::create(const std::string& name, const std::string& label, const std::string& hint, bool optional, bool mask, std::bitset<4> supportedComponents)
{
    InputDescriptionPtr ret(new InputDescription);
    ret->setProperty(kInputDescPropScriptName, name);
    ret->setProperty(kInputDescPropLabel, label);
    ret->setProperty(kInputDescPropHint, hint);
    ret->setProperty(kInputDescPropIsOptional, optional);
    ret->setProperty(kInputDescPropIsMask, mask);
    ret->setProperty(kInputDescPropSupportedComponents, supportedComponents);
    return ret;
}

void
InputDescription::initializeProperties() const
{
    createProperty<std::string>(kInputDescPropScriptName, std::string());
    createProperty<std::string>(kInputDescPropLabel, std::string());
    createProperty<std::string>(kInputDescPropHint, std::string());
    createProperty<std::bitset<4> >(kInputDescPropSupportedComponents, std::bitset<4>());
    createProperty<ImageFieldExtractionEnum>(kInputDescPropFieldingSupport, eImageFieldExtractionDouble);
    createProperty<bool>(kInputDescPropCanReceiveTransform3x3, false);
    createProperty<bool>(kInputDescPropCanReceiveDistortion, false);
    createProperty<bool>(kInputDescPropIsVisible, true);
    createProperty<bool>(kInputDescPropIsOptional, false);
    createProperty<bool>(kInputDescPropIsMask, false);
    createProperty<bool>(kInputDescPropSupportsTiles, true);
    createProperty<bool>(kInputDescPropSupportsTemporal, false);
    createProperty<bool>(kInputDescPropIsHostInput, false);

}

NATRON_NAMESPACE_EXIT
