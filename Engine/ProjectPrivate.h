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

#include <QDateTime>
#include <QMutex>
#include <QString>

#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFactory.h"

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

struct ProjectPrivate{
    QString _projectName;
    QString _projectPath;
    QString _lastAutoSaveFilePath;
    bool _hasProjectBeenSavedByUser;
    QDateTime _ageSinceLastSave;
    QDateTime _lastAutoSave;
    Choice_Knob* _formatKnob;
    Button_Knob* _addFormatKnob;
    Int_Knob* _viewsCount;
    boost::shared_ptr<TimeLine> _timeline; // global timeline
    
    std::map<std::string,int> _nodeCounters;
    bool _autoSetProjectFormat;
    mutable QMutex _projectDataLock;
    std::vector<Natron::Node*> _currentNodes;
    
    std::vector<Format> _availableFormats;
    
    Natron::Project* _project;
    
    int _knobsAge; //< the age of the knobs in the app. This is updated on each value changed.
    Natron::OutputEffectInstance* _lastTimelineSeekCaller;
    
    ProjectPrivate(Natron::Project* project);
    
    void restoreFromSerialization(const ProjectSerialization& obj);
};
    


}

#endif // PROJECTPRIVATE_H
