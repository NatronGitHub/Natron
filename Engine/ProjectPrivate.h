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

#include <QDateTime>
#include <QString>


#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobSerialization.h"

class TimeLine;
class NodeSerialization;
class ProjectSerialization;
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
    
    struct KnobClipBoard {
        KnobSerialization k; //< the serialized knob to copy
        bool isEmpty; //< is the clipboard empty
        bool copyAnimation; //< should we copy all the animation or not
    };

struct ProjectPrivate{
    QString projectName;
    QString projectPath;
    QString lastAutoSaveFilePath;
    bool hasProjectBeenSavedByUser;
    QDateTime ageSinceLastSave;
    QDateTime lastAutoSave;
    boost::shared_ptr<Choice_Knob> formatKnob;
    boost::shared_ptr<Button_Knob> addFormatKnob;
    boost::shared_ptr<Int_Knob> viewsCount;
    boost::shared_ptr<TimeLine> timeline; // global timeline
    
    std::map<std::string,int> nodeCounters;
    bool autoSetProjectFormat;
    std::vector<Natron::Node*> currentNodes;
    
    std::vector<Format> availableFormats;
    
    Natron::Project* project;
    
    int _knobsAge; //< the age of the knobs in the app. This is updated on each value changed.
    Natron::OutputEffectInstance* lastTimelineSeekCaller;

    int beginEndBracketsCount;
    int evaluationsCount;
    std::map<KnobHolder*,int> holdersWhoseBeginWasCalled;
    bool isSignificantChange;
    Knob* lastKnobChanged;
    KnobClipBoard knobClipBoard;
    
    ProjectPrivate(Natron::Project* project);
    
    void restoreFromSerialization(const ProjectSerialization& obj);
};
    


}

#endif // PROJECTPRIVATE_H
