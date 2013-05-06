#ifndef MODEL_H
#define MODEL_H
#include "Superviser/PwStrings.h"
#include <iostream>
#include <fstream>
#include <QtCore/QStringList>
#ifdef __POWITER_UNIX__
#include "dlfcn.h"
#endif
#include <map>
#include <ImfStandardAttributes.h>
#include "Core/inputnode.h"
#include "Core/settings.h"

using namespace Powiter_Enums;

#ifdef __POWITER_WIN32__
class PluginID{
public:
    PluginID(HINSTANCE first,const char* second){
        this->first=first;
        this->second=second;
    }
    ~PluginID(){
        
        FreeLibrary(first);
        delete[] second;
    }
    HINSTANCE first;
    const char* second;
};
#elif defined(__POWITER_UNIX__)
class PluginID{
public:
    PluginID(void* first,const char* second){
        this->first=first;
        this->second=second;
    }
    ~PluginID(){
        
        dlclose(first);
        delete[] second;
    }
    void* first;
    const char* second;
};
#endif
class CounterID{
public:
    CounterID(int first,const char* second){
        this->first=first;
        this->second=second;
        
    }
    ~CounterID(){
        delete[] second;
    }
    
    int first;
    const char* second;
};



class Controler;
class OutputNode;
class Viewer;
class Hash;
class VideoEngine;
class QMutex;
//class Reader;

class Model: public QObject
{
    Q_OBJECT
    

public:
    Model(QObject* parent=0);
    virtual ~Model();
    
    void LoadPluginsAndInitNameList();

    
    std::string removePrefixSpaces(std::string str);
    std::string getNextWord(std::string str);
	//void video_sequence_engine(OutputNode *output,std::vector<InputNode*> inputs);
    
	void getGraphInput(std::vector<InputNode*> &inputs,Node* output);
    //   int getProgression(){return progression;}
    void setControler(Controler* ctrl);
    UI_NODE_TYPE createNode(Node *&node,QString &name,QMutex* m);
    QStringList& getNodeNameList(){return nodeNameList;}
	QMutex* mutex(){return _mutex;}
    // debug
    void displayLoadedPlugins();
    
    
    void startVideoEngine(int nbFrames=-1){emit vengineNeeded(nbFrames);}
    void setVideoEngineRequirements(std::vector<InputNode*> inputs,OutputNode* output);
    VideoEngine* getVideoEngine(){return vengine;}
    
    Controler* getControler(){return ctrl;}
    
    void addFormat(DisplayFormat* frmt);
    DisplayFormat* find_existing_format(int w, int h, double pixel_aspect = 1.0);
    
    void qdebugAllNodes();
    
    Settings* getCurrentProject(){return _powiterSettings;}
signals:
    void vengineNeeded(int nbFrames);
    
    
private:
    UI_NODE_TYPE initCounterAndGetDescription(Node*& node);

    
    std::vector<const char*> formatNames;
    std::vector<Imath::V3f*> resolutions;
    std::vector<DisplayFormat*> formats_list;
    std::vector<CounterID*> counters;
    std::vector<PluginID*> plugins;
    QStringList nodeNameList;
    std::vector<Node*> allNodes;
    
    Settings* _powiterSettings;
    Controler* ctrl;
    VideoEngine* vengine; // Video Engine
    QMutex* _mutex; // mutex for workerthreads



};

#endif // MODEL_H
