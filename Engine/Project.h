//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef PROJECT_H
#define PROJECT_H

#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <QString>
#include <QDateTime>

#include "Global/GlobalDefines.h"

#include "Engine/Format.h"

class Node;
class TimeLine;
class AppInstance;
class ComboBox_Knob;
class Knob;
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


namespace Powiter{

class Project : public QObject{
    
    Q_OBJECT
        
    QString _projectName;
    QString _projectPath;
    bool _hasProjectBeenSavedByUser;
    QDateTime _ageSinceLastSave;
    QDateTime _lastAutoSave;
    ComboBox_Knob* _formatKnob;
    boost::shared_ptr<TimeLine> _timeline; // global timeline
    
    std::map<std::string,int> _nodeCounters;
    bool _autoSetProjectFormat;
    std::vector<Node*> _currentNodes;
    
    std::vector<Format> _availableFormats;
    
    std::vector<Knob*> _projectKnobs;
    AppInstance* _appInstance;//ptr to the appInstance
public:
    
    Project(AppInstance* appInstance);
    
    ~Project();
    
    const std::vector<Node*>& getCurrentNodes() const{return _currentNodes;}
    
    const QString& getProjectName() const WARN_UNUSED_RETURN {return _projectName;}
    
    void setProjectName(const QString& name) {_projectName = name;}
    
    const QString& getProjectPath() const WARN_UNUSED_RETURN {return _projectPath;}
    
    void setProjectPath(const QString& path) {_projectPath = path;}
    
    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN {return _hasProjectBeenSavedByUser;}
    
    void setHasProjectBeenSavedByUser(bool s) {_hasProjectBeenSavedByUser = s;}
    
    const QDateTime& projectAgeSinceLastSave() const WARN_UNUSED_RETURN {return _ageSinceLastSave;}
    
    void setProjectAgeSinceLastSave(const QDateTime& t) {_ageSinceLastSave = t;}
    
    const QDateTime& projectAgeSinceLastAutosave() const WARN_UNUSED_RETURN {return _lastAutoSave;}
    
    void setProjectAgeSinceLastAutosaveSave(const QDateTime& t) {_lastAutoSave = t;}
    
    const Format& getProjectDefaultFormat() const WARN_UNUSED_RETURN ;
    
    void setProjectDefaultFormat(const Format& f);
    
    bool shouldAutoSetProjectFormat() const {return _autoSetProjectFormat;}
    
    void setAutoSetProjectFormat(bool b){_autoSetProjectFormat = b;}
    
    const std::vector<Knob*>& getProjectKnobs() const;
    
    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN {return _timeline;}
    
    // TimeLine operations (to avoid duplicating the shared_ptr when possible)
    void setFrameRange(int first, int last);
    
    void seekFrame(int frame);
    
    void incrementCurrentFrame();
    
    void decrementCurrentFrame();
    
    int currentFrame() const WARN_UNUSED_RETURN;
    
    int firstFrame() const WARN_UNUSED_RETURN;
    
    int lastFrame() const WARN_UNUSED_RETURN;
    
    void initNodeCountersAndSetName(Node* n);
    
    void clearNodes();
    
    void loadProject(const QString& path,const QString& name,bool autoSave = false);
    
    void saveProject(const QString& path,const QString& filename,bool autoSave = false);
    
public slots:
    
    void onProjectFormatChanged();
    
signals:
    
    void projectFormatChanged(Format);

private:
    
    
    /*Serializes the active nodes in the editor*/
    QString serializeNodeGraph() const;
    
    /*restores the node graph from string*/
    void restoreGraphFromString(const QString& str);
    
    /*Analyses and takes action for 1 node ,given 1 attribute from the serialized version
     of the node graph.*/
    void analyseSerializedNodeString(Node* n,XMLProjectLoader::XMLParsedElement* v);
    
};
}

#endif // PROJECT_H
