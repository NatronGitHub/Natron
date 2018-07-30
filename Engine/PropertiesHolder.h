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

#ifndef PROPERTIESHOLDER_H
#define PROPERTIESHOLDER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <map>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class PropertiesHolder
{

public:

    /**
     * @class Base class for properties
     **/
    class PropertyBase
    {
    public:
        PropertyBase()
        {
        }

        virtual int getNDimensions() const = 0;

        virtual boost::shared_ptr<PropertyBase> createDuplicate() const = 0;

        virtual ~PropertyBase()
        {
        }

        virtual void operator=(const PropertyBase& other)= 0;

        virtual bool operator==(const PropertyBase& other) const = 0;

        bool operator!=(const PropertyBase& other) const
        {
            return !(*this == other);
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

        virtual int getNDimensions() const OVERRIDE FINAL
        {
            return (int)value.size();
        }

        virtual boost::shared_ptr<PropertyBase> createDuplicate() const OVERRIDE FINAL
        {
            boost::shared_ptr<Property<T> > ret = boost::make_shared<Property<T> >();
            ret->value = value;
            return ret;
        }

        void operator=(const PropertyBase& other) OVERRIDE FINAL
        {
            const Property<T> * othertype = dynamic_cast<const Property<T>* >(&other);
            if (!othertype) {
                return;
            }
            value = othertype->value;
        }

        virtual bool operator==(const PropertyBase& other) const OVERRIDE FINAL
        {
            const Property<T> * othertype = dynamic_cast<const Property<T>* >(&other);
            if (!othertype) {
                return false;
            }
            if (value.size() != othertype->value.size()) {
                return false;
            }
            for (std::size_t i = 0; i < value.size(); ++i) {
                if (value[i] != othertype->value[i]) {
                    return false;
                }
            }
            return true;

        }

        virtual ~Property()
        {
        }
    };

protected:
    
    /**
     * @brief Returns a pointer to the property matching the given unique name. 
     * @param failIfNotExisting If true, then this function throws an exception if the property cannot be found
     * or it is not of the requested data type T.
     **/
    template <typename T>
    boost::shared_ptr<Property<T> > getProp(const std::string& name, bool throwOnFailure = true) const
    {
        const boost::shared_ptr<PropertyBase>* propPtr = 0;
        if (!_properties) {
            if (throwOnFailure) {
                throw std::invalid_argument("CreateNodeArgs::getProp(): Invalid property " + name);
            }
            return boost::shared_ptr<Property<T> >();
        }

        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties->find(name);
        if (found == _properties->end()) {
            if (throwOnFailure) {
                throw std::invalid_argument("CreateNodeArgs::getProp(): Invalid property " + name);
            }
            return boost::shared_ptr<Property<T> >();
        }
        propPtr = &(found->second);
        boost::shared_ptr<Property<T> > propTemplate;
        propTemplate = boost::dynamic_pointer_cast<Property<T> >(*propPtr);
        assert(propPtr);
        if (!propTemplate) {
            if (throwOnFailure) {
                throw std::invalid_argument("CreateNodeArgs::getProp(): Invalid property type for " + name);
            }
            return boost::shared_ptr<Property<T> >();
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

        ensurePropertiesMap();

        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties->find(name);
        boost::shared_ptr<Property<T> > propTemplate;
        if (found != _properties->end()) {
            propTemplate = boost::dynamic_pointer_cast<Property<T> >(found->second);
        } else {
            propTemplate = boost::make_shared<Property<T> >();
            _properties->insert(std::make_pair(name, propTemplate));
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

    PropertiesHolder(const PropertiesHolder& other);

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
        ensurePropertiesMap();

        boost::shared_ptr<Property<T> > propTemplate;
        propTemplate = getProp<T>(name, failIfNotExisting);
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
        ensurePropertiesMap();

        boost::shared_ptr<Property<T> > propTemplate;
        propTemplate = getProp<T>(name, failIfNotExisting);
        if (!propTemplate) {
            propTemplate = createProperty<T>(name, values);
        }
        propTemplate->value = values;
    }

    /**
     * @brief Returns the number of dimensions of the property associated to the given name
     * @param throwIfFailed If true, then this function throws an exception if the property cannot be found
     **/
    int getPropertyDimension(const std::string& name, bool throwIfFailed = true) const
    {
        ensurePropertiesCreated();
        if (!_properties) {
            if (throwIfFailed) {
                throw std::invalid_argument("Invalid property " + name);
            } else {
                return 0;
            }
        }

        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties->find(name);
        if (found == _properties->end()) {
            if (throwIfFailed) {
                throw std::invalid_argument("Invalid property " + name);
            } else {
                return 0;
            }
        }

        return found->second->getNDimensions();
    }

    /**
     * @brief Returns true if the property exists
     **/
    bool hasProperty(const std::string& name) const
    {
        ensurePropertiesCreated();
        if (!_properties) {
            return false;
        }
        std::map<std::string, boost::shared_ptr<PropertyBase> >::const_iterator found = _properties->find(name);
        return found != _properties->end();
    }

    /**
     * @brief Returns the value of the property at the given index associated to the given name
     * This function returns false if it could not be found or the index
     * is invalid.
     **/
    template<typename T>
    bool getPropertySafe(const std::string& name, int index, T* value) const
    {
        ensurePropertiesCreated();
        if (!_properties) {
            return false;
        }

        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name, false);
        if (!propTemplate) {
            return false;
        }
        if (index < 0 || index >= (int)propTemplate->value.size()) {
            return false;
        }

        *value = propTemplate->value[index];
        return true;
    }

    /**
     * @brief Same as getPropertySafe
     * This function throws an invalid_argument exception if it could not be found or the index
     * is invalid.
     **/
    template<typename T>
    T getPropertyUnsafe(const std::string& name, int index = 0) const
    {
        ensurePropertiesCreated();
        if (!_properties) {
            throw std::invalid_argument("PropertiesHolder::getPropertyUnsafe(): no such property: " + name);
        }

        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name);

        if (index < 0 || index >= (int)propTemplate->value.size()) {
            throw std::invalid_argument("PropertiesHolder::getPropertyUnsafe(): index out of range for " + name);
        }

        return propTemplate->value[index];
        
    }

    /**
     * @brief Same as getProperty except that it returns all dimensions of the property at once.
     * This function returns false if it could not be found.
     **/
    template<typename T>
    bool getPropertyNSafe(const std::string& name, std::vector<T>* values) const
    {
        ensurePropertiesCreated();
        if (!_properties) {
            return false;
        }

        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name, false);
        if (!propTemplate) {
            return false;
        }
        *values = propTemplate->value;
        return true;
    }

    /**
     * @brief Same as getProperty except that it returns all dimensions of the property at once.
    * This function throws an invalid_argument exception if it could not be found
     **/
    template<typename T>
    const std::vector<T>& getPropertyNUnsafe(const std::string& name) const
    {
        ensurePropertiesCreated();
        boost::shared_ptr<Property<T> > propTemplate = getProp<T>(name);
        return propTemplate->value;
    }

    // Returns true if something changed
    bool cloneProperties(const PropertiesHolder& other);

    bool hasProperties() const
    {
        return _properties && !_properties->empty();
    }

    const std::map<std::string, boost::shared_ptr<PropertyBase> >& getProperties() const
    {
        assert(_properties);
        return *_properties;
    }


    // Returns true if something changed
    bool mergeProperties(const PropertiesHolder& other);


    bool operator==(const PropertiesHolder& other) const;
    bool operator!=(const PropertiesHolder& other) const
    {
        return !(*this == other);
    }

protected:

    /**
     * @brief Must be implemented to create all properties.
     * To do so call the appropriate createProperty() function.
     **/
    virtual void initializeProperties() const = 0;


    void ensurePropertiesMap() const
    {
        if (!_properties) {
            _properties.reset(new std::map<std::string, boost::shared_ptr<PropertyBase> >());
        }
    }

    mutable boost::scoped_ptr<std::map<std::string, boost::shared_ptr<PropertyBase> > > _properties;
    mutable bool _propertiesInitialized;


};

NATRON_NAMESPACE_EXIT

#endif // PROPERTIESHOLDER_H
