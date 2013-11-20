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

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"


#include "Readers/Reader.h"

#include "Gui/SequenceFileDialog.h"
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


/************************************KEYFRAME************************************/
KeyFrame::KeyFrame(double time,const Variant& initialValue)
    : _value(initialValue)
    , _time(time)
    , _leftTangent()
    , _rightTangent()
{

}

AnimationCurve::AnimationCurve(Interpolation type)
    : _interpolation(type)
{
}


Variant AnimationCurve::getValueAt(double t) const {
    assert(_controlPoints.size() >= 2);
}


/***********************************KNOB BASE******************************************/


Knob::Knob(KnobHolder* holder,const std::string& description,int dimension):
    _holder(holder)
  , _value()
  , _hashVector()
  , _dimension(dimension)
  , _description(description)
  , _name(description.c_str())
  , _newLine(true)
  , _itemSpacing(0)
  , _parentKnob(NULL)
  , _visible(true)
  , _enabled(true)
  , _canUndo(true)
  , _isInsignificant(false)
  , _tooltipHint()
  , _keys()
{
    
    if(_holder){
        _holder->addKnob(this);

        //default initialize the values map
        for(int i = 0; i < dimension ; ++i){
            _value.insert(std::make_pair(i,Variant()));
        }
    }
}

Knob::~Knob(){
    if(_holder){
        _holder->removeKnob(this);
    }
    _value.clear();
    emit deleted();

}

void Knob::onKnobUndoneChange(){
    _holder->triggerAutoSave();
}

void Knob::onKnobRedoneChange(){
    _holder->triggerAutoSave();
}

void Knob::updateHash(){
    fillHashVector();
    _holder->invalidateHash();
}

void Knob::restoreFromString(const std::string& str){
    _restoreFromString(str);
    if(!_isInsignificant)
        updateHash();
    processNewValue();
    _holder->onValueChanged(this,Knob::STARTUP_RESTORATION);

    for(std::map<int,Variant>::const_iterator it = _value.begin();it!=_value.end();++it){
        emit valueChanged(it->first,it->second);
    }
}

void Knob::setValueInternal(const Variant& v,int dimension){
    const std::map<int,Variant>::iterator it = _value.find(dimension);
    assert(it != _value.end());
    it->second = v;
    if(!_isInsignificant)
        updateHash();
    processNewValue();
    emit valueChanged(dimension,v);
    _holder->onValueChanged(this,Knob::PLUGIN_EDITED);
    _holder->evaluate(this,!_isInsignificant);

}

void Knob::setValueAtTimeInternal(double time, const Variant& v, int dimension){
    assert(canAnimate());
    MultiDimensionalKeys::iterator it = _keys.find(dimension);

    /*if the dimension has no key yet, insert a new map*/
    if( it == _keys.end() ){
        it = _keys.insert(std::make_pair(dimension,Keys())).first;
    }

    it->second.insert(std::make_pair(time,v));
}

Knob::Keys Knob::getKeys(int dimension){
    MultiDimensionalKeys::const_iterator foundDimension = _keys.find(dimension);
    if(foundDimension == _keys.end()){
        return Knob::Keys();
    }else{
        return foundDimension->second;
    }
}

Variant Knob::getValueAtTimeInternal(double time,int dimension) const{
    assert(canAnimate());
    MultiDimensionalKeys::const_iterator foundDimension = _keys.find(dimension);

    if(foundDimension == _keys.end()){
        /*if the knob as no keys at this time, return the value
        at the requested dimension.*/
        std::map<int,Variant>::const_iterator it = _value.find(dimension);
        if(it != _value.end()){
            return it->second;
        }else{
            return Variant();
        }
    }

    /*FOR NOW RETURN THE NEAREST KEY, INTERPOLATION NOT IMPLEMENTED YET*/

    const Keys& keys = foundDimension->second;

    if(keys.size() == 1){
        return keys.begin()->second;
    }
    //finding a matching or the first greater key
    Keys::const_iterator upper = keys.lower_bound(time);
    //decrement the iterator to find the previous
    Keys::const_iterator lower = upper;
    --lower;
    // if the found iterator points at the end of the container, return the last element
    if (upper == keys.end())
        return lower->second;
    //if we found a matching key or the key is the begin, return it
    if (upper == keys.begin() || upper->first == time)
        return upper->second;

    //we're in the middle interpolate between values
    //if the previous is closer to the searchedkey than the found iterator, return it
    if ((time - lower->first) < (upper->first - time))
        return lower->second;
    return upper->second;

}

void Knob::onValueChanged(int dimension,const Variant& variant){
    
    const std::map<int,Variant>::iterator it = _value.find(dimension);
    assert(it != _value.end());
    it->second = variant;
    if(!_isInsignificant)
        updateHash();
    processNewValue();
    _holder->onValueChanged(this,Knob::USER_EDITED);
    _holder->evaluate(this,!_isInsignificant);
    
}

