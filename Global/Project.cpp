//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Project.h"

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#include "Global/AppManager.h"

#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/OfxNode.h"
#include "Engine/Knob.h"

#include "Gui/NodeGui.h"

using namespace Powiter;
using namespace std;
using namespace boost;

Project::Project(AppInstance* appInstance):
_projectName("Untitled.rs"),
_hasProjectBeenSavedByUser(false),
_ageSinceLastSave(QDateTime::currentDateTime()),
_format(*appPTR->findExistingFormat(2048, 1556,1.)),
_timeline(new TimeLine())
,_appInstance(appInstance)
{
    
    
}
Project::~Project(){
    for (U32 i = 0; i < _currentNodes.size(); ++i) {
        _currentNodes[i]->deleteNode();
    }
    _currentNodes.clear();
}



void Project::initNodeCountersAndSetName(Node* n){
    assert(n);
    map<string,int>::iterator it = _nodeCounters.find(n->className());
    if(it != _nodeCounters.end()){
        it->second++;
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(it->second)).toStdString());
    }else{
        _nodeCounters.insert(make_pair(n->className(), 1));
        n->setName(QString(QString(n->className().c_str())+ "_" + QString::number(1)).toStdString());
    }
    _currentNodes.push_back(n);
}

void Project::clearNodes(){
    foreach(Node* n,_currentNodes){
        n->deleteNode();
    }
    _currentNodes.clear();
}

void Project::setFrameRange(int first, int last){
    _timeline->setFrameRange(first,last);
}

void Project::seekFrame(int frame){
    _timeline->seekFrame(frame);
}

void Project::incrementCurrentFrame() {
    _timeline->incrementCurrentFrame();
}
void Project::decrementCurrentFrame(){
    _timeline->decrementCurrentFrame();
}

int Project::currentFrame() const {
    return _timeline->currentFrame();
}

int Project::firstFrame() const {
    return _timeline->firstFrame();
}

int Project::lastFrame() const {
    return _timeline->lastFrame();
}


void Project::loadProject(const QString& path,const QString& name,bool autoSave){
    QFile file(path+name);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream in(&file);
    QString content = in.readAll();
    if(autoSave){
        QString toRemove = content.left(content.indexOf('\n')+1);
        content = content.remove(toRemove);
    }
    restoreGraphFromString(content);
    file.close();
    
    
    QDateTime time = QDateTime::currentDateTime();
    setAutoSetProjectFormat(false);
    setHasProjectBeenSavedByUser(true);
    setProjectName(name);
    setProjectPath(path);
    setProjectAgeSinceLastSave(time);
    setProjectAgeSinceLastAutosaveSave(time);
}
void Project::saveProject(const QString& path,const QString& filename,bool autoSave){
    if(autoSave){
        QFile file(AppInstance::autoSavesDir()+QDir::separator()+filename);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
            cout <<  file.errorString().toStdString() << endl;
            return;
        }
        QTextStream out(&file);
        if(!path.isEmpty())
            out << path << endl;
        else
            out << "unsaved" << endl;
        out << serializeNodeGraph();
        file.close();
    }else{
        QFile file(path+filename);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)){
            cout << (path+filename).toStdString() <<  file.errorString().toStdString() << endl;
            return;
        }
        QTextStream out(&file);
        out << serializeNodeGraph();
        file.close();
        
    }
}




