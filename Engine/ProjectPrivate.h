//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef PROJECTPRIVATE_H
#define PROJECTPRIVATE_H

#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QDateTime>
CLANG_DIAG_ON(deprecated)
#include <QString>
#include <QMutex>

#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFactory.h"

class TimeLine;
class NodeSerialization;
class ProjectSerialization;
class File_Knob;
namespace Natron{
class Node;
class OutputEffectInstance;
class Project;

inline QString generateStringFromFormat(const Format& f){
    QString formatStr;
    formatStr.append(f.getName().c_str());
    formatStr.append("  ");
    formatStr.append(QString::number(f.width()));
    formatStr.append(" x ");
    formatStr.append(QString::number(f.height()));
    formatStr.append("  ");
    formatStr.append(QString::number(f.getPixelAspect()));
    return formatStr;
}
    
   

struct ProjectPrivate {
    
    mutable QMutex projectLock; //< protects the whole project
    QString projectName; //< name of the project, e.g: "Untitled.EXT"
    QString projectPath; //< path of the project, e.g: /Users/Lala/Projects/
    QString lastAutoSaveFilePath; //< path + name of the last auto-save file
    bool hasProjectBeenSavedByUser; //< has this project ever been saved by the user?
    QDateTime ageSinceLastSave; //< the last time the user saved
    QDateTime lastAutoSave; //< the last time since autosave
    QDateTime projectCreationTime; //< the project creation time
    
    boost::shared_ptr<Choice_Knob> formatKnob;
    std::list<Format> builtinFormats;
    std::list<Format> additionalFormats; //< added by the user
    mutable QMutex formatMutex;
    
    boost::shared_ptr<Button_Knob> addFormatKnob;
    
    boost::shared_ptr<Int_Knob> viewsCount;
    mutable QMutex viewsCountMutex;
    
    boost::shared_ptr<Bool_Knob> previewMode; //< auto or manual
    mutable QMutex previewModeMutex;
    
    mutable QMutex timelineMutex;
    boost::shared_ptr<TimeLine> timeline; // global timeline
    
    
    mutable QMutex nodesLock; //< protects nodeCounters & currentNodes
    std::map<std::string,int> nodeCounters; //< basic counters to instantiate nodes with an index in the node graph
    bool autoSetProjectFormat; 
    std::vector<Natron::Node*> currentNodes;
    
    
    Natron::Project* project;
    
    Natron::OutputEffectInstance* lastTimelineSeekCaller;

    mutable QMutex beginEndMutex; //< protects begin/stack/end value change
    int beginEndBracketsCount;
    int evaluationsCount;
    
    ///for each knob holder, how many times begin was called, and what was the reason of the outtermost begin bracket.
    typedef std::map<KnobHolder*, std::pair<int,Natron::ValueChangedReason> > KnobsValueChangedMap;
    KnobsValueChangedMap holdersWhoseBeginWasCalled;
    
    bool isSignificantChange;
    KnobI* lastKnobChanged;
    
    mutable QMutex isLoadingProjectMutex;
    bool isLoadingProject; //< true when the project is loading
    
    mutable QMutex isSavingProjectMutex;
    bool isSavingProject; //< true when the project is saving
    
    ProjectPrivate(Natron::Project* project);
    
    void restoreFromSerialization(const ProjectSerialization& obj);
    
    bool findFormat(int index,Format* format) const;
        
};
    


}

#endif // PROJECTPRIVATE_H
