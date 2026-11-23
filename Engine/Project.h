/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <functional>

#include <map>
#include <vector>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDateTime>
#include <QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/AfterQuitProcessingI.h"
#include "Engine/Knob.h"
#include "Engine/Format.h"
#include "Engine/TimeLine.h"
#include "Engine/NodeGroup.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

struct ProjectPrivate;

class Project
    : public KnobHolder
    , public NodeCollection
    , public AfterQuitProcessingI
    , public std::enable_shared_from_this<Project>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    struct MakeSharedEnabler;

    // constructors should be privatized in any class that derives from std::enable_shared_from_this<>
    Project(const AppInstancePtr& appInstance);

public:
    static ProjectPtr create(const AppInstancePtr& appInstance);

    virtual ~Project();

    //these are per project thread-local data
    struct ProjectTLSData
    {
        std::vector<std::string> viewNames;
    };

    typedef std::shared_ptr<ProjectTLSData> ProjectDataTLSPtr;

    /**
     * @brief Loads the project with the given path and name corresponding to a file on disk.
     **/
    bool loadProject(const QString & path, const QString & name, bool isUntitledAutosave = false, bool attemptToLoadAutosave = true);

    /**
     * @brief Loads autoSaveName, the auto-save of a project in path, as loadProject() does when the user
     * chooses to use the auto-save of the project (in GUI mode): the project gets the name and the format
     * of the project that was auto-saved.
     **/
    bool restoreAutoSave(const QString & path, const QString & autoSaveName);


    /**
     * @brief Saves the project with the given path and name corresponding to a file on disk.
     * @param autoSave If true then it will save the project in a temporary file instead (see autoSave()).
     * @param format The format of the file, which becomes the format of the project if the save succeeds.
     * eProjectFileFormatNone: getFileFormatForSave().
     * @returns The actual filepath of the file saved
     **/
    bool saveProject(const QString & path, const QString & name, QString* newFilePath, ProjectFileFormatEnum format = eProjectFileFormatNone);


    /**
     * @brief Saves the project. A save other than the background auto-save waits for a running save to finish.
     * @param errorMessage If not null, set to the reason of the failure.
     * @param format See saveProject(). Ignored for auto-saves, which are always binary. It becomes the format
     * of the project only for a save that updates the project properties.
     * @returns false if the project could not be saved.
     **/
    bool saveProject_imp(const QString & path, const QString & name, bool autoSave, bool updateProjectProperties, QString* newFilePath = 0, QString* errorMessage = 0,
                         ProjectFileFormatEnum format = eProjectFileFormatNone);

    /**
     * @brief True while the project is being saved (also by the background auto-save), or a save waits to start.
     * The main thread processes events while a save waits (see saveProject_imp()): the project must not be closed then.
     **/
    bool isSaveInProgress() const WARN_UNUSED_RETURN;

    /**
     * @brief Tests only: while set, every save (of any project) calls hook after rotating the backups, before the
     * new file replaces the project file. If it returns false, the file is not replaced, as if that failed: the
     * save puts the backups back, and tries 3 times before it fails. An empty hook removes it.
     **/
    static void setSaveCommitHookForTesting(const std::function<bool()>& hook);

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
     * @brief Calls kOfxActionSyncPrivateData on all the OpenFX nodes of the project, as before a
     * save. Main thread only: the autosave calls it before it serializes the project in another
     * thread, where the nodes can't be synced.
     **/
    void syncOpenFXPrivateData();

    /**
     * @brief Returns the path to where the auto save files are stored on disk.
     **/
    static QString autoSavesDir() WARN_UNUSED_RETURN;


    /**
     * @brief Finds the auto-save of a project: <name>.binautosave (2.6) or <name>.autosave (2.5 and earlier).
     * If both exist, returns the most recent one, and the other one in otherAutoSaves.
     * Unless includeOlderThanProject is true, an auto-save older than the project file is ignored: it was
     * left by a crash before the project was saved again, e.g. by Natron 2.5, which ignores the binary
     * auto-saves. It is not offered, and not removed here; but, as any auto-save of the project, removeLastAutosave()
     * removes it at the first save or auto-save of the project in this session (unless --no-dialogs or --test, see canRemoveAutoSave()).
     **/
    bool findAutoSaveForProject(const QString& projectPath, const QString& projectName, QString* autoSaveFileName,
                                bool includeOlderThanProject = false, QStringList* otherAutoSaves = 0);

    /**
     * @brief Returns true if the project is currently loading.
     **/
    bool isLoadingProject() const;

    bool isLoadingProjectInternal() const;

    QString getProjectFilename() const WARN_UNUSED_RETURN;

    QString getLastAutoSaveFilePath() const;

    bool hasEverAutoSaved() const;

    QString getProjectPath() const WARN_UNUSED_RETURN;

    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN;

    /**
     * @brief The format of the file of this project: the format of the file it was loaded from, or the
     * one chosen when it was last saved by the user. eProjectFileFormatNone for a new project.
     * Restoring an auto-save (always binary) restores the format of the original project.
     **/
    ProjectFileFormatEnum getFileFormat() const WARN_UNUSED_RETURN;

    /**
     * @brief The format used by the next save: getFileFormat(), or the "Save projects as binary by
     * default" preference for a project that has no format yet.
     **/
    ProjectFileFormatEnum getFileFormatForSave() const WARN_UNUSED_RETURN;

    /**
     * @brief Sets the format of the next saves, e.g. the one chosen in the "Save As..." dialog.
     **/
    void setFileFormat(ProjectFileFormatEnum format);

    /// Whether the loaded project file has GUI data (false for a new project or a background save).
    /// It describes the loaded file, saving doesn't change it. See App.projectFileHasGuiData() in App.rst.
    bool fileHasGuiData() const WARN_UNUSED_RETURN;

    /// Whether something in the project file was not loaded, so that saving would lose it (set before
    /// afterProjectLoad; warnings don't set it). See App.projectLoadedWithErrors() in App.rst.
    bool fileLoadedWithErrors() const WARN_UNUSED_RETURN;

    /**
     * @brief Detects the format of a project file from its first bytes: an XML file starts with "<?xml"
     * (after an optional UTF-8 byte order mark and white space), a binary project with the signature of
     * PortableBinaryOArchive. Returns eProjectFileFormatNone if it is neither.
     **/
    static ProjectFileFormatEnum detectFileFormat(const char* data, std::size_t size) WARN_UNUSED_RETURN;
    static ProjectFileFormatEnum detectFileFormat(const QString& filePath) WARN_UNUSED_RETURN;

    /**
     * @brief If fileName is the name of an auto-save, ending with .ntp.autosave (text, by Natron 2.5 and earlier)
     * or .ntp.binautosave (binary, by Natron 2.6 and later), possibly followed by a time hash for an untitled
     * project (".<digits>"), returns the position of ".ntp" and sets suffixSize to the size of the auto-save suffix
     * after it. Returns -1 otherwise, e.g. for the temporary file of a save (<auto-save name>.XXXXXX, see QSaveFile).
     * Natron 2.5 only looks for ".ntp.autosave", so it never offers to restore a binary auto-save, which it can't load. Its Project::clearAutoSavesDir() (when the user declines to restore one of its
     * own untitled auto-saves, and Cache > Clear all caches) still removes the untitled ones in the Autosaves directory.
     **/
    static int findAutoSaveSuffix(const QString& fileName, int* suffixSize = 0) WARN_UNUSED_RETURN;

    bool isSaveUpToDate() const WARN_UNUSED_RETURN;

    //QDateTime getProjectAgeSinceLastSave() const WARN_UNUSED_RETURN ;

    //QDateTime getProjectAgeSinceLastAutosave() const WARN_UNUSED_RETURN;

    void getProjectDefaultFormat(Format *f) const;

    bool getProjectFormatAtIndex(int index, Format* f) const;

    void getProjectFormatEntries(std::vector<ChoiceOption>* formatStrings, int* currentValue) const;

    bool getFormatNameFromRect(const RectI& rect, double par, std::string* name) const;

    void getAdditionalFormats(std::list<Format> *formats) const;

    void setupProjectForStereo();

    void createProjectViews(const std::vector<std::string>& views);

    const std::vector<std::string>& getProjectViewNames() const;

    int getProjectViewsCount() const;

    bool isGPURenderingEnabledInProject() const;

    std::vector<std::string> getProjectDefaultLayerNames() const;
    std::list<ImagePlaneDesc> getProjectDefaultLayers() const;

    void addProjectDefaultLayer(const ImagePlaneDesc& comps);

    /**
     * @brief Add frmt to the project formats. If the project format is still to be auto-set
     * (empty project, see the "Auto-set project format" preference), it is set to frmt.
     * If mayChangeFormat is false (e.g. during a render), the project format is only auto-set
     * if it is already equal to frmt, so that it never changes while a render is running (#1294).
     **/
    void setOrAddProjectFormat(const Format & frmt, bool skipAdd = false, bool mayChangeFormat = true);

    /**
     * @brief Called on the main thread when the metadata of a Reader is computed: if the project
     * format is still to be auto-set and the Reader could decode its file, set the project format
     * from the Reader's format (if they are equal, just disable the auto-set), before any render starts (#1294).
     * If a sequence render (Writer or playback) is running, this is postponed until it finishes.
     **/
    void autoSetProjectFormatFromReader(const NodePtr& reader);

