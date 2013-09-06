//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#include "Knob.h"

#ifdef __POWITER_UNIX__
#include <dlfcn.h>
#endif

#include <iostream>

#include <QtCore/QDir>
#include <QtGui/QVector4D>

#include "Engine/PluginID.h"
#include "Engine/Node.h"
#include "Engine/Model.h"

#include "Global/AppManager.h"

#include "Readers/Reader.h"

#include "Gui/SequenceFileDialog.h"


using namespace std;
using namespace Powiter;

std::vector<Knob::Knob_Flags> Knob_Mask_to_Knobs_Flags(const Knob_Mask&m) {
    unsigned int i=0x1;
    std::vector<Knob::Knob_Flags> flags;
    if(m!=0){
        while(i<0x4){
            if((m & i)==i){
                flags.push_back((Knob::Knob_Flags)i);
            }
            i <<= 2;
        }
    }
    return flags;
}

/***********************************FACTORY******************************************/
KnobFactory::KnobFactory(){
    loadKnobPlugins();
}

KnobFactory::~KnobFactory(){
    for ( std::map<std::string,PluginID*>::iterator it = _loadedKnobs.begin(); it!=_loadedKnobs.end() ; ++it) {
        delete it->second;
    }
    _loadedKnobs.clear();
}

void KnobFactory::loadKnobPlugins(){
    QDir d(POWITER_PLUGINS_PATH);
    if (d.isReadable())
    {
        QStringList filters;
#ifdef __POWITER_WIN32__
        filters << "*.dll";
#elif defined(__POWITER_OSX__)
        filters << "*.dylib";
#elif defined(__POWITER_LINUX__)
        filters << "*.so";
#endif
        d.setNameFilters(filters);
		QStringList fileList = d.entryList();
        for(int i = 0 ; i < fileList.size() ; ++i) {
            QString filename = fileList.at(i);
            if(filename.contains(".dll") || filename.contains(".dylib") || filename.contains(".so")){
                QString className;
                int index = filename.size() -1;
                while(filename.at(index) != QChar('.')) {
                    --index;
                }
                className = filename.left(index);
                PluginID* plugin = 0;
#ifdef __POWITER_WIN32__
                HINSTANCE lib;
                string dll;
                dll.append(POWITER_PLUGINS_PATH);
                dll.append(className.toStdString());
                dll.append(".dll");
                lib=LoadLibrary(dll.c_str());
                if(lib==NULL){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    KnobBuilder builder=(KnobBuilder)GetProcAddress(lib,"BuildRead");
                    if(builder!=NULL){
                        std::string str("");
                        Knob* knob=builder(NULL,str,1,0);
                        plugin = new PluginID((HINSTANCE)builder,knob->name().c_str());
                        _loadedKnobs.insert(make_pair(knob->name(),plugin));
                        delete knob;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildKnob" << endl;
                        continue;
                    }
                    
                }
                
#elif defined(__POWITER_UNIX__)
                string dll;
                dll.append(POWITER_PLUGINS_PATH);
                dll.append(className.toStdString());
#ifdef __POWITER_OSX__
                dll.append(".dylib");
#elif defined(__POWITER_LINUX__)
                dll.append(".so");
#endif
                void* lib=dlopen(dll.c_str(),RTLD_LAZY);
                if(!lib){
                    cout << " couldn't open library " << qPrintable(className) << endl;
                }
                else{
                    // successfully loaded the library, we create now an instance of the class
                    //to find out the extensions it can decode and the name of the decoder
                    KnobBuilder builder=(KnobBuilder)dlsym(lib,"BuildKnob");
                    if(builder!=NULL){
                        std::string str("");
                        Knob* knob=builder(NULL,str,1,0);
                        plugin = new PluginID((void*)builder,knob->name().c_str());
                        _loadedKnobs.insert(make_pair(knob->name(),plugin));
                        delete knob;
                        
                    }else{
                        cout << "RunTime: couldn't call " << "BuildKnob" << endl;
                        continue;
                    }
                }
#endif
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
    PluginID *FILEKnobPlugin = new PluginID((HINSTANCE)&File_Knob::BuildKnob,fileKnob->name().c_str());
#else
    PluginID *FILEKnobPlugin = new PluginID((void*)&File_Knob::BuildKnob,fileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(fileKnob->name(),FILEKnobPlugin));
    delete fileKnob;
    
    Knob* intKnob = Int_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *INTKnobPlugin = new PluginID((HINSTANCE)&Int_Knob::BuildKnob,intKnob->name().c_str());
#else
    PluginID *INTKnobPlugin = new PluginID((void*)&Int_Knob::BuildKnob,intKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(intKnob->name(),INTKnobPlugin));
    delete intKnob;
    
    
    Knob* doubleKnob = Double_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *DOUBLEKnobPlugin = new PluginID((HINSTANCE)&Double_Knob::BuildKnob,doubleKnob->name().c_str());
#else
    PluginID *DOUBLEKnobPlugin = new PluginID((void*)&Double_Knob::BuildKnob,doubleKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(doubleKnob->name(),DOUBLEKnobPlugin));
    delete doubleKnob;
    
    Knob* boolKnob = Bool_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *BOOLKnobPlugin = new PluginID((HINSTANCE)&Bool_Knob::BuildKnob,boolKnob->name().c_str());
#else
    PluginID *BOOLKnobPlugin = new PluginID((void*)&Bool_Knob::BuildKnob,boolKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(boolKnob->name(),BOOLKnobPlugin));
    delete boolKnob;
    
    Knob* buttonKnob = Button_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *BUTTONKnobPlugin = new PluginID((HINSTANCE)&Button_Knob::BuildKnob,buttonKnob->name().c_str());
#else
    PluginID *BUTTONKnobPlugin = new PluginID((void*)&Button_Knob::BuildKnob,buttonKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(buttonKnob->name(),BUTTONKnobPlugin));
    delete buttonKnob;
    
    Knob* outputFileKnob = OutputFile_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *OUTPUTFILEKnobPlugin = new PluginID((HINSTANCE)&OutputFile_Knob::BuildKnob,outputFileKnob->name().c_str());
#else
    PluginID *OUTPUTFILEKnobPlugin = new PluginID((void*)&OutputFile_Knob::BuildKnob,outputFileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(outputFileKnob->name(),OUTPUTFILEKnobPlugin));
    delete outputFileKnob;
    
    Knob* comboBoxKnob = ComboBox_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *ComboBoxKnobPlugin = new PluginID((HINSTANCE)&ComboBox_Knob::BuildKnob,comboBoxKnob->name().c_str());
#else
    PluginID *ComboBoxKnobPlugin = new PluginID((void*)&ComboBox_Knob::BuildKnob,comboBoxKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(comboBoxKnob->name(),ComboBoxKnobPlugin));
    delete comboBoxKnob;
    
    
    Knob* separatorKnob = Separator_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *SeparatorKnobPlugin = new PluginID((HINSTANCE)&Separator_Knob::BuildKnob,separatorKnob->name().c_str());
#else
    PluginID *SeparatorKnobPlugin = new PluginID((void*)&Separator_Knob::BuildKnob,separatorKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(separatorKnob->name(),SeparatorKnobPlugin));
    delete separatorKnob;
    
    Knob* groupKnob = Group_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *GroupKnobPlugin = new PluginID((HINSTANCE)&Group_Knob::BuildKnob,groupKnob->name().c_str());
#else
    PluginID *GroupKnobPlugin = new PluginID((void*)&Group_Knob::BuildKnob,groupKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(groupKnob->name(),GroupKnobPlugin));
    delete groupKnob;
    
    Knob* rgbaKnob = RGBA_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *RGBAKnobPlugin = new PluginID((HINSTANCE)&RGBA_Knob::BuildKnob,rgbaKnob->name().c_str());
#else
    PluginID *RGBAKnobPlugin = new PluginID((void*)&RGBA_Knob::BuildKnob,rgbaKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(rgbaKnob->name(),RGBAKnobPlugin));
    delete rgbaKnob;
    
    
    Knob* tabKnob = Tab_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *TABKnobPlugin = new PluginID((HINSTANCE)&Tab_Knob::BuildKnob,tabKnob->name().c_str());
#else
    PluginID *TABKnobPlugin = new PluginID((void*)&Tab_Knob::BuildKnob,tabKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(tabKnob->name(),TABKnobPlugin));
    delete tabKnob;
    
    Knob* strKnob = String_Knob::BuildKnob(NULL,stub,1,0);
#ifdef __POWITER_WIN32__
    PluginID *STRKnobPlugin = new PluginID((HINSTANCE)&String_Knob::BuildKnob,strKnob->name().c_str());
#else
    PluginID *STRKnobPlugin = new PluginID((void*)&String_Knob::BuildKnob,strKnob->name().c_str());
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
        KnobBuilder builder = (KnobBuilder)(it->second->first);
        if(builder){
            Knob* ret = builder(node,description,dimension,flags);
            node->addToKnobVector(ret);
            return ret;
        }else{
            return NULL;
        }
    }
}

/***********************************KNOB BASE******************************************/


Knob::Knob(Node* node,const std::string& description,int dimension,Knob_Mask flags):
_value(QVariant(0)),
_node(node),
_description(description),
_flags(flags),
_dimension(dimension)
{
    _node->addToKnobVector(this);
    QObject::connect(this, SIGNAL(valueChanged(const QVariant&)), this, SIGNAL(valueChangedByUser()));
}

Knob::~Knob(){
    _node->removeKnob(this);
    emit aboutToBeDeleted();
}

Knob_Mask Knob::getFlags() const{
    return _flags;
}

void Knob::startRendering(bool initViewer){
    Node* viewer = Node::hasViewerConnected(_node);
    if(viewer){
       _node->getModel()->clearPlaybackCache();
       _node->getModel()->setCurrentGraph(dynamic_cast<OutputNode*>(viewer),true);
        int currentFrameCount = _node->getModel()->getVideoEngine()->getFrameCountForCurrentPlayback();
        if(initViewer){
            _node->getModel()->getApp()->triggerAutoSaveOnNextEngineRun();
            if (currentFrameCount > 1 || currentFrameCount == -1) {
                _node->getModel()->startVideoEngine(-1);
            }else{
                _node->getModel()->startVideoEngine(1);
            }
        }else{
            _node->getModel()->getVideoEngine()->seekRandomFrame(_node->getModel()->getCurrentOutput()->getTimeLine().currentFrame());
        }
    }
}


/***********************************FILE_KNOB*****************************************/

std::string File_Knob::serialize() const{
    return SequenceFileDialog::patternFromFilesList(_value.getQVariant().toStringList()).toStdString();
}

void File_Knob::_restoreFromString(const std::string& str){
    //    _name->setText(str.c_str());
    QStringList filesList = SequenceFileDialog::filesListFromPattern(str.c_str());
    _value.setValue(filesList);
}
void File_Knob::tryStartRendering(){
    emit filesSelected();
    QStringList files = _value.value<QStringList>();
    if(files.size() > 0){
        const std::string& className = _node->className();
        if(className == "Reader"){
            _node->getModel()->setCurrentGraph(NULL,false);
            dynamic_cast<Reader*>(_node)->showFilePreview();
        }
        startRendering(true);
    }
}

/***********************************OUTPUT_FILE_KNOB*****************************************/

std::string OutputFile_Knob::serialize() const{
    return _value.getQVariant().toString().toStdString();
}
void OutputFile_Knob::_restoreFromString(const std::string& str){
    _value.setValue(QVariant(str.c_str()));
}


void OutputFile_Knob::tryStartRendering(){
    emit filesSelected();
    startRendering(false);
}
/***********************************INT_KNOB*****************************************/
void Int_Knob::fillHashVector(){
    _hashVector.clear();
    QList<QVariant> list = _value.getQVariant().toList();
    for (int i = 0; i < list.size(); ++i) {
        _hashVector.push_back((U64)list.at(i).toInt());
    }
}


std::string Int_Knob::serialize() const{
    QList<QVariant> list = _value.getQVariant().toList();
    QString str;
    for (int i = 0; i < list.size(); ++i) {
        str.append("v");
        str.append(QString::number(i));
        str.append(" ");
        str.append(QString::number(list.at(i).toInt()));
        if(i < list.size()-1){
            str.append(" ");
        }
    }
    return str.toStdString();
}
void Int_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){
        int i = s.indexOf("v");
        QList<QVariant> values;
        while(i != -1){
            i+=3;
            QString vStr;
            while(i < s.size() && s.at(i).isDigit()){
                vStr.append(s.at(i));
                ++i;
            }
            values.append(QVariant(vStr.toInt()));
            i = s.indexOf("v",i);
        }
        _value.setValue(QVariant(values));
    }
}

