/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

#include "Gui/AnimationModuleBase.h"

#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"



NATRON_NAMESPACE_ENTER




class AnimationModulePrivate;
class AnimationModule
    : public QObject
    , public AnimationModuleBase
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

    struct MakeSharedEnabler;

    AnimationModule(Gui *gui,
                    AnimationModuleEditor* editor,
                    const TimeLinePtr &timeline);

public:
    static AnimationModulePtr create(Gui *gui,
                                     AnimationModuleEditor* editor,
                                     const TimeLinePtr &timeline);

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


    /// Overriden from AnimationModuleSelectionProvider
    virtual bool findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem, NodeAnimPtr* isNodeItem, ViewSetSpec* view, DimSpec* dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getTopLevelNodes(bool onlyVisible, std::vector<NodeAnimPtr>* nodes) const OVERRIDE FINAL;
    virtual AnimationModuleSelectionModelPtr getSelectionModel() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// Overriden from AnimationModuleBase
    virtual void pushUndoCommand(QUndoCommand *cmd) OVERRIDE FINAL;
    virtual TimeLinePtr getTimeline() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void refreshSelectionBboxAndUpdateView() OVERRIDE FINAL;
    virtual AnimationModuleView* getView() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isCurveVisible(const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getTreeColumnsCount() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    NodeAnimPtr findNodeAnim(const NodePtr& node) const;
    NodeAnimPtr findNodeAnim(const KnobIPtr &knob) const;
    KnobAnimPtr findKnobAnim(const KnobGuiConstPtr& knobGui) const;
    TableItemAnimPtr findTableItemAnim(const KnobTableItemPtr& item) const;
    /**
     * @brief If the given node is in a group, return the parent group anim
     **/
    NodeAnimPtr getGroupNodeAnim(const NodeAnimPtr& node) const;

    /**
     * @brief Given the node, finds what nodes should be inserted as children of this node
     **/
    std::vector<NodeAnimPtr> getChildrenNodes(const NodeAnimPtr& node) const;

    NodeAnimPtr getNearestTimeNodeFromOutputsInternal(const NodePtr& node) const;
    NodeAnimPtr getNearestTimeNodeFromOutputs(const NodeAnimPtr& node) const;

    NodeAnimPtr getNearestReaderInternal(const NodePtr& timeNode) const;
    NodeAnimPtr getNearestReader(const NodeAnimPtr& timeNode) const;
    AnimationModuleEditor* getEditor() const;

  

    void emit_mustRedrawView() {
        Q_EMIT mustRedrawView();
    }

    void renameSelectedNode();

    boost::shared_ptr<QUndoStack> getUndoStack() const;

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

inline AnimationModulePtr toAnimationModule(const AnimationModuleBasePtr& p)
{
    return boost::dynamic_pointer_cast<AnimationModule>(p);
}


NATRON_NAMESPACE_EXIT


#endif // NATRON_GUI_ANIMATIONMODULE_H
