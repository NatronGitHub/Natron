//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
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
typedef KnobHelper* (*KnobBuilder)(KnobHolder  *holder, const std::string &description, int dimension);

/***********************************FACTORY******************************************/
KnobFactory::KnobFactory()
{
    loadKnobPlugins();
}

KnobFactory::~KnobFactory()
{
    for (std::map<std::string, LibraryBinary *>::iterator it = _loadedKnobs.begin(); it != _loadedKnobs.end() ; ++it) {
        delete it->second;
    }
    _loadedKnobs.clear();
}

void KnobFactory::loadKnobPlugins()
{
    std::vector<LibraryBinary *> plugins = AppManager::loadPlugins(NATRON_KNOBS_PLUGINS_PATH);
    std::vector<std::string> functions;
    functions.push_back("BuildKnob");
    for (U32 i = 0; i < plugins.size(); ++i) {
        if (plugins[i]->loadFunctions(functions)) {
            std::pair<bool, KnobBuilder> builder = plugins[i]->findFunction<KnobBuilder>("BuildKnob");
            if (builder.first) {
                boost::shared_ptr<KnobHelper> knob(builder.second(NULL, "", 1));
                _loadedKnobs.insert(make_pair(knob->typeName(), plugins[i]));
            }
        } else {
            delete plugins[i];
        }
    }
    loadBultinKnobs();
}

template<typename K>
static std::pair<std::string,LibraryBinary *>
knobFactoryEntry()
{
    std::string stub;
    boost::shared_ptr<KnobHelper> knob(K::BuildKnob(NULL, stub, 1));

    std::map<std::string, void *> functions;
    functions.insert(make_pair("BuildKnob", (void *)&K::BuildKnob));
    LibraryBinary *knobPlugin = new LibraryBinary(functions);
    return make_pair(knob->typeName(), knobPlugin);
}

void KnobFactory::loadBultinKnobs()
{
    _loadedKnobs.insert(knobFactoryEntry<File_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Int_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Double_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Bool_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Button_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<OutputFile_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Choice_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Separator_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Group_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Tab_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Color_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<String_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Parametric_Knob>());
    _loadedKnobs.insert(knobFactoryEntry<Path_Knob>());
    //   _loadedKnobs.insert(knobFactoryEntry<Table_Knob>());
}

boost::shared_ptr<KnobHelper> KnobFactory::createKnob(const std::string &id,
                              KnobHolder  *holder,
                              const std::string &description, int dimension) const
{

    std::map<std::string, LibraryBinary *>::const_iterator it = _loadedKnobs.find(id);
    if (it == _loadedKnobs.end()) {
        return boost::shared_ptr<KnobHelper>();
    } else {
        std::pair<bool, KnobBuilder> builderFunc = it->second->findFunction<KnobBuilder>("BuildKnob");
        if (!builderFunc.first) {
            return boost::shared_ptr<KnobHelper>();
        }
        KnobBuilder builder = (KnobBuilder)(builderFunc.second);
        boost::shared_ptr<KnobHelper> knob(builder(holder, description, dimension));
        if (!knob) {
            boost::shared_ptr<KnobHelper>();
        }
        boost::shared_ptr<KnobSignalSlotHandler> handler(new KnobSignalSlotHandler(knob));
        knob->setSignalSlotHandler(handler);
        knob->populate();
        if(holder){
            holder->addKnob(knob);
        }
        return knob;
    }
}


