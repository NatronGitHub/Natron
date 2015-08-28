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

#include "KnobGuiFactory.h"

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"

#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

#include "Gui/KnobGui.h"
#include "Gui/File_KnobGui.h"
#include "Gui/Int_KnobGui.h"
#include "Gui/Double_KnobGui.h"
#include "Gui/Bool_KnobGui.h"
#include "Gui/Button_KnobGui.h"
#include "Gui/Choice_KnobGui.h"
#include "Gui/Separator_KnobGui.h"
#include "Gui/Group_KnobGui.h"
#include "Gui/Color_KnobGui.h"
#include "Gui/String_KnobGui.h"
#include "Gui/Bool_KnobGui.h"
#include "Gui/Parametric_KnobGui.h"
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
    std::map<std::string, void *> functions;

    functions.insert( make_pair("BuildKnobGui", (void *)&KG::BuildKnobGui) );
    LibraryBinary *knobPlugin = new LibraryBinary(functions);

    return make_pair(knob->typeName(), knobPlugin);
}

void
KnobGuiFactory::loadBultinKnobs()
{
    _loadedKnobs.insert( knobGuiFactoryEntry<File_Knob,File_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Int_Knob,Int_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Double_Knob,Double_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Bool_Knob,Bool_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Button_Knob,Button_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<OutputFile_Knob,OutputFile_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Choice_Knob,Choice_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Separator_Knob,Separator_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Group_Knob,Group_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Color_Knob,Color_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<String_Knob,String_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Bool_Knob,Bool_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Parametric_Knob, Parametric_KnobGui>() );
    _loadedKnobs.insert( knobGuiFactoryEntry<Path_Knob, Path_KnobGui>() );
    // _loadedKnobs.insert(knobGuiFactoryEntry<Table_Knob, Table_KnobGui>());
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

