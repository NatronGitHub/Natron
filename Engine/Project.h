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

#include <QtCore/QObject>

#include "Global/GlobalDefines.h"
#include "Engine/Knob.h"


class QString;
class QDateTime;
class Format;
class TimeLine;
class AppInstance;

namespace Natron{
class Node;
class OutputEffectInstance;
struct ProjectPrivate;
class Project : public QObject,  public KnobHolder {
    
    Q_OBJECT
    
    
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
    
    const std::vector<Node*>& getCurrentNodes() const;
    
    const QString& getProjectName() const WARN_UNUSED_RETURN ;
    
    void setProjectName(const QString& name) ;
    
    const QString& getLastAutoSaveFilePath() const;
    
    const QString& getProjectPath() const WARN_UNUSED_RETURN;
    
    void setProjectPath(const QString& path);
    
    bool hasProjectBeenSavedByUser() const WARN_UNUSED_RETURN;
    
    void setHasProjectBeenSavedByUser(bool s) ;
    
    const QDateTime& projectAgeSinceLastSave() const WARN_UNUSED_RETURN ;
    
    void setProjectAgeSinceLastSave(const QDateTime& t);
    
    const QDateTime& projectAgeSinceLastAutosave() const WARN_UNUSED_RETURN;
    
    void setProjectAgeSinceLastAutosaveSave(const QDateTime& t) ;
    
    const Format& getProjectDefaultFormat() const WARN_UNUSED_RETURN ;
    
    int getProjectViewsCount() const;
    
    void setProjectDefaultFormat(const Format& f);
    
    /*Returns the index of the format*/
    int tryAddProjectFormat(const Format& f);
    
    bool shouldAutoSetProjectFormat() const ;
    
    void setAutoSetProjectFormat(bool b);
    
    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN;
    
    // TimeLine operations (to avoid duplicating the shared_ptr when possible)
    void setFrameRange(int first, int last);
    
    int currentFrame() const WARN_UNUSED_RETURN;
    
    int firstFrame() const WARN_UNUSED_RETURN;
    
    int lastFrame() const WARN_UNUSED_RETURN;
    
    void initNodeCountersAndSetName(Node* n);
    
    void clearNodes();
    
    void loadProject(const QString& path,const QString& name,bool background);
    
    void saveProject(const QString& path,const QString& filename,bool autoSave = false);
    
    void lock() const ;
    
    void unlock() const ;

    void createNewFormat();
    
    void incrementKnobsAge() ;
    
    int getKnobsAge() const;
    
    void setLastTimelineSeekCaller(Natron::OutputEffectInstance* output);

public slots:

    void onTimeChanged(SequenceTime time,int reason);

private:

    boost::scoped_ptr<ProjectPrivate> _imp;

};

} // Natron

#endif // NATRON_PROJECT_H
