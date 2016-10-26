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


#ifndef NATRON_GUI_ANIMATIONMODULEBASE_H
#define NATRON_GUI_ANIMATIONMODULEBASE_H


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"
#include "Gui/AnimItemBase.h"



#define kReaderParamNameFirstFrame "firstFrame"
#define kReaderParamNameLastFrame "lastFrame"
#define kReaderParamNameStartingTime "startingTime"
#define kReaderParamNameTimeOffset "timeOffset"

#define kRetimeParamNameSpeed "speed"
#define kFrameRangeParamNameFrameRange "frameRange"
#define kTimeOffsetParamNameTimeOffset "timeOffset"

NATRON_NAMESPACE_ENTER;

class AnimationModuleSelectionProvider
{
public:

    /**
     * @brief Given the treeItem, finds in the model the corresponding item.
     * @param type[out] Returns the type of the given item. Only one of isKnob, isTableItem
     * or isNodeItem is valid at once and the type gives an hint on which one is valid.
     * @param view[out] The view corresponding to the treeItem
     * @param dimension[out] The dimension corresponding to the treeItem
     * @return True if the item could be found, false otherwise
     **/
    virtual bool findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem, NodeAnimPtr* isNodeItem, ViewSetSpec* view, DimSpec* dimension) const = 0;

    /**
     * @brief Return a list of all top-level items in the model
     **/
    virtual void getTopLevelItems(std::vector<KnobAnimPtr>* knobs, std::vector<NodeAnimPtr >* nodes, std::vector<TableItemAnimPtr>* tableItems) const = 0;

    /**
     * @brief Return a pointer to the selection model
     **/
    virtual AnimationModuleSelectionModelPtr getSelectionModel() const = 0;
};

struct AnimationModuleBasePrivate;
class AnimationModuleBase : public AnimationModuleSelectionProvider, public boost::enable_shared_from_this<AnimationModuleBase>
{
    Q_DECLARE_TR_FUNCTIONS(AnimationModuleBase);

public:

    AnimationModuleBase();

    virtual ~AnimationModuleBase();

    /**
     * @brief Push an undo redo to the stack
     **/
    virtual void pushUndoCommand(QUndoCommand *cmd) = 0;

    /**
     * @brief Get the timeline if it has one
     **/
    virtual TimeLinePtr getTimeline() const = 0;

    /**
     * @brief Refresh selected keyframes bbox from the selection
     **/
    virtual void refreshSelectionBboxAndUpdateView() = 0;

    /**
     * @brief Return a pointer to the curve widget if there's one
     **/
    virtual CurveWidget* getCurveWidget() const = 0;

    /**
     * @brief Set the selection
     **/
    void setCurrentSelection(const AnimItemDimViewKeyFramesMap &keys,
                             const std::vector<TableItemAnimPtr>& selectedTableItems,
                             const std::vector<NodeAnimPtr >& rangeNodes);

    /**
     * @brief Remove selected keyframes
     **/
    void deleteSelectedKeyframes();

    /**
     * @brief Move selected items
     **/
    void moveSelectedKeysAndNodes(double dt, double dv);

    /**
     * @brief Trim reader frame range on left
     **/
    void trimReaderLeft(const NodeAnimPtr &reader, double newFirstFrame);

    /**
     * @brief Trim reader frame range on right
     **/
    void trimReaderRight(const NodeAnimPtr &reader, double newLastFrame);

    /**
     * @brief Check if slipReader() will do anything
     **/
    bool canSlipReader(const NodeAnimPtr &reader) const;

    /**
     * @brief Slip the reader frame range on its original frame range
     **/
    void slipReader(const NodeAnimPtr &reader, double dt);

    /**
     * @brief Copy the current keyframe selection to the internal clipboard
     **/
    void copySelectedKeys();

    /**
     * @brief Get the keyframes clipboard content
     **/
    const AnimItemDimViewKeyFramesMap& getKeyFramesClipBoard() const;

    /**
     * @brief Paste given keyframes onto selected animation items.
     * Only a single item must be selected.
     * @param relative True if we should paste relative to the current time.
     **/
    void pasteKeys(const AnimItemDimViewKeyFramesMap& keys, bool relative);

    /**
     * @brief Set selected keyframes interpolation
     **/
    void setSelectedKeysInterpolation(KeyframeTypeEnum keyType);

    /**
     * @brief Transform selected keys by transform
     **/
    void transformSelectedKeys(const Transform::Matrix3x3& transform);


private:
    
    boost::scoped_ptr<AnimationModuleBasePrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_GUI_ANIMATIONMODULEBASE_H
