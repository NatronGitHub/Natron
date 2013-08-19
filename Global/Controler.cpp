//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




#include <QLabel>
#include "Gui/ViewerGL.h"
#include "Global/Controler.h"
#include "Gui/Gui.h"
#include "Engine/Model.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
#include "Engine/VideoEngine.h"
#include "Gui/TabWidget.h"
using namespace Powiter;
using namespace std;
Controler::Controler():_model(0),_gui(0){}

void Controler::initControler(Model *model,QLabel* loadingScreen){
    this->_model=model;
    _gui=new Gui();

    _gui->createGui();
    
    addBuiltinPluginToolButtons();
    for (U32 i = 0; i < _toolButtons.size(); i++) {
        string name = _toolButtons[i]->_pluginName;
        name.append("  [");
        name.append(_toolButtons[i]->_groups[0]);
        name.append("]");
        _gui->addPluginToolButton(name,
                                  _toolButtons[i]->_groups,
                                  _toolButtons[i]->_pluginName,
                                  _toolButtons[i]->_pluginIconPath,
                                  _toolButtons[i]->_groupIconPath);
        delete _toolButtons[i];
    }
    _toolButtons.clear();
    
    loadingScreen->hide();

    
    
#ifdef __POWITER_OSX__
	_gui->show();

#else
	_gui->showMaximized();
#endif

    delete loadingScreen;
    createNode("Viewer");
    
    
}

Controler::~Controler(){
    delete _model;
}

void Controler::addBuiltinPluginToolButtons(){
    vector<string> grouping;
    grouping.push_back("io");
    _gui->addPluginToolButton("Reader", grouping, "Reader", "", IMAGES_PATH"ioGroupingIcon.png");
    _gui->addPluginToolButton("Viewer", grouping, "Viewer", "", IMAGES_PATH"ioGroupingIcon.png");
    _gui->addPluginToolButton("Writer", grouping, "Writer", "", IMAGES_PATH"ioGroupingIcon.png");
}

const QStringList& Controler::getNodeNameList(){
    return _model->getNodeNameList();
}


void Controler::createNode(QString name){
   
    Node* node = 0;
    if(_model->createNode(node,name.toStdString())){
        _gui->createNodeGUI(node);
    }else{
        cout << "(Controler::createNode): Couldn't create Node " << name.toStdString() << endl;
    }
}

ViewerNode* Controler::getCurrentViewer(){
    Controler* ctrl = Controler::instance();
    return ctrl->getModel()->getVideoEngine()->getCurrentDAG().outputAsViewer();
}

Writer* Controler::getCurrentWriter(){
    Controler* ctrl = Controler::instance();
    return ctrl->getModel()->getVideoEngine()->getCurrentDAG().outputAsWriter();
}
void Controler::stackPluginToolButtons(const std::vector<std::string>& groups,
                                    const std::string& pluginName,
                                    const std::string& pluginIconPath,
                                    const std::string& groupIconPath){
    _toolButtons.push_back(new PluginToolButton(groups,pluginName,pluginIconPath,groupIconPath));
}
