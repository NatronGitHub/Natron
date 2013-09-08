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
#ifdef __POWITER_UNIX__
#include <dlfcn.h>
#endif
#include <iostream>


#include "Engine/LibraryBinary.h"
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
    QDir d(POWITER_PLUGINS_PATH);
    if (d.isReadable())
    {
        QStringList filters;
        filters << QString(QString("*.")+QString(LIBRARY_EXT));
        d.setNameFilters(filters);
		QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ; ++i) {
            QString filename = fileList.at(i);
            if(filename.endsWith(".dll") || filename.endsWith(".dylib") || filename.endsWith(".so")){
                QString className;
                int index = filename.lastIndexOf("."LIBRARY_EXT);
                className = filename.left(index);
                string dll = POWITER_PLUGINS_PATH + className.toStdString() + "." + LIBRARY_EXT;
                
                vector<string> functions;
                functions.push_back("BuildKnob");
                LibraryBinary* plugin = new LibraryBinary(dll,functions);
                if(!plugin->isValid()){
                    delete plugin;
                }
                pair<bool,KnobBuilder> builder = plugin->findFunction<KnobBuilder>("BuildKnob");
                if(!builder.first){
                    continue;
                }else{
                    Knob* knob = builder.second(NULL,str,1,0);
                    _loadedKnobs.insert(make_pair(knob->name(), plugin));
                    delete knob;
                }
            }else{
                continue;
            }
        }
    }
    loadBultinKnobs();
    
}

