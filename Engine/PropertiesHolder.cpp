/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "PropertiesHolder.h"

NATRON_NAMESPACE_ENTER

PropertiesHolder::PropertiesHolder()
: _properties()
, _propertiesInitialized(false)
{
}

PropertiesHolder::PropertiesHolder(const PropertiesHolder& other)
: _properties()
, _propertiesInitialized(false)
{
    cloneProperties(other);
}

bool
PropertiesHolder::cloneProperties(const PropertiesHolder& other)
{
    return mergeProperties(other);
}

bool
PropertiesHolder::mergeProperties(const PropertiesHolder& other)
{
    bool hasChanged = false;
    if (!other._properties) {
        return hasChanged;
    }

    if (!_properties) {
        hasChanged = true;
        _propertiesInitialized = true;
        _properties.reset(new std::map<std::string, boost::shared_ptr<PropertyBase> >());

    }
    for (std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator it = other._properties->begin(); it!=other._properties->end(); ++it) {

        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties->find(it->first);
        if (found == _properties->end()) {
            hasChanged = true;
            _properties->insert(std::make_pair(it->first,it->second->createDuplicate()));
        } else {
            if (*it->second != *found->second) {
                hasChanged = true;
                *found->second = *it->second;
            }

        }
    }

    return hasChanged;
}

bool
PropertiesHolder::operator==(const PropertiesHolder& other) const
{
    if (!_properties && !other._properties) {
        return true;
    }
    if (!_properties || !other._properties) {
        return false;
    }
    if (_properties->size() != other._properties->size()) {
        return false;
    }
    std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator it1 = _properties->begin();
    for (std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator it = other._properties->begin(); it!=other._properties->end(); ++it, ++it1) {
        if (*it1->second != *it->second) {
            return false;
        }

    }
    return true;
}

PropertiesHolder::~PropertiesHolder()
{
    
}

NATRON_NAMESPACE_EXIT
