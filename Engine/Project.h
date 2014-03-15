//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_PROJECT_H_
#define NATRON_ENGINE_PROJECT_H_

#include <map>
#include <vector>

#include <boost/noncopyable.hpp>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)

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
namespace Natron{
class Node;
class OutputEffectInstance;
struct ProjectPrivate;
    
class Project : public QObject,  public KnobHolder , public boost::noncopyable {
    
    Q_OBJECT
    
public:
    
    Project(AppInstance* appInstance);
    
    virtual ~Project();
    
    
    /**
     * @brief Loads the project with the given path and name corresponding to a file on disk.
     **/
    bool loadProject(const QString& path,const QString& name);
    
    /**
     * @brief Saves the project with the given path and name corresponding to a file on disk.
     * @param autoSave If true then it will save the project in a temporary file instead (see autoSave()).
     **/
    void saveProject(const QString& path,const QString& name,bool autoSave);
    
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
    std::vector<Node*> getCurrentNodes() const;
    
    bool connectNodes(int inputNumber,const std::string& inputName,Natron::Node* output);
    
    /**
     * @brief Connects the node 'input' to the node 'output' on the input number 'inputNumber'
     * of the node 'output'. If 'force' is true, then it will disconnect any previous connection
     * existing on 'inputNumber' and connect the previous input as input of the new 'input' node.
     **/
    bool connectNodes(int inputNumber,Natron::Node* input,Natron::Node* output,bool force = false);
    
    /**
     * @brief Disconnects the node 'input' and 'output' if any connection between them is existing.
     * If autoReconnect is true, after disconnecting 'input' and 'output', if the 'input' had only
     * 1 input, and it was connected, it will connect output to the input of  'input'.
     **/
    bool disconnectNodes(Natron::Node* input,Natron::Node* output,bool autoReconnect = false);
    
    /**
     * @brief Attempts to add automatically the node 'created' to the node graph.
     * 'selected' is the node currently selected by the user.
     **/
    bool autoConnectNodes(Natron::Node* selected,Natron::Node* created);
    
    QString getProjectName() const WARN_UNUSED_RETURN ;
    
    QString getLastAutoSaveFilePath() const;
    
    QString getProjectPath() const WARN_UNUSED_RETURN;
    
    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN;
    
    bool isSaveUpToDate() const WARN_UNUSED_RETURN;
    
    //QDateTime getProjectAgeSinceLastSave() const WARN_UNUSED_RETURN ;
    
    //QDateTime getProjectAgeSinceLastAutosave() const WARN_UNUSED_RETURN;
    
    void getProjectDefaultFormat(Format *f) const;
    
    void getProjectFormats(std::vector<Format> *formats) const;
    
    int getProjectViewsCount() const;
    
    void setOrAddProjectFormat(const Format& frmt,bool skipAdd = false);
    
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
    
    void addNodeToProject(Node* n);
    
    void clearNodes();
    
    void incrementKnobsAge();
    
    int getKnobsAge() const;
    
    void setLastTimelineSeekCaller(Natron::OutputEffectInstance* output);

    void beginProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller);

    void stackEvaluateRequest(Natron::ValueChangedReason reason, KnobHolder* caller, Knob *k, bool isSignificant);

    void endProjectWideValueChanges(KnobHolder* caller);

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
    
public slots:

    void onTimeChanged(SequenceTime time,int reason);

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
    int tryAddProjectFormat(const Format& f);
    
    void setProjectDefaultFormat(const Format& f);
    
    void loadProjectInternal(const QString& path,const QString& name);
    
    void saveProjectInternal(const QString& path,const QString& name,bool autosave = false);
    
    /**
     * @brief Remove all the autosave files from the disk.
     **/
    void removeAutoSaves() const;
    
    /**
     * @brief Resets the project state.
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
    virtual void evaluate(Knob* knob,bool isSignificant) OVERRIDE FINAL;
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReason reason) OVERRIDE FINAL;
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReason reason)  OVERRIDE FINAL;
    
    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(Knob* k,Natron::ValueChangedReason reason)  OVERRIDE FINAL;

    void save(ProjectSerialization* serializationObject) const;
    
    void load(const ProjectSerialization& obj);
    

    boost::scoped_ptr<ProjectPrivate> _imp;

};

} // Natron

#endif // NATRON_ENGINE_PROJECT_H_
