//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_PROJECT_H_
#define NATRON_ENGINE_PROJECT_H_

#include <map>
#include <vector>
#include <boost/noncopyable.hpp>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDateTime>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Engine/Knob.h"
#include "Engine/Format.h"
#include "Engine/TimeLine.h"

class QString;
class QDateTime;
class AppInstance;
class ProjectSerialization;
class KnobSerialization;
class ProjectGui;
class AddFormatDialog;
namespace Natron {
class Node;
class OutputEffectInstance;
struct ProjectPrivate;

class Project
    : public QObject,  public KnobHolder, public boost::noncopyable
{
    Q_OBJECT

public:

    Project(AppInstance* appInstance);

    virtual ~Project();


    /**
     * @brief Loads the project with the given path and name corresponding to a file on disk.
     **/
    bool loadProject(const QString & path,const QString & name);

    /**
     * @brief Saves the project with the given path and name corresponding to a file on disk.
     * @param autoSave If true then it will save the project in a temporary file instead (see autoSave()).
     * @returns The actual filepath of the file saved
     **/
    QString saveProject(const QString & path,const QString & name,bool autoSave);

    /**
     * @brief Same as saveProject except that it will save the project in a temporary file
     * so it doesn't overwrite the project.
     **/
    void autoSave();

    /**
     * @brief Same as autoSave() but the auto-save is run in a separate thread instead.
     **/
    void triggerAutoSave();

    /**
     * @brief Returns the path to where the auto save files are stored on disk.
     **/
    static QString autoSavesDir() WARN_UNUSED_RETURN;


    /** @brief Attemps to find an autosave. If found one,prompts the user
     * whether he/she wants to load it. If something was loaded this function
     * returns true,otherwise false.
     * DO NOT CALL THIS: This is called by AppInstance when the application is launching.
     **/
    bool findAndTryLoadAutoSave() WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if the project is currently loading.
     **/
    bool isLoadingProject() const;

    /**
     * @brief Constructs a vector with all the nodes in the project (even the ones the user
     * has deleted but were kept in the undo/redo stack.) This is MT-safe.
     **/
    std::vector< boost::shared_ptr<Natron::Node> > getCurrentNodes() const;

    bool hasNodes() const;

    bool connectNodes(int inputNumber,const std::string & inputName,Node* output);

    /**
     * @brief Connects the node 'input' to the node 'output' on the input number 'inputNumber'
     * of the node 'output'. If 'force' is true, then it will disconnect any previous connection
     * existing on 'inputNumber' and connect the previous input as input of the new 'input' node.
     **/
    bool connectNodes(int inputNumber,boost::shared_ptr<Natron::Node> input,Node* output,bool force = false);

    /**
     * @brief Disconnects the node 'input' and 'output' if any connection between them is existing.
     * If autoReconnect is true, after disconnecting 'input' and 'output', if the 'input' had only
     * 1 input, and it was connected, it will connect output to the input of  'input'.
     **/
    bool disconnectNodes(Node* input,Node* output,bool autoReconnect = false);

    /**
     * @brief Attempts to add automatically the node 'created' to the node graph.
     * 'selected' is the node currently selected by the user.
     **/
    bool autoConnectNodes(boost::shared_ptr<Natron::Node> selected,boost::shared_ptr<Natron::Node> created);

    QString getProjectName() const WARN_UNUSED_RETURN;

    QString getLastAutoSaveFilePath() const;

    bool hasEverAutoSaved() const;

    QString getProjectPath() const WARN_UNUSED_RETURN;

    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN;

    bool isSaveUpToDate() const WARN_UNUSED_RETURN;

    //QDateTime getProjectAgeSinceLastSave() const WARN_UNUSED_RETURN ;

    //QDateTime getProjectAgeSinceLastAutosave() const WARN_UNUSED_RETURN;

    void getProjectDefaultFormat(Format *f) const;

    void getAdditionalFormats(std::list<Format> *formats) const;

    int getProjectViewsCount() const;

    int getProjectMainView() const;

    void setOrAddProjectFormat(const Format & frmt,bool skipAdd = false);
    
    bool isAutoSetProjectFormatEnabled() const;
    
    void setAutoSetProjectFormatEnabled(bool enabled);

    bool isAutoPreviewEnabled() const;

    void toggleAutoPreview();

    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN;

    // TimeLine operations (to avoid duplicating the shared_ptr when possible)
    void setFrameRange(int first, int last);

    int currentFrame() const WARN_UNUSED_RETURN;

    int firstFrame() const WARN_UNUSED_RETURN;

    int lastFrame() const WARN_UNUSED_RETURN;

    int leftBound() const WARN_UNUSED_RETURN;

    int rightBound() const WARN_UNUSED_RETURN;

    void initNodeCountersAndSetName(Node* n);

    void addNodeToProject(boost::shared_ptr<Natron::Node> n);

    /**
     * @brief Remove the node n from the project so the pointer is no longer
     * referenced anywhere. This function is called on nodes that were already deleted by the user but were kept into
     * the undo/redo stack. That means this node is no longer references by any other node and can be safely deleted.
     * The first thing this function does is to assert that the node n is not active.
     **/
    void removeNodeFromProject(const boost::shared_ptr<Natron::Node> & n);

    void clearNodes(bool emitSignal = true);

    void setLastTimelineSeekCaller(Natron::OutputEffectInstance* output);
    Natron::OutputEffectInstance* getLastTimelineSeekCaller() const;


    /**
     * @brief Returns true if the project is considered as irrelevant and shouldn't be autosaved anyway.
     * Such a graph is for example a simple Viewer.
     **/
    bool isGraphWorthLess() const;

    bool tryLock() const;

    void unlock() const;

    qint64 getProjectCreationTime() const;

    /**
     * @brief Copy the current node counters into the counters map. MT-safe
     **/
    void getNodeCounters(std::map<std::string,int>* counters) const;

    /**
     * @brief Returns a pointer to a node whose name is the same as the name given in parameter.
     * If no such node could be found, NULL is returned.
     **/
    boost::shared_ptr<Natron::Node> getNodeByName(const std::string & name) const;

    /**
     * @brief Called exclusively by the Node class when it needs to retrieve the shared ptr
     * from the "this" pointer.
     **/
    boost::shared_ptr<Natron::Node> getNodePointer(Natron::Node* n) const;
    Natron::ViewerColorSpaceEnum getDefaultColorSpaceForBitDepth(Natron::ImageBitDepthEnum bitdepth) const;


    /**
     * @brief Remove all the autosave files from the disk.
     **/
    static void removeAutoSaves();
    virtual bool isProject() const
    {
        return true;
    }

    /**
     * @brief Returns the user environment variables for the project
     **/
    void getEnvironmentVariables(std::map<std::string,std::string>& env) const;
    /**
     * @brief Decode the project variables from the encoded version;
     **/
    static void makeEnvMap(const std::string& encoded,std::map<std::string,std::string>& variables);
    
    /**
     * @brief Expands the environment variables in the given string that are found in env
     **/
    static void expandVariable(const std::map<std::string,std::string>& env,std::string& str);
    static bool expandVariable(const std::string& varName,const std::string& varValue,std::string& str);
    
    /**
     * @brief Try to find in str a variable from env. The longest match is replaced by the variable name.
     **/
    static void findReplaceVariable(const std::map<std::string,std::string>& env,std::string& str);
    
    /**
     * @brief Make the given string relative to the given variable.
     * If the path indicated by varValue doesn't exist then str will be unchanged.
     **/
    static void makeRelativeToVariable(const std::string& varName,const std::string& varValue,std::string& str);
    
    static bool isRelative(const std::string& str);
    
    /**
     * @brief For all active nodes, find all file-paths that uses the given projectPathName and if the location was valid,
     * change the file-path to be relative to the newProjectPath.
     **/
    void fixRelativeFilePaths(const std::string& projectPathName,const std::string& newProjectPath,bool blockEval);

    
    /**
     * @brief For all active nodes, if it has a file-path parameter using the oldName of a variable, it will turn it into the
     * newName.
     **/
    void fixPathName(const std::string& oldName,const std::string& newName);
    
    /**
     * @brief If str is relative it will canonicalize the path, i.e expand all variables and '.' and '..' that may 
     * be. When returning from this function str will be an absolute path.
     * It internally uses expandVariable
     **/
    void canonicalizePath(std::string& str);
    
    /**
     * @brief Tries to find any variable that str could start with and simplify the given path with the longest
     * variable matching. It internally uses findReplaceVariable
     **/
    void simplifyPath(std::string& str);
    
    /**
     * @brief Same as simplifyPath but will only try with the [Project] variable
     **/
    void makeRelativeToProject(std::string& str);
    
    void onOCIOConfigPathChanged(const std::string& path,bool blockevaluation);


    static std::string escapeXML(const std::string &input);
    static std::string unescapeXML(const std::string &input);
    

    void setAllNodesAborted(bool aborted);
    
public slots:

    void onAutoSaveTimerTriggered();

    ///Closes the project, clearing all nodes and reseting the project name
    void closeProject()
    {
        reset();
    }

signals:

    void mustCreateFormat();

    void formatChanged(Format);

    void autoPreviewChanged(bool);

    void nodesCleared();

    void projectNameChanged(QString);

    void knobsInitialized();

private:

    void refreshViewersAndPreviews();

    /*Returns the index of the format*/
    int tryAddProjectFormat(const Format & f);

    void setProjectDefaultFormat(const Format & f);

    bool loadProjectInternal(const QString & path,const QString & name,bool isAutoSave,const QString& realFilePath);

    QString saveProjectInternal(const QString & path,const QString & name,bool autosave = false);

    
    bool fixFilePath(const std::string& projectPathName,const std::string& newProjectPath,
                        std::string& filePath);


    /**
     * @brief Resets the project state clearing all nodes and the project name.
     **/
    void reset();

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE FINAL;


    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(KnobI* knob,bool isSignificant,Natron::ValueChangedReasonEnum reason) OVERRIDE FINAL;

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReasonEnum reason) OVERRIDE FINAL;

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReasonEnum reason)  OVERRIDE FINAL;

    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(KnobI* k,Natron::ValueChangedReasonEnum reason,SequenceTime time)  OVERRIDE FINAL;

    void save(ProjectSerialization* serializationObject) const;

    bool load(const ProjectSerialization & obj,const QString& name,const QString& path,bool isAutoSave,const QString& realFilePath);


    boost::scoped_ptr<ProjectPrivate> _imp;
};
} // Natron

#endif // NATRON_ENGINE_PROJECT_H_
