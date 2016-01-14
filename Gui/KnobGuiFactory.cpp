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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "KnobGuiFactory.h"

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"

#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobGuiFile.h"
#include "Gui/KnobGuiInt.h"
#include "Gui/KnobGuiDouble.h"
#include "Gui/KnobGuiBool.h"
#include "Gui/KnobGuiButton.h"
#include "Gui/KnobGuiChoice.h"
#include "Gui/KnobGuiSeparator.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/KnobGuiColor.h"
#include "Gui/KnobGuiString.h"
#include "Gui/KnobGuiBool.h"
#include "Gui/KnobGuiParametric.h"
#include "Gui/DockablePanel.h"

using namespace Natron;
using std::make_pair;
using std::pair;


/*Class inheriting KnobGui, must have a function named BuildKnobGui with the following signature.
   This function should in turn call a specific class-based static function with the appropriate param.*/
typedef KnobHelper *(*KnobBuilder)(KnobHolder  *holder, const std::string &description, int dimension);
typedef KnobGui *(*KnobGuiBuilder)(boost::shared_ptr<KnobI> knob, DockablePanel* panel);

/***********************************FACTORY******************************************/
KnobGuiFactory::KnobGuiFactory()
{
    loadBultinKnobs();
}

KnobGuiFactory::~KnobGuiFactory()
{
    for (std::map<std::string, LibraryBinary *>::iterator it = _loadedKnobs.begin(); it != _loadedKnobs.end(); ++it) {
        delete it->second;
    }
    _loadedKnobs.clear();
}

template<typename K, typename KG>
static std::pair<std::string,LibraryBinary *>
knobGuiFactoryEntry()
{
    std::string stub;
    boost::scoped_ptr<KnobHelper> knob( K::BuildKnob(NULL, stub, 1) );
    std::map<std::string, void(*)()> functions;

    functions.insert( make_pair("BuildKnobGui", (void(*)())&KG::BuildKnobGui) );
    LibraryBinary *knobPlugin = new LibraryBinary(functions);

    return make_pair(knob->typeName(), knobPlugin);
}

void
KnobGuiFactory::loadBultinKnobs()
{
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobFile, KnobGuiFile>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobInt, KnobGuiInt>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobDouble, KnobGuiDouble>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobBool, KnobGuiBool>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobButton, KnobGuiButton>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobOutputFile, KnobGuiOutputFile>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobChoice, KnobGuiChoice>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobSeparator, KnobGuiSeparator>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobGroup, KnobGuiGroup>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobColor, KnobGuiColor>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobString, KnobGuiString>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobBool, KnobGuiBool>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobParametric, KnobGuiParametric>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<KnobPath, KnobGuiPath>() );
    // _loadedKnobs.insert(knobGuiFactoryEntry<KnobTable, KnobTableGui>());
}

KnobGui *
KnobGuiFactory::createGuiForKnob(boost::shared_ptr<KnobI> knob,
                                 DockablePanel *container) const
{
    assert(knob);
    std::map<std::string, LibraryBinary *>::const_iterator it = _loadedKnobs.find( knob->typeName() );
    if ( it == _loadedKnobs.end() ) {
        return NULL;
    } else {
        std::pair<bool, KnobGuiBuilder> guiBuilderFunc = it->second->findFunction<KnobGuiBuilder>("BuildKnobGui");
        if (!guiBuilderFunc.first) {
            return NULL;
        }
        KnobGuiBuilder guiBuilder = (KnobGuiBuilder)(guiBuilderFunc.second);

        return guiBuilder(knob, container);
    }
}

