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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ProjectPrivate.h"

#include <list>
#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Project.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"


NATRON_NAMESPACE_ENTER;

ProjectPrivate::ProjectPrivate(Project* project)
    : _publicInterface(project)
    , projectLock()
    , hasProjectBeenSavedByUser(false)
    , ageSinceLastSave( QDateTime::currentDateTime() )
    , lastAutoSave()
    , projectCreationTime(ageSinceLastSave)
    , builtinFormats()
    , additionalFormats()
    , formatMutex(QMutex::Recursive)
    , envVars()
    , projectName()
    , projectPath()
    , formatKnob()
    , addFormatKnob()
    , previewMode()
    , colorSpace8u()
    , colorSpace16u()
    , colorSpace32f()
    , natronVersion()
    , originalAuthorName()
    , lastAuthorName()
    , projectCreationDate()
    , saveDate()
    , onProjectLoadCB()
    , onProjectSaveCB()
    , onProjectCloseCB()
    , onNodeCreated()
    , onNodeDeleted()
    , timeline( new TimeLine(project) )
    , autoSetProjectFormat(appPTR->getCurrentSettings()->isAutoProjectFormatEnabled())
    , isLoadingProjectMutex()
    , isLoadingProject(false)
    , isLoadingProjectInternal(false)
    , isSavingProjectMutex()
    , isSavingProject(false)
    , autoSaveTimer( new QTimer() )
    , projectClosing(false)
    , tlsData(new TLSHolder<Project::ProjectTLSData>())
    
{
    
    autoSaveTimer->setSingleShot(true);

}

bool
ProjectPrivate::restoreFromSerialization(const ProjectSerialization & obj,
                                         const QString& name,
                                         const QString& path,
                                         bool* mustSave)
{
    
    /*1st OFF RESTORE THE PROJECT KNOBS*/
    bool ok;
    {
        CreatingNodeTreeFlag_RAII creatingNodeTreeFlag(_publicInterface->getApp());
        
        projectCreationTime = QDateTime::fromMSecsSinceEpoch( obj.getCreationDate() );
        
        _publicInterface->getApp()->updateProjectLoadStatus(QObject::tr("Restoring project settings..."));
        
        /*we must restore the entries in the combobox before restoring the value*/
        std::vector<std::string> entries;
        
        for (std::list<Format>::const_iterator it = builtinFormats.begin(); it != builtinFormats.end(); ++it) {
            QString formatStr = ProjectPrivate::generateStringFromFormat(*it);
            entries.push_back( formatStr.toStdString() );
        }
        
        const std::list<Format> & objAdditionalFormats = obj.getAdditionalFormats();
        for (std::list<Format>::const_iterator it = objAdditionalFormats.begin(); it != objAdditionalFormats.end(); ++it) {
            QString formatStr = ProjectPrivate::generateStringFromFormat(*it);
            entries.push_back( formatStr.toStdString() );
        }
        additionalFormats = objAdditionalFormats;
        
        formatKnob->populateChoices(entries);
        autoSetProjectFormat = false;
        
        const std::list< boost::shared_ptr<KnobSerialization> > & projectSerializedValues = obj.getProjectKnobsValues();
        const std::vector< KnobPtr > & projectKnobs = _publicInterface->getKnobs();
        
        /// 1) restore project's knobs.
        for (U32 i = 0; i < projectKnobs.size(); ++i) {
            ///try to find a serialized value for this knob
            for (std::list< boost::shared_ptr<KnobSerialization> >::const_iterator it = projectSerializedValues.begin(); it != projectSerializedValues.end(); ++it) {
                
                if ( (*it)->getName() == projectKnobs[i]->getName() ) {
                    
                    ///EDIT: Allow non persistent params to be loaded if we found a valid serialization for them
                    //if ( projectKnobs[i]->getIsPersistant() ) {
                    
                    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(projectKnobs[i].get());
                    if (isChoice) {
                        const TypeExtraData* extraData = (*it)->getExtraData();
                        const ChoiceExtraData* choiceData = dynamic_cast<const ChoiceExtraData*>(extraData);
                        assert(choiceData);
                        if (choiceData) {
                            KnobChoice* serializedKnob = dynamic_cast<KnobChoice*>((*it)->getKnob().get());
                            assert(serializedKnob);
                            if (serializedKnob) {
                                isChoice->choiceRestoration(serializedKnob, choiceData);
                            }
                        }
                    } else {
                        projectKnobs[i]->clone( (*it)->getKnob() );
                    }
                    //}
                    break;
                }
            }
            if (projectKnobs[i] == envVars) {
                
                ///For eAppTypeBackgroundAutoRunLaunchedFromGui don't change the project path since it is controlled
                ///by the main GUI process
                if (appPTR->getAppType() != AppManager::eAppTypeBackgroundAutoRunLaunchedFromGui) {
                    autoSetProjectDirectory(path);
                }
                _publicInterface->onOCIOConfigPathChanged(appPTR->getOCIOConfigPath(),false);
            } else if (projectKnobs[i] == natronVersion) {
                std::string v = natronVersion->getValue();
                if (v == "Natron v1.0.0") {
                    _publicInterface->getApp()->setProjectWasCreatedWithLowerCaseIDs(true);
                }
            }
            
        }
        
        /// 2) restore the timeline
        timeline->seekFrame(obj.getCurrentTime(), false, 0, eTimelineChangeReasonOtherSeek);
        
        
        /// 3) Restore the nodes
                
        std::map<std::string,bool> processedModules;
        ok = NodeCollectionSerialization::restoreFromSerialization(obj.getNodesSerialization().getNodesSerialization(),
                                                                        _publicInterface->shared_from_this(),true, &processedModules);
        for (std::map<std::string,bool>::iterator it = processedModules.begin(); it!=processedModules.end(); ++it) {
            if (it->second) {
                *mustSave = true;
                break;
            }
        }
        
        
        _publicInterface->getApp()->updateProjectLoadStatus(QObject::tr("Restoring graph stream preferences"));
        
    } // CreatingNodeTreeFlag_RAII creatingNodeTreeFlag(_publicInterface->getApp());
    
    _publicInterface->forceComputeInputDependentDataOnAllTrees();
    
    QDateTime time = QDateTime::currentDateTime();
    autoSetProjectFormat = false;
    hasProjectBeenSavedByUser = true;
    projectName->setValue(name.toStdString());
    projectPath->setValue(path.toStdString());
    ageSinceLastSave = time;
    lastAutoSave = time;
    _publicInterface->getApp()->setProjectWasCreatedWithLowerCaseIDs(false);
    
    if (obj.getVersion() < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
        _publicInterface->recomputeFrameRangeFromReaders();
    }
    
    return ok;

} // restoreFromSerialization

