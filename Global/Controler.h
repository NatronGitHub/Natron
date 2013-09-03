//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GLOBAL_CONTROLER_H_
#define POWITER_GLOBAL_CONTROLER_H_

#include <vector>
#include <string>

#include <QtCore/QObject>
#include <QtCore/QDateTime>

#include "Global/Macros.h"
#include "Engine/Singleton.h"

/*macro to get the unique pointer to the controler*/
#define ctrlPTR Controler::instance()

#define currentOutput Controler::getCurrentOutput()

#define currentViewer Controler::getCurrentViewer()

#define currentWriter Controler::getCurrentWriter()

class NodeGui;
class Node;
class Model;
class ViewerNode;
class Writer;
class Gui;
class OutputNode;
class QLabel;


class Project{
public:
    Project():_hasProjectBeenSavedByUser(false){
        _projectName = "Untitled.rs";
        _age = QDateTime::currentDateTime();
    }
    
    QString _projectName;
    QString _projectPath;
    bool _hasProjectBeenSavedByUser;
    QDateTime _age;
    QDateTime _lastAutoSave;
};

/*Controler (see Model-view-controler pattern on wikipedia). This class
 implements the singleton pattern to ensure there's only 1 single
 instance of the object living. Also you can access the controler
 by the handy macro ctrlPTR*/
class Controler : public QObject,public Singleton<Controler>
{
    class PluginToolButton{
    public:
        PluginToolButton( std::vector<std::string> groups,
                          std::string pluginName,
                          std::string pluginIconPath,
                          std::string groupIconPath):
        _groups(groups),
        _pluginName(pluginName),
        _pluginIconPath(pluginIconPath),
        _groupIconPath(groupIconPath)
        {
            
        }
        std::vector<std::string> _groups;
        std::string _pluginName;
        std::string _pluginIconPath;
        std::string _groupIconPath;
        
    };
    
    std::vector<PluginToolButton*> _toolButtons;
    
public:
    Controler();
    ~Controler();

    /*Create a new node  in the node graph.
     The name passed in parameter must match a valid node name,
     otherwise an exception is thrown. You should encapsulate the call
     by a try-catch block.*/
    Node* createNode(QString name);
    
    /*Get a reference to the list of all the node names 
     available. E.g : Viewer,Reader, Blur, etc...*/
    const QStringList& getNodeNameList();
    
    /*Pointer to the GUI*/
    Gui* getGui(){return _gui;}
    
    /*Pointer to the model*/
	Model* getModel(){return _model;}
    
    /*initialize the pointers to the model and the view. It also call 
     gui->createGUI() and build a viewer node.*/
    void initControler(Model* model,QLabel* loadingScreen,QString projectName = QString());
    
    static OutputNode* getCurrentOutput();
    
    static ViewerNode* getCurrentViewer();
    
    static Writer* getCurrentWriter();
    
    void stackPluginToolButtons(const std::vector<std::string>& groups,
                             const std::string& pluginName,
                             const std::string& pluginIconPath,
                             const std::string& groupIconPath);
    
    const std::vector<NodeGui*>& getAllActiveNodes() const;
    
    const QString& getCurrentProjectName() const {return _currentProject._projectName;}
    
    const QString& getCurrentProjectPath() const {return _currentProject._projectPath;}
    
    void setCurrentProjectName(const QString& name) {_currentProject._projectName = name;}
    
    void loadProject(const QString& path,const QString& name);
    
    void saveProject(const QString& path,const QString& name,bool autoSave);
    
    void autoSave();
    
    void triggerAutoSaveOnNextEngineRun();
        
    bool hasProjectBeenSavedByUser() const {return _currentProject._hasProjectBeenSavedByUser;}
    
    void resetCurrentProject();
    
    void clearInternalNodes();
    
    void clearNodeGuis();
    
    bool isSaveUpToDate() const;
    
    void deselectAllNodes() const;
    
    void showErrorDialog(const QString& title,const QString& message) const;
    
    static const QString autoSavesDir();
    
    
    
private:
	void removeAutoSaves() const;
    
    /*Attemps to find an autosave. If found one,prompts the user
     whether he/she wants to load it. If something was loaded this function
     returns true,otherwise false.*/
    bool findAutoSave();
    
    void addBuiltinPluginToolButtons();
    
    Model* _model; // the model of the MVC pattern
    Gui* _gui; // the view of the MVC pattern
    
    Project _currentProject;
};


#endif // POWITER_GLOBAL_CONTROLER_H_

