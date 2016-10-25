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

#ifndef NATRON_GUI_ANIMATIONMODULE_H
#define NATRON_GUI_ANIMATIONMODULE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

#include "Gui/AnimItemBase.h"
#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"


#define kReaderParamNameFirstFrame "firstFrame"
#define kReaderParamNameLastFrame "lastFrame"
#define kReaderParamNameStartingTime "startingTime"
#define kReaderParamNameTimeOffset "timeOffset"

#define kRetimeParamNameSpeed "speed"
#define kFrameRangeParamNameFrameRange "frameRange"
#define kTimeOffsetParamNameTimeOffset "timeOffset"


NATRON_NAMESPACE_ENTER;


// typedefs

const int QT_ROLE_CONTEXT_TYPE = Qt::UserRole + 1;

// Stores a value corresponding to a dimension index in a Knob
const int QT_ROLE_CONTEXT_DIM = Qt::UserRole + 2;

// Stores a value corresponding to a view index in a Knob
const int QT_ROLE_CONTEXT_VIEW = Qt::UserRole + 3;

// Stores a value indicating whether an item is animated
const int QT_ROLE_CONTEXT_IS_ANIMATED = Qt::UserRole + 4;


class AnimationModulePrivate;
class AnimationModule
    : public QObject
    , public boost::enable_shared_from_this<AnimationModule>
{
    Q_OBJECT


    AnimationModule(Gui *gui,
                    AnimationModuleEditor* editor,
                    const TimeLinePtr &timeline);

public:

    static AnimationModulePtr create(Gui *gui,
                                     AnimationModuleEditor* editor,
                                     const TimeLinePtr &timeline)
    {
        AnimationModulePtr ret(new AnimationModule(gui, editor, timeline));
        ret->ensureSelectionModel();
        return ret;
    }

    virtual ~AnimationModule();

    /**
     * @brief Returns whether the given node currently has an animation
     **/
    static bool isNodeAnimated(const NodeGuiPtr &nodeGui);

    /**
     * @brief Returns whether the given node can have an animation
     **/
    static bool getNodeCanAnimate(const NodePtr &node);

    /**
     * @brief Returns a list of all node anim object created in the module
     **/
    const std::list<NodeAnimPtr>& getNodes() const;

    /**
     * @brief Register a node in the animation module. If it can be animated and currently has an animation
     * it will show up in the AnimationModuleTreeView.
     **/
    void addNode(const NodeGuiPtr& nodeGui);

    /**
     * @brief Removes a node that was previously added with addNode()
     **/
    void removeNode(const NodeGuiPtr& node);

    /**
     * @brief Given the treeItem, finds in the model the corresponding item.
     * @param type[out] Returns the type of the given item. Only one of isKnob, isTableItem
     * or isNodeItem is valid at once and the type gives an hint on which one is valid.
     * @param view[out] The view corresponding to the treeItem
     * @param dimension[out] The dimension corresponding to the treeItem
     * @return True if the item could be found, false otherwise
     **/
    bool findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem, NodeAnimPtr* isNodeItem, ViewSetSpec* view, DimSpec* dimension) const;

    NodeAnimPtr findNodeAnim(const NodePtr& node) const;
    NodeAnimPtr findNodeAnim(const KnobIPtr &knob) const;
    KnobAnimPtr findKnobAnim(const KnobGuiConstPtr& knobGui) const;

    /**
     * @brief If the given node is in a group, return the parent group anim
     **/
    NodeAnimPtr getGroupNodeAnim(const NodeAnimPtr& node) const;

    /**
     * @brief Given the node, finds what nodes should be inserted as children of this node
     **/
    std::vector<NodeAnimPtr > getChildrenNodes(const NodeAnimPtr& node) const;


    NodeAnimPtr getNearestTimeNodeFromOutputs(const NodeAnimPtr& node) const;
    NodePtr getNearestReader(const NodeAnimPtr& timeNode) const;
    AnimationModuleSelectionModel& getSelectionModel() const;
    AnimationModuleEditor* getEditor() const;

    // User interaction
    void deleteSelectedKeyframes();

    void moveSelectedKeysAndNodes(double dt, double dv);
    void trimReaderLeft(const NodeAnimPtr &reader, double newFirstFrame);
    void trimReaderRight(const NodeAnimPtr &reader, double newLastFrame);

    bool canSlipReader(const NodeAnimPtr &reader) const;

    void slipReader(const NodeAnimPtr &reader, double dt);

    /**
     * @brief Copy the current keyframe selection to the internal clipboard
     **/
    void copySelectedKeys();

    /**
     * @brief Paste keyframes in the internal clipboard onto selected items
     **/
    void pasteKeys(bool relative);

    void pasteKeys(const AnimItemDimViewKeyFramesMap& keys, bool relative);

    void setSelectedKeysInterpolation(KeyframeTypeEnum keyType);

    void transformSelectedKeys(const Transform::Matrix3x3& transform);

    void emit_mustRedrawView() {
        Q_EMIT mustRedrawView();
    }

    // Other
    SequenceTime getCurrentFrame() const;

    void renameSelectedNode();

    QUndoStack* getUndoStack() const;

Q_SIGNALS:
    
    void mustRedrawView();
    void nodeAdded(const NodeAnimPtr& NodeAnim);
    void nodeAboutToBeRemoved(const NodeAnimPtr& NodeAnim);
    void nodeRemoved(const NodeAnimPtr& NodeAnim);


private Q_SLOTS:

    void onNodeNameEditDialogFinished();

private:

    void ensureSelectionModel();

    boost::scoped_ptr<AnimationModulePrivate> _imp;
};



NATRON_NAMESPACE_EXIT;


#endif // NATRON_GUI_ANIMATIONMODULE_H
