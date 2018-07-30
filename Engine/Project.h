/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_PROJECT_H
#define NATRON_ENGINE_PROJECT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <vector>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDateTime>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/AfterQuitProcessingI.h"
#include "Engine/Knob.h"
#include "Engine/ChoiceOption.h"
#include "Engine/Format.h"
#include "Engine/TimeLine.h"
#include "Engine/NodeGroup.h"
#include "Engine/ViewIdx.h"
#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct ProjectPrivate;

class Project
    :  public KnobHolder
    , public NodeCollection
    , public AfterQuitProcessingI
    , public boost::noncopyable
    , public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from KnobHolder
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    Project(const AppInstancePtr& appInstance);

public:
    static ProjectPtr create(const AppInstancePtr& appInstance) WARN_UNUSED_RETURN
    {
        return ProjectPtr( new Project(appInstance) );
    }

    ProjectPtr shared_from_this() {
        return boost::dynamic_pointer_cast<Project>(KnobHolder::shared_from_this());
    }

    virtual NodeCollectionPtr getThisShared() OVERRIDE FINAL
    {
        return shared_from_this();
    }


    virtual ~Project();

    //these are per project thread-local data
    struct ProjectTLSData
    {
        std::vector<std::string> viewNames;
    };

    typedef boost::shared_ptr<ProjectTLSData> ProjectDataTLSPtr;

    /**
     * @brief Loads the project with the given path and name corresponding to a file on disk.
     **/
    bool loadProject(const QString & path, const QString & name, bool isUntitledAutosave = false, bool attemptToLoadAutosave = true);


    /**
     * @brief Saves the project with the given path and name corresponding to a file on disk.
     * @param autoSave If true then it will save the project in a temporary file instead (see autoSave()).
     * @returns The actual filepath of the file saved
     **/
    bool saveProject(const QString & path, const QString & name, QString* newFilePath);


    bool saveProject_imp(const QString & path, const QString & name, bool autoSave, bool updateProjectProperties, QString* newFilePath = 0);

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


    bool findAutoSaveForProject(const QString& projectPath, const QString& projectName, QString* autoSaveFileName);

    /**
     * @brief Returns true if the project is currently loading.
     **/
    bool isLoadingProject() const;

    bool isLoadingProjectInternal() const;

    bool getProjectLoadedVersionInfo(SERIALIZATION_NAMESPACE::ProjectBeingLoadedInfo* info) const;

    QString getProjectFilename() const WARN_UNUSED_RETURN;

    QString getLastAutoSaveFilePath() const;

    bool hasEverAutoSaved() const;

    QString getProjectPath() const WARN_UNUSED_RETURN;

    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN;

    bool isSaveUpToDate() const WARN_UNUSED_RETURN;

    void getProjectDefaultFormat(Format *f) const;

    bool getProjectFormatAtIndex(int index, Format* f) const;

    void getProjectFormatEntries(std::vector<ChoiceOption>* formatStrings, int* currentValue) const;

    bool getFormatNameFromRect(const RectI& rect, double par, std::string* name) const;

    void getAdditionalFormats(std::list<Format> *formats) const;

    void setupProjectForStereo();

    void createProjectViews(const std::vector<std::string>& views);

    const std::vector<std::string>& getProjectViewNames() const;

    std::string getViewName(ViewIdx view) const;

    static bool getViewIndex(const std::vector<std::string>& viewNames, const std::string& viewName, ViewIdx* view);
    bool getViewIndex(const std::string& viewName, ViewIdx* view) const;

    int getProjectViewsCount() const;

    bool isGPURenderingEnabledInProject() const;

    std::vector<std::string> getProjectDefaultLayerNames() const;
    std::list<ImagePlaneDesc> getProjectDefaultLayers() const;

    void addProjectDefaultLayer(const ImagePlaneDesc& comps);

    void setOrAddProjectFormat(const Format & frmt, bool skipAdd = false);

    bool isAutoSetProjectFormatEnabled() const;

    void setAutoSetProjectFormatEnabled(bool enabled);

    bool isAutoPreviewEnabled() const;

    void toggleAutoPreview();

    TimeLinePtr getTimeLine() const WARN_UNUSED_RETURN;

    int currentFrame() const WARN_UNUSED_RETURN;


    /**
     * @brief Returns true if the project is considered as irrelevant and shouldn't be autosaved anyway.
     * Such a graph is for example a simple Viewer.
     **/
    bool isGraphWorthLess() const;

    bool tryLock() const;

    void unlock() const;


    /**
     * @brief Called exclusively by the Node class when it needs to retrieve the shared ptr
     * from the "this" pointer.
     **/
    ViewerColorSpaceEnum getDefaultColorSpaceForBitDepth(ImageBitDepthEnum bitdepth) const;


    /**
     * @brief Remove all the autosave files from the disk.
     **/
    static void clearAutoSavesDir();
    void removeLastAutosave();

    QString getLockAbsoluteFilePath() const;
    void createLockFile();
    void removeLockFile();
    bool getLockFileInfos(const QString& projectPath, const QString& projectName, QString* authorName, QString* lastSaveDate, QString* host, qint64* appPID) const;

    virtual bool isProject() const OVERRIDE
    {
        return true;
    }

    /**
     * @brief Returns the user environment variables for the project
     **/
    void getEnvironmentVariables(std::map<std::string, std::string>& env) const;

    /**
     * @brief Expands the environment variables in the given string that are found in env
     **/
    static void expandVariable(const std::map<std::string, std::string>& env, std::string& str);
    static bool expandVariable(const std::string& varName, const std::string& varValue, std::string& str);

    /**
     * @brief Try to find in str a variable from env. The longest match is replaced by the variable name.
     **/
    static void findReplaceVariable(const std::map<std::string, std::string>& env, std::string& str);

    /**
     * @brief Make the given string relative to the given variable.
     * If the path indicated by varValue doesn't exist then str will be unchanged.
     **/
    static void makeRelativeToVariable(const std::string& varName, const std::string& varValue, std::string& str);
    static bool isRelative(const std::string& str);


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

    bool fixFilePath(const std::string& projectPathName, const std::string& newProjectPath,
                     std::string& filePath);


    void onOCIOConfigPathChanged(const std::string& path, bool blockevaluation);


    static std::string escapeXML(const std::string &input);
    static std::string unescapeXML(const std::string &input);

    double getProjectFrameRate() const;

    KnobPathPtr getEnvVarKnob() const;
    std::string getOnProjectLoadCB() const;
    std::string getOnProjectSaveCB() const;
    std::string getOnProjectCloseCB() const;
    std::string getOnNodeCreatedCB() const;
    std::string getOnNodeDeleteCB() const;

    bool isProjectClosing() const;

    bool isFrameRangeLocked() const;

    void getFrameRange(TimeValue* first, TimeValue* last) const;

    void unionFrameRangeWith(TimeValue first, TimeValue last);

    void recomputeFrameRangeFromReaders();

    void createViewer();

    void resetProject();


    /**
     * @brief Calls quitAnyProcessing for all nodes in the group and in each subgroup
     * This is called only when calling AppManager::abortAnyProcessing()
     * @returns True if a node is in the project and a watcher was installed, false otherwise
     **/
    bool quitAnyProcessingForAllNodes(AfterQuitProcessingI* receiver, const GenericWatcherCallerArgsPtr& args);

    bool isOpenGLRenderActivated() const;

    void refreshOpenGLRenderingFlagOnNodes();

    virtual bool invalidateHashCacheInternal(std::set<HashableObject*>* invalidatedObjects) OVERRIDE ;

