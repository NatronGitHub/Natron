/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <map>
#include <vector>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

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
#include "Engine/NodeGroup.h"

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
    :  public KnobHolder, public NodeCollection,  public boost::noncopyable, public boost::enable_shared_from_this<Natron::Project>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    Project(AppInstance* appInstance);

    virtual ~Project();

    /**
     * @brief Loads the project with the given path and name corresponding to a file on disk.
     **/
    bool loadProject(const QString & path,const QString & name, bool isUntitledAutosave = false);

    
    
    /**
     * @brief Saves the project with the given path and name corresponding to a file on disk.
     * @param autoSave If true then it will save the project in a temporary file instead (see autoSave()).
     * @returns The actual filepath of the file saved
     **/
    bool saveProject(const QString & path,const QString & name, QString* newFilePath);
    
    
    bool saveProject_imp(const QString & path,const QString & name,bool autoSave, bool updateProjectProperties, QString* newFilePath = 0);
    
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


    
    bool findAutoSaveForProject(const QString& projectPath,const QString& projectName,QString* autoSaveFileName);

    /**
     * @brief Returns true if the project is currently loading.
     **/
    bool isLoadingProject() const;
    
    bool isLoadingProjectInternal() const;

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

    int currentFrame() const WARN_UNUSED_RETURN;

    void ensureAllProcessingThreadsFinished();

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
    Natron::ViewerColorSpaceEnum getDefaultColorSpaceForBitDepth(Natron::ImageBitDepthEnum bitdepth) const;


    /**
     * @brief Remove all the autosave files from the disk.
     **/
    static void clearAutoSavesDir();
    void removeLastAutosave();
    
    QString getLockAbsoluteFilePath() const;
    void createLockFile();
    void removeLockFile();
    bool getLockFileInfos(const QString& projectPath,const QString& projectName,QString* authorName,QString* lastSaveDate,qint64* appPID) const;
    
    virtual bool isProject() const OVERRIDE
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
    
    bool fixFilePath(const std::string& projectPathName,const std::string& newProjectPath,
                     std::string& filePath);

    
    void onOCIOConfigPathChanged(const std::string& path,bool blockevaluation);


    static std::string escapeXML(const std::string &input);
    static std::string unescapeXML(const std::string &input);
    
    double getProjectFrameRate() const;
    
    boost::shared_ptr<KnobPath> getEnvVarKnob() const;
    
    std::string getOnProjectLoadCB() const;
    std::string getOnProjectSaveCB() const;
    std::string getOnProjectCloseCB() const;
    
    std::string getOnNodeCreatedCB() const;
    std::string getOnNodeDeleteCB() const;
    
    bool isProjectClosing() const;

    bool isFrameRangeLocked() const;
    
    void getFrameRange(double* first,double* last) const;
    
    void unionFrameRangeWith(int first,int last);
    
    void recomputeFrameRangeFromReaders();
    
    void createViewer();
    
    void resetProject();
    
    
    struct TreeOutput
    {
        boost::shared_ptr<Natron::Node> node;
        std::list<std::pair<int,Natron::Node*> > outputs;
    };
    
    struct TreeInput
    {
        boost::shared_ptr<Natron::Node> node;
        std::vector<boost::shared_ptr<Natron::Node> > inputs;
    };
    
    struct NodesTree
    {
        TreeOutput output;
        std::list<TreeInput> inputs;
        std::list<boost::shared_ptr<Natron::Node> > inbetweenNodes;
    };
    
    static void extractTreesFromNodes(const std::list<boost::shared_ptr<Natron::Node> >& nodes,std::list<Project::NodesTree>& trees);
    
    void closeProject(bool aboutToQuit)
    {
        reset(aboutToQuit);
    }
    
    bool addFormat(const std::string& formatSpec);
    
public Q_SLOTS:

    void onAutoSaveTimerTriggered();

    ///Closes the project, clearing all nodes and reseting the project name
    void closeProject()
    {
        closeProject(false);
    }
    
    void onAutoSaveFutureFinished();

Q_SIGNALS:

    void mustCreateFormat();

    void formatChanged(Format);
    
    void frameRangeChanged(int,int);

    void autoPreviewChanged(bool);

    void projectNameChanged(QString);

    void knobsInitialized();
    

private:

    /*Returns the index of the format*/
    int tryAddProjectFormat(const Format & f);

    void setProjectDefaultFormat(const Format & f);

    bool loadProjectInternal(const QString & path,const QString & name,bool isAutoSave,
                             bool isUntitledAutosave, bool* mustSave);

    QString saveProjectInternal(const QString & path,const QString & name,bool autosave, bool updateProjectProperties);

    
    

    /**
     * @brief Resets the project state clearing all nodes and the project name.
     **/
    void reset(bool aboutToQuit);

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
    virtual void onKnobValueChanged(KnobI* k,Natron::ValueChangedReasonEnum reason,SequenceTime time,
                                    bool originatedFromMainThread)  OVERRIDE FINAL;

    void save(ProjectSerialization* serializationObject) const;

    bool load(const ProjectSerialization & obj,const QString& name,const QString& path, bool* mustSave);


    boost::scoped_ptr<ProjectPrivate> _imp;
};
} // Natron

#endif // NATRON_ENGINE_PROJECT_H
