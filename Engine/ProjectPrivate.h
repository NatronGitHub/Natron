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

#ifndef PROJECTPRIVATE_H
#define PROJECTPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <list>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDateTime>
#include <QString>
#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)


#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobFactory.h"
#include "Engine/ThreadStorage.h"

class QTimer;
class TimeLine;
class NodeSerialization;
class ProjectSerialization;
class KnobFile;
namespace Natron {
class Node;
class OutputEffectInstance;
class Project;

inline QString
generateStringFromFormat(const Format & f)
{
    QString formatStr;

    formatStr.append(f.getName().c_str());
    formatStr.append("  ");
    formatStr.append(QString::number(f.width()));
    formatStr.append(" x ");
    formatStr.append(QString::number(f.height()));
    formatStr.append("  ");
    formatStr.append(QString::number(f.getPixelAspectRatio()));

    return formatStr;
}
    
    
inline bool
generateFormatFromString(const QString& spec, Format* f)
{
    QStringList splits = spec.split(' ');
    if (splits.size() != 3) {
        return false;
    }
    
    QStringList sizes = splits[1].split('x');
    if (sizes.size() != 2) {
        return false;
    }
    
    f->setName(splits[0].toStdString());
    f->x1 = 0;
    f->y1 = 0;
    f->x2 = sizes[0].toInt();
    f->y2 = sizes[1].toInt();
    
    f->setPixelAspectRatio(splits[2].toDouble());
    return true;
}

struct ProjectPrivate
{
    Natron::Project* _publicInterface;
    mutable QMutex projectLock; //< protects the whole project
    QString lastAutoSaveFilePath; //< absolute file path of the last auto-save file
    bool hasProjectBeenSavedByUser; //< has this project ever been saved by the user?
    QDateTime ageSinceLastSave; //< the last time the user saved
    QDateTime lastAutoSave; //< the last time since autosave
    QDateTime projectCreationTime; //< the project creation time
    
    std::list<Format> builtinFormats;
    std::list<Format> additionalFormats; //< added by the user
    mutable QMutex formatMutex; //< protects builtinFormats & additionalFormats
    

    ///Project parameters (settings)
    boost::shared_ptr<KnobPath> envVars;
    boost::shared_ptr<KnobString> projectName; //< name of the project, e.g: "Untitled.ntp"
    boost::shared_ptr<KnobString> projectPath;  //< path of the project, e.g: /Users/Lala/Projects/
    boost::shared_ptr<KnobChoice> formatKnob; //< built from builtinFormats & additionalFormats
    boost::shared_ptr<KnobButton> addFormatKnob;
    boost::shared_ptr<KnobPath> viewsList;
    boost::shared_ptr<KnobButton> setupForStereoButton;
    boost::shared_ptr<KnobBool> previewMode; //< auto or manual
    boost::shared_ptr<KnobChoice> colorSpace8u;
    boost::shared_ptr<KnobChoice> colorSpace16u;
    boost::shared_ptr<KnobChoice> colorSpace32f;
    boost::shared_ptr<KnobDouble> frameRate;
    boost::shared_ptr<KnobInt> frameRange;
    boost::shared_ptr<KnobBool> lockFrameRange;
    
    boost::shared_ptr<KnobString> natronVersion;
    boost::shared_ptr<KnobString> originalAuthorName,lastAuthorName;
    boost::shared_ptr<KnobString> projectCreationDate;
    boost::shared_ptr<KnobString> saveDate;
    
    boost::shared_ptr<KnobString> onProjectLoadCB;
    boost::shared_ptr<KnobString> onProjectSaveCB;
    boost::shared_ptr<KnobString> onProjectCloseCB;
    boost::shared_ptr<KnobString> onNodeCreated;
    boost::shared_ptr<KnobString> onNodeDeleted;
    
    boost::shared_ptr<TimeLine> timeline; // global timeline
    bool autoSetProjectFormat;

    mutable QMutex isLoadingProjectMutex;
    bool isLoadingProject; //< true when the project is loading
    bool isLoadingProjectInternal; //< true when loading the internal project (not gui)
    mutable QMutex isSavingProjectMutex;
    bool isSavingProject; //< true when the project is saving
    boost::shared_ptr<QTimer> autoSaveTimer;
    std::list<boost::shared_ptr<QFutureWatcher<void> > > autoSaveFutures;
    
    mutable QMutex projectClosingMutex;
    bool projectClosing;
    
    ThreadStorage<std::vector<std::string> > viewNamesTLS;
    
    ProjectPrivate(Natron::Project* project);

    bool restoreFromSerialization(const ProjectSerialization & obj,const QString& name,const QString& path, bool* mustSave);

    bool findFormat(int index,Format* format) const;
    
    /**
     * @brief Auto fills the project directory parameter given the project file path
     **/
    void autoSetProjectDirectory(const QString& path);
    
    std::string runOnProjectSaveCallback(const std::string& filename,bool autoSave);
    
    void runOnProjectCloseCallback();
    
    void runOnProjectLoadCallback();
    
    void setProjectFilename(const std::string& filename);
    std::string getProjectFilename() const;
    
    void setProjectPath(const std::string& path);
    std::string getProjectPath() const;
    
};
}

#endif // PROJECTPRIVATE_H