void KnobHolder::beginValuesChanged(Knob::ValueChangedReason reason){
    _betweenBeginEndParamChanged = true ;
    beginKnobsValuesChanged(reason);
}

void KnobHolder::endValuesChanged(Knob::ValueChangedReason reason){
    assert(_betweenBeginEndParamChanged);
    _betweenBeginEndParamChanged = false ;
    endKnobsValuesChanged(reason);
}

void KnobHolder::onValueChanged(Knob* k,Knob::ValueChangedReason reason){
    bool wasBeginCalled = true;
    if(!_betweenBeginEndParamChanged){
        beginValuesChanged(reason);
        wasBeginCalled = false;
    }
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


void Knob::cloneValue(const Knob& other){
    assert(_name == other._name);
    _hashVector = other._hashVector;
    _value = other._value;
    cloneExtraData(other);
}

void KnobHolder::invalidateHash(){
    _app->incrementKnobsAge();
}
int KnobHolder::getAppAge() const{
    return _app->getKnobsAge();
}

void Knob::turnOffNewLine(){
    _newLine = false;
}

void Knob::setSpacingBetweenItems(int spacing){
    _itemSpacing = spacing;
}
void Knob::setEnabled(bool b){
    _enabled = b;
    emit enabled(b);
}

void Knob::setVisible(bool b){
    _visible = b;
    emit visible(b);
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


void KnobHolder::removeKnob(Knob* knob){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        if (_knobs[i] == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
}

void KnobHolder::triggerAutoSave(){
    _app->triggerAutoSave();
}
/***********************************FILE_KNOB*****************************************/
std::string File_Knob::serialize() const{
    return SequenceFileDialog::patternFromFilesList(getValue<QStringList>()).toStdString();
}

void File_Knob::_restoreFromString(const std::string& str){
    QStringList filesList = SequenceFileDialog::filesListFromPattern(str.c_str());
    setValue<QStringList>(filesList);
}
void File_Knob::fillHashVector(){
    _hashVector.clear();
    QStringList files = getValue<QStringList>();
    for (int i = 0; i < files.size(); ++i) {
        const QString& file = files.at(i);
        for (int j = 0; j < file.size(); ++j) {
            _hashVector.push_back(file.at(j).unicode());
        }
    }
}

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

std::string OutputFile_Knob::serialize() const{
    return getValue<QString>().toStdString();
}
void OutputFile_Knob::_restoreFromString(const std::string& str){
    setValue<QString>(str.c_str());
}

std::string OutputFile_Knob::getFileName() const{
    return  getValue<QString>().toStdString();
}

void OutputFile_Knob::fillHashVector(){
    _hashVector.clear();
    QString file = getValue<QString>();
    for (int j = 0; j < file.size(); ++j) {
        _hashVector.push_back(file.at(j).unicode());
    }
}

/***********************************INT_KNOB*****************************************/
void Int_Knob::fillHashVector(){
    _hashVector.clear();
    for(std::map<int,Variant>::const_iterator it = _value.begin();it!=_value.end();++it){
        _hashVector.push_back((U64)it->second.toInt());
    }
}

std::string Int_Knob::serialize() const{
    QString str;
    for(std::map<int,Variant>::const_iterator it = _value.begin();it!=_value.end();++it){
        str.append("v");
        str.append(QString::number(it->first));
        str.append(" ");
        str.append(QString::number(it->second.toInt()));
        std::map<int,Variant>::const_iterator next = it;
        ++next;
        if(next != _value.end()){
            str.append(" ");
        }

    }
    return str.toStdString();
}
void Int_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){

        int i = s.indexOf("v");
        while(i != -1){
            ++i; // skip " "
            QString dimStr;
            while(i < s.size() && s.at(i).isDigit()){
                dimStr.append(s.at(i));
                ++i;
            }
            ++i;//skip " "
            QString vStr;
            while(i < s.size() && s.at(i).isDigit()){
                vStr.append(s.at(i));
                ++i;
            }
            setValue<int>(vStr.toInt(),dimStr.toInt());
            i = s.indexOf("v",i);
        }
    }
}


/***********************************BOOL_KNOB*****************************************/
void Bool_Knob::fillHashVector(){
    _hashVector.clear();
    _hashVector.push_back((U64)getValue<bool>());
}

std::string Bool_Knob::serialize() const{
    return getValue<bool>() ? "1" : "0";
}
void Bool_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){
        int val = s.toInt();
        setValue<bool>((bool)val);
    }
    
}


