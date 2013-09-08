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

#include <QtGui/QVector4D>


#include "Engine/Node.h"
#include "Engine/Model.h"

#include "Global/AppManager.h"
#include "Global/KnobInstance.h"

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



/***********************************KNOB BASE******************************************/


Knob::Knob(Node* node,const std::string& description,int dimension,Knob_Mask flags):
_node(node),
_value(QVariant(0)),
_instance(NULL),
_description(description),
_flags(flags),
_dimension(dimension),
_parentKnob(NULL)
{
    QObject::connect(this, SIGNAL(valueChanged(const Variant&)), this, SIGNAL(valueChangedByUser()));
}

Knob::~Knob(){
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

void Knob::deleteKnob(){
    delete _instance;
}

void Knob::turnOffNewLine(){
    _instance->turnOffNewLine();
}

void Knob::setSpacingBetweenItems(int spacing){
    _instance->setSpacingBetweenItems(spacing);
}
void Knob::setEnabled(bool b){
    _instance->setEnabled(b);
}

void Knob::setVisible(bool b){
    _instance->setVisible(b);
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
/***********************************FILE_KNOB*****************************************/

std::string File_Knob::serialize() const{
    return SequenceFileDialog::patternFromFilesList(_value.getQVariant().toStringList()).toStdString();
}

void File_Knob::_restoreFromString(const std::string& str){
    //    _name->setText(str.c_str());
    QStringList filesList = SequenceFileDialog::filesListFromPattern(str.c_str());
    _value.setValue(Variant(filesList));
}
void File_Knob::tryStartRendering(){
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
    _value.setValue(Variant(QString(str.c_str())));
}


void OutputFile_Knob::tryStartRendering(){
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
        _value.setValue(Variant(values));
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
        _value.setValue(Variant((bool)val));
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
        _value.setValue(Variant(values));
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
    (void)str;
    QObject::connect(this,SIGNAL(valueChangedByUser()),this,str);
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
    _value.setValue(Variant(QString(str.c_str()).toInt()));
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
    _value.setValue(Variant(v));

}
/***********************************STRING_KNOB*****************************************/
void String_Knob::fillHashVector(){
    _hashVector.clear();
    QString str(_value.getQVariant().toString());
    for (int i = 0; i < str.size(); ++i) {
        _hashVector.push_back(str.at(i).unicode());
    }
}

std::string String_Knob::serialize() const{
    return _value.getQVariant().toString().toStdString();
}

void String_Knob::_restoreFromString(const std::string& str){
    _value.setValue(Variant(QString(str.c_str())));
}
/***********************************GROUP_KNOB*****************************************/
Group_Knob::Group_Knob(Node* node, const std::string& description,int dimension, Knob_Mask flags):
Knob(node,description,dimension,flags){
    
}

void Group_Knob::addKnob(Knob* k){
    dynamic_cast<Group_KnobInstance*>(_instance)->addKnob(k);
    k->setParentKnob(this);
}
/***********************************TAB_KNOB*****************************************/


void Tab_Knob::addTab(const std::string& name){
    dynamic_cast<Tab_KnobInstance*>(_instance)->addTab(name);
}

void Tab_Knob::addKnob(const std::string& tabName,Knob* k){
    dynamic_cast<Tab_KnobInstance*>(_instance)->addKnob(tabName,k);
    k->setParentKnob(this);
}

