//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/
#include "KnobInstance.h"


#include "Global/GlobalDefines.h"
#include "Global/NodeInstance.h"
#include "Global/AppManager.h"
#ifdef __POWITER_UNIX__
#include <dlfcn.h>
#endif
#include <iostream>


#include "Global/LibraryBinary.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"

#include "Gui/KnobGui.h"

#include <QtCore/QDir>


using namespace std;
using namespace Powiter;
/*Class inheriting Knob and KnobGui, must have a function named BuildKnob and BuildKnobGui with the following signature.
 This function should in turn call a specific class-based static function with the appropriate param.*/
typedef Knob* (*KnobBuilder)(Node* node,const std::string& description,int dimension,Knob_Mask flags);

typedef KnobGui* (*KnobGuiBuilder)(Knob* knob);

/***********************************FACTORY******************************************/
KnobFactory::KnobFactory(){
    loadKnobPlugins();
}

KnobFactory::~KnobFactory(){
    for ( std::map<std::string,LibraryBinary*>::iterator it = _loadedKnobs.begin(); it!=_loadedKnobs.end() ; ++it) {
        delete it->second;
    }
    _loadedKnobs.clear();
}

void KnobFactory::loadKnobPlugins(){
    vector<LibraryBinary*> plugins = AppManager::loadPlugins(POWITER_KNOBS_PLUGINS_PATH);
    vector<string> functions;
    functions.push_back("BuildKnob");
    functions.push_back("BuildKnobGui");
    for (U32 i = 0; i < plugins.size(); ++i) {
        if (plugins[i]->loadFunctions(functions)) {
            pair<bool,KnobBuilder> builder = plugins[i]->findFunction<KnobBuilder>("BuildKnob");
            if(builder.first){
                 Knob* knob = builder.second(NULL,"",1,0);
                _loadedKnobs.insert(make_pair(knob->name(), plugins[i]));
                delete knob;
            }
        }else{
            delete plugins[i];
        }
    }
    loadBultinKnobs();
}

