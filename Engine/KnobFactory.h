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

#ifndef Engine_KnobFactory_h
#define Engine_KnobFactory_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/******************************KNOB_FACTORY**************************************/


class KnobFactory
{
public:
    KnobFactory();

    ~KnobFactory();

    template <typename K>
    boost::shared_ptr<K> createKnob(const KnobHolderPtr& holder,
                                    const std::string &label,
                                    int dimension = 1,
                                    bool declaredByPlugin = true) const
    {
        return boost::dynamic_pointer_cast<K>( createKnob(K::typeNameStatic(), holder, label, dimension, declaredByPlugin) );
    }

    template <typename K>
    boost::shared_ptr<K> checkIfKnobExistsWithNameOrCreate(const KnobHolderPtr& holder,
                                                           const std::string& scriptName,
                                                           const std::string& label,
                                                           int dimension = 1) const
    {
        assert(holder);
        boost::shared_ptr<K> isType = boost::dynamic_pointer_cast<K>(getHolderKnob(holder, scriptName));
        if (isType) {
            //Remove from the parent if it exists, because it will be added again afterwards
            isType->resetParent();
            return isType;
        }

        // If we reach here, either :
        // - we found a knob with the same script-name but not the same type or dimension, so create a new one which will have an altered script-name (with a number appended)
        // - we did not find such a knob, create it
        boost::shared_ptr<K> ret = createKnob<K>(holder, label, dimension, true);
        ret->setName(scriptName);
        
        return ret;
    }

private:

    KnobIPtr getHolderKnob(const KnobHolderPtr& holder,
                           const std::string& scriptName) const;

    KnobHelperPtr createKnob(const std::string &id,
                                             const KnobHolderPtr& holder,
                                             const std::string &label,
                                             int dimension = 1,
                                             bool declaredByPlugin = true) const WARN_UNUSED_RETURN;
    const std::map<std::string, LibraryBinary *> &getLoadedKnobs() const
    {
        return _loadedKnobs;
    }

    void loadBultinKnobs();

private:
    std::map<std::string, LibraryBinary *> _loadedKnobs;
};

NATRON_NAMESPACE_EXIT;

#endif // Engine_KnobFactory_h