private:
    /// true if a sequence render (other than the one of ignoredEngine) is running; connects to their renderFinished signal
    bool isSequenceRenderRunning(const QObject* ignoredEngine);

    /// auto-set the project format from the first pending Reader, once no sequence render is running
    void applyPendingAutoSetProjectFormat(const QObject* ignoredEngine);

    /// set (or add) the project format from the Reader's output format
    void setProjectFormatFromReader(const NodePtr& reader);

    /// check the pending Readers again in 100 ms (at most one check is scheduled at a time)
    void scheduleAutoSetProjectFormatRetry();

public:

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

    qint64 getProjectCreationTime() const;


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

    /**
     * @brief Can removeLastAutosave() remove this auto-save file? Always, except with
     * --no-dialogs, where only the auto-saves this session wrote may be removed.
     **/
    bool canRemoveAutoSave(const QString& filePath) const;

    /// Did this project write this auto-save file during this session (--no-dialogs only)?
    bool wroteAutoSave(const QString& filePath) const;

    /// Did a project of this session write this auto-save file (--no-dialogs only)?
    static bool isAutoSaveOfThisSession(const QString& filePath);

private:
    void forgetAutoSave(const QString& filePath);

    /// --no-dialogs: do not auto-save if an auto-save of another session exists for this project
    void refreshAutoSaveDisabled();

