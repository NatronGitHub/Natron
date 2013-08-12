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
#include "Gui/GLViewer.h"
#include "Superviser/controler.h"
#include "Gui/mainGui.h"
#include "Core/model.h"
#include "Gui/viewerTab.h"
#include "Gui/node_ui.h"
#include "Core/VideoEngine.h"
#include "Gui/tabwidget.h"
using namespace Powiter;
using namespace std;
Controler::Controler():_model(0),_gui(0){}

void Controler::initControler(Model *model,QLabel* loadingScreen){
    this->_model=model;
    _gui=new Gui();

    _gui->createGui();
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
    createNode("Viewer",0,0);
    
    
}

Controler::~Controler(){
    delete _model;
}


QStringList& Controler::getNodeNameList(){
    return _model->getNodeNameList();
}


void Controler::createNode(QString name,double x,double y){
   
    Node* node = 0;
    if(_model->createNode(node,name.toStdString())){
        _gui->createNodeGUI(node,x,y);
    }else{
        cout << "(Controler::createNode): Couldn't create Node " << name.toStdString() << endl;
    }
}

Viewer* Controler::getCurrentViewer(){
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
