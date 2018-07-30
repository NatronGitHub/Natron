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

#ifndef TABWIDGETI_H
#define TABWIDGETI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <string>

#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class TabWidgetI : public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
public:

    TabWidgetI()
    {

    }

    virtual ~TabWidgetI()
    {

    }

    virtual void setClosable(bool closable) = 0;

    virtual bool isAnchor() const = 0;

    virtual void setAsAnchor(bool b) = 0;

    virtual int activeIndex() const = 0;

    virtual std::string getScriptName() const = 0;

    virtual void setScriptName(const std::string & str) = 0;

    virtual void setCurrentIndex(int index) = 0;

    virtual std::list<std::string> getTabScriptNames() const = 0;

    virtual void setTabsFromScriptNames(const std::list<std::string>& tabs) = 0;

    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

};

NATRON_NAMESPACE_EXIT

#endif // TABWIDGETI_H
