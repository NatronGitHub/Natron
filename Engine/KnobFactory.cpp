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

#include "Engine/Knob.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"

#include "Readers/Reader.h"

using namespace Natron;
using std::make_pair;
using std::pair;


/*Class inheriting Knob and KnobGui, must have a function named BuildKnob and BuildKnobGui with the following signature.
 This function should in turn call a specific class-based static function with the appropriate param.*/
typedef Knob *(*KnobBuilder)(KnobHolder  *holder, const std::string &description, int dimension);

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
                Knob *knob = builder.second(NULL, "", 1);
                _loadedKnobs.insert(make_pair(knob->typeName(), plugins[i]));
                delete knob;
            }
        } else {
            delete plugins[i];
        }
    }
    loadBultinKnobs();
}

void KnobFactory::loadBultinKnobs()
{
    std::string stub;
    Knob *fileKnob = File_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> FILEfunctions;
    FILEfunctions.insert(make_pair("BuildKnob", (void *)&File_Knob::BuildKnob));
    LibraryBinary *FILEKnobPlugin = new LibraryBinary(FILEfunctions);
    _loadedKnobs.insert(make_pair(fileKnob->typeName(), FILEKnobPlugin));
    delete fileKnob;

    Knob *intKnob = Int_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> INTfunctions;
    INTfunctions.insert(make_pair("BuildKnob", (void *)&Int_Knob::BuildKnob));
    LibraryBinary *INTKnobPlugin = new LibraryBinary(INTfunctions);
    _loadedKnobs.insert(make_pair(intKnob->typeName(), INTKnobPlugin));
    delete intKnob;


    Knob *doubleKnob = Double_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> DOUBLEfunctions;
    DOUBLEfunctions.insert(make_pair("BuildKnob", (void *)&Double_Knob::BuildKnob));
    LibraryBinary *DOUBLEKnobPlugin = new LibraryBinary(DOUBLEfunctions);
    _loadedKnobs.insert(make_pair(doubleKnob->typeName(), DOUBLEKnobPlugin));
    delete doubleKnob;

    Knob *boolKnob = Bool_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> BOOLfunctions;
    BOOLfunctions.insert(make_pair("BuildKnob", (void *)&Bool_Knob::BuildKnob));
    LibraryBinary *BOOLKnobPlugin = new LibraryBinary(BOOLfunctions);
    _loadedKnobs.insert(make_pair(boolKnob->typeName(), BOOLKnobPlugin));
    delete boolKnob;

    Knob *buttonKnob = Button_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> BUTTONfunctions;
    BUTTONfunctions.insert(make_pair("BuildKnob", (void *)&Button_Knob::BuildKnob));
    LibraryBinary *BUTTONKnobPlugin = new LibraryBinary(BUTTONfunctions);
    _loadedKnobs.insert(make_pair(buttonKnob->typeName(), BUTTONKnobPlugin));
    delete buttonKnob;

    Knob *outputFileKnob = OutputFile_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> OFfunctions;
    OFfunctions.insert(make_pair("BuildKnob", (void *)&OutputFile_Knob::BuildKnob));
    LibraryBinary *OUTPUTFILEKnobPlugin = new LibraryBinary(OFfunctions);
    _loadedKnobs.insert(make_pair(outputFileKnob->typeName(), OUTPUTFILEKnobPlugin));
    delete outputFileKnob;

    Knob *comboBoxKnob = ComboBox_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> CBBfunctions;
    CBBfunctions.insert(make_pair("BuildKnob", (void *)&ComboBox_Knob::BuildKnob));
    LibraryBinary *ComboBoxKnobPlugin = new LibraryBinary(CBBfunctions);
    _loadedKnobs.insert(make_pair(comboBoxKnob->typeName(), ComboBoxKnobPlugin));
    delete comboBoxKnob;


    Knob *separatorKnob = Separator_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> Sepfunctions;
    Sepfunctions.insert(make_pair("BuildKnob", (void *)&Separator_Knob::BuildKnob));
    LibraryBinary *SeparatorKnobPlugin = new LibraryBinary(Sepfunctions);
    _loadedKnobs.insert(make_pair(separatorKnob->typeName(), SeparatorKnobPlugin));
    delete separatorKnob;

    Knob *groupKnob = Group_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> Grpfunctions;
    Grpfunctions.insert(make_pair("BuildKnob", (void *)&Group_Knob::BuildKnob));
    LibraryBinary *GroupKnobPlugin = new LibraryBinary(Grpfunctions);
    _loadedKnobs.insert(make_pair(groupKnob->typeName(), GroupKnobPlugin));
    delete groupKnob;

    Knob *rgbaKnob = Color_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> RGBAfunctions;
    RGBAfunctions.insert(make_pair("BuildKnob", (void *)&Color_Knob::BuildKnob));
    LibraryBinary *RGBAKnobPlugin = new LibraryBinary(RGBAfunctions);
    _loadedKnobs.insert(make_pair(rgbaKnob->typeName(), RGBAKnobPlugin));
    delete rgbaKnob;

    Knob *strKnob = String_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> STRfunctions;
    STRfunctions.insert(make_pair("BuildKnob", (void *)&String_Knob::BuildKnob));
    LibraryBinary *STRKnobPlugin = new LibraryBinary(STRfunctions);
    _loadedKnobs.insert(make_pair(strKnob->typeName(), STRKnobPlugin));
    delete strKnob;

    Knob *richTxtKnob = RichText_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> RICHTXTfunctions;
    RICHTXTfunctions.insert(make_pair("BuildKnob", (void *)&RichText_Knob::BuildKnob));
    LibraryBinary *RICHTXTKnobPlugin = new LibraryBinary(RICHTXTfunctions);
    _loadedKnobs.insert(make_pair(richTxtKnob->typeName(), RICHTXTKnobPlugin));
    delete richTxtKnob;
}


Knob *KnobFactory::createKnob(const std::string &id,
                              KnobHolder  *holder,
                              const std::string &description, int dimension) const
{

    std::map<std::string, LibraryBinary *>::const_iterator it = _loadedKnobs.find(id);
    if (it == _loadedKnobs.end()) {
        return NULL;
    } else {
        std::pair<bool, KnobBuilder> builderFunc = it->second->findFunction<KnobBuilder>("BuildKnob");
        if (!builderFunc.first) {
            return NULL;
        }
        KnobBuilder builder = (KnobBuilder)(builderFunc.second);
        Knob *knob = builder(holder, description, dimension);
        if (!knob) {
            return NULL;
        }
        return knob;
    }
}


