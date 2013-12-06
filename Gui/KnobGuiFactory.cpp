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
#include "KnobGuiFactory.h"

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"

#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobGuiFile.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/DockablePanel.h"

using namespace Natron;
using std::make_pair;
using std::pair;


/*Class inheriting KnobGui, must have a function named BuildKnobGui with the following signature.
 This function should in turn call a specific class-based static function with the appropriate param.*/
typedef Knob *(*KnobBuilder)(KnobHolder  *holder, const std::string &description, int dimension);
typedef KnobGui *(*KnobGuiBuilder)(Knob *knob, DockablePanel *);

/***********************************FACTORY******************************************/
KnobGuiFactory::KnobGuiFactory()
{
    loadKnobPlugins();
}

KnobGuiFactory::~KnobGuiFactory()
{
    for (std::map<std::string, LibraryBinary *>::iterator it = _loadedKnobs.begin(); it != _loadedKnobs.end() ; ++it) {
        delete it->second;
    }
    _loadedKnobs.clear();
}

void KnobGuiFactory::loadKnobPlugins()
{
    std::vector<LibraryBinary *> plugins = AppManager::loadPlugins(NATRON_KNOBS_PLUGINS_PATH);
    std::vector<std::string> functions;
    functions.push_back("BuildKnobGui");
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

void KnobGuiFactory::loadBultinKnobs()
{
    std::string stub;
    Knob *fileKnob = File_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> FILEfunctions;
    FILEfunctions.insert(make_pair("BuildKnobGui", (void *)&File_KnobGui::BuildKnobGui));
    LibraryBinary *FILEKnobPlugin = new LibraryBinary(FILEfunctions);
    _loadedKnobs.insert(make_pair(fileKnob->typeName(), FILEKnobPlugin));
    delete fileKnob;

    Knob *intKnob = Int_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> INTfunctions;
    INTfunctions.insert(make_pair("BuildKnobGui", (void *)&Int_KnobGui::BuildKnobGui));
    LibraryBinary *INTKnobPlugin = new LibraryBinary(INTfunctions);
    _loadedKnobs.insert(make_pair(intKnob->typeName(), INTKnobPlugin));
    delete intKnob;


    Knob *doubleKnob = Double_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> DOUBLEfunctions;
    DOUBLEfunctions.insert(make_pair("BuildKnobGui", (void *)&Double_KnobGui::BuildKnobGui));
    LibraryBinary *DOUBLEKnobPlugin = new LibraryBinary(DOUBLEfunctions);
    _loadedKnobs.insert(make_pair(doubleKnob->typeName(), DOUBLEKnobPlugin));
    delete doubleKnob;

    Knob *boolKnob = Bool_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> BOOLfunctions;
    BOOLfunctions.insert(make_pair("BuildKnobGui", (void *)&Bool_KnobGui::BuildKnobGui));
    LibraryBinary *BOOLKnobPlugin = new LibraryBinary(BOOLfunctions);
    _loadedKnobs.insert(make_pair(boolKnob->typeName(), BOOLKnobPlugin));
    delete boolKnob;

    Knob *buttonKnob = Button_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> BUTTONfunctions;
    BUTTONfunctions.insert(make_pair("BuildKnobGui", (void *)&Button_KnobGui::BuildKnobGui));
    LibraryBinary *BUTTONKnobPlugin = new LibraryBinary(BUTTONfunctions);
    _loadedKnobs.insert(make_pair(buttonKnob->typeName(), BUTTONKnobPlugin));
    delete buttonKnob;

    Knob *outputFileKnob = OutputFile_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> OFfunctions;
    OFfunctions.insert(make_pair("BuildKnobGui", (void *)&OutputFile_KnobGui::BuildKnobGui));
    LibraryBinary *OUTPUTFILEKnobPlugin = new LibraryBinary(OFfunctions);
    _loadedKnobs.insert(make_pair(outputFileKnob->typeName(), OUTPUTFILEKnobPlugin));
    delete outputFileKnob;

    Knob *comboBoxKnob = ComboBox_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> CBBfunctions;
    CBBfunctions.insert(make_pair("BuildKnobGui", (void *)&ComboBox_KnobGui::BuildKnobGui));
    LibraryBinary *ComboBoxKnobPlugin = new LibraryBinary(CBBfunctions);
    _loadedKnobs.insert(make_pair(comboBoxKnob->typeName(), ComboBoxKnobPlugin));
    delete comboBoxKnob;


    Knob *separatorKnob = Separator_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> Sepfunctions;
    Sepfunctions.insert(make_pair("BuildKnobGui", (void *)&Separator_KnobGui::BuildKnobGui));
    LibraryBinary *SeparatorKnobPlugin = new LibraryBinary(Sepfunctions);
    _loadedKnobs.insert(make_pair(separatorKnob->typeName(), SeparatorKnobPlugin));
    delete separatorKnob;

    Knob *groupKnob = Group_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> Grpfunctions;
    Grpfunctions.insert(make_pair("BuildKnobGui", (void *)&Group_KnobGui::BuildKnobGui));
    LibraryBinary *GroupKnobPlugin = new LibraryBinary(Grpfunctions);
    _loadedKnobs.insert(make_pair(groupKnob->typeName(), GroupKnobPlugin));
    delete groupKnob;

    Knob *rgbaKnob = Color_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> RGBAfunctions;
    RGBAfunctions.insert(make_pair("BuildKnobGui", (void *)&Color_KnobGui::BuildKnobGui));
    LibraryBinary *RGBAKnobPlugin = new LibraryBinary(RGBAfunctions);
    _loadedKnobs.insert(make_pair(rgbaKnob->typeName(), RGBAKnobPlugin));
    delete rgbaKnob;

    Knob *strKnob = String_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> STRfunctions;
    STRfunctions.insert(make_pair("BuildKnobGui", (void *)&String_KnobGui::BuildKnobGui));
    LibraryBinary *STRKnobPlugin = new LibraryBinary(STRfunctions);
    _loadedKnobs.insert(make_pair(strKnob->typeName(), STRKnobPlugin));
    delete strKnob;

    Knob *richTxtKnob = RichText_Knob::BuildKnob(NULL, stub, 1);

    std::map<std::string, void *> RICHTXTfunctions;
    RICHTXTfunctions.insert(make_pair("BuildKnobGui", (void *)&RichText_KnobGui::BuildKnobGui));
    LibraryBinary *RICHTXTKnobPlugin = new LibraryBinary(RICHTXTfunctions);
    _loadedKnobs.insert(make_pair(richTxtKnob->typeName(), RICHTXTKnobPlugin));
    delete richTxtKnob;
}


KnobGui *KnobGuiFactory::createGuiForKnob(Knob *knob, DockablePanel *container) const
{
    assert(knob);
    std::map<std::string, LibraryBinary *>::const_iterator it = _loadedKnobs.find(knob->typeName());
    if (it == _loadedKnobs.end()) {
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

