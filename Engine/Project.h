//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_PROJECT_H_
#define NATRON_ENGINE_PROJECT_H_

#include <map>
#include <vector>

#include <boost/noncopyable.hpp>

#include <QtCore/QObject>

#include "Global/GlobalDefines.h"
#include "Engine/Knob.h"
#include "Engine/Format.h"
#include "Engine/TimeLine.h"

class QString;
class QDateTime;
class AppInstance;
class ProjectSerialization;
class KnobSerialization;
class ProjectGui;
class AddFormatDialog;
namespace Natron{
class Node;
class OutputEffectInstance;
struct ProjectPrivate;
class Project : public QObject,  public KnobHolder , public boost::noncopyable {
    
    Q_OBJECT
    
    ////////All the public functions in Project must be only referred to
    //////// by the AppInstance which is responsible for thread-safety of this class.
    friend class ::AppInstance;
    friend struct ProjectPrivate;
    friend class ::ProjectSerialization;
    friend class ::ProjectGui;
    friend class ::AddFormatDialog;
public:
    
    virtual ~Project();

private:
    
    Project(AppInstance* appInstance);
    
    
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
    
    void setProjectLastAutoSavePath(const QString& str);
    
    const QDateTime& projectAgeSinceLastAutosave() const WARN_UNUSED_RETURN;
    
    void setProjectAgeSinceLastAutosaveSave(const QDateTime& t) ;
    
    const Format& getProjectDefaultFormat() const WARN_UNUSED_RETURN ;
    
    const std::vector<Format>& getProjectFormats() const;
    
    int getProjectViewsCount() const;
    
    void setProjectDefaultFormat(const Format& f);
    
    /*Returns the index of the format*/
    int tryAddProjectFormat(const Format& f);
    
    bool shouldAutoSetProjectFormat() const ;
    
    void setAutoSetProjectFormat(bool b);
    
    bool isAutoPreviewEnabled() const;
    
    void toggleAutoPreview();
    
    boost::shared_ptr<TimeLine> getTimeLine() const WARN_UNUSED_RETURN;
    
    // TimeLine operations (to avoid duplicating the shared_ptr when possible)
    void setFrameRange(int first, int last);
    
    int currentFrame() const WARN_UNUSED_RETURN;
    
    int firstFrame() const WARN_UNUSED_RETURN;
    
    int lastFrame() const WARN_UNUSED_RETURN;
    
    void initNodeCountersAndSetName(Node* n);
    
    void clearNodes();
    
    void incrementKnobsAge() ;
    
    int getKnobsAge() const;
    
    friend void TimeLine::seekFrame(SequenceTime,Natron::OutputEffectInstance*);
    void setLastTimelineSeekCaller(Natron::OutputEffectInstance* output);

    void save(ProjectSerialization* serializationObject) const;
    
    void load(const ProjectSerialization& obj);
    

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReason reason) OVERRIDE FINAL;
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReason reason)  OVERRIDE FINAL;
    
    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(Knob* k,Natron::ValueChangedReason reason)  OVERRIDE FINAL;
    

    
    void beginProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller);

    void stackEvaluateRequest(Natron::ValueChangedReason reason, KnobHolder* caller, Knob *k, bool isSignificant);

    void endProjectWideValueChanges(KnobHolder* caller);

public slots:

    void onTimeChanged(SequenceTime time,int reason);

signals:
    
    void mustCreateFormat();
    
    void formatChanged(Format);
    
    void autoPreviewChanged(bool);
    
private:

    boost::scoped_ptr<ProjectPrivate> _imp;

};

} // Natron

#endif // NATRON_ENGINE_PROJECT_H_
