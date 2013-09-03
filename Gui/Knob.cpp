//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Gui/Knob.h"

#include <climits>
#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
GCC_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
GCC_DIAG_ON(unused-private-field);
#include <QKeyEvent>
#include <QColorDialog>
#include <QGroupBox>
#include <QStyleFactory>


#include "Global/Controler.h"
#include "Engine/Node.h"
#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/Settings.h"
#include "Engine/PluginID.h"
#include "Gui/SettingsPanel.h"
#include "Gui/Button.h"
#include "Gui/ViewerTab.h"
#include "Gui/Timeline.h"
#include "Gui/Gui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/ComboBox.h"
#include "Readers/Reader.h"

using namespace Powiter;
using namespace std;

std::vector<Knob::Knob_Flags> Knob_Mask_to_Knobs_Flags(const Knob_Mask&m) {
    unsigned int i=0x1;
    std::vector<Knob::Knob_Flags> flags;
    if(m!=0){
        while(i<0x4){
            if((m & i)==i){
                flags.push_back((Knob::Knob_Flags)i);
            }
            i*=2;
        }
    }
    return flags;
}

KnobCallback::KnobCallback(SettingsPanel *panel, Node *node){
    this->panel=panel;
    this->node=node;
    
}

KnobCallback::~KnobCallback(){
    //    for (U32 i = 0 ; i< knobs.size(); ++i) {
    //        delete knobs[i];
    //    }
    knobs.clear();
}
void KnobCallback::initNodeKnobsVector(){
    for(U32 i=0;i<knobs.size();++i) {
        Knob* pair=knobs[i];
        node->addToKnobVector(pair);
    }
    
}
void KnobCallback::createKnobDynamically(){
    const std::vector<Knob*>& node_knobs=node->getKnobs();
    foreach(Knob* knob,knobs){
        bool already_exists=false;
        for(U32 i=0;i<node_knobs.size();++i) {
            if(node_knobs[i]==knob){
                already_exists=true;
            }
        }
        if(!already_exists){
            node->addToKnobVector(knob);
            panel->addKnobDynamically(knob);
        }
    }
}

void KnobCallback::removeAndDeleteKnob(Knob* knob){
    node->removeKnob(knob);
    for (U32 i = 0; i< knobs.size(); ++i) {
        if (knobs[i] == knob) {
            knobs.erase(knobs.begin()+i);
            break;
        }
    }
    panel->removeAndDeleteKnob(knob);
}


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
                        Knob* knob=builder(NULL,str,0);
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
                        Knob* knob=builder(NULL,str,0);
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
    Knob* fileKnob = File_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *FILEKnobPlugin = new PluginID((HINSTANCE)&File_Knob::BuildKnob,fileKnob->name().c_str());
#else
    PluginID *FILEKnobPlugin = new PluginID((void*)&File_Knob::BuildKnob,fileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(fileKnob->name(),FILEKnobPlugin));
    delete fileKnob;
    
    Knob* intKnob = Int_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *INTKnobPlugin = new PluginID((HINSTANCE)&Int_Knob::BuildKnob,intKnob->name().c_str());
#else
    PluginID *INTKnobPlugin = new PluginID((void*)&Int_Knob::BuildKnob,intKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(intKnob->name(),INTKnobPlugin));
    delete intKnob;
    
    Knob* int2DKnob = Int2D_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *INT2DKnobPlugin = new PluginID((HINSTANCE)&Int2D_Knob::BuildKnob,int2DKnob->name().c_str());
#else
    PluginID *INT2DKnobPlugin = new PluginID((void*)&Int2D_Knob::BuildKnob,int2DKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(int2DKnob->name(),INT2DKnobPlugin));
    delete int2DKnob;
    
    
    Knob* doubleKnob = Double_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *DOUBLEKnobPlugin = new PluginID((HINSTANCE)&Double_Knob::BuildKnob,doubleKnob->name().c_str());
#else
    PluginID *DOUBLEKnobPlugin = new PluginID((void*)&Double_Knob::BuildKnob,doubleKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(doubleKnob->name(),DOUBLEKnobPlugin));
    delete doubleKnob;
    
    Knob* double2DKnob = Double2D_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *DOUBLE2DKnobPlugin = new PluginID((HINSTANCE)&Double2D_Knob::BuildKnob,double2DKnob->name().c_str());
#else
    PluginID *DOUBLE2DKnobPlugin = new PluginID((void*)&Double2D_Knob::BuildKnob,double2DKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(double2DKnob->name(),DOUBLE2DKnobPlugin));
    delete double2DKnob;
    
    Knob* boolKnob = Bool_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *BOOLKnobPlugin = new PluginID((HINSTANCE)&Bool_Knob::BuildKnob,boolKnob->name().c_str());
#else
    PluginID *BOOLKnobPlugin = new PluginID((void*)&Bool_Knob::BuildKnob,boolKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(boolKnob->name(),BOOLKnobPlugin));
    delete boolKnob;
    
    Knob* buttonKnob = Button_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *BUTTONKnobPlugin = new PluginID((HINSTANCE)&Button_Knob::BuildKnob,buttonKnob->name().c_str());
#else
    PluginID *BUTTONKnobPlugin = new PluginID((void*)&Button_Knob::BuildKnob,buttonKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(buttonKnob->name(),BUTTONKnobPlugin));
    delete buttonKnob;
    
    Knob* outputFileKnob = OutputFile_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *OUTPUTFILEKnobPlugin = new PluginID((HINSTANCE)&OutputFile_Knob::BuildKnob,outputFileKnob->name().c_str());
#else
    PluginID *OUTPUTFILEKnobPlugin = new PluginID((void*)&OutputFile_Knob::BuildKnob,outputFileKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(outputFileKnob->name(),OUTPUTFILEKnobPlugin));
    delete outputFileKnob;
    
    Knob* comboBoxKnob = ComboBox_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *ComboBoxKnobPlugin = new PluginID((HINSTANCE)&ComboBox_Knob::BuildKnob,comboBoxKnob->name().c_str());
#else
    PluginID *ComboBoxKnobPlugin = new PluginID((void*)&ComboBox_Knob::BuildKnob,comboBoxKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(comboBoxKnob->name(),ComboBoxKnobPlugin));
    delete comboBoxKnob;
    
    
    Knob* separatorKnob = Separator_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *SeparatorKnobPlugin = new PluginID((HINSTANCE)&Separator_Knob::BuildKnob,separatorKnob->name().c_str());