void KnobFactory::loadBultinKnobs(){
    std::string stub;
    Knob* fileKnob = File_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> FILEfunctions;
    FILEfunctions.insert(make_pair("builder",(HINSTANCE)&File_Knob::BuildKnob));
    FILEfunctions.insert(make_pair("guibuilder",(HINSTANCE)&File_KnobGui::BuildKnobGui));
    PluginID *FILEKnobPlugin = new PluginID(FILEfunctions,fileKnob->name().c_str());
#else
    std::map<std::string,void*> FILEfunctions;
    FILEfunctions.insert(make_pair("builder",(void*)&File_Knob::BuildKnob));
    FILEfunctions.insert(make_pair("guibuilder",(void*)&File_KnobGui::BuildKnobGui));
    PluginID *FILEKnobPlugin = new PluginID(FILEfunctions,fileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(fileKnob->name(),FILEKnobPlugin));
    delete fileKnob;
    
    Knob* intKnob = Int_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> INTfunctions;
    INTfunctions.insert(make_pair("builder",(HINSTANCE)&Int_Knob::BuildKnob));
    INTfunctions.insert(make_pair("guibuilder",(HINSTANCE)&Int_KnobGui::BuildKnobGui));
    PluginID *INTKnobPlugin = new PluginID(INTfunctions,intKnob->name().c_str());
#else
    std::map<std::string,void*> INTfunctions;
    INTfunctions.insert(make_pair("builder",(void*)&Int_Knob::BuildKnob));
    INTfunctions.insert(make_pair("guibuilder",(void*)&Int_KnobGui::BuildKnobGui));
    PluginID *INTKnobPlugin = new PluginID(INTfunctions,intKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(intKnob->name(),INTKnobPlugin));
    delete intKnob;
    
    
    Knob* doubleKnob = Double_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> DOUBLEfunctions;
    DOUBLEfunctions.insert(make_pair("builder",(HINSTANCE)&Double_Knob::BuildKnob));
    DOUBLEfunctions.insert(make_pair("guibuilder",(HINSTANCE)&Double_KnobGui::BuildKnobGui));
    PluginID *DOUBLEKnobPlugin = new PluginID(DOUBLEfunctions,doubleKnob->name().c_str());
#else
    std::map<std::string,void*> DOUBLEfunctions;
    DOUBLEfunctions.insert(make_pair("builder",(void*)&Double_Knob::BuildKnob));
    DOUBLEfunctions.insert(make_pair("guibuilder",(void*)&Double_KnobGui::BuildKnobGui));
    PluginID *DOUBLEKnobPlugin = new PluginID(DOUBLEfunctions,doubleKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(doubleKnob->name(),DOUBLEKnobPlugin));
    delete doubleKnob;
    
    Knob* boolKnob = Bool_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> BOOLfunctions;
    BOOLfunctions.insert(make_pair("builder",(HINSTANCE)&Bool_Knob::BuildKnob));
    BOOLfunctions.insert(make_pair("guibuilder",(HINSTANCE)&Bool_KnobGui::BuildKnobGui));
    PluginID *BOOLKnobPlugin = new PluginID(BOOLfunctions,boolKnob->name().c_str());
#else
    std::map<std::string,void*> BOOLfunctions;
    BOOLfunctions.insert(make_pair("builder",(void*)&Bool_Knob::BuildKnob));
    BOOLfunctions.insert(make_pair("guibuilder",(void*)&Bool_KnobGui::BuildKnobGui));
    PluginID *BOOLKnobPlugin = new PluginID(BOOLfunctions,boolKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(boolKnob->name(),BOOLKnobPlugin));
    delete boolKnob;
    
    Knob* buttonKnob = Button_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> BUTTONfunctions;
    BUTTONfunctions.insert(make_pair("builder",(HINSTANCE)&Button_Knob::BuildKnob));
    BUTTONfunctions.insert(make_pair("guibuilder",(HINSTANCE)&Button_KnobGui::BuildKnobGui));
    PluginID *BUTTONKnobPlugin = new PluginID(BUTTONfunctions,buttonKnob->name().c_str());
#else
    std::map<std::string,void*> BUTTONfunctions;
    BUTTONfunctions.insert(make_pair("builder",(void*)&Button_Knob::BuildKnob));
    BUTTONfunctions.insert(make_pair("guibuilder",(void*)&Button_KnobGui::BuildKnobGui));
    PluginID *BUTTONKnobPlugin = new PluginID(BUTTONfunctions,buttonKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(buttonKnob->name(),BUTTONKnobPlugin));
    delete buttonKnob;
    
    Knob* outputFileKnob = OutputFile_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> OFfunctions;
    OFfunctions.insert(make_pair("builder",(HINSTANCE)&OutputFile_Knob::BuildKnob));
    OFfunctions.insert(make_pair("guibuilder",(HINSTANCE)&OutputFile_KnobGui::BuildKnobGui));
    PluginID *OUTPUTFILEKnobPlugin = new PluginID(OFfunctions,outputFileKnob->name().c_str());
#else
    std::map<std::string,void*> OFfunctions;
    OFfunctions.insert(make_pair("builder",(void*)&OutputFile_Knob::BuildKnob));
    OFfunctions.insert(make_pair("guibuilder",(void*)&OutputFile_KnobGui::BuildKnobGui));
    PluginID *OUTPUTFILEKnobPlugin = new PluginID(OFfunctions,outputFileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(outputFileKnob->name(),OUTPUTFILEKnobPlugin));
    delete outputFileKnob;
    
    Knob* comboBoxKnob = ComboBox_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> CBBfunctions;
    CBBfunctions.insert(make_pair("builder",(HINSTANCE)&ComboBox_Knob::BuildKnob));
    CBBfunctions.insert(make_pair("guibuilder",(HINSTANCE)&ComboBox_KnobGui::BuildKnobGui));
    PluginID *ComboBoxKnobPlugin = new PluginID(CBBfunctions,comboBoxKnob->name().c_str());
#else
    std::map<std::string,void*> CBBfunctions;
    CBBfunctions.insert(make_pair("builder",(void*)&ComboBox_Knob::BuildKnob));
    CBBfunctions.insert(make_pair("guibuilder",(void*)&ComboBox_KnobGui::BuildKnobGui));
    PluginID *ComboBoxKnobPlugin = new PluginID(CBBfunctions,comboBoxKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(comboBoxKnob->name(),ComboBoxKnobPlugin));
    delete comboBoxKnob;
    
    
    Knob* separatorKnob = Separator_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> Sepfunctions;
    Sepfunctions.insert(make_pair("builder",(HINSTANCE)&Separator_Knob::BuildKnob));
    Sepfunctions.insert(make_pair("guibuilder",(HINSTANCE)&Separator_KnobGui::BuildKnobGui));
    PluginID *SeparatorKnobPlugin = new PluginID(Sepfunctions,separatorKnob->name().c_str());
#else
    std::map<std::string,void*> Sepfunctions;
    Sepfunctions.insert(make_pair("builder",(void*)&Separator_Knob::BuildKnob));
    Sepfunctions.insert(make_pair("guibuilder",(void*)&Separator_KnobGui::BuildKnobGui));
    PluginID *SeparatorKnobPlugin = new PluginID(Sepfunctions,separatorKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(separatorKnob->name(),SeparatorKnobPlugin));
    delete separatorKnob;
    
    Knob* groupKnob = Group_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> Grpfunctions;
    Grpfunctions.insert(make_pair("builder",(HINSTANCE)&Group_Knob::BuildKnob));
    Grpfunctions.insert(make_pair("guibuilder",(HINSTANCE)&Group_KnobGui::BuildKnobGui));
    PluginID *GroupKnobPlugin = new PluginID(Grpfunctions,groupKnob->name().c_str());
#else
    std::map<std::string,void*> Grpfunctions;
    Grpfunctions.insert(make_pair("builder",(void*)&Group_Knob::BuildKnob));
    Grpfunctions.insert(make_pair("guibuilder",(void*)&Group_KnobGui::BuildKnobGui));
    PluginID *GroupKnobPlugin = new PluginID(Grpfunctions,groupKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(groupKnob->name(),GroupKnobPlugin));
    delete groupKnob;
    
    Knob* rgbaKnob = RGBA_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> RGBAfunctions;
    RGBAfunctions.insert(make_pair("builder",(HINSTANCE)&RGBA_Knob::BuildKnob));
    RGBAfunctions.insert(make_pair("guibuilder",(HINSTANCE)&RGBA_KnobGui::BuildKnobGui));
    PluginID *RGBAKnobPlugin = new PluginID(RGBAfunctions,rgbaKnob->name().c_str());
#else
    std::map<std::string,void*> RGBAfunctions;
    RGBAfunctions.insert(make_pair("builder",(void*)&RGBA_Knob::BuildKnob));
    RGBAfunctions.insert(make_pair("guibuilder",(void*)&RGBA_KnobGui::BuildKnobGui));
    PluginID *RGBAKnobPlugin = new PluginID(RGBAfunctions,rgbaKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(rgbaKnob->name(),RGBAKnobPlugin));
    delete rgbaKnob;
    
    
    Knob* tabKnob = Tab_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> TABfunctions;
    TABfunctions.insert(make_pair("builder",(HINSTANCE)&Tab_Knob::BuildKnob));
    TABfunctions.insert(make_pair("guibuilder",(HINSTANCE)&Tab_KnobGui::BuildKnobGui));
    PluginID *TABKnobPlugin = new PluginID(TABfunctions,tabKnob->name().c_str());
#else
    std::map<std::string,void*> TABfunctions;
    TABfunctions.insert(make_pair("builder",(void*)&Tab_Knob::BuildKnob));
    TABfunctions.insert(make_pair("guibuilder",(void*)&Tab_KnobGui::BuildKnobGui));
    PluginID *TABKnobPlugin = new PluginID(TABfunctions,tabKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(tabKnob->name(),TABKnobPlugin));
    delete tabKnob;
    Knob* strKnob = String_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    std::map<std::string,HINSTANCE> STRfunctions;
    STRfunctions.insert(make_pair("builder",(HINSTANCE)&String_Knob::BuildKnob));
    STRfunctions.insert(make_pair("guibuilder",(HINSTANCE)&String_KnobGui::BuildKnobGui));
    PluginID *STRKnobPlugin = new PluginID(STRfunctions,strKnob->name().c_str());
#else
    std::map<std::string,void*> STRfunctions;
    STRfunctions.insert(make_pair("builder",(void*)&String_Knob::BuildKnob));
    STRfunctions.insert(make_pair("guibuilder",(void*)&String_KnobGui::BuildKnobGui));
    PluginID *STRKnobPlugin = new PluginID(STRfunctions,strKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(strKnob->name(),STRKnobPlugin));
    delete strKnob;
    
    
}