const std::vector<int> Int_Knob::getValues() const {
    vector<int> ret;
    QList<QVariant> list = _value.getQVariant().toList();
    for (int i = 0; i < list.size(); ++i) {
        ret.push_back(list.at(i).toInt());
    }
    return ret;
}

void Int_Knob::tryStartRendering(){
    startRendering(false);
}

/***********************************BOOL_KNOB*****************************************/
void Bool_Knob::fillHashVector(){
    _hashVector.clear();
    _hashVector.push_back((U64)_value.getQVariant().toBool());
}


std::string Bool_Knob::serialize() const{
    bool value = _value.getQVariant().toBool();
    return value ? "1" : "0";
}
void Bool_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){
        int val = s.toInt();
        _value.setValue(QVariant((bool)val));
    }
    
}

void Bool_Knob::tryStartRendering(){
    startRendering(false);
}

/***********************************DOUBLE_KNOB*****************************************/
void Double_Knob::fillHashVector(){
    _hashVector.clear();
    QList<QVariant> list = _value.getQVariant().toList();
    for (int i = 0; i < list.size(); ++i) {
        _hashVector.push_back((U64)list.at(i).toInt());
    }
}


std::string Double_Knob::serialize() const{
    QList<QVariant> list = _value.getQVariant().toList();
    QString str;
    for (int i = 0; i < list.size(); ++i) {
        str.append("v");
        str.append(QString::number(i));
        str.append(" ");
        str.append(QString::number(list.at(i).toDouble()));
        if(i < list.size()-1){
            str.append(" ");
        }
    }
    return str.toStdString();
}
void Double_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){
        int i = s.indexOf("v");
        QList<QVariant> values;
        while(i != -1){
            i+=3;
            QString vStr;
            while(i < s.size() && s.at(i).isDigit()){
                vStr.append(s.at(i));
                ++i;
            }
            values.append(QVariant(vStr.toDouble()));
            i = s.indexOf("v",i);
        }
        _value.setValue(QVariant(values));
    }
}

