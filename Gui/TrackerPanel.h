/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef TRACKERPANEL_H
#define TRACKERPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <set>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief This is the new tracker panel, the previous version TrackerPanelV1 (used for TrackerPM) can be found in MultiInstancePanel.h
 **/
struct TrackerPanelPrivate;
class TrackerPanel
    : public QWidget
{
    Q_OBJECT

public:

    TrackerPanel(const NodeGuiPtr& n,
                 NodeSettingsPanel* parent);

    virtual ~TrackerPanel();

    void addTableRow(const TrackMarkerPtr & marker);

    void insertTableRow(const TrackMarkerPtr & marker, int index);

    void removeRow(int row);

    void removeMarker(const TrackMarkerPtr & marker);

    TableItem* getItemAt(int row, int column) const;

    int getMarkerRow(const TrackMarkerPtr & marker) const;

    TableItem* getItemAt(const TrackMarkerPtr & marker, int column) const;

    TrackMarkerPtr getRowMarker(int row) const;

    KnobIPtr getKnobAt(int row, int column, int* dimension) const;
    TrackerContextPtr getContext() const;

    NodeGuiPtr getNode() const;

    void getSelectedRows(std::set<int>* rows) const;

    void blockSelection();
    void unblockSelection();

    void pushUndoCommand(QUndoCommand* command);

    void clearAndSelectTracks(const std::list<TrackMarkerPtr>& markers, int reason);

public Q_SLOTS:

    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void onSelectAllButtonClicked();
    void onResetButtonClicked();
    void onAverageButtonClicked();

    void onGoToPrevKeyframeButtonClicked();
    void onGoToNextKeyframeButtonClicked();
    void onAddKeyframeButtonClicked();
    void onRemoveKeyframeButtonClicked();
    void onRemoveAnimationButtonClicked();

    void onModelSelectionChanged(const QItemSelection & oldSelection, const QItemSelection & newSelection);
    void onItemDataChanged(TableItem* item);
    void onItemEnabledCheckBoxChecked(bool checked);
    void onItemMotionModelChanged(int index);
    void onItemRightClicked(TableItem* item);

    void onContextSelectionAboutToChange(int reason);
    void onContextSelectionChanged(int reason);

    void onTrackKeyframeSet(const TrackMarkerPtr& marker, int key);
    void onTrackKeyframeRemoved(const TrackMarkerPtr& marker, int key);
    void onTrackAllKeyframesRemoved(const TrackMarkerPtr& marker);

    void onKeyframeSetOnTrackCenter(const TrackMarkerPtr &marker, int key);
    void onKeyframeRemovedOnTrackCenter(const TrackMarkerPtr &marker, int key);
    void onMultipleKeysRemovedOnTrackCenter(const TrackMarkerPtr& marker, const std::list<double>& keys);
    void onAllKeyframesRemovedOnTrackCenter(const TrackMarkerPtr &marker);
    void onMultipleKeyframesSetOnTrackCenter(const TrackMarkerPtr &marker, const std::list<double>& keys);

    void onSettingsPanelClosed(bool closed);
    void onTrackAboutToClone(const TrackMarkerPtr& marker);
    void onTrackCloned(const TrackMarkerPtr& marker);

    void onTrackInserted(const TrackMarkerPtr& marker, int index);
    void onTrackRemoved(const TrackMarkerPtr& marker);

    void onEnabledChanged(const TrackMarkerPtr& marker, int reason);

    void onCenterKnobValueChanged(const TrackMarkerPtr& marker, int, int);
    void onOffsetKnobValueChanged(const TrackMarkerPtr& marker, int, int);
    void onErrorKnobValueChanged(const TrackMarkerPtr &marker, int, int);
    void onMotionModelKnobValueChanged(const TrackMarkerPtr &marker, int, int);

    /*void onPatternTopLeftKnobValueChanged(const TrackMarkerPtr &marker,int,int);
       void onPatternTopRightKnobValueChanged(const TrackMarkerPtr &marker,int,int);
       void onPatternBtmRightKnobValueChanged(const TrackMarkerPtr &marker,int,int);
       void onPatternBtmLeftKnobValueChanged(const TrackMarkerPtr &marker,int,int);

       void onSearchBtmLeftKnobValueChanged(const TrackMarkerPtr &marker,int,int);
       void onSearchTopRightKnobValueChanged(const TrackMarkerPtr &marker,int,int);*/


    void onTimeChanged(SequenceTime time, int reason);

private:

    void onSelectionAboutToChangeInternal(const std::list<TrackMarkerPtr>& markers);
    void selectInternal(const std::list<TrackMarkerPtr>& markers, int reason);
    TrackMarkerPtr makeTrackInternal();

    std::unique_ptr<TrackerPanelPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // TRACKERPANEL_H