#else
    PluginID *SeparatorKnobPlugin = new PluginID((void*)&Separator_Knob::BuildKnob,separatorKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(separatorKnob->name(),SeparatorKnobPlugin));
    delete separatorKnob;
    
    Knob* groupKnob = Group_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *GroupKnobPlugin = new PluginID((HINSTANCE)&Group_Knob::BuildKnob,groupKnob->name().c_str());
#else
    PluginID *GroupKnobPlugin = new PluginID((void*)&Group_Knob::BuildKnob,groupKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(groupKnob->name(),GroupKnobPlugin));
    delete groupKnob;
    
    Knob* rgbaKnob = RGBA_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *RGBAKnobPlugin = new PluginID((HINSTANCE)&RGBA_Knob::BuildKnob,rgbaKnob->name().c_str());
#else
    PluginID *RGBAKnobPlugin = new PluginID((void*)&RGBA_Knob::BuildKnob,rgbaKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(rgbaKnob->name(),RGBAKnobPlugin));
    delete rgbaKnob;
    
    
    Knob* tabKnob = Tab_Knob::BuildKnob(NULL,stub,0);
#ifdef __POWITER_WIN32__
    PluginID *TABKnobPlugin = new PluginID((HINSTANCE)&Tab_Knob::BuildKnob,tabKnob->name().c_str());
#else
    PluginID *TABKnobPlugin = new PluginID((void*)&Tab_Knob::BuildKnob,tabKnob->name().c_str());
#endif
    _loadedKnobs.insert(make_pair(tabKnob->name(),TABKnobPlugin));
    delete tabKnob;
    
    Knob* strKnob = String_Knob::BuildKnob(NULL,stub,0);
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
Knob* KnobFactory::createKnob(const std::string& name, KnobCallback* callback, const std::string& description, Knob_Mask flags){
    const std::map<std::string,PluginID*>& loadedPlugins = KnobFactory::instance()->getLoadedKnobs();
    std::map<std::string,PluginID*>::const_iterator it = loadedPlugins.find(name);
    if(it == loadedPlugins.end()){
        return NULL;
    }else{
        KnobBuilder builder = (KnobBuilder)(it->second->first);
        if(builder){
            Knob* ret = builder(callback,description,flags);
            return ret;
        }else{
            return NULL;
        }
    }
}

Knob::Knob( KnobCallback *cb,const std::string& description):QWidget(),cb(cb),_description(description)
{
    layout=new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    foreach(QWidget* ele,elements){
        layout->addWidget(ele);
    }
    setLayout(layout);
    //cb->addKnob(this);
    setVisible(true);
}
void Knob::changeLayout(QHBoxLayout* newLayout){
    QList<QWidget*> allKnobs;
    for (int i = 0; i < layout->count() ; ++i) {
        QWidget* w = layout->itemAt(i)->widget();
        if (w) {
            allKnobs << w;
        }
    }
    for (int i = 0; i < allKnobs.size(); ++i) {
        newLayout->addWidget(allKnobs.at(i));
    }
}
void Knob::setLayoutMargin(int pix){
    layout->setContentsMargins(0, 0, pix, 0);
}
Knob::~Knob(){
    foreach(QWidget* el,elements){
        delete el;
    }
    elements.clear();
    delete layout;
    values.clear();
}

void Knob::enqueueForDeletion(){
    cb->removeAndDeleteKnob(this);
}
void Knob::pushUndoCommand(QUndoCommand* cmd){
    cb->getNode()->getNodeUi()->pushUndoCommand(cmd);
}
void Knob::validateEvent(bool initViewer){
    Node* node = getCallBack()->getNode();
    NodeGui* nodeUI = node->getNodeUi();
    NodeGui* viewer = NodeGui::hasViewerConnected(nodeUI);
    if(viewer){
        ctrlPTR->getModel()->clearPlaybackCache();
        ctrlPTR->getModel()->setVideoEngineRequirements(viewer->getNode(),true);
        int currentFrameCount = ctrlPTR->getModel()->getVideoEngine()->getFrameCountForCurrentPlayback();
        if(initViewer){
            ctrlPTR->triggerAutoSaveOnNextEngineRun();
            if (currentFrameCount > 1 || currentFrameCount == -1) {
                ctrlPTR->getModel()->startVideoEngine(-1);
            }else{
                ctrlPTR->getModel()->startVideoEngine(1);
            }
        }else{
            ctrlPTR->getModel()->getVideoEngine()->seekRandomFrame(currentViewer->getUiContext()->frameSeeker->currentFrame());
        }
    }
}

//================================================================

//================================================================

Knob* Int_Knob::BuildKnob(KnobCallback *cb, const std::string &description, Knob_Mask flags){
    Int_Knob* knob=new Int_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}

