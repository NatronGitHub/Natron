/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "KnobFactory.h"

#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"

using namespace Natron;
using std::make_pair;
using std::pair;


/*Class inheriting Knob and KnobGui, must have a function named BuildKnob and BuildKnobGui with the following signature.
   This function should in turn call a specific class-based static function with the appropriate param.*/
typedef KnobHelper* (*KnobBuilder)(KnobHolder* holder, const std::string &label, int dimension, bool declaredByPlugin);

/***********************************FACTORY******************************************/
KnobFactory::KnobFactory()
{
    loadBultinKnobs();
}

KnobFactory::~KnobFactory()
{
    for (std::map<std::string, LibraryBinary *>::iterator it = _loadedKnobs.begin(); it != _loadedKnobs.end(); ++it) {
        delete it->second;
    }
    _loadedKnobs.clear();
}



template<typename K>
static std::pair<std::string,LibraryBinary *>
knobFactoryEntry()
{
    std::string stub;
    //boost::shared_ptr<KnobHelper> knob( K::BuildKnob(NULL, stub, 1) );
    std::map<std::string, void(*)()> functions;

    functions.insert( make_pair("BuildKnob", (void(*)())&K::BuildKnob) );
    LibraryBinary *knobPlugin = new LibraryBinary(functions);

    return make_pair(K::typeNameStatic(), knobPlugin);
}

void
KnobFactory::loadBultinKnobs()
{
    _loadedKnobs.insert( knobFactoryEntry<KnobFile>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobInt>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobDouble>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobBool>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobButton>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobOutputFile>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobChoice>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobSeparator>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobGroup>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobColor>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobString>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobParametric>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobPath>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobPage>() );
}

boost::shared_ptr<KnobHelper> KnobFactory::createKnob(const std::string &id,
                                                      KnobHolder*  holder,
                                                      const std::string &label,
                                                      int dimension,
                                                      bool declaredByPlugin) const
{
    std::map<std::string, LibraryBinary *>::const_iterator it = _loadedKnobs.find(id);

    if ( it == _loadedKnobs.end() ) {
        return boost::shared_ptr<KnobHelper>();
    } else {
        std::pair<bool, KnobBuilder> builderFunc = it->second->findFunction<KnobBuilder>("BuildKnob");
        if (!builderFunc.first) {
            return boost::shared_ptr<KnobHelper>();
        }
        KnobBuilder builder = (KnobBuilder)(builderFunc.second);
        boost::shared_ptr<KnobHelper> knob( builder(holder, label, dimension, declaredByPlugin) );
        if (!knob) {
            boost::shared_ptr<KnobHelper>();
        }
        knob->populate();
        if (holder) {
            holder->addKnob(knob);
        }

        return knob;
    }
}

