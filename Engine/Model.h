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

#ifndef POWITER_ENGINE_MODEL_H_
#define POWITER_ENGINE_MODEL_H_

#include <vector>
#include <map>
#include <string>
#include <utility>
#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#endif
#include <QtCore/QString>
#include <QtCore/QObject>

/*This is the core class of Powiter. It is where the plugins get loaded.
 *This class is the front-end of the core (processing part) of the software.
 **/


namespace XMLProjectLoader {
    

class XMLParsedElement{
public:
    QString _element;
    XMLParsedElement(const QString& element):
    _element(element)
    {
    }
};

class InputXMLParsedElement : public XMLParsedElement{
public:
    int _number;
    QString _name;
    InputXMLParsedElement(const QString& name, int number):XMLParsedElement("Input"),
    _number(number),_name(name)
    {
    }
};

class KnobXMLParsedElement :  public XMLParsedElement{
public:
    QString _description;
    QString _param;
    KnobXMLParsedElement(const QString& description, const QString& param):XMLParsedElement("Knob"),
    _description(description),_param(param)
    {
    }
};

class NodeGuiXMLParsedElement :  public XMLParsedElement{
public:
    double _x,_y;
    NodeGuiXMLParsedElement(double x, double y):XMLParsedElement("Gui"),
    _x(x),_y(y)
    {
    }
};

}

class AppInstance;
class OutputNode;
class VideoEngine;
class ViewerNode;
class QMutex;
class Node;
class OutputNode;

namespace Powiter{
    class LibraryBinary;
}


class Model: public QObject,public boost::noncopyable
{
    Q_OBJECT
    

public:
    Model(AppInstance* appInstance);
    
    ~Model();
  
	/*Create a new node internally*/
    Node* createNode(const std::string& name);
    
    /*starts the videoEngine for nbFrames. It will re-init the viewer so the
     *frame fit in the viewer.*/
    void startVideoEngine(int nbFrames=-1);
    
    VideoEngine* getVideoEngine() const;

    OutputNode* getCurrentOutput() const {return _currentOutput;}

	/*Refresh the graph used by the output's videoEngine.*/
    void updateDAG(OutputNode* output,bool isViewer);

    void checkViewersConnection();
    
    void loadProject(const QString& filename,bool autoSave = false);
    
    void saveProject(const QString& path,const QString& filename,bool autoSave = false);
    
    AppInstance* getApp() const {return _appInstance;}
        
    QMutex* getGeneralMutex() const {return _generalMutex;}
    
    void triggerAutoSaveOnNextEngineRun();
    
    bool connect(int inputNumber,const std::string& inputName,Node* output);
    
    bool connect(int inputNumber,Node* input,Node* output);
    
    bool disconnect(Node* input,Node* output);
    
    void initNodeCountersAndSetName(Node* n);

    void clearNodes();
    
    void connectViewersToViewerCache();
    
    void disconnectViewersFromViewerCache();
    

private:
    

    /*Serializes the active nodes in the editor*/
    QString serializeNodeGraph() const;
    
    /*restores the node graph from string*/
    void restoreGraphFromString(const QString& str);
        
    /*Analyses and takes action for 1 node ,given 1 attribute from the serialized version
     of the node graph.*/
    void analyseSerializedNodeString(Node* n,XMLProjectLoader::XMLParsedElement* v);
    
    
    AppInstance* _appInstance;

    OutputNode* _currentOutput; /*The output of the last used graph.*/
    
    std::vector<Node*> _currentNodes;
    
    std::map<std::string,int> _nodeCounters;
        
    QMutex* _generalMutex;//mutex to synchronize all VideoEngine's
};

#endif // POWITER_ENGINE_MODEL_H_
