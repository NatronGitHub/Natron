//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_PROJECT_H
#define NATRON_PROJECT_H

#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <QMutex>
#include <QString>
#include <QDateTime>

#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/Format.h"

class TimeLine;
class AppInstance;

namespace Natron{
class Node;

class Project :  public KnobHolder {
    
    
    QString _projectName;
    QString _projectPath;
    QString _lastAutoSaveFilePath;
    bool _hasProjectBeenSavedByUser;
    QDateTime _ageSinceLastSave;
    QDateTime _lastAutoSave;
    ComboBox_Knob* _formatKnob;
    Button_Knob* _addFormatKnob;
    Int_Knob* _viewsCount;
    boost::shared_ptr<TimeLine> _timeline; // global timeline
    
    std::map<std::string,int> _nodeCounters;
    bool _autoSetProjectFormat;
    mutable QMutex _projectDataLock;
    std::vector<Node*> _currentNodes;
    
    std::vector<Format> _availableFormats;
    
    int _knobsAge; //< the age of the knobs in the app. This is updated on each value changed.
    
public:
    
    Project(AppInstance* appInstance);
    
    virtual ~Project();
    
    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE;
    
    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    virtual void evaluate(Knob* knob,bool isSignificant) OVERRIDE;
    
    const std::vector<Node*>& getCurrentNodes() const{return _currentNodes;}
    
    const QString& getProjectName() const WARN_UNUSED_RETURN {return _projectName;}
    
    void setProjectName(const QString& name) {_projectName = name;}
    
    const QString& getLastAutoSaveFilePath() const {return _lastAutoSaveFilePath;}
    
    const QString& getProjectPath() const WARN_UNUSED_RETURN {return _projectPath;}
    
    void setProjectPath(const QString& path) {_projectPath = path;}
    
    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN {return _hasProjectBeenSavedByUser;}
    
    void setHasProjectBeenSavedByUser(bool s) {_hasProjectBeenSavedByUser = s;}
    
    const QDateTime& projectAgeSinceLastSave() const WARN_UNUSED_RETURN {return _ageSinceLastSave;}
    
    void setProjectAgeSinceLastSave(const QDateTime& t) {_ageSinceLastSave = t;}
    
    const QDateTime& projectAgeSinceLastAutosave() const WARN_UNUSED_RETURN {return _lastAutoSave;}
    
    void setProjectAgeSinceLastAutosaveSave(const QDateTime& t) {_lastAutoSave = t;}
    
    const Format& getProjectDefaultFormat() const WARN_UNUSED_RETURN ;
    
    int getProjectViewsCount() const;
    
    void setProjectDefaultFormat(const Format& f);
    
    /*Returns the index of the format*/
    int tryAddProjectFormat(const Format& f);
    
    bool shouldAutoSetProjectFormat() const {return _autoSetProjectFormat;}
    
    void setAutoSetProjectFormat(bool b){_autoSetProjectFormat = b;}
    
    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN {return _timeline;}
    
    // TimeLine operations (to avoid duplicating the shared_ptr when possible)
    void setFrameRange(int first, int last);
    
    void seekFrame(int frame);
    
    void incrementCurrentFrame();
    
    void decrementCurrentFrame();
    
    int currentFrame() const WARN_UNUSED_RETURN;
    
    int firstFrame() const WARN_UNUSED_RETURN;
    
    int lastFrame() const WARN_UNUSED_RETURN;
    
    void initNodeCountersAndSetName(Node* n);
    
    void clearNodes();
    
    void loadProject(const QString& path,const QString& name,bool background);
    
    void saveProject(const QString& path,const QString& filename,bool autoSave = false);
    
    void lock() const {_projectDataLock.lock();}
    
    void unlock() const { assert(!_projectDataLock.tryLock());_projectDataLock.unlock();}

    void createNewFormat();
    
    void incrementKnobsAge() {
        if(_knobsAge < 99999)
            ++_knobsAge;
        else
            _knobsAge = 0;
    }
    
    int getKnobsAge() const {return _knobsAge;}
    
};

} // Natron

#endif // NATRON_PROJECT_H