public:

    /**
     * @brief Stops the pending auto-save and waits for the one in progress, if any, so that
     * no auto-save is written afterwards (e.g. before removeLastAutosave() when quitting).
     * While it waits, it processes the events except user input (the auto-save may need the main
     * thread), as saveProject_imp() does.
     **/
    void stopAutoSave();

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

    void getFrameRange(double* first, double* last) const;

    void unionFrameRangeWith(int first, int last);

    void recomputeFrameRangeFromReaders();

    void createViewer();

    void resetProject();


    /**
     * @brief Calls quitAnyProcessing for all nodes in the group and in each subgroup
     * This is called only when calling AppManager::abortAnyProcessing()
     * @returns True if a node is in the project and a watcher was installed, false otherwise
     **/
    bool quitAnyProcessingForAllNodes(AfterQuitProcessingI* receiver, const GenericWatcherCallerArgsPtr& args);

    void refreshOpenGLRenderingFlagOnNodes();

private:

    Project(const Project&) = delete;

    Project& operator=(const Project&) = delete;

    virtual void afterQuitProcessingCallback(const GenericWatcherCallerArgsPtr& args) OVERRIDE FINAL;

    /// Stops the processing of all the nodes and waits for it, without the Python GIL (#1575).
    void quitAnyProcessingForAllNodes_blocking();

public:

    struct TreeOutput
    {
        NodePtr node;
        std::list<std::pair<int, NodeWPtr> > outputs;
    };

    struct TreeInput
    {
        NodePtr node;
        std::vector<NodePtr> inputs;
    };

    struct NodesTree
    {
        TreeOutput output;
        std::list<TreeInput> inputs;
        NodesList inbetweenNodes;
    };

    static void extractTreesFromNodes(const NodesList& nodes, std::list<Project::NodesTree>& trees);

    void closeProject_blocking(bool aboutToQuit);

    bool addFormat(const std::string& formatSpec);

    void setTimeLine(const TimeLinePtr& timeline);


    /**
     * @brief Resets the project state clearing all nodes and the project name.
     **/
    void reset(bool aboutToQuit, bool blocking);

public Q_SLOTS:

    void onQuitAnyProcessingWatcherTaskFinished(int taskID, const GenericWatcherCallerArgsPtr& args);

    void onAutoSaveTimerTriggered();

    void onAutoSaveFutureFinished();

    void onProjectFormatPopulated();

    void onSequenceRenderFinishedAutoSetProjectFormat();

    void onAutoSetProjectFormatRetry();

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
    int tryAddProjectFormat(const Format & f, bool* existed);

    void setProjectDefaultFormat(const Format & f);

    bool loadProjectImpl(const QString & path, const QString & name, bool isUntitledAutosave, bool attemptToLoadAutosave, bool isNamedAutoSave);
    NodesRestoreStatus loadProjectInternal(const QString & path, const QString & name, bool isAutoSave,
                                           bool isUntitledAutosave, bool* mustSave) WARN_UNUSED_RETURN;

    QString saveProjectInternal(const QString & path, const QString & name, bool autosave, bool updateProjectProperties, QStringList* warnings,
                                ProjectFileFormatEnum format);



    // An error stopped the load (see fileLoadedWithErrors())
    void setFileLoadedWithErrors();

    void doResetEnd(bool aboutToQuit);

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE FINAL;

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(ValueChangedReasonEnum reason) OVERRIDE FINAL;

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(ValueChangedReasonEnum reason)  OVERRIDE FINAL;

    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual bool onKnobValueChanged(KnobI* k,
                                    ValueChangedReasonEnum reason,
                                    double time,
                                    ViewSpec view,
                                    bool originatedFromMainThread)  OVERRIDE FINAL;

    void save(ProjectSerialization* serializationObject) const;

    NodesRestoreStatus load(const ProjectSerialization & obj, const QString& name, const QString& path, bool* mustSave) WARN_UNUSED_RETURN;


    std::unique_ptr<ProjectPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_PROJECT_H