QString Project::serializeNodeGraph() const{
    /*Locking the nodes while autosaving*/
    QMutexLocker l(_appInstance->getAutoSaveMutex());
    
    const std::vector<Node*>& activeNodes = _appInstance->getAllActiveNodes();
    QString ret;
    
    QXmlStreamWriter writer(&ret);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("Nodes");
    foreach(Node* n, activeNodes){
        //serialize inputs
        assert(n);
        writer.writeStartElement("Node");
        
        if(!n->isOpenFXNode()){
            writer.writeAttribute("ClassName",n->className().c_str());
        }else{
            OfxNode* ofxNode = dynamic_cast<OfxNode*>(n);
            QString name = ofxNode->className().c_str();
            QStringList groups = ofxNode->getPluginGrouping();
            if (groups.size() >= 1) {
                name.append("  [");
                name.append(groups[0]);
                name.append("]");
            }
            writer.writeAttribute("ClassName",name);
        }
        writer.writeAttribute("Label", n->getName().c_str());
        
        writer.writeStartElement("Inputs");
        const Node::InputMap& inputs = n->getInputs();
        for (Node::InputMap::const_iterator it = inputs.begin();it!=inputs.end();++it) {
            if(it->second){
                writer.writeStartElement("Input");
                writer.writeAttribute("Number", QString::number(it->first));
                writer.writeAttribute("Name", it->second->getName().c_str());
                writer.writeEndElement();// end input
            }
        }
        writer.writeEndElement(); // end inputs
                                  //serialize knobs
        const std::vector<Knob*>& knobs = n->getKnobs();
        writer.writeStartElement("Knobs");
        for (U32 i = 0; i < knobs.size(); ++i) {
            writer.writeStartElement("Knob");
            writer.writeAttribute("Description", knobs[i]->getDescription().c_str());
            writer.writeAttribute("Param", knobs[i]->serialize().c_str());
            writer.writeEndElement();//end knob
        }
        writer.writeEndElement(); // end knobs
        
        //serialize gui infos
        
        writer.writeStartElement("Gui");
        
        NodeGui* nodeGui = _appInstance->getNodeGui(n);
        double x=0,y=0;
        if(nodeGui){
            x = nodeGui->pos().x();
            y = nodeGui->pos().y();
        }
        writer.writeAttribute("X", QString::number(x));
        writer.writeAttribute("Y", QString::number(y));
        writer.writeEndElement();//end gui
        
        writer.writeEndElement();//end node
    }
    writer.writeEndElement(); // end nodes
    writer.writeEndDocument();
    return ret;
}