/*Calls the unique instance of the KnobFactory and
 calls the appropriate pointer to function to create a knob.*/
Knob* KnobFactory::createKnob(const std::string& name, Node* node, const std::string& description,int dimension, Knob_Mask flags){
    const std::map<std::string,PluginID*>& loadedPlugins = KnobFactory::instance()->getLoadedKnobs();
    std::map<std::string,PluginID*>::const_iterator it = loadedPlugins.find(name);
    if(it == loadedPlugins.end()){
        return NULL;
    }else{
        Knob* knob = 0;
        KnobGui* knobGui = 0;
        std::pair<bool,KnobBuilder> builderFunc = it->second->findFunction<KnobBuilder>("builder");
        if(!builderFunc.first){
            return NULL;
        }
        KnobBuilder builder = (KnobBuilder)(builderFunc.second);
        knob = builder(node,description,dimension,flags);
        
        std::pair<bool,KnobGuiBuilder> guiBuilderFunc = it->second->findFunction<KnobGuiBuilder>("guibuilder");
        if(!guiBuilderFunc.first){
            return NULL;
        }
        KnobGuiBuilder guiBuilder = (KnobGuiBuilder)(guiBuilderFunc.second);
        knobGui = guiBuilder(knob);
        
        if(!knob || !knobGui)
            return NULL;
        
        KnobInstance* instance = new KnobInstance(knob,knobGui,node->getNodeInstance());
        node->getNodeInstance()->addKnobInstance(instance);
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
