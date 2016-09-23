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

#ifndef PROPERTIESHOLDER_H
#define PROPERTIESHOLDER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <map>
#include <stdexcept>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class PropertiesHolder
{

    /**
     * @class Base class for properties
     **/
    class PropertyBase
    {
    public:
        PropertyBase()
        {
        }

        virtual int getDimension() const = 0;

        virtual ~PropertyBase()
        {
        }
    };

    /**
     * @class Templated property that holds multi-dimensional datas
     **/
    template<typename T>
    class Property : public PropertyBase
    {
    public:
        std::vector<T> value;

        Property()
        : PropertyBase()
        , value()
        {
        }

        virtual int getDimension() const OVERRIDE FINAL
        {
            return (int)value.size();
        }

        virtual ~Property()
        {
        }
    };

    /**
     * @brief Returns a pointer to the property matching the given unique name. 
     * @param failIfNotExisting If true, then this function throws an exception if the property cannot be found
     * or it is not of the requested data type T.
     **/
    template <typename T>
    boost::shared_ptr<Property<T> > getProp(const std::string& name, bool failIfNotExisting = true) const
    {
        const boost::shared_ptr<PropertyBase>* propPtr = 0;
        boost::shared_ptr<Property<T> > propTemplate;

        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties.find(name);
        if (found == _properties.end()) {
            if (failIfNotExisting) {
                throw std::invalid_argument("PropertiesHolder::getProp(): Invalid property " + name);
            }
            return propTemplate;
        }
        propPtr = &(found->second);
        propTemplate = boost::dynamic_pointer_cast<Property<T> >(*propPtr);
        assert(propPtr);
        if (!propTemplate) {
            if (failIfNotExisting) {
                throw std::invalid_argument("PropertiesHolder::getProp(): Invalid property type for " + name);
            }
            return propTemplate;
        }
        assert(propTemplate);

        return propTemplate;
    }

    /**
     * @brief Creates a property associated to the given unique name.
     **/
    template <typename T>
    boost::shared_ptr<Property<T> > createPropertyInternal(const std::string& name) const
    {
        boost::shared_ptr<PropertyBase>* propPtr = 0;
        propPtr = &_properties[name];

        boost::shared_ptr<Property<T> > propTemplate;
        if (!*propPtr) {
            propTemplate.reset(new Property<T>);
            *propPtr = propTemplate;
        } else {
            propTemplate = boost::dynamic_pointer_cast<Property<T> >(*propPtr);
        }
        assert(propTemplate);

        return propTemplate;
    }


    void ensurePropertiesCreated() const
    {
        if (_propertiesInitialized) {
            return;
        }
        initializeProperties();
        _propertiesInitialized = true;
    }

protected:

    /**
     * @brief Creates a one dimensional property
     **/
    template <typename T>
    boost::shared_ptr<Property<T> > createProperty(const std::string& name, const T& defaultValue) const
    {
        boost::shared_ptr<Property<T> > p = createPropertyInternal<T>(name);
        p->value.push_back(defaultValue);

        return p;
    }

    /**
     * @brief Creates a two dimensional property
     **/
    template <typename T>
    boost::shared_ptr<Property<T> > createProperty(const std::string& name, const T& defaultValue1, const T& defaultValue2) const
    {
        boost::shared_ptr<Property<T> > p = createPropertyInternal<T>(name);
        p->value.push_back(defaultValue1);
        p->value.push_back(defaultValue2);

        return p;
    }

    /**
     * @brief Creates a N dimensional property
     **/
    template <typename T>
    boost::shared_ptr<Property<T> > createProperty(const std::string& name, const std::vector<T>& defaultValue) const
    {
        boost::shared_ptr<Property<T> > p = createPropertyInternal<T>(name);
        p->value = defaultValue;
        
        return p;
    }


public:

    /**
     * @brief Creates an empty properties holder. The properties are initialized once the first public method
     * is called.
     * This object is NOT thread-safe
     **/
    PropertiesHolder();

    virtual ~PropertiesHolder();

    /**
     * @brief Set the property matching the given name to the given value. 
     * @param index The value at the given index will be set. If index is out of range of the data vector
     * it will be extended to fit the value in it.
     * @param failIfNotExisting If true, then this function throws an exception if the property cannot be found
     * or it is not of the requested data type T.
     **/
    template <typename T>
    void setProperty(const std::string& name, const T& value, int index = 0, bool failIfNotExisting = true)
    {
        ensurePropertiesCreated();

        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name, failIfNotExisting);
        if (!propTemplate) {
            propTemplate = createProperty<T>(name, value);
        }
        if (index >= (int)propTemplate->value.size()) {
            propTemplate->value.resize(index + 1);
        }
        propTemplate->value[index] = value;
    }

    /**
     * @brief Same as setProperty except that all dimensions of the property are set at once.
     **/
    template <typename T>
    void setPropertyN(const std::string& name, const std::vector<T>& values, bool failIfNotExisting = true)
    {
        ensurePropertiesCreated();

        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name, failIfNotExisting);
        if (propTemplate) {
            propTemplate->value = values;
        } else {
            propTemplate = createProperty<T>(name, values);
        }
    }

    /**
     * @brief Returns the number of dimensions of the property associated to the given name
     * @param throwIfFailed If true, then this function throws an exception if the property cannot be found
     **/
    int getPropertyDimension(const std::string& name, bool throwIfFailed = true) const
    {
        ensurePropertiesCreated();

        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties.find(name);
        if (found == _properties.end()) {
            if (throwIfFailed) {
                throw std::invalid_argument("Invalid property " + name);
            } else {
                return 0;
            }
        }

        return found->second->getDimension();
    }


    /**
     * @brief Returns the value of the property at the given index associated to the given name
     * This function throws an invalid_argument exception if it could not be found or the index
     * is invalid.
     **/
    template<typename T>
    T getProperty(const std::string& name, int index = 0) const
    {
        ensurePropertiesCreated();

        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name);
        if (index < 0 || index >= (int)propTemplate->value.size()) {
            throw std::invalid_argument("PropertiesHolder::getProperty(): index out of range for " + name);
        }

        return propTemplate->value[index];
    }

    /**
     * @brief Same as getProperty except that it returns all dimensions of the property at once.
     **/
    template<typename T>
    std::vector<T> getPropertyN(const std::string& name) const
    {
        ensurePropertiesCreated();

        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name);
        
        return propTemplate->value;
    }

protected:

    /**
     * @brief Must be implemented to create all properties.
     * To do so call the appropriate createProperty() function.
     **/
    virtual void initializeProperties() const = 0;

    mutable std::map<std::string, boost::shared_ptr<PropertyBase> > _properties;
    mutable bool _propertiesInitialized;


};

NATRON_NAMESPACE_EXIT

#endif // PROPERTIESHOLDER_H
