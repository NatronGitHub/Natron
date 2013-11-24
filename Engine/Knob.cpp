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

CurvePath::CurvePath(const KeyFrame& cp)
    : _keyFrames()
    , _bbox()
    ,_betweendBeginAndEndRecord(false)
{
    setStart(cp);
}


double CurvePath::getMinimumTimeCovered() const{
    assert(!_keyFrames.empty());
    return _keyFrames.front().getTime();
}

double CurvePath::getMaximumTimeCovered() const{
    assert(!_keyFrames.empty());
    return _keyFrames.back().getTime();
}

void CurvePath::setStart(const KeyFrame& cp){
    if(!_keyFrames.empty()){
        //remove the already existing key frame
        _keyFrames.pop_front();
    }
    _keyFrames.push_front(cp);
}

void CurvePath::setEnd(const KeyFrame& cp){
    if(!_keyFrames.empty()){
        //remove the already existing key frame
        _keyFrames.pop_back();
    }
    _keyFrames.push_back(cp);
}

void CurvePath::addControlPoint(const KeyFrame& cp)
{
    KeyFrames::iterator newKeyIt = _keyFrames.end();
    if(_keyFrames.empty()){
        newKeyIt = _keyFrames.insert(_keyFrames.end(),cp);
    }else{
        //finding a matching or the first greater key
        KeyFrames::iterator upper = _keyFrames.end();
        for(KeyFrames::iterator it = _keyFrames.begin();it!=_keyFrames.end();++it){
            if((*it).getTime() > cp.getTime()){
                upper = it;
                break;
            }else if((*it).getTime() == cp.getTime()){
                //if the key already exists at this time, just modify it.
                (*it).setValue(cp.getValue());
                return;
            }
        }
        //if we found no key that has a greater time, just append the key
        if(upper == _keyFrames.end()){
            newKeyIt = _keyFrames.insert(_keyFrames.end(),cp);
            return;
        }
        //if all the keys have a greater time, just insert this key at the begining
        if(upper == _keyFrames.begin()){
            newKeyIt = _keyFrames.insert(_keyFrames.begin(),cp);
            return;
        }

        //if we reach here, that means we're in the middle of 2 keys, insert it before the upper bound
        newKeyIt = _keyFrames.insert(upper,cp);

    }
    refreshTangents(newKeyIt);
}

void CurvePath::refreshTangents(KeyFrames::iterator key){
    double tcur = key->getTime();
    double vcur = key->getValue().value<double>();
    
    double tprev,vprev,tnext,vnext;
    if(key == _keyFrames.begin()){
        tprev = tcur;
        vprev = vcur;
    }else{
        KeyFrames::const_iterator prev = key;
        --prev;
        tprev = prev->getTime();
        vprev = prev->getValue().value<double>();
    }
    
    KeyFrames::const_iterator next = key;
    ++next;
    if(next == _keyFrames.end()){
        tnext = tcur;
        vnext = vcur;
    }else{
        tnext = next->getTime();
        vnext = next->getValue().value<double>();
    }
    
    double tcur_rightTan,vcur_rightTan,tcur_leftTan,vcur_leftTan;
    Natron::autoComputeTangents(key->getInterpolation(),
                                tprev, vprev,
                                tcur, vcur,
                                tnext, vnext,
                                &tcur_leftTan, &vcur_leftTan,
                                &tcur_rightTan, &vcur_rightTan);
    
    key->setLeftTangent(tcur_leftTan, Variant(vcur_leftTan));
    key->setRightTangent(tcur_rightTan, Variant(vcur_rightTan));
}



Variant CurvePath::getValueAt(double t) const{
    assert(!_keyFrames.empty());
    const Variant& firstKeyValue = _keyFrames.front().getValue();
    double v;
    switch(firstKeyValue.type()){
        case QVariant::Int :
            v = (double)getValueAtInternal<int>(t);
            break;
        case QVariant::Double :
            v = getValueAtInternal<double>(t);
            break;
        default:
        std::string exc("The type requested ( ");
        exc.append(firstKeyValue.typeName());
        exc.append(") is not interpolable, it cannot animate!");
        throw std::invalid_argument(exc);
    }
    if(_betweendBeginAndEndRecord){
        if( v  < _bbox.bottom() )
            _bbox.set_bottom(v);
        if( v > _bbox.top() )
            _bbox.set_top(v);
        if( t < _bbox.left() )
            _bbox.set_left(t);
        if( t > _bbox.right())
            _bbox.set_right(t);
    }
    return Variant(v);


}