bool
ProjectPrivate::findFormat(int index,
                           Format* format) const
{
    if ( index >= (int)( builtinFormats.size() + additionalFormats.size() ) ) {
        return false;
    }

    int i = 0;
    if ( index >= (int)builtinFormats.size() ) {
        ///search in the additional formats
        index -= builtinFormats.size();

        for (std::list<Format>::const_iterator it = additionalFormats.begin(); it != additionalFormats.end(); ++it) {
            if (i == index) {
                assert( !it->isNull() );
                *format = *it;

                return true;
            }
            ++i;
        }
    } else {
        ///search in the builtins formats
        for (std::list<Format>::const_iterator it = builtinFormats.begin(); it != builtinFormats.end(); ++it) {
            if (i == index) {
                assert( !it->isNull() );
                *format = *it;

                return true;
            }
            ++i;
        }
    }

    return false;
}
    
void
ProjectPrivate::autoSetProjectDirectory(const QString& path)
{
    std::string pathCpy = path.toStdString();
    if (!pathCpy.empty() && pathCpy[pathCpy.size() -1] == '/') {
        pathCpy.erase(pathCpy.size() - 1, 1);
    }
    std::string env = envVars->getValue();
    std::map<std::string, std::string> envMap;
    Project::makeEnvMap(env, envMap);
    
    ///If there was already a OCIO variable, update it, otherwise create it
    
    std::map<std::string, std::string>::iterator foundPROJECT = envMap.find(NATRON_PROJECT_ENV_VAR_NAME);
    if (foundPROJECT != envMap.end()) {
        foundPROJECT->second = pathCpy;
    } else {
        envMap.insert(std::make_pair(NATRON_PROJECT_ENV_VAR_NAME, pathCpy));
    }
    
    std::string newEnv;
    for (std::map<std::string, std::string>::iterator it = envMap.begin(); it != envMap.end(); ++it) {
        newEnv += NATRON_ENV_VAR_NAME_START_TAG;
        // In order to use XML tags, the text inside the tags has to be escaped.
        newEnv += Project::escapeXML(it->first);
        newEnv += NATRON_ENV_VAR_NAME_END_TAG;
        newEnv += NATRON_ENV_VAR_VALUE_START_TAG;
        newEnv += Project::escapeXML(it->second);
        newEnv += NATRON_ENV_VAR_VALUE_END_TAG;
    }
    if (env != newEnv) {
        if (appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled()) {
            _publicInterface->fixRelativeFilePaths(NATRON_PROJECT_ENV_VAR_NAME, pathCpy,false);
        }
        envVars->setValue(newEnv);
    }
}
    