Int_Knob::Int_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags):Knob(cb,description),integer(0){
    QLabel* desc=new QLabel(description.c_str());
    box=new FeedbackSpinBox(this,false);
    QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
    box->setMaximum(INT_MAX);
    box->setMinimum(INT_MIN);
    box->setValue(0);
    layout->addWidget(desc);
    layout->addWidget(box);
    std::vector<Knob_Flags> f=Knob_Mask_to_Knobs_Flags(flags);
    foreach(Knob_Flags flag,f){
        if(flag==INVISIBLE){
            setVisible(false);
        }else if(flag==READ_ONLY){
            box->setReadOnly(true);
        }
        
    }
}
void Int_Knob::onValueChanged(double v){
    pushUndoCommand(new IntCommand(this,*integer,(int)v));
    emit valueChanged((int)v);
    
}
void Int_Knob::setValue(int value){
    *integer = value;
    box->setValue(value);
    setValues();
}
void Int_Knob::setValues(){
    values.clear();
    values.push_back((U64)integer);
}
void Int_Knob::setMaximum(int v){
    box->setMaximum(v);
}
void Int_Knob::setMinimum(int v){
    box->setMinimum(v);
}
void IntCommand::undo(){
    _knob->setValue(_old);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void IntCommand::redo(){
    _knob->setValue(_new);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
bool IntCommand::mergeWith(const QUndoCommand *command){
    const IntCommand *_command = static_cast<const IntCommand *>(command);
    Int_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    _new = knob->getValue();
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}

std::string Int_Knob::serialize() const{
    int v = 0;
    if(integer){
        v = *integer;
    }
    QString str = QString::number(v);
    return str.toStdString();
}
void Int_Knob::restoreFromString(const std::string& str){
    assert(integer);
    QString s(str.c_str());
    if(!s.isEmpty()){
        int i = 0;
        QString vStr;
        while(i < s.size() && s.at(i).isDigit()){
            vStr.append(s.at(i));
            ++i;
        }
        setValue(vStr.toInt());
        validateEvent(false);
    }
}
/******INT2D*****/


Knob* Int2D_Knob::BuildKnob(KnobCallback *cb, const std::string &description, Knob_Mask flags){
    Int2D_Knob* knob=new Int2D_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}

Int2D_Knob::Int2D_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags):Knob(cb,description),_value1(0),_value2(0){
    QLabel* desc=new QLabel(description.c_str());
    _box1=new FeedbackSpinBox(this,false);
    _box2=new FeedbackSpinBox(this,false);
    QObject::connect(_box1, SIGNAL(valueChanged(double)), this, SLOT(onValue1Changed(double)));
    QObject::connect(_box2, SIGNAL(valueChanged(double)), this, SLOT(onValue2Changed(double)));
    _box1->setMaximum(INT_MAX);
    _box1->setMinimum(INT_MIN);
    _box1->setValue(0);
    _box2->setMaximum(INT_MAX);
    _box2->setMinimum(INT_MIN);
    _box2->setValue(0);
    layout->addWidget(desc);
    layout->addWidget(_box1);
    layout->addWidget(_box2);
    layout->addStretch();
    std::vector<Knob_Flags> f=Knob_Mask_to_Knobs_Flags(flags);
    foreach(Knob_Flags flag,f){
        if(flag==INVISIBLE){
            setVisible(false);
        }else if(flag==READ_ONLY){
            _box1->setReadOnly(true);
            _box2->setReadOnly(true);
        }
        
    }
}
void Int2D_Knob::onValue1Changed(double v){
    pushUndoCommand(new Int2DCommand(this,*_value1,*_value2,(int)v,*_value2));
    emit value1Changed((int)v);
}
void Int2D_Knob::onValue2Changed(double v){
    pushUndoCommand(new Int2DCommand(this,*_value1,*_value2,*_value1,(int)v));
    emit value2Changed((int)v);
    
}
void Int2D_Knob::setValue1(int value){
    *_value1 = value;
    _box1->setValue(value);
    setValues();
}
void Int2D_Knob::setValue2(int value){
    *_value2 = value;
    _box2->setValue(value);
    setValues();
}
void Int2DCommand::undo(){
    _knob->setValue1(_old1);
    _knob->setValue2(_old2);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void Int2DCommand::redo(){
    
    _knob->setValue1(_new1);
    _knob->setValue2(_new2);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
bool Int2DCommand::mergeWith(const QUndoCommand *command){
    const Int2DCommand *_command = static_cast<const Int2DCommand *>(command);
    Int2D_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    _new1 = knob->getValue1();
    _new2 = knob->getValue2();
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}

void Int2D_Knob::setValues(){
    values.clear();
    values.push_back((U64)_value1);
    values.push_back((U64)_value2);
}
void Int2D_Knob::setMaximum1(int v){
    _box1->setMaximum(v);
}
void Int2D_Knob::setMinimum1(int v){
    _box1->setMinimum(v);
}
void Int2D_Knob::setMaximum2(int v){
    _box2->setMaximum(v);
}
void Int2D_Knob::setMinimum2(int v){
    _box2->setMinimum(v);
}
std::string Int2D_Knob::serialize() const{
    int v1 = 0,v2 = 0;
    if(_value1){
        v1 = *_value1;
    }
    if(_value2){
        v1 = *_value2;
    }
    
    QString str1 = QString::number(v1);
    QString str2 = QString::number(v2);
    return QString("v1 " + str1 + " v2 " + str2).toStdString();
}
void Int2D_Knob::restoreFromString(const std::string& str){
    assert(_value1 && _value2);
    QString s(str.c_str());
    if(!s.isEmpty()){
        int i = s.indexOf("v1");
        i+=3;
        QString v1Str,v2Str;
        while(i < s.size() && s.at(i).isDigit()){
            v1Str.append(s.at(i));
            ++i;
        }
        
        ++i;//the ' ' character
        i+=3;
        while(i < s.size() && s.at(i).isDigit()){
            v2Str.append(s.at(i));
            ++i;
        }
        setValue1(v1Str.toInt());
        setValue2(v2Str.toInt());
        validateEvent(false);
    }
    
}

//================================================================
FileQLineEdit::FileQLineEdit(File_Knob *knob):LineEdit(knob){
    this->knob=knob;
}
void FileQLineEdit::keyPressEvent(QKeyEvent *e){
    if(e->key()==Qt::Key_Return){
        QString str=this->text();
		QStringList strlist(str);
		if(strlist!=*(knob->getFileNames())){
			knob->pushUndoCommand(new FileCommand(knob,*(knob->getFileNames()),strlist));
            std::string className=knob->getCallBack()->getNode()->className();
			if(className == std::string("Reader")){
                Node* node=knob->getCallBack()->getNode();
                static_cast<Reader*>(node)->showFilePreview();
                knob->validateEvent(true);
            }
		}
    }
	QLineEdit::keyPressEvent(e);
}

Knob* File_Knob::BuildKnob(KnobCallback *cb, const std::string &description, Knob_Mask flags){
    File_Knob* knob=new File_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
void File_Knob::open_file(){
    filesList->clear();
    
    
    QStringList strlist;
    std::vector<std::string> filters = Settings::getPowiterCurrentSettings()->_readersSettings.supportedFileTypes();
    
    SequenceFileDialog dialog(this,filters,true,SequenceFileDialog::OPEN_DIALOG,_lastOpened.toStdString());
    if(dialog.exec()){
        strlist = dialog.selectedFiles();
    }
    
    if(!strlist.isEmpty()){
        updateLastOpened(strlist[0]);
        pushUndoCommand(new FileCommand(this,*filesList,strlist));        
    }
    
}
void File_Knob::updateLastOpened(const QString& str){
    QString withoutPath = SequenceFileDialog::removePath(str);
    int pos = str.indexOf(withoutPath);
    _lastOpened = str.left(pos);
}
void FileCommand::undo(){
    _knob->setFileNames(_oldList);
    _knob->setLineEditText(SequenceFileDialog::patternFromFilesList(_oldList).toStdString());
    _knob->setValues();
    
    std::string className= _knob->getCallBack()->getNode()->className();
    if(className == string("Reader")){
        Node* node= _knob->getCallBack()->getNode();
        ctrlPTR->getModel()->setVideoEngineRequirements(NULL,false);
        static_cast<Reader*>(node)->showFilePreview();
    }
    _knob->validateEvent(true);
    
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void FileCommand::redo(){
    _knob->setFileNames(_newList);
    _knob->setLineEditText(SequenceFileDialog::patternFromFilesList(_newList).toStdString());
    _knob->setValues();
    
    std::string className= _knob->getCallBack()->getNode()->className();
    if(className == string("Reader")){
        Node* node= _knob->getCallBack()->getNode();
        ctrlPTR->getModel()->setVideoEngineRequirements(NULL,false);
        static_cast<Reader*>(node)->showFilePreview();
    }
    _knob->validateEvent(true);    
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
bool FileCommand::mergeWith(const QUndoCommand *command){
    const FileCommand *fileCommand = static_cast<const FileCommand *>(command);
    File_Knob* knob = fileCommand->_knob;
    if(_knob != knob)
        return false;
    _newList = (*knob->getFileNames());
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}

File_Knob::File_Knob(KnobCallback *cb, const std::string &description, Knob_Mask ):Knob(cb,description),filesList(0),_lastOpened("")
{
    
    QLabel* desc=new QLabel(description.c_str());
    _name=new FileQLineEdit(this);
    _name->setPlaceholderText(QString("File path..."));
	
    QPushButton* openFile=new Button(_name);
    QImage img(POWITER_IMAGES_PATH"open-file.png");
    QPixmap pix=QPixmap::fromImage(img);
    pix.scaled(10,10);
    openFile->setIcon(QIcon(pix));
    QObject::connect(openFile,SIGNAL(clicked()),this,SLOT(open_file()));
    layout->addWidget(desc);
    layout->addWidget(_name);
    layout->addWidget(openFile);
    
    //flags handling: no Knob_Flags makes sense (yet) for the File_Knob. We keep it in parameters in case in the future there're some changes to be made.
    
}
void File_Knob::setFileNames(const QStringList& str){
    for(int i =0;i< str.size();++i) {
        
        QString el = str[i];
        
        this->filesList->append(el);
    }
    emit filesSelected();
}

void File_Knob::setValues(){
    values.clear();
    // filenames of the sequence should not be involved in hash key computation as it defeats all the purpose of the cache
    // explanation: The algorithm doing the look-up in the cache already takes care of the current frame name.
    // go at VideoEngine::videoEngine(...) at line starting with  key = FrameEntry::computeHashKey to understand the call.
}
void File_Knob::setLineEditText(const std::string& str){
    _name->setText(str.c_str());
}
std::string File_Knob::getLineEditText() const{
    return _name->text().toStdString();
}

std::string File_Knob::serialize() const{
    return getLineEditText();
}
void File_Knob::restoreFromString(const std::string& str){
    _name->setText(str.c_str());
    *filesList = SequenceFileDialog::filesListFromPattern(str.c_str());
    
    setValues();
    if(filesList->size() > 0){
        std::string className=getCallBack()->getNode()->className();
        if(className == string("Reader")){
            Node* node=getCallBack()->getNode();
            ctrlPTR->getModel()->setVideoEngineRequirements(NULL,false);
            static_cast<Reader*>(node)->showFilePreview();
        }
        validateEvent(true);
    }
}

Knob* Bool_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
	Bool_Knob* knob=new Bool_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
	return knob;
    
}
void Bool_Knob::onToggle(bool b){
    pushUndoCommand(new BoolCommand(this,*_boolean,b));
    emit triggered(b);
}
void BoolCommand::undo(){
    _knob->setChecked(_old);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void BoolCommand::redo(){
    _knob->setChecked(_new);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
bool BoolCommand::mergeWith(const QUndoCommand *command){
    const BoolCommand *_command = static_cast<const BoolCommand *>(command);
    Bool_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    _new = knob->isChecked();
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}


void Bool_Knob::setChecked(bool b){
    *_boolean = b;
    checkbox->setChecked(b);
    setValues();
}

void Bool_Knob::setValues(){
    values.clear();
	if(*_boolean){
		values.push_back(1);
	}else{
		values.push_back(0);
	}
}

Bool_Knob::Bool_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags):Knob(cb,description) ,_boolean(0){
	Q_UNUSED(flags);
    QLabel* _label = new QLabel(description.c_str(),this);
	checkbox=new QCheckBox(this);
	checkbox->setChecked(false);
	QObject::connect(checkbox,SIGNAL(clicked(bool)),this,SLOT(onToggle(bool)));
    layout->addWidget(_label);
	layout->addWidget(checkbox);
    layout->addStretch();
}
std::string Bool_Knob::serialize() const{
    if (_boolean) {
        return *_boolean ? "1" : "0";
    }else{
        return "";
    }
}
void Bool_Knob::restoreFromString(const std::string& str){
    assert(_boolean);
    QString s(str.c_str());
    if(!s.isEmpty()){
        int val = s.toInt();
        setChecked(val);
    }
    validateEvent(false);
    
}
//================================================================

void Double_Knob::setValues(){
    values.clear();
    values.push_back(*(reinterpret_cast<U64*>(_value)));
}
Double_Knob::Double_Knob(KnobCallback * cb, const std::string& description, Knob_Mask flags):Knob(cb,description),_value(0){
    QLabel* desc=new QLabel(description.c_str());
    box=new FeedbackSpinBox(this,true);
    QObject::connect(box, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
    box->setMaximum(INT_MAX);
    box->setMinimum(INT_MIN);
    box->setValue(0);
    layout->addWidget(desc);
    layout->addWidget(box);
    std::vector<Knob_Flags> f=Knob_Mask_to_Knobs_Flags(flags);
    foreach(Knob_Flags flag,f){
        if(flag==INVISIBLE){
            setVisible(false);
        }else if(flag==READ_ONLY){
            box->setReadOnly(true);
        }
        
    }
}
void Double_Knob::onValueChanged(double d){
    pushUndoCommand(new DoubleCommand(this,*_value,d));
    emit valueChanged(d);
    
}
void Double_Knob::setValue(double value){
    *_value = value;
    box->setValue(value);
    setValues();
}

void DoubleCommand::undo(){
    _knob->setValue(_old);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void DoubleCommand::redo(){
    _knob->setValue(_new);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
bool DoubleCommand::mergeWith(const QUndoCommand *command){
    const DoubleCommand *_command = static_cast<const DoubleCommand *>(command);
    Double_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    _new = knob->getValue();
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}

void Double_Knob::setMaximum(double d){
    box->setMaximum(d);
}
void Double_Knob::setMinimum(double d){
    box->setMinimum(d);
}

void Double_Knob::setIncrement(double d){
    box->setIncrement(d);
}

Knob* Double_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    Double2D_Knob* knob=new Double2D_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
std::string Double_Knob::serialize() const{
    double v = 0.;
    if(_value){
        v = *_value;
    }
    QString str = QString::number(v);
    return str.toStdString();
}
void Double_Knob::restoreFromString(const std::string& str){
    assert(_value);
    QString s(str.c_str());
    if(!s.isEmpty()){
        int i = 0;
        QString vStr;
        while(i < s.size() && s.at(i).isDigit()){
            vStr.append(s.at(i));
            ++i;
        }
        setValue(vStr.toDouble());
        validateEvent(false);
    }
}
/*********Double2D******/

Knob* Double2D_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    Double_Knob* knob=new Double_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
void Double2D_Knob::setValues(){
    values.clear();
    values.push_back(*(reinterpret_cast<U64*>(_value1)));
    values.push_back(*(reinterpret_cast<U64*>(_value2)));
}
Double2D_Knob::Double2D_Knob(KnobCallback * cb, const std::string& description, Knob_Mask flags):Knob(cb,description),_value1(0),_value2(0){
    QLabel* desc=new QLabel(description.c_str());
    _box1=new FeedbackSpinBox(this,true);
    _box2=new FeedbackSpinBox(this,true);
    QObject::connect(_box1, SIGNAL(valueChanged(double)), this, SLOT(onValue1Changed(double)));
    QObject::connect(_box2, SIGNAL(valueChanged(double)), this, SLOT(onValue2Changed(double)));
    _box1->setMaximum(INT_MAX);
    _box1->setMinimum(INT_MIN);
    _box1->setValue(0);
    _box2->setMaximum(INT_MAX);
    _box2->setMinimum(INT_MIN);
    _box2->setValue(0);
    layout->addWidget(desc);
    layout->addWidget(_box1);
    layout->addWidget(_box2);
    layout->addStretch();
    std::vector<Knob_Flags> f=Knob_Mask_to_Knobs_Flags(flags);
    foreach(Knob_Flags flag,f){
        if(flag==INVISIBLE){
            setVisible(false);
        }else if(flag==READ_ONLY){
            _box1->setReadOnly(true);
            _box2->setReadOnly(true);
        }
        
    }
}
void Double2D_Knob::onValue1Changed(double d){
    pushUndoCommand(new Double2DCommand(this,*_value1,*_value2,d,*_value2));
    emit value1Changed(d);
    
}
void Double2D_Knob::onValue2Changed(double d){
    pushUndoCommand(new Double2DCommand(this,*_value1,*_value2,*_value1,d));
    emit value2Changed(d);
    }
void Double2D_Knob::setValue1(double value){
    *_value1 = value;
    _box1->setValue(value);
    setValues();
}
void Double2D_Knob::setValue2(double value){
    *_value2 = value;
    _box2->setValue(value);
    setValues();
}
void Double2DCommand::undo(){
    _knob->setValue1(_old1);
    _knob->setValue2(_old2);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void Double2DCommand::redo(){
    
    _knob->setValue1(_new1);
    _knob->setValue2(_new2);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}

bool Double2DCommand::mergeWith(const QUndoCommand *command){
    const Double2DCommand *_command = static_cast<const Double2DCommand *>(command);
    Double2D_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    _new1 = knob->getValue1();
    _new2 = knob->getValue2();
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}

void Double2D_Knob::setMaximum1(double d){
    _box1->setMaximum(d);
}
void Double2D_Knob::setMinimum1(double d){
    _box1->setMinimum(d);
}

void Double2D_Knob::setIncrement1(double d){
    _box1->setIncrement(d);
}

void Double2D_Knob::setMaximum2(double d){
    _box2->setMaximum(d);
}
void Double2D_Knob::setMinimum2(double d){
    _box2->setMinimum(d);
}

void Double2D_Knob::setIncrement2(double d){
    _box2->setIncrement(d);
}
std::string Double2D_Knob::serialize() const{
    double v1 = 0.,v2 = 0.;
    if(_value1){
        v1 = *_value1;
    }
    if(_value2){
        v2 = *_value2;
    }
    QString str1 = QString::number(v1);
    QString str2 = QString::number(v2);
    return QString("v1 " + str1 + " v2 " + str2).toStdString();
}
void Double2D_Knob::restoreFromString(const std::string& str){
    assert(_value1 && _value2);
    QString s(str.c_str());
    if(!s.isEmpty()){
        int i = s.indexOf("v1");
        i+=3;
        QString v1Str,v2Str;
        while(i < s.size() && s.at(i).isDigit()){
            v1Str.append(s.at(i));
            ++i;
        }
        
        ++i;//the ' ' character
        i+=3;
        while(i < s.size() && s.at(i).isDigit()){
            v2Str.append(s.at(i));
            ++i;
        }
        setValue1(v1Str.toDouble());
        setValue2(v2Str.toDouble());
        validateEvent(false);
    }
    
}
/*******/

Knob* Button_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    Button_Knob* knob=new Button_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
Button_Knob::Button_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags):Knob(cb,description),button(0){
    Q_UNUSED(flags);
    button = new Button(QString(description.c_str()),this);
    QObject::connect(button, SIGNAL(pressed()),this,SLOT(onButtonPressed()));
    layout->addWidget(button);
    layout->addStretch();
}
void Button_Knob::connectButtonToSlot(QObject* object,const char* slot){
    QObject::connect(button, SIGNAL(pressed()), object, slot);
}

void Button_Knob::onButtonPressed(){
    validateEvent(false);
}

std::string Button_Knob::serialize() const{
    return "";
}
void Button_Knob::restoreFromString(const std::string& str){
    (void)str;
}

/*******/


Knob* OutputFile_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    OutputFile_Knob* knob=new OutputFile_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}

OutputFile_Knob::OutputFile_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags):Knob(cb,description),str(0){
    Q_UNUSED(flags);
    QLabel* desc=new QLabel(description.c_str());
    _name=new OutputFileQLineEdit(this);
    _name->setPlaceholderText(QString("File path..."));
	
    QPushButton* openFile=new Button(_name);
    QImage img(POWITER_IMAGES_PATH"open-file.png");
    QPixmap pix=QPixmap::fromImage(img);
    pix.scaled(10,10);
    openFile->setIcon(QIcon(pix));
    QObject::connect(openFile,SIGNAL(clicked()),this,SLOT(open_file()));
    QObject::connect(_name,SIGNAL(textEdited(const QString&)),this,SLOT(setStr(const QString&)));
    layout->addWidget(desc);
    layout->addWidget(_name);
    layout->addWidget(openFile);
}
void OutputFile_Knob::setStr(const QString& str){
    *(this->str) = str.toStdString();
    _name->setText(str);
}

void OutputFile_Knob::setValues(){
    values.clear();
    
}
void OutputFile_Knob::updateLastOpened(const QString& str){
    QString withoutPath = SequenceFileDialog::removePath(str);
    int pos = str.indexOf(withoutPath);
    _lastOpened = str.left(pos);
}

void OutputFile_Knob::open_file(){
    str->clear();
    
    std::vector<std::string> filters = Settings::getPowiterCurrentSettings()->_readersSettings.supportedFileTypes();
    SequenceFileDialog dialog(this,filters,true,SequenceFileDialog::SAVE_DIALOG,_lastOpened.toStdString());
    
    if(dialog.exec()){
        updateLastOpened(SequenceFileDialog::removePath(str->c_str()));
        pushUndoCommand(new OutputFileCommand(this,*str,dialog.filesToSave().toStdString()));
        emit filesSelected();
    }
    
}

void OutputFileCommand::undo(){
    _knob->setStr(_old.c_str());
    _knob->setValues();
    _knob->validateEvent(true);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void OutputFileCommand::redo(){
    _knob->setStr(_new.c_str());
    _knob->setValues();
    _knob->validateEvent(true);

}
bool OutputFileCommand::mergeWith(const QUndoCommand *command){
    const OutputFileCommand *fileCommand = static_cast<const OutputFileCommand *>(command);
    OutputFile_Knob* knob = fileCommand->_knob;
    if(_knob != knob)
        return false;
    _new = (*knob->getStr());
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}

OutputFileQLineEdit::OutputFileQLineEdit(OutputFile_Knob* knob):LineEdit(knob){
    this->knob = knob;
}

void OutputFileQLineEdit::keyPressEvent(QKeyEvent *e){
    if(e->key()==Qt::Key_Return){
        QString str=this->text();
		if(str.toStdString()!=*(knob->getStr())){
			knob->pushUndoCommand(new OutputFileCommand(knob,*(knob->getStr()),str.toStdString()));
		}
    }
	QLineEdit::keyPressEvent(e);
}
std::string OutputFile_Knob::serialize() const{
    return _name->text().toStdString();
}
void OutputFile_Knob::restoreFromString(const std::string& str){
    _name->setText(str.c_str());
    *(this->str) = str;
    validateEvent(false);
}
/*===============================*/

Knob* ComboBox_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    ComboBox_Knob* knob=new ComboBox_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
    
}
ComboBox_Knob::ComboBox_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags):Knob(cb,description),_currentItem(0){
    Q_UNUSED(flags);
    _comboBox = new ComboBox(this);
    QLabel* desc = new QLabel(description.c_str());
    QObject::connect(_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
    layout->addWidget(desc);
    layout->addWidget(_comboBox);
    layout->addStretch();
}
void ComboBox_Knob::populate(const std::vector<std::string>& entries){
    for (U32 i = 0; i < entries.size(); ++i) {
        _comboBox->addItem(entries[i].c_str());
    }
    setCurrentItem(0);
}
void ComboBox_Knob::onCurrentIndexChanged(int i){
    pushUndoCommand(new ComboBoxCommand(this,_comboBox->itemIndex(_currentItem->c_str()),i));
    emit entryChanged(i);
   
        
}
const std::string ComboBox_Knob::activeEntry() const{
    return _comboBox->itemText(_comboBox->activeIndex()).toStdString();
}
int ComboBox_Knob::currentIndex() const{
    return _comboBox->activeIndex();
}
void ComboBoxCommand::undo(){
    _knob->setCurrentItem(_old);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void ComboBoxCommand::redo(){
    _knob->setCurrentItem(_new);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}

bool ComboBoxCommand::mergeWith(const QUndoCommand *command){
    const ComboBoxCommand *_command = static_cast<const ComboBoxCommand *>(command);
    ComboBox_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    _new = knob->currentIndex();
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}

void ComboBox_Knob::setCurrentItem(int index){
    QString str = _comboBox->itemText(index);
    *_currentItem = str.toStdString();
    _comboBox->setCurrentText(str);
    setValues();
}

void ComboBox_Knob::setPointer(std::string* str){
    _currentItem = str;
}

void ComboBox_Knob::setValues(){
    values.clear();
    QString out(_currentItem->c_str());
    for (int i =0; i< out.size(); ++i) {
        values.push_back(out.at(i).unicode());
    }
}
std::string ComboBox_Knob::serialize() const{
    return _comboBox->itemText(_comboBox->activeIndex()).toStdString();
}
void ComboBox_Knob::restoreFromString(const std::string& str){
    assert(_currentItem);
    _comboBox->setCurrentText(str.c_str());
    *_currentItem = str;
    setValues();
    validateEvent(false);
}
/*============================*/

Knob* Separator_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    Separator_Knob* knob=new Separator_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
Separator_Knob::Separator_Knob(KnobCallback *cb, const std::string& description, Knob_Mask):Knob(cb,description){
    QLabel* name = new QLabel(description.c_str(),this);
    layout->addWidget(name);
    line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(line);
    
}

/******************************/
Knob* Group_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    Group_Knob* knob=new Group_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}

Group_Knob::Group_Knob(KnobCallback *cb, const std::string& description, Knob_Mask):Knob(cb,description){
    _box = new QGroupBox(description.c_str(),this);
    _boxLayout = new QVBoxLayout(_box);
    QObject::connect(_box, SIGNAL(clicked(bool)), this, SLOT(setChecked(bool)));
    _box->setLayout(_boxLayout);
    _box->setCheckable(true);
    layout->addWidget(_box);
}


void Group_Knob::addKnob(Knob* k){
    k->setParent(this);
    _boxLayout->addWidget(k);
    k->setVisible(_box->isChecked());
    _knobs.push_back(k);
}
void Group_Knob::setChecked(bool b){
    _box->setChecked(b);
    for(U32 i = 0 ; i < _knobs.size() ;++i) {
        _knobs[i]->setVisible(b);
    }
}

std::string Group_Knob::serialize() const{
    return "";
}
/*****************************/
RGBA_Knob::RGBA_Knob(KnobCallback *cb, const std::string& description, Knob_Mask /*flags*/):Knob(cb,description),
_r(0),_g(0),_b(0),_a(0),_alphaEnabled(true){
    _rBox = new FeedbackSpinBox(this,true);
    QObject::connect(_rBox, SIGNAL(valueChanged(double)), this, SLOT(onRedValueChanged(double)));
    _gBox = new FeedbackSpinBox(this,true);
    QObject::connect(_gBox, SIGNAL(valueChanged(double)), this, SLOT(onGreenValueChanged(double)));
    _bBox = new FeedbackSpinBox(this,true);
    QObject::connect(_bBox, SIGNAL(valueChanged(double)), this, SLOT(onBlueValueChanged(double)));
    _aBox = new FeedbackSpinBox(this,true);
    QObject::connect(_aBox, SIGNAL(valueChanged(double)), this, SLOT(onAlphaValueChanged(double)));
    
    _rBox->setMaximum(1.);
    _rBox->setMinimum(0.);
    _rBox->setIncrement(0.1);
    
    _gBox->setMaximum(1.);
    _gBox->setMinimum(0.);
    _gBox->setIncrement(0.1);
    
    _bBox->setMaximum(1.);
    _bBox->setMinimum(0.);
    _bBox->setIncrement(0.1);
    
    _aBox->setMaximum(1.);
    _aBox->setMinimum(0.);
    _aBox->setIncrement(0.1);
    
    
    _rLabel = new QLabel("r",this);
    _gLabel = new QLabel("g",this);
    _bLabel = new QLabel("b",this);
    _aLabel = new QLabel("a",this);
    
    QLabel* nameLabel = new QLabel(description.c_str(),this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(nameLabel);
    layout->addWidget(_rLabel);
    layout->addWidget(_rBox);
    layout->addWidget(_gLabel);
    layout->addWidget(_gBox);
    layout->addWidget(_bLabel);
    layout->addWidget(_bBox);
    layout->addWidget(_aLabel);
    layout->addWidget(_aBox);
    
    _colorLabel = new QLabel(this);
    layout->addWidget(_colorLabel);
    
    QImage buttonImg(POWITER_IMAGES_PATH"colorwheel.png");
    QPixmap buttonPix = QPixmap::fromImage(buttonImg);
    buttonPix = buttonPix.scaled(25, 20);
    QIcon buttonIcon(buttonPix);
    _colorDialogButton = new Button(buttonIcon,"",this);
    
    QObject::connect(_colorDialogButton, SIGNAL(pressed()), this, SLOT(showColorDialog()));
    layout->addWidget(_colorDialogButton);
}
void RGBA_Knob::showColorDialog(){
    QColorDialog dialog(this);
    if(dialog.exec()){
        QColor color = dialog.getColor();
        *_r = color.redF();
        *_g = color.greenF();
        *_b = color.blueF();
        if(_a) {
            *_a = color.alphaF();
        }
        setValues();
        updateLabel(color);
        validateEvent(false);
        emit colorChanged(color);
    }
}

void RGBA_Knob::onRedValueChanged(double r){
    QColor color;
    color.setRedF(*_r);
    color.setGreenF(*_g);
    color.setBlueF(*_b);
    if(_a)
        color.setAlphaF(*_a);
    pushUndoCommand(new RGBACommand(this,
                                    color.redF(),color.greenF(),color.blueF(),color.alphaF(),
                                    r,color.greenF(),color.blueF(),color.alphaF()));
    *_r = r;
    color.setRedF(r);
    validateEvent(false);
    emit colorChanged(color);
}
void RGBA_Knob::onGreenValueChanged(double g){
    QColor color;
    color.setRedF(*_r);
    color.setGreenF(*_g);
    color.setBlueF(*_b);
    if(_a)
        color.setAlphaF(*_a);
    pushUndoCommand(new RGBACommand(this,
                                    color.redF(),color.greenF(),color.blueF(),color.alphaF(),
                                    color.redF(),g,color.blueF(),color.alphaF()));
    *_g = g;
    color.setGreenF(g);
    validateEvent(false);
    emit colorChanged(color);
}
void RGBA_Knob::onBlueValueChanged(double b){
    QColor color;
    color.setRedF(*_r);
    color.setGreenF(*_g);
    color.setBlueF(*_b);
    if(_a)
        color.setAlphaF(*_a);
    pushUndoCommand(new RGBACommand(this,
                                    color.redF(),color.greenF(),color.blueF(),color.alphaF(),
                                    color.redF(),color.greenF(),b,color.alphaF()));
    *_b = b;
    color.setBlueF(b);
    validateEvent(false);
    emit colorChanged(color);
}
void RGBA_Knob::onAlphaValueChanged(double a){
    QColor color;
    color.setRedF(*_r);
    color.setGreenF(*_g);
    color.setBlueF(*_b);
    if(_a)
        color.setAlphaF(*_a);
    pushUndoCommand(new RGBACommand(this,
                                   color.redF(),color.greenF(),color.blueF(),color.alphaF(),
                                    color.redF(),color.greenF(),color.blueF(),a));
    if(_a)
        *_a = a;
    color.setAlphaF(a);
    validateEvent(false);
    
    emit colorChanged(color);
}

void RGBACommand::undo(){
    _knob->setRGBA(_oldr, _oldg, _oldb, _olda);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void RGBACommand::redo(){
    _knob->setRGBA(_newr, _newg, _newb, _newa);
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
bool RGBACommand::mergeWith(const QUndoCommand *command){
    const RGBACommand *_command = static_cast<const RGBACommand *>(command);
    RGBA_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    double r,g,b,a;
    knob->getColor(&r, &g, &b, &a);
    _newr = r;
    _newg = g;
    _newb = b;
    _newa = a;
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}
void RGBA_Knob::setValues(){
    values.clear();
    values.push_back(*(reinterpret_cast<U64*>(_r)));
    values.push_back(*(reinterpret_cast<U64*>(_g)));
    values.push_back(*(reinterpret_cast<U64*>(_b)));
    if(_a)
        values.push_back(*(reinterpret_cast<U64*>(_a)));
}

Knob* RGBA_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    RGBA_Knob* knob=new RGBA_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
void RGBA_Knob::updateLabel(const QColor& color){
    QImage img(20,20,QImage::Format_RGB32);
    img.fill(color.rgb());
    QPixmap pix=QPixmap::fromImage(img);
    _colorLabel->setPixmap(pix);
}
void RGBA_Knob::setPointers(double *r,double *g,double *b,double *a){
    _r = r; _g = g; _b = b; _a = a;
    QColor color;
    color.setRedF(*_r);
    color.setGreenF(*_g);
    color.setBlueF(*_b);
    if(_a)
        color.setAlphaF(*_a);
    else
        color.setAlphaF(1.0);
    updateLabel(color);
    
}
void RGBA_Knob::getColor(double *r,double *g,double *b,double *a){
    assert(_r && _g && _b);
    *r = *_r;
    *g = *_g;
    *b = *_b;
    if(_a)
        *a = *_a;
    else
        *a = 1.0;
}
void RGBA_Knob::setRGBA(double r,double g,double b,double a){
    QColor color;
    color.setRedF(r);
    color.setGreenF(g);
    color.setBlueF(b);
    color.setAlphaF(a);
    updateLabel(color);
    setValues();
    _rBox->setValue(r);
    _gBox->setValue(g);
    _bBox->setValue(b);
    *_r = r;
    *_g = g;
    *_b = b;
    if(_a)
        *_a = a;
    if(_alphaEnabled)
        _aBox->setValue(a);
}
void RGBA_Knob::disablePermantlyAlpha(){
    _alphaEnabled = false;
    _aLabel->setVisible(false);
    _aLabel->setEnabled(false);
    _aBox->setVisible(false);
    _aBox->setEnabled(false);
}
std::string RGBA_Knob::serialize() const {
    double r = 0. , g = 0., b = 0., a = 1.;
    if(_r)
        r = *_r;
    if(_g)
        g = *_g;
    if(_b)
        b = *_b;
    if(_a)
        a = *_a;
    return ("r " + QString::number(r) + " g " + QString::number(g) + " b " +  QString::number(b) + " a " +  QString::number(a)).toStdString();
}
void RGBA_Knob::restoreFromString(const std::string& str){
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
    setRGBA(rStr.toDouble(), gStr.toDouble(), bStr.toDouble(), aStr.toDouble());
    setValues();
    validateEvent(false);
    
}
/*************/

Tab_Knob::Tab_Knob(KnobCallback *cb, const std::string& description, Knob_Mask /*flags*/):Knob(cb,description){
    _tabWidget = new TabWidget(TabWidget::NONE,this);
    layout->addWidget(_tabWidget);
}


Knob* Tab_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    Tab_Knob* knob=new Tab_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}


void Tab_Knob::addTab(const std::string& name){
    QWidget* newTab = new QWidget(_tabWidget);
    _tabWidget->appendTab(name.c_str(), newTab);
    QVBoxLayout* newLayout = new QVBoxLayout(newTab);
    newTab->setLayout(newLayout);
    vector<Knob*> knobs;
    _knobs.insert(make_pair(name, make_pair(newLayout, knobs)));
}
void Tab_Knob::addKnob(const std::string& tabName,Knob* k){
    KnobsTabMap::iterator it = _knobs.find(tabName);
    if(it!=_knobs.end()){
        it->second.first->addWidget(k);
        it->second.second.push_back(k);
    }
}

/*********************/
Knob* String_Knob::BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags){
    String_Knob* knob=new String_Knob(cb,description,flags);
    if(cb)
        cb->addKnob(knob);
    return knob;
}
String_Knob::String_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags):Knob(cb,description){
    QLabel* name = new QLabel(description.c_str(),this);
    layout->addWidget(name);
    _lineEdit = new LineEdit(this);
    layout->addWidget(_lineEdit);
    QObject::connect(_lineEdit, SIGNAL(textEdited(QString)), this, SLOT(onStringChanged(QString)));
    if(flags == READ_ONLY){
        _lineEdit->setReadOnly(true);
    }
}
void String_Knob::onStringChanged(const QString& str){
    pushUndoCommand(new StringCommand(this,*_string,str.toStdString()));
    emit stringChanged(str);
}
void String_Knob::setValues(){
    values.clear();
    QString str(_string->c_str());
    for (int i = 0; i < str.size(); ++i) {
        values.push_back(str.at(i).unicode());
    }
}
void String_Knob::setString(QString str){
    _lineEdit->setText(str);
    setValues();
}
void StringCommand::undo(){
    _knob->setString(_old.c_str());
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
void StringCommand::redo(){
    _knob->setString(_new.c_str());
    _knob->validateEvent(false);
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
}
bool StringCommand::mergeWith(const QUndoCommand *command){
    const StringCommand *_command = static_cast<const StringCommand *>(command);
    String_Knob* knob = _command->_knob;
    if(_knob != knob)
        return false;
    
    _new = knob->getString();
    
    setText(QObject::tr("Change %1")
            .arg(_knob->getDescription().c_str()));
    return true;
}
std::string String_Knob::serialize() const{
    return _string ?  *_string : "";
}

void String_Knob::restoreFromString(const std::string& str){
    assert(_string);
    *_string = str;
    setString(str.c_str());
    validateEvent(false);
}