const std::vector<double> Double_Knob::getValues() const {
    vector<double> ret;
    QList<QVariant> list = _value.getQVariant().toList();
    for (int i = 0; i < list.size(); ++i) {
        ret.push_back(list.at(i).toInt());
    }
    return ret;
}

void Double_Knob::tryStartRendering(){
    startRendering(false);
}
/***********************************BUTTON_KNOB*****************************************/

Button_Knob::Button_Knob(Node* node, const std::string& description,int dimension, Knob_Mask flags):
Knob(node,description,dimension,flags)
{
    QObject::connect(this,SIGNAL(valueChanged(const Variant&)),this,SLOT(connectToSlot(const Variant&)));
}

void Button_Knob::connectToSlot(const Variant& v){
    const char* str = v.getQVariant().toString().toStdString().c_str();
    QObject::connect(this,SIGNAL(valueChangedByUser()),this,SLOT(str));
}

/***********************************COMBOBOX_KNOB*****************************************/
void ComboBox_Knob::fillHashVector(){
    _hashVector.clear();
    QString out(_value.getQVariant().toString());
    for (int i =0; i< out.size(); ++i) {
        _hashVector.push_back(out.at(i).unicode());
    }
}
std::string ComboBox_Knob::serialize() const{
    return QString::number(_value.getQVariant().toInt()).toStdString();
}
void ComboBox_Knob::_restoreFromString(const std::string& str){
    _value.setValue(QVariant(QString(str.c_str()).toInt()));
}

