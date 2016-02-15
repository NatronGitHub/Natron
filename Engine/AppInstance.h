/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef APPINSTANCE_H
#define APPINSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include <QStringList>

#include "Global/GlobalDefines.h"
#include "Engine/RectD.h"
#include "Engine/TimeLineKeyFrames.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct AppInstancePrivate;

struct CreateNodeArgs
{
    typedef std::list< boost::shared_ptr<KnobSerialization> > DefaultValuesList;

    ///The pluginID corresponds to Plugin::getPluginID()
    QString pluginID;
    
    //The version of the plug-in we want to instanciate
    int majorV,minorV;
    
    //If the node is a multi-instance child, this is the script-name of the parent
    std::string multiInstanceParentName;
    
    //A hint for the initial x,y position in the NodeGraph
    double xPosHint, yPosHint;
    
    //If we do not want the name of the node to be generated automatically, we can set this
    //to force the node to have this name
    QString fixedName;

    //A list of parameter values that will be set prior to calling createInstanceAction
    DefaultValuesList paramValues;
    
    //The group into which the node should be. All "user" nodes should at least be in the Project
    //or in a NodeGroup
    boost::shared_ptr<NodeCollection> group;
    
    //How was this node created
    CreateNodeReason reason;
    
    //Should we create the NodeGui or keep the node internal only
    bool createGui;
    
    //If false the node will not be serialized, indicating it's just used by Natron internals
    bool addToProject;
    
    //When copy/pasting or loading from a project, this is a pointer to this node serialization to load
    //data from
    boost::shared_ptr<NodeSerialization> serialization;
    
    explicit CreateNodeArgs(const QString& pluginID, CreateNodeReason reason, const boost::shared_ptr<NodeCollection>& group)
    : pluginID(pluginID)
    , majorV(-1)
    , minorV(-1)
    , multiInstanceParentName()
    , xPosHint(INT_MIN)
    , yPosHint(INT_MIN)
    , fixedName()
    , paramValues()
    , group(group)
    , reason(reason)
    , createGui(true)
    , addToProject(true)
    , serialization()
    {

    }
    
};



class FlagSetter {
    
    bool* p;
    QMutex* lock;
    
public:
    
    FlagSetter(bool initialValue,bool* p);
    
    FlagSetter(bool initialValue,bool* p, QMutex* mutex);
    
    ~FlagSetter();
};