void Project::restoreGraphFromString(const QString& str){
    // pair< Node, pair< AttributeName, AttributeValue> >
    std::vector<std::pair<Node*, XMLProjectLoader::XMLParsedElement* > > actionsMap;
    QXmlStreamReader reader(str);
    
    while(!reader.atEnd() && !reader.hasError()){
        QXmlStreamReader::TokenType token = reader.readNext();
        /* If token is just StartDocument, we'll go to next.*/
        if(token == QXmlStreamReader::StartDocument) {
            continue;
        }
        /* If token is StartElement, we'll see if we can read it.*/
        if(token == QXmlStreamReader::StartElement) {
            /* If it's named Nodes, we'll go to the next.*/
            if(reader.name() == "Nodes") {
                continue;
            }
            /* If it's named Node, we'll dig the information from there.*/
            if(reader.name() == "Node") {
                /* Let's check that we're really getting a Node. */
                if(reader.tokenType() != QXmlStreamReader::StartElement &&
                   reader.name() == "Node") {
                    QString err = QString("Unable to restore the graph:\n") + reader.errorString();
                    Powiter::errorDialog("Loader", err.toStdString());
                    return;
                }
                QString className,label;
                QXmlStreamAttributes nodeAttributes = reader.attributes();
                if(nodeAttributes.hasAttribute("ClassName")) {
                    className = nodeAttributes.value("ClassName").toString();
                }
                if(nodeAttributes.hasAttribute("Label")) {
                    label = nodeAttributes.value("Label").toString();
                }
                
                assert(_appInstance);
                _appInstance->deselectAllNodes();
                Node* n = _appInstance->createNode(className);
                if(!n){
                    QString text("Failed to restore the graph! \n The node ");
                    text.append(className);
                    text.append(" was found in the auto-save script but doesn't seem \n"
                                "to exist in the currently loaded plug-ins.");
                    Powiter::errorDialog("Autosave", text.toStdString() );
                    _appInstance->clearNodes();
                    (void)_appInstance->createNode("Viewer");
                    return;
                }
                n->setName(label.toStdString());
                
                reader.readNext();
                while(!(reader.tokenType() == QXmlStreamReader::EndElement &&
                        reader.name() == "Node")) {
                    if(reader.tokenType() == QXmlStreamReader::StartElement) {
                        
                        if(reader.name() == "Inputs") {
                            
                            while(!(reader.tokenType() == QXmlStreamReader::EndElement &&
                                    reader.name() == "Inputs")) {
                                reader.readNext();
                                if(reader.tokenType() == QXmlStreamReader::StartElement) {
                                    if(reader.name() == "Input") {
                                        QXmlStreamAttributes inputAttributes = reader.attributes();
                                        int number = -1;
                                        QString name;
                                        if(inputAttributes.hasAttribute("Number")){
                                            number = inputAttributes.value("Number").toString().toInt();
                                        }
                                        if(inputAttributes.hasAttribute("Name")){
                                            name = inputAttributes.value("Name").toString();
                                        }
                                        actionsMap.push_back(make_pair(n,new XMLProjectLoader::InputXMLParsedElement(name,number)));
                                    }
                                    
                                }
                            }
                        }
                        
                        if(reader.name() == "Knobs") {
                            
                            while(!(reader.tokenType() == QXmlStreamReader::EndElement &&
                                    reader.name() == "Knobs")) {
                                reader.readNext();
                                if(reader.tokenType() == QXmlStreamReader::StartElement) {
                                    if(reader.name() == "Knob") {
                                        QXmlStreamAttributes knobAttributes = reader.attributes();
                                        QString description,param;
                                        if(knobAttributes.hasAttribute("Description")){
                                            description = knobAttributes.value("Description").toString();
                                        }
                                        if(knobAttributes.hasAttribute("Param")){
                                            param = knobAttributes.value("Param").toString();
                                        }
                                        actionsMap.push_back(make_pair(n,new XMLProjectLoader::KnobXMLParsedElement(description,param)));
                                    }
                                }
                            }
                            
                        }
                        
                        if(reader.name() == "Gui") {
                            double x = 0,y = 0;
                            QXmlStreamAttributes posAttributes = reader.attributes();
                            QString description,param;
                            if(posAttributes.hasAttribute("X")){
                                x = posAttributes.value("X").toString().toDouble();
                            }
                            if(posAttributes.hasAttribute("Y")){
                                y = posAttributes.value("Y").toString().toDouble();
                            }
                            
                            
                            actionsMap.push_back(make_pair(n,new XMLProjectLoader::NodeGuiXMLParsedElement(x,y)));
                            
                        }
                    }
                    reader.readNext();
                }
            }
        }
    }
    if(reader.hasError()){
        QString err =  QString("Unable to restore the graph :\n") + reader.errorString();
        Powiter::errorDialog("Loader",err.toStdString());
        return;
    }
    //adjusting knobs & connecting nodes now
    for (U32 i = 0; i < actionsMap.size(); ++i) {
        pair<Node*,XMLProjectLoader::XMLParsedElement*>& action = actionsMap[i];
        analyseSerializedNodeString(action.first, action.second);
    }
    
}
void Project::analyseSerializedNodeString(Node* n,XMLProjectLoader::XMLParsedElement* v){
    assert(n);
    if(v->_element == "Input"){
        XMLProjectLoader::InputXMLParsedElement* inputEl = static_cast<XMLProjectLoader::InputXMLParsedElement*>(v);
        assert(inputEl);
        int inputNb = inputEl->_number;
        if(!_appInstance->connect(inputNb,inputEl->_name.toStdString(), n)){
            cout << "Failed to connect " << inputEl->_name.toStdString()  << " to " << n->getName() << endl;
        }
        //  cout << "Input: " << inputEl->_number << " :" << inputEl->_name.toStdString() << endl;
    }else if(v->_element == "Knob"){
        XMLProjectLoader::KnobXMLParsedElement* inputEl = static_cast<XMLProjectLoader::KnobXMLParsedElement*>(v);
        assert(inputEl);
        const std::vector<Knob*>& knobs = n->getKnobs();
        for (U32 j = 0; j < knobs.size(); ++j) {
            if (knobs[j]->getDescription() == inputEl->_description.toStdString()) {
                knobs[j]->restoreFromString(inputEl->_param.toStdString());
                break;
            }
        }
        //cout << "Knob: " << inputEl->_description.toStdString() << " :" << inputEl->_param.toStdString() << endl;
    }else if(v->_element == "Gui"){
        XMLProjectLoader::NodeGuiXMLParsedElement* inputEl = static_cast<XMLProjectLoader::NodeGuiXMLParsedElement*>(v);
        assert(inputEl);
        NodeGui* nodeGui = _appInstance->getNodeGui(n);
        if(nodeGui){
            nodeGui->refreshPosition(inputEl->_x,inputEl->_y);
        }
        //  cout << "Gui/Pos: " << inputEl->_x << " , " << inputEl->_y << endl;
    }
}


