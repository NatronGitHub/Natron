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
#include "Knob.h"

#include <QtCore/QDir>
#include <QtCore/QDataStream>
#include <QtCore/QByteArray>

#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"


#include "Readers/Reader.h"

#include "Gui/KnobGui.h"
#include "Gui/DockablePanel.h"

using namespace Natron;
using std::make_pair; using std::pair;


/*Class inheriting Knob and KnobGui, must have a function named BuildKnob and BuildKnobGui with the following signature.
 This function should in turn call a specific class-based static function with the appropriate param.*/
typedef Knob* (*KnobBuilder)(KnobHolder*  holder,const std::string& description,int dimension);

typedef KnobGui* (*KnobGuiBuilder)(Knob* knob,DockablePanel*);

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
    std::vector<LibraryBinary*> plugins = AppManager::loadPlugins(NATRON_KNOBS_PLUGINS_PATH);
    std::vector<std::string> functions;
    functions.push_back("BuildKnob");
    functions.push_back("BuildKnobGui");
    for (U32 i = 0; i < plugins.size(); ++i) {
        if (plugins[i]->loadFunctions(functions)) {
            std::pair<bool,KnobBuilder> builder = plugins[i]->findFunction<KnobBuilder>("BuildKnob");
            if(builder.first){
                Knob* knob = builder.second(NULL,"",1);
                _loadedKnobs.insert(make_pair(knob->typeName(), plugins[i]));
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
    Knob* fileKnob = File_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> FILEfunctions;
    FILEfunctions.insert(make_pair("BuildKnob",(void*)&File_Knob::BuildKnob));
    FILEfunctions.insert(make_pair("BuildKnobGui",(void*)&File_KnobGui::BuildKnobGui));
    LibraryBinary *FILEKnobPlugin = new LibraryBinary(FILEfunctions);
    _loadedKnobs.insert(make_pair(fileKnob->typeName(),FILEKnobPlugin));
    delete fileKnob;
    
    Knob* intKnob = Int_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> INTfunctions;
    INTfunctions.insert(make_pair("BuildKnob",(void*)&Int_Knob::BuildKnob));
    INTfunctions.insert(make_pair("BuildKnobGui",(void*)&Int_KnobGui::BuildKnobGui));
    LibraryBinary *INTKnobPlugin = new LibraryBinary(INTfunctions);
    _loadedKnobs.insert(make_pair(intKnob->typeName(),INTKnobPlugin));
    delete intKnob;
    
    
    Knob* doubleKnob = Double_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> DOUBLEfunctions;
    DOUBLEfunctions.insert(make_pair("BuildKnob",(void*)&Double_Knob::BuildKnob));
    DOUBLEfunctions.insert(make_pair("BuildKnobGui",(void*)&Double_KnobGui::BuildKnobGui));
    LibraryBinary *DOUBLEKnobPlugin = new LibraryBinary(DOUBLEfunctions);
    _loadedKnobs.insert(make_pair(doubleKnob->typeName(),DOUBLEKnobPlugin));
    delete doubleKnob;
    
    Knob* boolKnob = Bool_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> BOOLfunctions;
    BOOLfunctions.insert(make_pair("BuildKnob",(void*)&Bool_Knob::BuildKnob));
    BOOLfunctions.insert(make_pair("BuildKnobGui",(void*)&Bool_KnobGui::BuildKnobGui));
    LibraryBinary *BOOLKnobPlugin = new LibraryBinary(BOOLfunctions);
    _loadedKnobs.insert(make_pair(boolKnob->typeName(),BOOLKnobPlugin));
    delete boolKnob;
    
    Knob* buttonKnob = Button_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> BUTTONfunctions;
    BUTTONfunctions.insert(make_pair("BuildKnob",(void*)&Button_Knob::BuildKnob));
    BUTTONfunctions.insert(make_pair("BuildKnobGui",(void*)&Button_KnobGui::BuildKnobGui));
    LibraryBinary *BUTTONKnobPlugin = new LibraryBinary(BUTTONfunctions);
    _loadedKnobs.insert(make_pair(buttonKnob->typeName(),BUTTONKnobPlugin));
    delete buttonKnob;
    
    Knob* outputFileKnob = OutputFile_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> OFfunctions;
    OFfunctions.insert(make_pair("BuildKnob",(void*)&OutputFile_Knob::BuildKnob));
    OFfunctions.insert(make_pair("BuildKnobGui",(void*)&OutputFile_KnobGui::BuildKnobGui));
    LibraryBinary *OUTPUTFILEKnobPlugin = new LibraryBinary(OFfunctions);
    _loadedKnobs.insert(make_pair(outputFileKnob->typeName(),OUTPUTFILEKnobPlugin));
    delete outputFileKnob;
    
    Knob* comboBoxKnob = ComboBox_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> CBBfunctions;
    CBBfunctions.insert(make_pair("BuildKnob",(void*)&ComboBox_Knob::BuildKnob));
    CBBfunctions.insert(make_pair("BuildKnobGui",(void*)&ComboBox_KnobGui::BuildKnobGui));
    LibraryBinary *ComboBoxKnobPlugin = new LibraryBinary(CBBfunctions);
    _loadedKnobs.insert(make_pair(comboBoxKnob->typeName(),ComboBoxKnobPlugin));
    delete comboBoxKnob;
    
    
    Knob* separatorKnob = Separator_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> Sepfunctions;
    Sepfunctions.insert(make_pair("BuildKnob",(void*)&Separator_Knob::BuildKnob));
    Sepfunctions.insert(make_pair("BuildKnobGui",(void*)&Separator_KnobGui::BuildKnobGui));
    LibraryBinary *SeparatorKnobPlugin = new LibraryBinary(Sepfunctions);
    _loadedKnobs.insert(make_pair(separatorKnob->typeName(),SeparatorKnobPlugin));
    delete separatorKnob;
    
    Knob* groupKnob = Group_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> Grpfunctions;
    Grpfunctions.insert(make_pair("BuildKnob",(void*)&Group_Knob::BuildKnob));
    Grpfunctions.insert(make_pair("BuildKnobGui",(void*)&Group_KnobGui::BuildKnobGui));
    LibraryBinary *GroupKnobPlugin = new LibraryBinary(Grpfunctions);
    _loadedKnobs.insert(make_pair(groupKnob->typeName(),GroupKnobPlugin));
    delete groupKnob;
    
    Knob* rgbaKnob = Color_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> RGBAfunctions;
    RGBAfunctions.insert(make_pair("BuildKnob",(void*)&Color_Knob::BuildKnob));
    RGBAfunctions.insert(make_pair("BuildKnobGui",(void*)&Color_KnobGui::BuildKnobGui));
    LibraryBinary *RGBAKnobPlugin = new LibraryBinary(RGBAfunctions);
    _loadedKnobs.insert(make_pair(rgbaKnob->typeName(),RGBAKnobPlugin));
    delete rgbaKnob;
    
    Knob* strKnob = String_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> STRfunctions;
    STRfunctions.insert(make_pair("BuildKnob",(void*)&String_Knob::BuildKnob));
    STRfunctions.insert(make_pair("BuildKnobGui",(void*)&String_KnobGui::BuildKnobGui));
    LibraryBinary *STRKnobPlugin = new LibraryBinary(STRfunctions);
    _loadedKnobs.insert(make_pair(strKnob->typeName(),STRKnobPlugin));
    delete strKnob;
    
    Knob* richTxtKnob = RichText_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> RICHTXTfunctions;
    RICHTXTfunctions.insert(make_pair("BuildKnob",(void*)&RichText_Knob::BuildKnob));
    RICHTXTfunctions.insert(make_pair("BuildKnobGui",(void*)&RichText_KnobGui::BuildKnobGui));
    LibraryBinary *RICHTXTKnobPlugin = new LibraryBinary(RICHTXTfunctions);
    _loadedKnobs.insert(make_pair(richTxtKnob->typeName(),RICHTXTKnobPlugin));
    delete richTxtKnob;
}


Knob* KnobFactory::createKnob(const std::string& id,
                              KnobHolder*  holder,
                              const std::string& description,int dimension) const{

    std::map<std::string,LibraryBinary*>::const_iterator it = _loadedKnobs.find(id);
    if(it == _loadedKnobs.end()){
        return NULL;
    }else{
        std::pair<bool,KnobBuilder> builderFunc = it->second->findFunction<KnobBuilder>("BuildKnob");
        if(!builderFunc.first){
            return NULL;
        }
        KnobBuilder builder = (KnobBuilder)(builderFunc.second);
        Knob* knob = builder(holder,description,dimension);
        if(!knob){
            return NULL;
        }
        return knob;
    }
}
KnobGui* KnobFactory::createGuiForKnob(Knob* knob,DockablePanel* container) const {
    std::map<std::string,LibraryBinary*>::const_iterator it = _loadedKnobs.find(knob->typeName());
    if(it == _loadedKnobs.end()){
        return NULL;
    }else{
        std::pair<bool,KnobGuiBuilder> guiBuilderFunc = it->second->findFunction<KnobGuiBuilder>("BuildKnobGui");
        if(!guiBuilderFunc.first){
            return NULL;
        }
        KnobGuiBuilder guiBuilder = (KnobGuiBuilder)(guiBuilderFunc.second);
        return guiBuilder(knob,container);
    }
}





/***********************************KNOB BASE******************************************/

struct Knob::KnobPrivate {
    KnobHolder*  _holder;
    std::vector<U64> _hashVector;
    std::string _description;//< the text label that will be displayed  on the GUI
    QString _name;//< the knob can have a name different than the label displayed on GUI.
    //By default this is the same as _description but can be set by calling setName().
    bool _newLine;
    int _itemSpacing;

    Knob* _parentKnob;
    bool _secret;
    bool _enabled;
    bool _canUndo;
    bool _isInsignificant; //< if true, a value change will never trigger an evaluation
    bool _isPersistent;//will it be serialized?
    std::string _tooltipHint;
    bool _isAnimationEnabled;

    KnobPrivate(KnobHolder*  holder,const std::string& description)
        : _holder(holder)
        , _hashVector()
        , _description(description)
        , _name(description.c_str())
        , _newLine(true)
        , _itemSpacing(0)
        , _parentKnob(NULL)
        , _secret(false)
        , _enabled(true)
        , _canUndo(true)
        , _isInsignificant(false)
        , _isPersistent(true)
        , _tooltipHint()
        , _isAnimationEnabled(true)
    {

    }

    void updateHash(const std::map<int,Variant>& value){

        _hashVector.clear();

        for(std::map<int,Variant>::const_iterator it = value.begin();it!=value.end();++it){
            QByteArray data;
            QDataStream ds(&data,QIODevice::WriteOnly);
            ds << it->second;
            data = data.toBase64();
            QString str(data);
            for (int i = 0; i < str.size(); ++i) {
                _hashVector.push_back(str.at(i).unicode());
            }
        }

        _holder->invalidateHash();
    }

};

Knob::Knob(KnobHolder* holder,const std::string& description,int dimension)
    : AnimatingParam(dimension)
    , _imp(new KnobPrivate(holder,description))
{
    
    if(_imp->_holder){
        _imp->_holder->addKnob(boost::shared_ptr<Knob>(this));
    }
}


Knob::~Knob(){    
    remove();
}

void Knob::remove(){
    emit deleted();
    if(_imp->_holder){
        _imp->_holder->removeKnob(this);
    }
}

//void Knob::onKnobUndoneChange(){
//    _imp->_holder->triggerAutoSave();
//}

//void Knob::onKnobRedoneChange(){
//    _imp->_holder->triggerAutoSave();
//}


void Knob::onStartupRestoration(const AnimatingParam& other){
    clone(other); //set the value
    bool wasBeginCalled = true;
    if(!_imp->_holder->wasBeginCalled()){
        wasBeginCalled = false;
        _imp->_holder->beginValuesChanged(AnimatingParam::PLUGIN_EDITED,true);
    }
    for(int i = 0 ; i < getDimension();++i){
        evaluateValueChange(i,AnimatingParam::PLUGIN_EDITED);
    }
    if(!wasBeginCalled){
        _imp->_holder->endValuesChanged(AnimatingParam::PLUGIN_EDITED);
    }
    emit restorationComplete();
}



void Knob::onValueChanged(int dimension,const Variant& variant){
    setValue(variant, dimension,AnimatingParam::USER_EDITED);
}


void Knob::evaluateAnimationChange(){
    
    SequenceTime time = _imp->_holder->getApp()->getTimeLine()->currentFrame();
    bool wasBeginCalled = true;
    if(!_imp->_holder->wasBeginCalled()){
        wasBeginCalled = false;
        beginValueChange(AnimatingParam::PLUGIN_EDITED);
    }
    for(int i = 0; i < getDimension();++i){
        boost::shared_ptr<Curve> curve = getCurve(i);
        if(curve && curve->isAnimated()){
            Variant v = getValueAtTime(time,i);
            setValue(v,i);
        }
    }
    if(!wasBeginCalled){
        endValueChange(AnimatingParam::PLUGIN_EDITED);
    }
}

void Knob::beginValueChange(AnimatingParam::ValueChangedReason reason) {
    _imp->_holder->beginValuesChanged(reason,!_imp->_isInsignificant);
}

void Knob::endValueChange(AnimatingParam::ValueChangedReason reason) {
    _imp->_holder->endValuesChanged(reason);
}

void Knob::evaluateValueChange(int dimension,AnimatingParam::ValueChangedReason reason){
    if(!_imp->_isInsignificant)
        _imp->updateHash(getValueForEachDimension());
    processNewValue();
    if(reason != AnimatingParam::USER_EDITED){
        emit valueChanged(dimension);
    }
    _imp->_holder->onValueChanged(this,reason,!_imp->_isInsignificant);
}

void Knob::onTimeChanged(SequenceTime time){
    if(isAnimationEnabled()){
        _imp->_holder->beginValuesChanged(AnimatingParam::TIME_CHANGED,false); // we do not want to force a re-evaluation
        for(int i = 0; i < getDimension();++i){
            boost::shared_ptr<Curve> curve = getCurve(i);
            if(curve && curve->isAnimated()){
                Variant v = getValueAtTime(time,i);
                setValue(v,i);
            }
        }
        _imp->_holder->endValuesChanged(AnimatingParam::TIME_CHANGED);

    }
}



void Knob::cloneValue(const Knob& other){
    assert(_imp->_name == other._imp->_name);
    _imp->_hashVector = other._imp->_hashVector;
    clone(dynamic_cast<const AnimatingParam&>(other));
    cloneExtraData(other);
}

void Knob::turnOffNewLine(){
    _imp->_newLine = false;
}

void Knob::setSpacingBetweenItems(int spacing){
    _imp->_itemSpacing = spacing;
}
void Knob::setEnabled(bool b){
    _imp->_enabled = b;
    emit enabledChanged();
}

void Knob::setSecret(bool b){
    _imp->_secret = b;
    emit secretChanged();
}

int Knob::determineHierarchySize() const{
    int ret = 0;
    Knob* current = getParentKnob();
    while(current){
        ++ret;
        current = current->getParentKnob();
    }
    return ret;
}


const std::string& Knob::getDescription() const { return _imp->_description; }

const std::vector<U64>& Knob::getHashVector() const { return _imp->_hashVector; }

KnobHolder*  Knob::getHolder() const { return _imp->_holder; }

void Knob::turnOffAnimation() { _imp->_isAnimationEnabled = false; }

bool Knob::isAnimationEnabled() const { return canAnimate() && _imp->_isAnimationEnabled; }

void Knob::setName(const std::string& name) {_imp->_name = QString(name.c_str());}

std::string Knob::getName() const {return _imp->_name.toStdString();}

void Knob::setParentKnob(Knob* knob){ _imp->_parentKnob = knob;}

Knob* Knob::getParentKnob() const {return _imp->_parentKnob;}

bool Knob::isSecret() const {return _imp->_secret;}

bool Knob::isEnabled() const {return _imp->_enabled;}

void Knob::setInsignificant(bool b) {_imp->_isInsignificant = b;}

bool Knob::isPersistent() const { return _imp->_isPersistent; }

void Knob::setPersistent(bool b) { _imp->_isPersistent = b; }

void Knob::turnOffUndoRedo() {_imp->_canUndo = false;}

bool Knob::canBeUndone() const {return _imp->_canUndo;}

bool Knob::isInsignificant() const {return _imp->_isInsignificant;}

void Knob::setHintToolTip(const std::string& hint) {_imp->_tooltipHint = hint;}

const std::string& Knob::getHintToolTip() const {return _imp->_tooltipHint;}

/***************************KNOB HOLDER******************************************/

KnobHolder::KnobHolder(AppInstance* appInstance):
    _app(appInstance)
  , _knobs()
  , _betweenBeginEndParamChanged(false)
  , _insignificantChange(false)
  , _knobsChanged()
{}

KnobHolder::~KnobHolder(){
    _knobs.clear();
}

void KnobHolder::invalidateHash(){
    _app->incrementKnobsAge();
}
int KnobHolder::getAppAge() const{
    return _app->getKnobsAge();
}


void KnobHolder::beginValuesChanged(AnimatingParam::ValueChangedReason reason, bool isSignificant){
    if(_betweenBeginEndParamChanged)
        return;
    _betweenBeginEndParamChanged = true ;
    _insignificantChange = !isSignificant;
    beginKnobsValuesChanged(reason);
}

void KnobHolder::endValuesChanged(AnimatingParam::ValueChangedReason reason){
    if(!_betweenBeginEndParamChanged){
        return;
    }

    if(reason == AnimatingParam::USER_EDITED){
        triggerAutoSave();
    }
    _betweenBeginEndParamChanged = false ;
    if(!_knobsChanged.empty() && reason != AnimatingParam::TIME_CHANGED){
        evaluate(_knobsChanged.back(),!_insignificantChange);
    }
    _knobsChanged.clear();
    endKnobsValuesChanged(reason);
}

void KnobHolder::onValueChanged(Knob* k, AnimatingParam::ValueChangedReason reason, bool isSignificant){
    bool wasBeginCalled = true;
    if(!_betweenBeginEndParamChanged){
        beginValuesChanged(reason,isSignificant);
        wasBeginCalled = false;
    }
    _knobsChanged.push_back(k);
    onKnobValueChanged(k,reason);
    if(!wasBeginCalled){
        endValuesChanged(reason);
    }

}
void KnobHolder::cloneKnobs(const KnobHolder& other){
    assert(_knobs.size() == other._knobs.size());
    for(U32 i = 0 ; i < other._knobs.size();++i){
        _knobs[i]->cloneValue(*(other._knobs[i]));
    }
}



void KnobHolder::removeKnob(Knob* knob){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        if (_knobs[i].get() == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
}

void KnobHolder::triggerAutoSave(){
    _app->triggerAutoSave();
}

void KnobHolder::refreshAfterTimeChange(SequenceTime time){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        _knobs[i]->onTimeChanged(time);
    }
}
/***********************************FILE_KNOB*****************************************/


void File_Knob::processNewValue(){
    QMutexLocker locker(&_fileSequenceLock);
    _filesSequence.clear();
    QStringList fileNameList = getValue<QStringList>();
    bool first_time = true;
    QString originalName;
    for(int i = 0 ; i < fileNameList.size();++i){
        QString fileName = fileNameList.at(i);
        QString unModifiedName = fileName;
        if(first_time){
            int extensionPos = fileName.lastIndexOf('.');
            if(extensionPos != -1){
                fileName = fileName.left(extensionPos);
            }else{
                continue;
            }
            int j = fileName.size()-1;
            QString frameIndexStr;
            while(j > 0 && fileName.at(j).isDigit()){ frameIndexStr.push_front(fileName.at(j)); --j;}
            if(j > 0){
                int frameNumber = 0;
                if(fileNameList.size() > 1){
                    frameNumber = frameIndexStr.toInt();
                }
                _filesSequence.insert(make_pair(frameNumber,unModifiedName));
                originalName = fileName.left(fileName.indexOf(frameIndexStr));
                
            }else{
                _filesSequence.insert(make_pair(0,unModifiedName));
            }
            first_time=false;
        }else{
            if(fileName.contains(originalName)){
                int extensionPos = fileName.lastIndexOf('.');
                if(extensionPos != -1){
                    fileName = fileName.left(extensionPos);
                }else{
                    continue;
                }
                int j = fileName.size()-1;
                QString frameIndexStr;
                while(j > 0 && fileName.at(j).isDigit()){ frameIndexStr.push_front(fileName.at(j)); --j;}
                if(j > 0){
                    int number = frameIndexStr.toInt();
                    _filesSequence.insert(make_pair(number,unModifiedName));
                }else{
                    std::cout << " File_Knob : WARNING !! several frames in sequence but no frame count found in their name " << std::endl;
                }
            }
        }
    }

}

void File_Knob::cloneExtraData(const Knob& other){
    _filesSequence = dynamic_cast<const File_Knob&>(other)._filesSequence;
}


int File_Knob::firstFrame() const{
    QMutexLocker locker(&_fileSequenceLock);
    std::map<int,QString>::const_iterator it = _filesSequence.begin();
    if (it == _filesSequence.end()) {
        return INT_MIN;
    }
    return it->first;
}
int File_Knob::lastFrame() const {
    QMutexLocker locker(&_fileSequenceLock);
    std::map<int,QString>::const_iterator it = _filesSequence.end();
    if(it == _filesSequence.begin()) {
        return INT_MAX;
    }
    --it;
    return it->first;
}
int File_Knob::nearestFrame(int f) const{
    int first = firstFrame();
    int last = lastFrame();
    if(f < first) return first;
    if(f > last) return last;
    
    std::map<int,int> distanceMap;
    QMutexLocker locker(&_fileSequenceLock);
    for (std::map<int,QString>::const_iterator it = _filesSequence.begin(); it!=_filesSequence.end(); ++it) {
        distanceMap.insert(make_pair(std::abs(f - it->first), it->first));
    }
    if(!distanceMap.empty())
        return distanceMap.begin()->second;
    else
        return 0;
}
QString File_Knob::getRandomFrameName(int f,bool loadNearestIfNotFound) const{
    if(loadNearestIfNotFound)
        f = nearestFrame(f);
    QMutexLocker locker(&_fileSequenceLock);
    std::map<int, QString>::const_iterator it = _filesSequence.find(f);
    if(it!=_filesSequence.end()){
        return it->second;
    }else{
        return "";
    }
}

/***********************************OUTPUT_FILE_KNOB*****************************************/


std::string OutputFile_Knob::getFileName() const{
    return  getValue<QString>().toStdString();
}



/***********************************GROUP_KNOB*****************************************/


void Group_Knob::addKnob(Knob* k){
    _children.push_back(k);
}
/***********************************TAB_KNOB*****************************************/


void Tab_Knob::addTab(const std::string& name){
    _knobs.insert(make_pair(name, std::vector<Knob*>()));
}

void Tab_Knob::addKnob(const std::string& tabName,Knob* k){
    std::map<std::string,std::vector<Knob*> >::iterator it = _knobs.find(tabName);
    if(it == _knobs.end()){
        pair<std::map<std::string,std::vector<Knob*> >::iterator,bool> ret = _knobs.insert(make_pair(tabName, std::vector<Knob*>()));
        ret.first->second.push_back(k);
    }else{
        it->second.push_back(k);
    }
}