class AppInstance
    : public QObject, public boost::noncopyable, public TimeLineKeyFrames
{
    Q_OBJECT

public:


    AppInstance(int appID);

    virtual ~AppInstance();

    virtual void aboutToQuit();
    
    virtual bool isBackground() const { return true; }
    
    
    struct RenderWork {
        OutputEffectInstance* writer;
        int firstFrame;
        int lastFrame;
        int frameStep;
        bool useRenderStats;
        bool isRestart;
        
        RenderWork()
        : writer(0)
        , firstFrame(0)
        , lastFrame(0)
        , frameStep(0)
        , useRenderStats(false)
        , isRestart(false)
        {
            
        }
        
        RenderWork(OutputEffectInstance* writer,
                   int firstFrame,
                   int lastFrame,
                   int frameStep,
                   bool useRenderStats)
        : writer(writer)
        , firstFrame(firstFrame)
        , lastFrame(lastFrame)
        , frameStep(frameStep)
        , useRenderStats(useRenderStats)
        , isRestart(false)
        {
            
        }
    };
    
    virtual void load(const CLArgs& cl,bool makeEmptyInstance);

    int getAppID() const;

    /** @brief Create a new node  in the node graph.
     **/
    NodePtr createNode(const CreateNodeArgs & args);
  
    NodePtr getNodeByFullySpecifiedName(const std::string & name) const;
    
    boost::shared_ptr<Project> getProject() const;
    
    boost::shared_ptr<TimeLine> getTimeLine() const;

    /*true if the user is NOT scrubbing the timeline*/
    virtual bool shouldRefreshPreview() const
    {
        return false;
    }

    virtual void connectViewersToViewerCache()
    {
    }

    virtual void disconnectViewersFromViewerCache()
    {
    }

    virtual void errorDialog(const std::string & title,const std::string & message,bool useHtml) const;
    virtual void errorDialog(const std::string & title,const std::string & message,bool* stopAsking,bool useHtml) const;
    virtual void warningDialog(const std::string & title,const std::string & message,bool useHtml) const;
    virtual void warningDialog(const std::string & title,const std::string & message,bool* stopAsking,bool useHtml) const;
    virtual void informationDialog(const std::string & title,const std::string & message,bool useHtml) const;
    virtual void informationDialog(const std::string & title,const std::string & message,bool* stopAsking,bool useHtml) const;
    
    virtual StandardButtonEnum questionDialog(const std::string & title,
                                                      const std::string & message,
                                                      bool useHtml,
                                                      StandardButtons buttons =
                                                      StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                                  StandardButtonEnum defaultButton = eStandardButtonNoButton) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Asks a question to the user and returns the reply.
     * @param stopAsking Set to true if the user do not want Natron to ask the question again.
     **/
    virtual StandardButtonEnum questionDialog(const std::string & /*title*/,
                                                      const std::string & /*message*/,
                                                      bool /*useHtml*/,
                                                      StandardButtons /*buttons*/,
                                                      StandardButtonEnum /*defaultButton*/,
                                                      bool* /*stopAsking*/)
    {
        return eStandardButtonYes;
    }

    
    virtual void loadProjectGui(boost::archive::xml_iarchive & /*archive*/) const
    {
    }

    virtual void saveProjectGui(boost::archive::xml_oarchive & /*archive*/)
    {
    }

    virtual void setupViewersForViews(const std::vector<std::string>& /*viewNames*/)
    {
    }

    virtual void notifyRenderStarted(const QString & /*sequenceName*/,
                                     int /*firstFrame*/,
                                     int /*lastFrame*/,
                                     int /*frameStep*/,
                                     bool /*canPause*/,
                                     OutputEffectInstance* /*writer*/,
                                     const boost::shared_ptr<ProcessHandler> & /*process*/)
    {
    }
    
    

    virtual bool isShowingDialog() const
    {
        return false;
    }
    
    virtual bool isGuiFrozen() const { return false; }

    virtual void progressStart(const NodePtr& node,
                               const std::string &message,
                               const std::string &messageid,
                               bool canCancel = true)
    {
        Q_UNUSED(node);
        Q_UNUSED(message);
        Q_UNUSED(messageid);
        Q_UNUSED(canCancel);
    }

    virtual void progressEnd(const NodePtr& /*node*/)
    {
    }

    virtual bool progressUpdate(const NodePtr& /*node*/,
                                double /*t*/)
    {
        return true;
    }

    /**
     * @brief Checks for a new version of Natron
     **/
    void checkForNewVersion() const;
    virtual void onMaxPanelsOpenedChanged(int /*maxPanels*/)
    {
    }
    
    virtual void onRenderQueuingChanged(bool /*queueingEnabled*/)
    {
    }

    ViewerColorSpaceEnum getDefaultColorSpaceForBitDepth(ImageBitDepthEnum bitdepth) const;
    
    double getProjectFrameRate() const;

    virtual std::string openImageFileDialog() { return std::string(); }
    virtual std::string saveImageFileDialog() { return std::string(); }

    
    void onOCIOConfigPathChanged(const std::string& path);
    
  
    /**
     * @brief Given writer names, start rendering the given RenderRequest. If empty all Writers in the project
     * will be rendered using the frame ranges.
     **/
    void startWritersRenderingFromNames(bool enableRenderStats,
                               bool doBlockingRender,
                               const std::list<std::string>& writers,
                               const std::list<std::pair<int,std::pair<int,int> > >& frameRanges);
    void startWritersRendering(bool doBlockingRender, const std::list<RenderWork>& writers);

    

public:
    
    virtual void clearViewersLastRenderedTexture() {}

    virtual void toggleAutoHideGraphInputs() {}

    /**
     * @brief In Natron v1.0.0 plug-in IDs were lower case only due to a bug in OpenFX host support.
     * To be able to load projects created in Natron v1.0.0 we must identity that the project was created in this version
     * and try to load plug-ins with their lower case ID instead.
     **/
    void setProjectWasCreatedWithLowerCaseIDs(bool b);
    bool wasProjectCreatedWithLowerCaseIDs() const;
    
    bool isCreatingPythonGroup() const;
    
    bool isCreatingNodeTree() const;
    
    void setIsCreatingNodeTree(bool b);
    
    virtual void appendToScriptEditor(const std::string& str);
    
    virtual void printAutoDeclaredVariable(const std::string& str);
    
    void getFrameRange(double* first,double* last) const;
    
    virtual void setLastViewerUsingTimeline(const NodePtr& /*node*/) {}
    
    virtual ViewerInstance* getLastViewerUsingTimeline() const { return 0; }
    
    bool loadPythonScript(const QFileInfo& file);
    
    NodePtr createWriter(const std::string& filename,
                                         CreateNodeReason reason,
                                         const boost::shared_ptr<NodeCollection>& collection,
                                         int firstFrame = INT_MIN, int lastFrame = INT_MAX);
    virtual void queueRedrawForAllViewers() {}
    
    virtual void renderAllViewers(bool /* canAbort*/) {}
    
    virtual void abortAllViewers() {}
    
    virtual void declareCurrentAppVariable_Python();

    void execOnProjectCreatedCallback();
    
    virtual void createLoadProjectSplashScreen(const QString& /*projectFile*/) {}
    
    virtual void updateProjectLoadStatus(const QString& /*str*/) {}
    
    virtual void closeLoadPRojectSplashScreen() {}
    
    std::string getAppIDString() const;
    
    void setCreatingNode(bool b);
    bool isCreatingNode() const;
    
    virtual bool isDraftRenderEnabled() const { return false; }
    
    virtual void setUserIsPainting(const NodePtr& /*rotopaintNode*/,
                                   const boost::shared_ptr<RotoStrokeItem>& /*stroke*/,
                                  bool /*isPainting*/) {}
    
    virtual void getActiveRotoDrawingStroke(NodePtr* /*node*/,
                                            boost::shared_ptr<RotoStrokeItem>* /*stroke*/,
                                            bool* /*isPainting*/) const { }
    
    virtual bool isRenderStatsActionChecked() const { return false; }
    
    bool saveTemp(const std::string& filename);
    virtual bool save(const std::string& filename);
    virtual bool saveAs(const std::string& filename);
    virtual AppInstance* loadProject(const std::string& filename);
    
    ///Close the current project but keep the window
    virtual bool resetProject();
    
    ///Reset + close window, quit if last window
    virtual bool closeProject();
    
    ///Opens a new window
    virtual AppInstance* newProject();
    
    virtual void* getOfxHostOSHandle() const { return NULL; }
    
    virtual void updateLastPaintStrokeData(int /*newAge*/,
                                           const std::list<std::pair<Point,double> >& /*points*/,
                                           const RectD& /*lastPointsBbox*/,
                                           int /*strokeIndex*/) {}
    
    virtual void getLastPaintStrokePoints(std::list<std::list<std::pair<Point,double> > >* /*strokes*/, int* /*strokeIndex*/) const {}

    virtual int getStrokeLastIndex() const { return -1; }
    
    virtual void getStrokeAndMultiStrokeIndex(boost::shared_ptr<RotoStrokeItem>* /*stroke*/, int* /*strokeIndex*/) const {}
    
    virtual void getRenderStrokeData(RectD* /*lastStrokeMovementBbox*/, std::list<std::pair<Point,double> >* /*lastStrokeMovementPoints*/,
                                     double */*distNextIn*/, boost::shared_ptr<Image>* /*strokeImage*/) const {}
    
    virtual void updateStrokeImage(const boost::shared_ptr<Image>& /*image*/, double /*distNextOut*/, bool /*setDistNextOut*/) {}
    
    virtual RectD getLastPaintStrokeBbox() const { return RectD(); }
    
    virtual RectD getPaintStrokeWholeBbox() const { return RectD(); }
    
    void removeRenderFromQueue(OutputEffectInstance* writer);
    
public Q_SLOTS:
    
    void quit();

    virtual void redrawAllViewers() {}

    void triggerAutoSave();

    void clearOpenFXPluginsCaches();

    void clearAllLastRenderedImages();

    void newVersionCheckDownloaded();

    void newVersionCheckError();

    void onBackgroundRenderProcessFinished();

    void onQueuedRenderFinished(int retCode);
    
Q_SIGNALS:

    void pluginsPopulated();

protected:
    
    virtual void onGroupCreationFinished(const NodePtr& node, CreateNodeReason reason);

    virtual void createNodeGui(const NodePtr& /*node*/,
                               const NodePtr&  /*parentmultiinstance*/,
                               const CreateNodeArgs& /*args*/)
    {
    }
    
private:
    
    void startNextQueuedRender(OutputEffectInstance* finishedWriter);
        
    
    void getWritersWorkForCL(const CLArgs& cl,std::list<AppInstance::RenderWork>& requests);


    NodePtr createNodeInternal(const CreateNodeArgs& args);
    
    void setGroupLabelIDAndVersion(const NodePtr& node,
                                   const QString& pythonModulePath,
                                   const QString &pythonModule);
    
    NodePtr createNodeFromPythonModule(Plugin* plugin,
                                                       const boost::shared_ptr<NodeCollection>& group,
                                                       CreateNodeReason reason,
                                                       const boost::shared_ptr<NodeSerialization>& serialization);
    
    boost::scoped_ptr<AppInstancePrivate> _imp;
};

class CreatingNodeTreeFlag_RAII
{
    AppInstance* _app;
public:
    
    CreatingNodeTreeFlag_RAII(AppInstance* app)
    : _app(app)
    {
        app->setIsCreatingNodeTree(true);
    }
    
    ~CreatingNodeTreeFlag_RAII()
    {
        _app->setIsCreatingNodeTree(false);
    }
};

NATRON_NAMESPACE_EXIT;

#endif // APPINSTANCE_H