void KnobFactory::loadBultinKnobs(){
    std::string stub;
    Knob* fileKnob = File_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> FILEfunctions;
    FILEfunctions.insert(make_pair("BuildKnob",(void*)&File_Knob::BuildKnob));
    FILEfunctions.insert(make_pair("BuildKnobGui",(void*)&File_KnobGui::BuildKnobGui));
    LibraryBinary *FILEKnobPlugin = new LibraryBinary(FILEfunctions);
    _loadedKnobs.insert(make_pair(fileKnob->name(),FILEKnobPlugin));
    delete fileKnob;
    
    Knob* intKnob = Int_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> INTfunctions;
    INTfunctions.insert(make_pair("BuildKnob",(void*)&Int_Knob::BuildKnob));
    INTfunctions.insert(make_pair("BuildKnobGui",(void*)&Int_KnobGui::BuildKnobGui));
    LibraryBinary *INTKnobPlugin = new LibraryBinary(INTfunctions);
    _loadedKnobs.insert(make_pair(intKnob->name(),INTKnobPlugin));
    delete intKnob;
    
    
    Knob* doubleKnob = Double_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> DOUBLEfunctions;
    DOUBLEfunctions.insert(make_pair("BuildKnob",(void*)&Double_Knob::BuildKnob));
    DOUBLEfunctions.insert(make_pair("BuildKnobGui",(void*)&Double_KnobGui::BuildKnobGui));
    LibraryBinary *DOUBLEKnobPlugin = new LibraryBinary(DOUBLEfunctions);
    _loadedKnobs.insert(make_pair(doubleKnob->name(),DOUBLEKnobPlugin));
    delete doubleKnob;
    
    Knob* boolKnob = Bool_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> BOOLfunctions;
    BOOLfunctions.insert(make_pair("BuildKnob",(void*)&Bool_Knob::BuildKnob));
    BOOLfunctions.insert(make_pair("BuildKnobGui",(void*)&Bool_KnobGui::BuildKnobGui));
    LibraryBinary *BOOLKnobPlugin = new LibraryBinary(BOOLfunctions);
    _loadedKnobs.insert(make_pair(boolKnob->name(),BOOLKnobPlugin));
    delete boolKnob;
    
    Knob* buttonKnob = Button_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> BUTTONfunctions;
    BUTTONfunctions.insert(make_pair("BuildKnob",(void*)&Button_Knob::BuildKnob));
    BUTTONfunctions.insert(make_pair("BuildKnobGui",(void*)&Button_KnobGui::BuildKnobGui));
    LibraryBinary *BUTTONKnobPlugin = new LibraryBinary(BUTTONfunctions);
    _loadedKnobs.insert(make_pair(buttonKnob->name(),BUTTONKnobPlugin));
    delete buttonKnob;
    
    Knob* outputFileKnob = OutputFile_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> OFfunctions;
    OFfunctions.insert(make_pair("BuildKnob",(void*)&OutputFile_Knob::BuildKnob));
    OFfunctions.insert(make_pair("BuildKnobGui",(void*)&OutputFile_KnobGui::BuildKnobGui));
    LibraryBinary *OUTPUTFILEKnobPlugin = new LibraryBinary(OFfunctions);
    _loadedKnobs.insert(make_pair(outputFileKnob->name(),OUTPUTFILEKnobPlugin));
    delete outputFileKnob;
    
    Knob* comboBoxKnob = ComboBox_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> CBBfunctions;
    CBBfunctions.insert(make_pair("BuildKnob",(void*)&ComboBox_Knob::BuildKnob));
    CBBfunctions.insert(make_pair("BuildKnobGui",(void*)&ComboBox_KnobGui::BuildKnobGui));
    LibraryBinary *ComboBoxKnobPlugin = new LibraryBinary(CBBfunctions);
    _loadedKnobs.insert(make_pair(comboBoxKnob->name(),ComboBoxKnobPlugin));
    delete comboBoxKnob;
    
    
    Knob* separatorKnob = Separator_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> Sepfunctions;
    Sepfunctions.insert(make_pair("BuildKnob",(void*)&Separator_Knob::BuildKnob));
    Sepfunctions.insert(make_pair("BuildKnobGui",(void*)&Separator_KnobGui::BuildKnobGui));
    LibraryBinary *SeparatorKnobPlugin = new LibraryBinary(Sepfunctions);
    _loadedKnobs.insert(make_pair(separatorKnob->name(),SeparatorKnobPlugin));
    delete separatorKnob;
    
    Knob* groupKnob = Group_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> Grpfunctions;
    Grpfunctions.insert(make_pair("BuildKnob",(void*)&Group_Knob::BuildKnob));
    Grpfunctions.insert(make_pair("BuildKnobGui",(void*)&Group_KnobGui::BuildKnobGui));
    LibraryBinary *GroupKnobPlugin = new LibraryBinary(Grpfunctions);
    _loadedKnobs.insert(make_pair(groupKnob->name(),GroupKnobPlugin));
    delete groupKnob;
    
    Knob* rgbaKnob = RGBA_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> RGBAfunctions;
    RGBAfunctions.insert(make_pair("BuildKnob",(void*)&RGBA_Knob::BuildKnob));
    RGBAfunctions.insert(make_pair("BuildKnobGui",(void*)&RGBA_KnobGui::BuildKnobGui));
    LibraryBinary *RGBAKnobPlugin = new LibraryBinary(RGBAfunctions);
    _loadedKnobs.insert(make_pair(rgbaKnob->name(),RGBAKnobPlugin));
    delete rgbaKnob;
    
    
    Knob* tabKnob = Tab_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> TABfunctions;
    TABfunctions.insert(make_pair("BuildKnob",(void*)&Tab_Knob::BuildKnob));
    TABfunctions.insert(make_pair("BuildKnobGui",(void*)&Tab_KnobGui::BuildKnobGui));
    LibraryBinary *TABKnobPlugin = new LibraryBinary(TABfunctions);
    _loadedKnobs.insert(make_pair(tabKnob->name(),TABKnobPlugin));
    delete tabKnob;
    Knob* strKnob = String_Knob::BuildKnob(NULL,stub,1,0);

    std::map<std::string,void*> STRfunctions;
    STRfunctions.insert(make_pair("BuildKnob",(void*)&String_Knob::BuildKnob));
    STRfunctions.insert(make_pair("BuildKnobGui",(void*)&String_KnobGui::BuildKnobGui));
    LibraryBinary *STRKnobPlugin = new LibraryBinary(STRfunctions);
    _loadedKnobs.insert(make_pair(strKnob->name(),STRKnobPlugin));
    delete strKnob;
    
    
}