/***********************************DOUBLE_KNOB*****************************************/
void Double_Knob::fillHashVector(){
    _hashVector.clear();
    for(std::map<int,Variant>::const_iterator it = _value.begin();it!=_value.end();++it){
        double d = it->second.toDouble();
        _hashVector.push_back(*(reinterpret_cast<U64*>(&(d))));
    }
}

std::string Double_Knob::serialize() const{
    QString str;
    for(std::map<int,Variant>::const_iterator it = _value.begin();it!=_value.end();++it){
        str.append("v");
        str.append(QString::number(it->first));
        str.append(" ");
        str.append(QString::number(it->second.toDouble()));
        std::map<int,Variant>::const_iterator next = it;
        ++next;
        if(next != _value.end()){
            str.append(" ");
        }

    }
    return str.toStdString();
    
}
void Double_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){

        int i = s.indexOf("v");
        while(i != -1){
            ++i; // skip " "
            QString dimStr;
            while(i < s.size() && s.at(i).isDigit()){
                dimStr.append(s.at(i));
                ++i;
            }
            ++i;//skip " "
            QString vStr;
            while(i < s.size() && s.at(i).isDigit()){
                vStr.append(s.at(i));
                ++i;
            }
            setValue<double>(vStr.toDouble(),dimStr.toInt());
            i = s.indexOf("v",i);
        }
    }
}



/***********************************COMBOBOX_KNOB*****************************************/
void ComboBox_Knob::fillHashVector(){
    _hashVector.clear();
    _hashVector.push_back(getValue<int>());
}

std::string ComboBox_Knob::serialize() const{
    return QString::number(getValue<int>()).toStdString();
}
void ComboBox_Knob::_restoreFromString(const std::string& str){
    setValue<int>(QString(str.c_str()).toInt());
}


/***********************************RGBA_KNOB*****************************************/
void Color_Knob::fillHashVector(){
    double r = getValue<double>(0);
    _hashVector.push_back(*(reinterpret_cast<U64*>(&(r))));

    if(getDimension() >= 3){
        double g = getValue<double>(1);
        double b = getValue<double>(2);
        _hashVector.push_back(*(reinterpret_cast<U64*>(&(g))));
        _hashVector.push_back(*(reinterpret_cast<U64*>(&(b))));
    }
    if(getDimension() >= 4){
        double a = getValue<double>(3);
        _hashVector.push_back(*(reinterpret_cast<U64*>(&(a))));
    }
}
std::string Color_Knob::serialize() const{
    QString ret;
    ret.append(QString("r " + QString::number(getValue<double>(0))));

    if(getDimension() >= 3){
        ret.append(QString(" g " + QString::number(getValue<double>(1))));
        ret.append(QString(" b " + QString::number(getValue<double>(2))));
    }
    if(getDimension() >= 4){
        ret.append(QString(" a " + QString::number(getValue<double>(3))));
    }
    return ret.toStdString();
}

void Color_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    int i = s.indexOf("r");
    if(i == -1)
        return;
    i+=2;
    QString rStr,gStr,bStr,aStr;
    while(i < s.size() && s.at(i).isDigit()){
        rStr.append(s.at(i));
        ++i;
    }
    setValue(rStr.toDouble(),0);
    i = s.indexOf("g");
    if(i == -1)
        return;
    i+=2;
    while(i < s.size() && s.at(i).isDigit()){
        gStr.append(s.at(i));
        ++i;
    }
    setValue(gStr.toDouble(),1);
    i = s.indexOf("b");
    if(i == -1)
        return;
    i+=2;
    while(i < s.size() && s.at(i).isDigit()){
        bStr.append(s.at(i));
        ++i;
    }
    setValue(bStr.toDouble(),2);
    i = s.indexOf("a");
    if(i == -1)
        return;
    i+=2;
    while(i < s.size() && s.at(i).isDigit()){
        aStr.append(s.at(i));
        ++i;
    }
    setValue(aStr.toDouble(),3);
    
}
/***********************************STRING_KNOB*****************************************/
void String_Knob::fillHashVector(){
    _hashVector.clear();
    QString str = getValue<QString>();
    for (int i = 0; i < str.size(); ++i) {
        _hashVector.push_back(str.at(i).unicode());
    }
}


std::string String_Knob::serialize() const{
    return getValue<QString>().toStdString();
}

void String_Knob::_restoreFromString(const std::string& str){
    setValue<QString>(str.c_str());
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
/******************************RichText_Knob**************************************/

void RichText_Knob::fillHashVector(){
    _hashVector.clear();
    QString str = getValue<QString>();
    for (int i = 0; i < str.size(); ++i) {
        _hashVector.push_back(str.at(i).unicode());
    }
}

std::string RichText_Knob::serialize() const{
    return getValue<QString>().toStdString();
}

void RichText_Knob::_restoreFromString(const std::string& str){
    setValue<QString>(str.c_str());
}