void ComboBox_Knob::tryStartRendering(){
    startRendering(false);
}
/***********************************RGBA_KNOB*****************************************/
void RGBA_Knob::fillHashVector(){
    QVector4D values = _value.getQVariant().value<QVector4D>();
    _hashVector.clear();
    double x[1]; x[0] = values.x();
    double y[1]; y[0] = values.y();
    double z[1]; z[0] = values.z();
    double w[1]; w[0] = values.w();
    _hashVector.push_back(*(reinterpret_cast<U64*>(&(x))));
    _hashVector.push_back(*(reinterpret_cast<U64*>(&(y))));
    _hashVector.push_back(*(reinterpret_cast<U64*>(&(z))));
    _hashVector.push_back(*(reinterpret_cast<U64*>(&(w))));
}
std::string RGBA_Knob::serialize() const{
    QVector4D values = _value.getQVariant().value<QVector4D>();
    return QString("r "+ QString::number(values.x()) + " g " + QString::number(values.y()) +
                   " b " + QString::number(values.z()) + " a " +QString::number(values.w())).toStdString();
}

void RGBA_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    int i = s.indexOf("r");
    i+=2;
    QString rStr,gStr,bStr,aStr;
    while(i < s.size() && s.at(i).isDigit()){
        rStr.append(s.at(i));
        ++i;
    }
    i = s.indexOf("g");
    i+=2;
    while(i < s.size() && s.at(i).isDigit()){
        gStr.append(s.at(i));
        ++i;
    }
    
    i = s.indexOf("b");
    i+=2;
    while(i < s.size() && s.at(i).isDigit()){
        bStr.append(s.at(i));
        ++i;
    }
    
    i = s.indexOf("a");
    i+=2;
    while(i < s.size() && s.at(i).isDigit()){
        aStr.append(s.at(i));
        ++i;
    }
    QVector4D v(rStr.toDouble(),gStr.toDouble(),bStr.toDouble(),aStr.toDouble());
    _value.setValue(QVariant(v));

}

/***********************************GROUP_KNOB*****************************************/
Group_Knob::Group_Knob(Node* node, const std::string& description,int dimension, Knob_Mask flags):
Knob(node,description,dimension,flags){
    
}


