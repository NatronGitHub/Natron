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

#ifndef NATRON_ENGINE_KNOBFACTORY_H
#define NATRON_ENGINE_KNOBFACTORY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"


/******************************KNOB_FACTORY**************************************/


class KnobFactory
{
public:
    KnobFactory();

    ~KnobFactory();

    template <typename K>
    boost::shared_ptr<K> createKnob(KnobHolder*  holder,
                                    const std::string &label,
                                    int dimension = 1,
                                    bool declaredByPlugin = true) const
    {
        return boost::dynamic_pointer_cast<K>( createKnob(K::typeNameStatic(), holder, label, dimension, declaredByPlugin) );
    }

private:
    boost::shared_ptr<KnobHelper> createKnob(const std::string &id,
                                             KnobHolder* holder,
                                             const std::string &label,
                                             int dimension = 1,
                                             bool declaredByPlugin = true) const WARN_UNUSED_RETURN;

    const std::map<std::string, Natron::LibraryBinary *> &getLoadedKnobs() const
    {
        return _loadedKnobs;
    }

    void loadBultinKnobs();

private:
    std::map<std::string, Natron::LibraryBinary *> _loadedKnobs;
};


#endif // NATRON_ENGINE_KNOBFACTORY_H