private:

    virtual void afterQuitProcessingCallback(const GenericWatcherCallerArgsPtr& args) OVERRIDE FINAL;

public:

    void closeProject_blocking(bool aboutToQuit);

    /**
     * @brief Add a format to the default formats of the Project. This will not be seriliazed in the user project.
     **/
    bool addDefaultFormat(const std::string& formatSpec);

    void setTimeLine(const TimeLinePtr& timeline);


    /**
     * @brief Resets the project state clearing all nodes and the project name.
     **/
    void reset(bool aboutToQuit, bool blocking);


    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;



public Q_SLOTS:

    void onQuitAnyProcessingWatcherTaskFinished(int taskID, const GenericWatcherCallerArgsPtr& args);

    void onAutoSaveTimerTriggered();

    void onAutoSaveFutureFinished();

    void onProjectFormatPopulated();

Q_SIGNALS:

    void mustCreateFormat();

    void formatChanged(Format);

    void frameRangeChanged(int, int);

    void autoPreviewChanged(bool);

    void projectNameChanged(QString name, bool modified);

    void knobsInitialized();

    void projectViewsChanged();

private:


    /*Returns the index of the format*/
    int tryAddProjectFormat(const Format & f, bool addAsAdditionalFormat, bool* existed);

    void setProjectDefaultFormat(const Format & f);


    bool loadProjectInternal(const QString & path, const QString & name, bool isAutoSave, bool isUntitledAutosave);

    QString saveProjectInternal(const QString & path, const QString & name, bool autosave, bool updateProjectProperties);



    void doResetEnd(bool aboutToQuit);

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE FINAL;

    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual bool onKnobValueChanged(const KnobIPtr& k,
                                    ValueChangedReasonEnum reason,
                                    TimeValue time,
                                    ViewSetSpec view)  OVERRIDE FINAL;

    bool load(const SERIALIZATION_NAMESPACE::ProjectSerialization & obj, const QString& name, const QString& path);


    boost::scoped_ptr<ProjectPrivate> _imp;
};


inline ProjectPtr
toProject(const KnobHolderPtr& holder)
{
    return boost::dynamic_pointer_cast<Project>(holder);
}


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_PROJECT_H