std::string
ProjectPrivate::runOnProjectSaveCallback(const std::string& filename, bool autoSave)
{
    std::string onProjectSave = _publicInterface->getOnProjectSaveCB();
    if (!onProjectSave.empty()) {
        
        std::vector<std::string> args;
        std::string error;
        try {
            Python::getFunctionArguments(onProjectSave, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor(std::string("Failed to run onProjectSave callback: ")
                                                             + e.what());
            return filename;
        }
        if (!error.empty()) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + error);
            return filename;
        } else {
            
            std::string signatureError;
            signatureError.append("The on project save callback supports the following signature(s):\n");
            signatureError.append("- callback(filename,app,autoSave)");
            if (args.size() != 3) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + signatureError);
                return filename;
            }
            if (args[0] != "filename" || args[1] != "app" || args[2] != "autoSave") {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + signatureError);
                return filename;
            }
            std::string appID = _publicInterface->getApp()->getAppIDString();
            
            std::stringstream ss;
            if (appID != "app") {
                ss << "app = " << appID << "\n";
            }
            ss << "ret = " << onProjectSave << "(" << filename << "," << appID << ",";
            if (autoSave) {
                ss << "True)\n";
            } else {
                ss << "False)\n";
            }
            
            onProjectSave = ss.str();
            std::string err;
            std::string output;
            if (!Python::interpretPythonScript(onProjectSave, &err, &output)) {
                _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectSave callback: " + err);
                return filename;
            } else {
                PyObject* mainModule = Python::getMainModule();
                assert(mainModule);
                PyObject* ret = PyObject_GetAttrString(mainModule, "ret");
                if (!ret) {
                    return filename;
                }
                std::string filePath = filename;
                if (ret) {
                    filePath = Python::PY3String_asString(ret);
                    std::string script = "del ret\n";
                    bool ok = Python::interpretPythonScript(script, &err, 0);
                    assert(ok);
                    if (!ok) {
                        throw std::runtime_error("ProjectPrivate::runOnProjectSaveCallback(): interpretPythonScript("+script+") failed!");
                    }
                }
                if (!output.empty()) {
                    _publicInterface->getApp()->appendToScriptEditor(output);
                }
                return filePath;
            }
            
        }
        
    }
    return filename;
}
    
void
ProjectPrivate::runOnProjectCloseCallback()
{
    std::string onProjectClose = _publicInterface->getOnProjectCloseCB();
    if (!onProjectClose.empty()) {
        
        std::vector<std::string> args;
        std::string error;
        try {
            Python::getFunctionArguments(onProjectClose, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor(std::string("Failed to run onProjectClose callback: ")
                                                             + e.what());
            return;
        }
        if (!error.empty()) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + error);
            return;
        }
        
        std::string signatureError;
        signatureError.append("The on project close callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if (args.size() != 1) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + signatureError);
            return;
        }
        if (args[0] != "app") {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + signatureError);
            return;
        }
        std::string appID = _publicInterface->getApp()->getAppIDString();
        std::string script;
        if (appID != "app") {
            script = script +  "app = " + appID;
        }
        script = script + "\n" + onProjectClose + "(" + appID + ")\n";
        std::string err;
        std::string output;
        if (!Python::interpretPythonScript(script, &err, &output)) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectClose callback: " + err);
        } else {
            if (!output.empty()) {
                _publicInterface->getApp()->appendToScriptEditor(output);
            }
        }
        
    }
}
    
void
ProjectPrivate::runOnProjectLoadCallback()
{
    std::string cb = _publicInterface->getOnProjectLoadCB();
    if (!cb.empty()) {
        
        std::vector<std::string> args;
        std::string error;
        try {
            Python::getFunctionArguments(cb, &error, &args);
        } catch (const std::exception& e) {
            _publicInterface->getApp()->appendToScriptEditor(std::string("Failed to run onProjectLoaded callback: ")
                                                             + e.what());
            return;
        }
        if (!error.empty()) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + error);
            return;
        }
        
        std::string signatureError;
        signatureError.append("The on  project loaded callback supports the following signature(s):\n");
        signatureError.append("- callback(app)");
        if (args.size() != 1) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + signatureError);
            return;
        }
        if (args[0] != "app") {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + signatureError);
            return;
        }
        
        std::string appID = _publicInterface->getApp()->getAppIDString();
        std::string script;
        if (appID != "app") {
            script =  script + "app = " + appID;
        }
        script =  script + "\n" + cb + "(" + appID + ")\n";
        std::string err;
        std::string output;
        if (!Python::interpretPythonScript(script, &err, &output)) {
            _publicInterface->getApp()->appendToScriptEditor("Failed to run onProjectLoaded callback: " + err);
        } else {
            if (!output.empty()) {
                _publicInterface->getApp()->appendToScriptEditor(output);
            }
        }
        
    }

}
    
void
ProjectPrivate::setProjectFilename(const std::string& filename)
{
    projectName->setValue(filename);
}
    
std::string
ProjectPrivate::getProjectFilename() const
{
    return projectName->getValue();
}
    
void
ProjectPrivate::setProjectPath(const std::string& path)
{
    projectPath->setValue(path);
}
    
std::string
ProjectPrivate::getProjectPath() const
{
    return projectPath->getValue();
}

NATRON_NAMESPACE_EXIT;
