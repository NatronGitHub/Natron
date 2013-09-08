//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/
#include <utility>

#include "NodeInstance.h"

#include "Global/GlobalDefines.h"
#include "Global/AppManager.h"
#include "Global/KnobInstance.h"

#include "Gui/NodeGui.h"
#include "Gui/Edge.h"
#include "Gui/ViewerTab.h"

#include "Engine/Node.h"
#include "Engine/Model.h"
#include "Engine/ViewerNode.h"


using namespace std;

NodeInstance::NodeInstance(Node* node,AppInstance* app):
_app(app),
_node(node),
_gui(NULL)
{

    QObject::connect(_gui, SIGNAL(nameChanged(QString)), this, SLOT(setName(QString)));

}

NodeInstance::~NodeInstance(){
    for (OutputMap::iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        _app->disconnect(this, it->second);
    }
    for (InputMap::iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        _app->disconnect(it->second, this);
    }
    delete _gui;
    if(!_node->isOpenFXNode())
        delete _node;
    
    for (U32 i = 0; i < _knobs.size(); ++i) {
        delete _knobs[i];
    }
}

bool NodeInstance::isOpenFXNode() const{
    return _node->isOpenFXNode();
}

bool NodeInstance::isInputNode() const{
    return _node->isInputNode();
}

bool NodeInstance::isOutputNode() const{
    return _node->isOutputNode();
}

const std::string NodeInstance::className() const{
    return _node->className();
}

void NodeInstance::setName(const QString& name){
    _gui->setName(name);
    _node->setName(name.toStdString());
}

const std::string& NodeInstance::getName() const{
    return _node->getName();
}


bool NodeInstance::connectInput(NodeInstance* input,int inputNumber){
    InputMap::const_iterator it = _inputs.find(inputNumber);
    if (it == _inputs.end()) {
        return false;
    }else{
        if (it->second == NULL) {
            if(!_gui->connectEdge(inputNumber, input->_gui)){
                return false;
            }
            _inputs.insert(make_pair(inputNumber,input));
            return true;
        }else{
            return false;
        }
    }
}

void NodeInstance::connectOutput(NodeInstance* output,int outputNumber ){
    _outputs.insert(make_pair(outputNumber,output));
}

bool NodeInstance::disconnectInput(int inputNumber){
    InputMap::const_iterator it = _inputs.find(inputNumber);
    if (it == _inputs.end()) {
        return false;
    }else{
        if(it->second == NULL){
            return false;
        }else{
            if(!_gui->connectEdge(inputNumber, NULL)){
                return false;
            }
            _inputs.insert(make_pair(inputNumber, NULL));
            return true;
        }
    }
}

bool NodeInstance::disconnectInput(NodeInstance* input){
    for (InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        if (it->second == input) {
            if(!_gui->connectEdge(it->first, NULL)){
                return false;
            }
            _inputs.insert(make_pair(it->first, NULL));
            return true;
        }else{
            return false;
        }
    }
    return false;
}

bool NodeInstance::disconnectOutput(int outputNumber){
    OutputMap::const_iterator it = _inputs.find(outputNumber);
    if (it == _outputs.end()) {
        return false;
    }else{
        if(it->second == NULL){
            return false;
        }else{
            _outputs.insert(make_pair(outputNumber, NULL));
            return true;
        }
    }

}

bool NodeInstance::disconnectOutput(NodeInstance* output){
    for (OutputMap::const_iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if (it->second == output) {
            _outputs.insert(make_pair(it->first, NULL));
            return true;
        }else{
            return false;
        }
    }
    return false;
}

void NodeInstance::setPosGui(double x,double y){
    _gui->setPos(x,y);
    refreshEdgesGui();
}

void NodeInstance::refreshEdgesGui(){
    for (OutputMap::const_iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        it->second->refreshEdgesGui();
    }
    _gui->refreshEdges();
}

QPointF NodeInstance::getPosGui() const{
    return _gui->pos();
}

