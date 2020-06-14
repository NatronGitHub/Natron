/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include <QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"
#include "Gui/AnimItemBase.h"



#define kReaderParamNameStartingTime "startingTime"
#define kReaderParamNameTimeOffset "timeOffset"

#define kRetimeParamNameSpeed "speed"
#define kFrameRangeParamNameFrameRange "frameRange"
#define kTimeOffsetParamNameTimeOffset "timeOffset"

NATRON_NAMESPACE_ENTER

// typedefs

const int QT_ROLE_CONTEXT_TYPE = Qt::UserRole + 1;

// Stores a value corresponding to a dimension index in a Knob
const int QT_ROLE_CONTEXT_DIM = Qt::UserRole + 2;

// Stores a value corresponding to a view index in a Knob
const int QT_ROLE_CONTEXT_VIEW = Qt::UserRole + 3;

// Stores a value indicating whether an item is animated
const int QT_ROLE_CONTEXT_IS_ANIMATED = Qt::UserRole + 4;

// Stores a pointer to the corresponding model item for fast access
const int QT_ROLE_CONTEXT_ITEM_POINTER = Qt::UserRole + 5;

// Stores a boolean indicating whether the visible icon is chechked
const int QT_ROLE_CONTEXT_ITEM_VISIBLE = Qt::UserRole + 6;

class AnimationModuleSelectionProvider
{
public:

    AnimationModuleSelectionProvider() {}

    virtual ~AnimationModuleSelectionProvider() {}

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
    virtual void getTopLevelKnobs(bool /*onlyVisible*/, std::vector<KnobAnimPtr>* knobs) const
    {
        Q_UNUSED(knobs);
    }

    virtual void getTopLevelNodes(bool /*onlyVisible*/, std::vector<NodeAnimPtr>* nodes) const
    {
        Q_UNUSED(nodes);
    }

    virtual void getTopLevelTableItems(bool /*onlyVisible*/, std::vector<TableItemAnimPtr>* tableItems) const
    {
        Q_UNUSED(tableItems);
    }

    /**
     * @brief Return a pointer to the selection model
     **/
    virtual AnimationModuleSelectionModelPtr getSelectionModel() const = 0;


    /**
     * @brief Should determine whether the curve should be drawn for the given item or not
     **/
    virtual bool isCurveVisible(const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view) const = 0;

    /**
     * @brief Return true if at least of of the curves in the given dimension/view is visible
     **/
    bool isCurveVisible(const AnimItemBasePtr& item, DimSpec dimension, ViewSetSpec view) const;

};

struct AnimationModuleBasePrivate;
class AnimationModuleBase : public AnimationModuleSelectionProvider, public boost::enable_shared_from_this<AnimationModuleBase>
{
    Q_DECLARE_TR_FUNCTIONS(AnimationModuleBase)

public:

    AnimationModuleBase(Gui* gui);

    virtual ~AnimationModuleBase();

    /**
     * @brief Push an undo redo to the stack
     **/
    virtual void pushUndoCommand(QUndoCommand *cmd) = 0;
    void pushUndoCommand(const UndoCommandPtr& command);

    // Takes ownership, command is deleted when returning call
    void pushUndoCommand(UndoCommand* command);

    /**
     * @brief Get the timeline if it has one
     **/
    virtual TimeLinePtr getTimeline() const = 0;

    /**
     * @brief Refresh selected keyframes bbox from the selection
     **/
    virtual void refreshSelectionBboxAndUpdateView() = 0;

    /**
     * @brief Returns the number of columns used by the tree representing the items
     **/
    virtual int getTreeColumnsCount() const = 0;

    /**
     * @brief Return a pointer to the view
     **/
    virtual AnimationModuleView* getView() const = 0;

    /**
     * @brief Set the selection
     **/
    void setCurrentSelection(const AnimItemDimViewKeyFramesMap &keys,
                             const std::vector<TableItemAnimPtr>& selectedTableItems,
                             const std::vector<NodeAnimPtr>& rangeNodes);

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

    /**
     * @brief Helper function to get the visibility icon for an item
     **/
    QIcon getItemVisibilityIcon(bool visible);

    /**
     * @brief Notify that the whole model is about to tear-down. As a result of it, any item
     * in the model should not delete their tree items.
     **/
    void setAboutToBeDestroyed(bool d);

    /**
     * @brief Returns the whole model is about to tear-down. As a result of it, any item
     * in the model should not delete their tree items.
     **/
    bool isAboutToBeDestroyed() const;


private:
    
    boost::scoped_ptr<AnimationModuleBasePrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_ANIMATIONMODULEBASE_H
