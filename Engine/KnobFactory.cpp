/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include <cassert>
#include <stdexcept>

#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"

NATRON_NAMESPACE_ENTER

//using std::make_pair;
//using std::pair;



typedef KnobHelperPtr (*KnobBuilder)(const KnobHolderPtr& holder, const std::string &scriptName, int dimension);
typedef KnobHelperPtr (*KnobRenderCloneBuilder)(const KnobHolderPtr& holder, const KnobIPtr& mainInstance);

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
static std::pair<std::string, LibraryBinary *>
knobFactoryEntry()
{
    std::string stub;
    //KnobHelperPtr knob( K::BuildKnob(NULL, stub, 1) );
    std::map<std::string, void (*)()> functions;
    functions.insert( std::make_pair("create", ( void (*)() ) (KnobBuilder)K::create) );
    functions.insert( std::make_pair("createRenderClone", ( void (*)() )K::createRenderClone) );
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
    _loadedKnobs.insert( knobFactoryEntry<KnobChoice>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobSeparator>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobGroup>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobColor>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobString>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobParametric>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobPath>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobPage>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobLayers>() );
    _loadedKnobs.insert( knobFactoryEntry<KnobKeyFrameMarkers>() );
}

KnobIPtr
KnobFactory::getHolderKnob(const KnobHolderPtr& holder,
                           const std::string& scriptName) const
{
    return holder->getKnobByName(scriptName);
}

KnobHelperPtr
KnobFactory::createRenderCloneKnob(const KnobIPtr& mainInstanceKnob, const KnobHolderPtr& holder) const
{
    std::map<std::string, LibraryBinary *>::const_iterator it = _loadedKnobs.find(mainInstanceKnob->typeName());

    if ( it == _loadedKnobs.end() ) {
        return KnobHelperPtr();
    }
    std::pair<bool, KnobRenderCloneBuilder> builderFunc = it->second->findFunction<KnobRenderCloneBuilder>("createRenderClone");
    if (!builderFunc.first) {
        return KnobHelperPtr();
    }


    // If the knob does not evaluate on change, do not copy it anyway since the value is irrelevant to the render.
    if (!mainInstanceKnob->getEvaluateOnChange()) {
        KnobHelperPtr mainInstance = toKnobHelper(mainInstanceKnob);
        mainInstance->setActualCloneForHolder(holder);
        holder->addKnob(mainInstanceKnob);
        return mainInstance;
    }

    KnobRenderCloneBuilder builder = (KnobRenderCloneBuilder)(builderFunc.second);
    KnobHelperPtr knob = ( builder(holder, mainInstanceKnob) );
    if (!knob) {
        KnobHelperPtr();
    }
    knob->populate();


    if (holder) {
        holder->addKnob(knob);
    }
    return knob;

} // createRenderCloneKnob

KnobHelperPtr KnobFactory::createKnob(const std::string &id,
                                      const KnobHolderPtr& holder,
                                      const std::string &name,
                                      int dimension) const
{
    std::map<std::string, LibraryBinary *>::const_iterator it = _loadedKnobs.find(id);

    if ( it == _loadedKnobs.end() ) {
        return KnobHelperPtr();
    }

    KnobHelperPtr knob;

    std::pair<bool, KnobBuilder> builderFunc = it->second->findFunction<KnobBuilder>("create");
    if (!builderFunc.first) {
        return KnobHelperPtr();
    }

    KnobBuilder builder = (KnobBuilder)(builderFunc.second);
    knob = ( builder(holder, name, dimension) );
    if (!knob) {
        KnobHelperPtr();
    }
    knob->setName(name);


    knob->populate();


    if (holder) {
        holder->addKnob(knob);
    }

    return knob;
}

NATRON_NAMESPACE_EXIT
