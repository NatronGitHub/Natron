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

#ifndef MULTIINSTANCEPANEL_H
#define MULTIINSTANCEPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QThread>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Knob.h"
#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief This class represents a multi-instance settings panel.
 **/
struct MultiInstancePanelPrivate;
class MultiInstancePanel
    :  public NamedKnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    MultiInstancePanel(const NodeGuiPtr & node);

    virtual ~MultiInstancePanel();

    void createMultiInstanceGui(QVBoxLayout* layout);

    bool isGuiCreated() const;

    void addRow(const NodePtr & node);

    void removeRow(int index);

    int getNodeIndex(const NodePtr & node) const;

    const std::list< std::pair<NodeWPtr,bool > > & getInstances() const;
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL;
    NodePtr getMainInstance() const;
    
    NodeGuiPtr getMainInstanceGui() const;

    void getSelectedInstances(std::list<Node*>* instances) const;

    void resetAllInstances();

    KnobPtr getKnobForItem(TableItem* item,int* dimension) const;
    Gui* getGui() const;
    virtual void setIconForButton(KnobButton* /*knob*/)
    {
    }

    NodePtr createNewInstance(bool useUndoRedoStack);

    void selectNode(const NodePtr & node,bool addToSelection);

    void selectNodes(const std::list<Node*> & nodes,bool addToSelection);

    void removeNodeFromSelection(const NodePtr & node);

    void clearSelection();

    bool isSettingsPanelVisible() const;
        
    void removeInstances(const NodesList& instances);
    void addInstances(const NodesList& instances);

    void onChildCreated(const NodePtr& node);
    
    void setRedrawOnSelectionChanged(bool redraw);
    
public Q_SLOTS:

    void onAddButtonClicked();

    void onRemoveButtonClicked();

    void onSelectAllButtonClicked();

    void onSelectionChanged(const QItemSelection & oldSelection,const QItemSelection & newSelection);

    void onItemDataChanged(TableItem* item);

    void onCheckBoxChecked(bool checked);

    void onDeleteKeyPressed();

    void onInstanceKnobValueChanged(const ViewIdx& view,int dim,int reason);

    void resetSelectedInstances();

    void onSettingsPanelClosed(bool closed);

    void onItemRightClicked(TableItem* item);

protected:

    virtual void appendExtraGui(QVBoxLayout* /*layout*/)
    {
    }

    virtual void appendButtons(QHBoxLayout* /*buttonLayout*/)
    {
    }

    NodePtr addInstanceInternal(bool useUndoRedoStack);
    virtual void initializeExtraKnobs()
    {
    }

    virtual void showMenuForInstance(Node* /*instance*/)
    {
    }

private:

    virtual void onButtonTriggered(KnobButton* button);

    void resetInstances(const std::list<Node*> & instances);

    void removeInstancesInternal();

    virtual void evaluate(KnobI* knob,bool isSignificant,ValueChangedReasonEnum reason) OVERRIDE;
    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual void onKnobValueChanged(KnobI* k,ValueChangedReasonEnum reason,double time, const ViewIdx& view,
                                    bool originatedFromMainThread) OVERRIDE;
    boost::scoped_ptr<MultiInstancePanelPrivate> _imp;
};

struct TrackerPanelPrivate;
class TrackerPanel
    : public MultiInstancePanel
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    TrackerPanel(const NodeGuiPtr & node);

    virtual ~TrackerPanel();

    ///Each function below returns true if there is a selection, false otherwise
    bool trackBackward();
    bool trackForward();
    bool trackPrevious();
    bool trackNext();
    void stopTracking();

    void clearAllAnimationForSelection();

    void clearBackwardAnimationForSelection();

    void clearForwardAnimationForSelection();

    void setUpdateViewerOnTracking(bool update);

    bool isUpdateViewerOnTrackingEnabled() const;
public Q_SLOTS:

    void onAverageTracksButtonClicked();
    void onExportButtonClicked();

    void onTrackingStarted();
    
    void onTrackingFinished();
    
    void onTrackingProgressUpdate(double progress);
    
Q_SIGNALS:
    
    void trackingEnded();
    
private:

    virtual void initializeExtraKnobs() OVERRIDE FINAL;
    virtual void appendExtraGui(QVBoxLayout* layout) OVERRIDE FINAL;
    virtual void appendButtons(QHBoxLayout* buttonLayout) OVERRIDE FINAL;
    virtual void setIconForButton(KnobButton* knob) OVERRIDE FINAL;
    virtual void onButtonTriggered(KnobButton* button) OVERRIDE FINAL;
    virtual void showMenuForInstance(Node* item) OVERRIDE FINAL;

    boost::scoped_ptr<TrackerPanelPrivate> _imp;
};

struct TrackSchedulerPrivate;
class TrackScheduler : public QThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    TrackScheduler(const TrackerPanel* panel);
    
    virtual ~TrackScheduler();
    
    /**
     * @brief Track the selectedInstances, calling the instance change action on each button (either the previous or
     * next button) in a separate thread. 
     * @param start the first frame to track, if forward is true then start < end
     * @param end the next frame after the last frame to track (a la STL iterators), if forward is true then end > start
     **/
    void track(int start,int end,bool forward,const std::list<KnobButton*> & selectedInstances);
    
    void abortTracking();
    
    void quitThread();
    
    bool isWorking() const;
    
Q_SIGNALS:
    
    void trackingStarted();
    
    void trackingFinished();
    
    void progressUpdate(double progress);

private:
    
    virtual void run() OVERRIDE FINAL;
    
    boost::scoped_ptr<TrackSchedulerPrivate> _imp;
    
};

NATRON_NAMESPACE_EXIT;

#endif // MULTIINSTANCEPANEL_H