/***********************************KNOB BASE******************************************/


Knob::Knob(KnobHolder* holder,const std::string& description,int dimension):
    _holder(holder)
  , _hashVector()
  , _value(dimension)
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
  , _isAnimationEnabled(true)
{
    
    if(_holder){
        _holder->addKnob(boost::shared_ptr<Knob>(this));
    }
}

Knob::~Knob(){
    if(_holder){
        _holder->removeKnob(this);
    }
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

void Knob::onStartupRestoration(const MultidimensionalValue& other){
    _value = other; //set the value
    if(!_isInsignificant)
        updateHash(); //refresh hash if needed
    processNewValue(); // process new value if needed
    _holder->onValueChanged(this,Knob::USER_EDITED); // notify plugin the value changed

    //notify gui for each value changed
    const std::map<int, Variant>& value = _value.getValueForEachDimension();
    for(std::map<int,Variant>::const_iterator it = value.begin();it!=value.end();++it){
        emit valueChanged(it->first,it->second);
    }
}

void Knob::fillHashVector(){
    _hashVector.clear();
    const std::map<int,Variant>& value = _value.getValueForEachDimension();
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
}

void Knob::setValueInternal(const Variant& v,int dimension){
    _value.setValue(v, dimension);
    if(!_isInsignificant)
        updateHash();
    processNewValue();
    emit valueChanged(dimension,v);
    _holder->onValueChanged(this,Knob::PLUGIN_EDITED);
    _holder->evaluate(this,!_isInsignificant);

}

void Knob::onValueChanged(int dimension,const Variant& variant){
    _value.setValue(variant, dimension);
    if(!_isInsignificant)
        updateHash();
    processNewValue();
    _holder->onValueChanged(this,Knob::USER_EDITED);
    _holder->evaluate(this,!_isInsignificant);
    
}

const Variant& MultidimensionalValue::getValue(int dimension) const{
    std::map<int,Variant>::const_iterator it = _value.find(dimension);
    assert(it != _value.end());
    return it->second;
}

void MultidimensionalValue::setValue(const Variant& v,int dimension){
    std::map<int,Variant>::iterator it = _value.find(dimension);
    assert(it != _value.end());
    it->second = v;

}

void MultidimensionalValue::setValueAtTime(double time, const Variant& v, int dimension){
    KeyFrame key(time,v);
    CurvesMap::iterator foundDimension = _curves.find(dimension);
    assert(foundDimension != _curves.end());
    foundDimension->second.addControlPoint(key);
}

const CurvePath& Knob::getCurve(int dimension) const{
    return _value.getCurve(dimension);
}

const CurvePath& MultidimensionalValue::getCurve(int dimension) const {
    CurvesMap::const_iterator foundDimension = _curves.find(dimension);
    assert(foundDimension != _curves.end());
    return foundDimension->second;
}

Variant MultidimensionalValue::getValueAtTime(double time,int dimension) const{
    CurvesMap::const_iterator foundDimension = _curves.find(dimension);
    const CurvePath& curve = getCurve(dimension);
    if(!curve.isAnimated()){
        /*if the knob as no keys at this dimension, return the value
        at the requested dimension.*/
        std::map<int,Variant>::const_iterator it = _value.find(dimension);
        if(it != _value.end()){
            return it->second;
        }else{
            return Variant();
        }
    }else{
        try{
            return foundDimension->second.getValueAt(time);
        }catch(const std::exception& e){
            std::cout << e.what() << std::endl;
            assert(false);
        }

    }
}


RectD MultidimensionalValue::getCurvesBoundingBox() const{
    RectD ret;
    for(CurvesMap::const_iterator it = _curves.begin() ; it!=_curves.end();++it){
        ret.merge(it->second.getBoundingBox());
    }
    return ret;
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
        if (_knobs[i].get() == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
}

void KnobHolder::triggerAutoSave(){
    _app->triggerAutoSave();
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

