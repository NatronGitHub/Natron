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

using namespace Powiter;
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
    std::vector<LibraryBinary*> plugins = AppManager::loadPlugins(POWITER_KNOBS_PLUGINS_PATH);
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
    
    Knob* rgbaKnob = RGBA_Knob::BuildKnob(NULL,stub,1);
    
    std::map<std::string,void*> RGBAfunctions;
    RGBAfunctions.insert(make_pair("BuildKnob",(void*)&RGBA_Knob::BuildKnob));
    RGBAfunctions.insert(make_pair("BuildKnobGui",(void*)&RGBA_KnobGui::BuildKnobGui));
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


Knob::Knob(KnobHolder* holder,const std::string& description,int dimension):
_holder(holder),
_value(),
_hashVector(),
_dimension(dimension),
_description(description),
_name(description.c_str()),
_newLine(true),
_itemSpacing(0),
_parentKnob(NULL),
_visible(true),
_enabled(true),
_canUndo(true),
_isInsignificant(false),
_tooltipHint(),
_hasPostedValue(false)
{
    
    if(_holder){
        _holder->addKnob(this);
    }
}

Knob::~Knob(){
    if(_holder)
        _holder->removeKnob(this);
    emit deleted();
}

void Knob::onKnobUndoneChange(){
    _holder->triggerAutoSave();
}

void Knob::onKnobRedoneChange(){
    _holder->triggerAutoSave();
}


void Knob::restoreFromString(const std::string& str){
    _restoreFromString(str);
    fillHashVector();
    processNewValue();
    emit valueChanged(_value);
}

void Knob::setValueInternal(const Variant& v){
    _value = v;
    fillHashVector();
    processNewValue();
    emit valueChanged(v);
    _holder->onValueChanged(this,Knob::PLUGIN_EDITED);
    if(!_isInsignificant)
        _holder->evaluate(this);

}

void Knob::onValueChanged(const Variant& variant){
    
    _value = variant;
    fillHashVector();
    processNewValue();
    _holder->onValueChanged(this,Knob::USER_EDITED);
    if(!_isInsignificant)
        _holder->evaluate(this);
    
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
    _value = other._value;
    cloneExtraData(other);
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
    return SequenceFileDialog::patternFromFilesList(_value.toStringList()).toStdString();
}

void File_Knob::_restoreFromString(const std::string& str){
    QStringList filesList = SequenceFileDialog::filesListFromPattern(str.c_str());
    _value.setValue(filesList);
}
void File_Knob::fillHashVector(){
    _hashVector.clear();
    QStringList files = _value.toStringList();
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
    QStringList fileNameList = _value.toStringList();
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
    return _value.toString().toStdString();
}
void OutputFile_Knob::_restoreFromString(const std::string& str){
    _value.setValue(str);
}


void OutputFile_Knob::fillHashVector(){
    _hashVector.clear();
    QString file = _value.toString();
    for (int j = 0; j < file.size(); ++j) {
        _hashVector.push_back(file.at(j).unicode());
    }
}

/***********************************INT_KNOB*****************************************/
void Int_Knob::fillHashVector(){
    _hashVector.clear();
    if(_dimension > 1){
        QList<QVariant> list = _value.toList();
        for (int i = 0; i < list.size(); ++i) {
            _hashVector.push_back((U64)list.at(i).toInt());
        }
    }else{
        _hashVector.push_back((U64)_value.toInt());
    }
}

std::string Int_Knob::serialize() const{
    QList<QVariant> list = _value.toList();
    QString str;
    if(_dimension > 1){
        for (int i = 0; i < list.size(); ++i) {
            str.append("v");
            str.append(QString::number(i));
            str.append(" ");
            str.append(QString::number(list.at(i).toInt()));
            if(i < list.size()-1){
                str.append(" ");
            }
        }
    }else{
        str.append(QString::number(_value.toInt()));
    }
    return str.toStdString();
}
void Int_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){
        if(_dimension > 1){
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
            _value.setValue(values);
        } else {
            _value.setValue(s.toInt());
        }
        
    }
}