void NodeInstance::updateNodeChannelsGui(const ChannelSet& channels){
    _gui->updateChannelsTooltip(channels);
}
void NodeInstance::updatePreviewImageGUI(){
    _gui->updatePreviewImageForReader();
}
void NodeInstance::deactivate(){
    _gui->deactivate();
    
    for (NodeInstance::InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        _app->disconnect(it->second,this);
    }
    NodeGui* firstChild = 0;
    for (NodeInstance::OutputMap::const_iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(it == _outputs.begin()){
            firstChild = it->second->getNodeGui();
        }
       _app->disconnect(this, it->second);
    }
    if(firstChild){
        _app->triggerAutoSaveOnNextEngineRun();
        checkIfViewerConnectedAndRefresh();
    }
}

void NodeInstance::activate(){
    _gui->activate();
    for (NodeInstance::InputMap::const_iterator it = _inputs.begin(); it!=_inputs.end(); ++it) {
        _app->connect(it->first,it->second, _node->getNodeInstance());
    }
    NodeGui* firstChild = 0;
    for (NodeInstance::OutputMap::const_iterator it = _outputs.begin(); it!=_outputs.end(); ++it) {
        if(it == _outputs.begin()){
            firstChild = it->second->getNodeGui();
        }
        int inputNb = 0;
        for (NodeInstance::InputMap::const_iterator it2 = it->second->getInputs().begin();
             it2 != it->second->getInputs().end(); ++it2) {
            if (it2->second == _node->getNodeInstance()) {
                inputNb = it2->first;
                break;
            }
        }
        _app->connect(inputNb,_node->getNodeInstance(), it->second);
    }
    if(firstChild){
        _app->triggerAutoSaveOnNextEngineRun();
        checkIfViewerConnectedAndRefresh();
    }
}

void NodeInstance::checkIfViewerConnectedAndRefresh() const{
    Node* viewer = Node::hasViewerConnected(_node);
    if(viewer){
        //if(foundSrc){
        OutputNode* output = dynamic_cast<OutputNode*>(viewer);
        std::pair<int,bool> ret = _app->setCurrentGraph(output,true);
        if(_app->isRendering()){
            _app->changeDAGAndStartRendering(output);
        }else{
            if(ret.second){
                _app->startRendering(1);
            }
            else if(ret.first == 0){ // no inputs, disconnect viewer
                ViewerNode* v = dynamic_cast<ViewerNode*>(viewer);
                if(v){
                    ViewerTab* tab = v->getUiContext();
                    tab->disconnectViewer();
                }
            }
        }
    }
    
}

NodeInstance* NodeInstance::input(int inputNb) const{
    InputMap::const_iterator it = _inputs.find(inputNb);
    if(it == _inputs.end()){
        return NULL;
    }else{
        return it->second;
    }
}

void NodeInstance::initializeInputs(){
    _gui->initializeInputs();
    _node->initializeInputs();
    int inputCount = _node->maximumInputs();
    for (int i = 0; i < inputCount; ++i) {
        _inputs.insert(make_pair(i,NULL));
    }
}

void NodeInstance::initializeKnobs(){
    /*All OFX knobs have been created so far.
     The next call will create Knobs for Powiter-only
     nodes.*/
    _node->initKnobs();
    

    /*All KnobInstance are now created. The member
     _knobs is filled with the knobs.
     We can just create their gui.*/
    _gui->initializeKnobs();
}
void NodeInstance::removeKnobInstance(KnobInstance* knob){
    for (U32 i = 0; i < _knobs.size(); ++i) {
        if (_knobs[i] == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
    // should also remove the knob from gui
}
void NodeInstance::pushUndoCommand(QUndoCommand* cmd){
    _gui->pushUndoCommand(cmd);
}
void NodeInstance::createKnobGuiDynamically(){
    _gui->initializeKnobs();
}

bool NodeInstance::isInputConnected(int inputNb) const{
    InputMap::const_iterator it = _inputs.find(inputNb);
    if(it != _inputs.end()){
        return it->second != NULL;
    }else{
        return false;
    }
        
}

bool NodeInstance::hasOutputConnected() const{
    for(OutputMap::const_iterator it = _outputs.begin();it!=_outputs.end();++it){
        if(it->second){
            return true;
        }
    }
    return false;
}