/*Calls the unique instance of the KnobFactory and
 calls the appropriate pointer to function to create a knob.*/
Knob* KnobFactory::createKnob(const std::string& name, Node* node, const std::string& description,int dimension, Knob_Mask flags){
    const std::map<std::string,LibraryBinary*>& loadedPlugins = KnobFactory::instance()->getLoadedKnobs();
    std::map<std::string,LibraryBinary*>::const_iterator it = loadedPlugins.find(name);
    if(it == loadedPlugins.end()){
        return NULL;
    }else{
        Knob* knob = 0;
        KnobGui* knobGui = 0;
        std::pair<bool,KnobBuilder> builderFunc = it->second->findFunction<KnobBuilder>("BuildKnob");
        if(!builderFunc.first){
            return NULL;
        }
        KnobBuilder builder = (KnobBuilder)(builderFunc.second);
        knob = builder(node,description,dimension,flags);
        
        std::pair<bool,KnobGuiBuilder> guiBuilderFunc = it->second->findFunction<KnobGuiBuilder>("BuildKnobGui");
        if(!guiBuilderFunc.first){
            return NULL;
        }
        KnobGuiBuilder guiBuilder = (KnobGuiBuilder)(guiBuilderFunc.second);
        knobGui = guiBuilder(knob);
        if(!knob || !knobGui)
            return NULL;
        
        KnobInstance* instance = 0;
        if(knob->name() == "Tab"){
            instance = new Tab_KnobInstance(knob,knobGui,node->getNodeInstance());
        }else if(knob->name() == "Group"){
            instance = new Group_KnobInstance(knob,knobGui,node->getNodeInstance());
        }else{
            instance = new KnobInstance(knob,knobGui,node->getNodeInstance());
        }
        node->getNodeInstance()->addKnobInstance(instance);
        knob->setKnobInstancePTR(instance);

        return knob;
    }
}

/**************KNOB_INSTANCE*********************/

KnobInstance::~KnobInstance(){
    _knob->getNode()->getNodeInstance()->removeKnobInstance(this);
    if(_gui){
        delete _gui;
    }
    delete _knob;
}

/*Turn off the  new line for the gui's layout
 before inserting the knob in the interface.*/
void KnobInstance::turnOffNewLine(){
    _gui->turnOffNewLine();
}

/*Set the spacing between items in the knob
 gui.*/
void KnobInstance::setSpacingBetweenItems(int spacing){
    _gui->setSpacingBetweenItems(spacing);
}

/*Whether the knob should be visible on the gui.*/
void KnobInstance::setVisible(bool b){
    _gui->setVisible(b);
}

/*Whether the knob should be enabled. When disabled
 the user will not be able to interact with it and
 the value will not refresh.*/
void KnobInstance::setEnabled(bool b){
    _gui->setEnabled(b);
}


void Group_KnobInstance::addKnob(Knob* knob){
    dynamic_cast<Group_KnobGui*>(_gui)->addKnob(knob->getKnobInstance()->getKnobGui());
}

void Tab_KnobInstance::addTab(const std::string& name){
    dynamic_cast<Tab_KnobGui*>(_gui)->addTab(name);

}

void Tab_KnobInstance::addKnob(const std::string& tabName,Knob* k){
    dynamic_cast<Tab_KnobGui*>(_gui)->addKnob(tabName,k->getKnobInstance()->getKnobGui());

}