std::vector<int> Int_Knob::getValues() const {
    std::vector<int> ret;
    if(_dimension > 1){
    QList<QVariant> list = _value.toList();
    for (int i = 0; i < list.size(); ++i) {
        ret.push_back(list.at(i).toInt());
    }
    }else{
        ret.push_back(_value.toInt());
    }
    return ret;
}



/***********************************BOOL_KNOB*****************************************/
void Bool_Knob::fillHashVector(){
    _hashVector.clear();
    _hashVector.push_back((U64)_value.toBool());
}

std::string Bool_Knob::serialize() const{
    bool value = _value.toBool();
    return value ? "1" : "0";
}
void Bool_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){
        int val = s.toInt();
        _value.setValue((bool)val);
    }
    
}


/***********************************DOUBLE_KNOB*****************************************/
void Double_Knob::fillHashVector(){
    _hashVector.clear();
    if(_dimension > 1){
        QList<QVariant> list = _value.toList();
        for (int i = 0; i < list.size(); ++i) {
            double value = list.at(i).toDouble();
            _hashVector.push_back(*(reinterpret_cast<U64*>(&(value))));
        }
    }else{
        double value = _value.toDouble();
        _hashVector.push_back(*(reinterpret_cast<U64*>(&(value))));
    }
}

std::string Double_Knob::serialize() const{
    QString str;
    if(_dimension > 1){
        QList<QVariant> list = _value.toList();
        for (int i = 0; i < list.size(); ++i) {
            str.append("v");
            str.append(QString::number(i));
            str.append(" ");
            str.append(QString::number(list.at(i).toDouble()));
            if(i < list.size()-1){
                str.append(" ");
            }
        }
        
    }else{
        double value = _value.toDouble();
        str.append(QString::number(value));
    }
    return str.toStdString();
    
}
void Double_Knob::_restoreFromString(const std::string& str){
    QString s(str.c_str());
    if(!s.isEmpty()){
        if(_dimension > 1){
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
            _value.setValue(values);
        } else {
            _value.setValue(s.toDouble());
        }
        
    }
}

std::vector<double> Double_Knob::getValues() const {
    std::vector<double> ret;
    if(_dimension > 1){
        QList<QVariant> list = _value.toList();
        for (int i = 0; i < list.size(); ++i) {
            ret.push_back(list.at(i).toDouble());
        }
    }else{
        ret.push_back(_value.toDouble());
    }
    return ret;
}






/***********************************COMBOBOX_KNOB*****************************************/
void ComboBox_Knob::fillHashVector(){
    _hashVector.clear();
    int value = _value.toInt();
    _hashVector.push_back(value);
}

std::string ComboBox_Knob::serialize() const{
    return QString::number(_value.toInt()).toStdString();
}
void ComboBox_Knob::_restoreFromString(const std::string& str){
    _value.setValue(QString(str.c_str()).toInt());
}


/***********************************RGBA_KNOB*****************************************/
void RGBA_Knob::fillHashVector(){
    QVector4D values = _value.value<QVector4D>();
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
    QVector4D values = _value.value<QVector4D>();
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
    _value.setValue(v);
    
}
/***********************************STRING_KNOB*****************************************/
void String_Knob::fillHashVector(){
    _hashVector.clear();
    QString str(_value.toString());
    for (int i = 0; i < str.size(); ++i) {
        _hashVector.push_back(str.at(i).unicode());
    }
}


std::string String_Knob::serialize() const{
    return _value.toString().toStdString();
}

void String_Knob::_restoreFromString(const std::string& str){
    _value.setValue(str);
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
    QString str(_value.toString());
    for (int i = 0; i < str.size(); ++i) {
        _hashVector.push_back(str.at(i).unicode());
    }
}

std::string RichText_Knob::serialize() const{
    return _value.toString().toStdString();
}

void RichText_Knob::_restoreFromString(const std::string& str){
    _value.setValue(str);
}
