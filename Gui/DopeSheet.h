/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef DOPESHEET_H
#define DOPESHEET_H

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
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTreeWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"


#define kReaderParamNameFirstFrame "firstFrame"
#define kReaderParamNameLastFrame "lastFrame"
#define kReaderParamNameStartingTime "startingTime"
#define kReaderParamNameTimeOffset "timeOffset"

#define kRetimeParamNameSpeed "speed"
#define kFrameRangeParamNameFrameRange "frameRange"
#define kTimeOffsetParamNameTimeOffset "timeOffset"


NATRON_NAMESPACE_ENTER

class DopeSheetPrivate;
class DopeSheetSelectionModel;
class DopeSheetSelectionModelPrivate;
class DSKnobPrivate;
class DSNodePrivate;

class DSNode;
class DopeSheet;

typedef std::map<QTreeWidgetItem *, DSNodePtr> DSTreeItemNodeMap;
typedef std::map<QTreeWidgetItem *, DSKnobPtr> DSTreeItemKnobMap;
// typedefs

const int QT_ROLE_CONTEXT_TYPE = Qt::UserRole + 1;
const int QT_ROLE_CONTEXT_DIM = Qt::UserRole + 2;
const int QT_ROLE_CONTEXT_IS_ANIMATED = Qt::UserRole + 3;


bool nodeHasAnimation(const NodeGuiPtr &nodeGui);


/**
 * @brief The DSNode class describes a node context in the dope sheet.
 *
 * A node context is basically composed of a QTreeWidgetItem and a
 * NodeGui object.
 *
 * Use getTreeItem() to retrieve the tree widget item.
 *
 *
 * --- Retrieve node data ---
 *
 * A node context store a NodeGui object, which can be retrieved by using
 * getNodeGui().
 *
 * Use getInternalNode() to access to the Node object itself.
 *
 *
 * --- Child contexts ---
 *
 * According to the structure of a node, a DSNode stores a map composed of :
 * - a tree widget item named with a knob context text (knob name, dimension name).
 * - a DSKnob object (see below).
 *
 * Use getItemKnobMap() to get this map.
 *
 * The DSNode/DSKnob API is the dopesheet version of the Node/Knob organization.
 *
 *
 * -- Item types --
 *
 * Remember, the DopeSheetItemType enum describes each possible type for a
 * context.
 *
 * A DSNode stores this type because the dope sheet editor treats some types
 * of node differently. A DSNode can be a node exclusively composed of the
 * keyframes of its knobs. Another can be a reader node, which must be
 * drawn in a different way in the editor.
 *
 * Use getItemType() to retrieve this context type. Use isTimeNode() and
 * isRangeDrawingEnabled() to have more detailed infos about it.
 *
 * If you created a new node context type, please modify
 * isRangeDrawingEnabled() and canContainOtherNodeContexts() to handle it.
 *
 *
 * /!\ A better conception should remove the QTreeWidgetItem attribute
 * from this class.
 */
class DSNode
{
public:
    DSNode(DopeSheet *model,
           DopeSheetItemType itemType,
           const NodeGuiPtr &nodeGui,
           QTreeWidgetItem *nameItem);
    ~DSNode();

    QTreeWidgetItem * getTreeItem() const;

    NodeGuiPtr getNodeGui() const;
    NodePtr getInternalNode() const;

    const DSTreeItemKnobMap& getItemKnobMap() const;

    DopeSheetItemType getItemType() const;

    bool isTimeNode() const;

    bool isRangeDrawingEnabled() const;

    bool canContainOtherNodeContexts() const;
    bool containsNodeContext() const;

private:
    boost::scoped_ptr<DSNodePrivate> _imp;
};


/**
 * @brief The DSKnob class describes a knob context in the dope sheet.
 *
 * A knob context is basically composed of a QTreeWidgetItem and a
 * KnobGui object.
 *
 *
 * --- Simple and multidimensional knobs ---
 *
 * How the dope sheet treats the knobs of a node ?
 *
 * The main rule is : one knob context per dimension.
 *
 * According to Natron, a knob can have more than one dimension. In this case,
 * a DSKnob is created for each dimension and for the knob name.
 *
 * Example :
 * A knob named "Color" has three dimensions : r, g, b. The dope sheet will
 * treat it as :
 *
 * Color
 * - r
 * - g
 * - b
 *
 * So four contexts.
 *
 *
 * --- Dealing with dimensions ---
 *
 * A knob context also stores the dimension of the associated knob :
 * - 0 if the knob has one dimension.
 * - (N - X) if the knob has N dimensions. X refers to the current dimension.
 * - (-1) if the knob context is the root context of a multidim knob.
 *
 * getDimension() returns the dimension of the context.
 * isMuldiDimRoot() is a convenience function to check if the context is the
 * root of a multidim knob.
 *
 *
 * --- The QTreeWidgetItem-related API  ---
 *
 * Use getTreeItem() to retrieve the tree widget item.
 *
 * If 'this' is a knob root context and you want to obtain a pointer on the
 * tree widget item associated with a X dimension, use findDimTreeItem().
 *
 *
 * --- Retrieve knob data ---
 *
 * A knob context store a Knob object and the associated KnobGui object.
 *
 * Use getKnobGui() and getInternalKnob() to access to them.
 *
 *
 * /!\ A better conception should remove the QTreeWidgetItem attribute
 * from this class.
 */
class DSKnob
{
public:
    DSKnob(int dimension,
           QTreeWidgetItem *nameItem,
           const KnobGuiPtr& knobGui);
    ~DSKnob();

    QTreeWidgetItem * getTreeItem() const;
    QTreeWidgetItem * findDimTreeItem(int dimension) const;

    KnobGuiPtr getKnobGui() const;
    KnobIPtr getInternalKnob() const;

    bool isMultiDimRoot() const;
    int getDimension() const;

private:
    boost::scoped_ptr<DSKnobPrivate> _imp;
};

/**
 * @brief The DopeSheetKey struct describes a keyframe in a knob context.
 *
 * It is and should be used only to handle keyframe selections.
 */
class DopeSheetKey
{
public:
    DopeSheetKey(const DSKnobPtr &knob,
                 const KeyFrame& kf)
        : context(knob)
        , key(kf)
    {
        DSKnobPtr knobContext = context.lock();

        assert(knobContext);
    }

    DopeSheetKey(const DopeSheetKey &other)
        : context(other.context)
        , key(other.key)
    {}

    friend bool operator==(const DopeSheetKey &key,
                           const DopeSheetKey &other)
    {
        DSKnobPtr knobContext = key.context.lock();
        DSKnobPtr otherKnobContext = other.context.lock();

        if (!knobContext || !otherKnobContext) {
            return false;
        }

        if (knobContext != otherKnobContext) {
            return false;
        }

        if ( key.key.getTime() != other.key.getTime() ) {
            return false;
        }

        if ( knobContext->getTreeItem() != otherKnobContext->getTreeItem() ) {
            return false;
        }

        if ( knobContext->getDimension() != otherKnobContext->getDimension() ) {
            return false;
        }

        return true;
    }

    DSKnobPtr getContext() const
    {
        DSKnobPtr ret = context.lock();

        assert(ret);

        return ret;
    }

    DSKnobWPtr context;
    KeyFrame key;
};

typedef DopeSheetKeyPtr DopeSheetKeyPtr;
typedef std::list<DopeSheetKeyPtr> DopeSheetKeyPtrList;

/**
 * @brief The DopeSheet class is the model part of the dope sheet editor.
 *
 * A DopeSheet is basically a bundle of "contexts". A context is a instance
 * of a class composed with a QTreeWidgetItem and a Natron data. Currently,
 * the class handles two types of contexts :
 *
 * - Node context : described by the DSNode class.
 * - Knob context : described by the DSKnob class.
 *
 * It provides several functions to find contexts and some of their datas.
 *
 *
 * --- User interactions ---
 *
 * The model handles each type of user interaction (that modify the model,
 * of course). For example, a specific function move the selected keyframes,
 * another move a reader...
 *
 * If you want to create a custom undoable modification, you must add a public
 * function that push a QUndoCommand-derived instance on the model's undo stack.
 */
class DopeSheet
    : public QObject
{
    Q_OBJECT

public:


    DopeSheet(Gui *gui,
              DopeSheetEditor* editor,
              const TimeLinePtr &timeline);
    ~DopeSheet();

    // Model specific
    DSTreeItemNodeMap getItemNodeMap() const;

    void addNode(NodeGuiPtr nodeGui);
    void removeNode(NodeGui *node);

    DSNodePtr mapNameItemToDSNode(QTreeWidgetItem *nodeTreeItem) const;
    DSKnobPtr mapNameItemToDSKnob(QTreeWidgetItem *knobTreeItem) const;
    DSNodePtr findParentDSNode(QTreeWidgetItem *treeItem) const;
    DSNodePtr findDSNode(Node *node) const;
    DSNodePtr findDSNode(const KnobIPtr &knob) const;
    DSKnobPtr findDSKnob(const KnobGui* knobGui) const;

    bool isPartOfGroup(DSNode *dsNode) const;
    DSNodePtr getGroupDSNode(DSNode *dsNode) const;
    std::vector<DSNodePtr> getImportantNodes(DSNode *dsNode) const;
    DSNodePtr getNearestTimeNodeFromOutputs(DSNode *dsNode) const;
    Node * getNearestReader(DSNode *timeNode) const;
    DopeSheetSelectionModel * getSelectionModel() const;
    DopeSheetEditor* getEditor() const;

    // User interaction
    void deleteSelectedKeyframes();

    void moveSelectedKeysAndNodes(double dt);
    void trimReaderLeft(const DSNodePtr &reader, double newFirstFrame);
    void trimReaderRight(const DSNodePtr &reader, double newLastFrame);

    bool canSlipReader(const DSNodePtr &reader) const;

    void slipReader(const DSNodePtr &reader, double dt);
    void copySelectedKeys();
    void pasteKeys(bool relative);

    void pasteKeys(const std::vector<DopeSheetKey>& keys, bool relative);

    void setSelectedKeysInterpolation(KeyframeTypeEnum keyType);

    void transformSelectedKeys(const Transform::Matrix3x3& transform);

    void emit_modelChanged();

    // Other
    SequenceTime getCurrentFrame() const;

    void renameSelectedNode();

    QUndoStack* getUndoStack() const;

Q_SIGNALS:
    void modelChanged();
    void nodeAdded(DSNode *dsNode);
    void nodeAboutToBeRemoved(DSNode *dsNode);
    void nodeRemoved(DSNode *dsNode);
    void keyframeSetOrRemoved(DSKnob *dsKnob);

private: /* functions */
    DSNodePtr createDSNode(const NodeGuiPtr &nodeGui, DopeSheetItemType itemType);

private Q_SLOTS:
    void onNodeNameChanged(const QString &name);
    void onKeyframeSetOrRemoved();
    void onNodeNameEditDialogFinished();

private:
    boost::scoped_ptr<DopeSheetPrivate> _imp;
};


/**
 * @brief The DopeSheetSelectionModel class
 *
 *
 */
class DopeSheetSelectionModel
    : public QObject
{
    Q_OBJECT

public:
    enum SelectionType
    {
        SelectionTypeNoSelection = 0x0,
        SelectionTypeClear = 0x1,
        SelectionTypeAdd = 0x2,
        SelectionTypeToggle = 0x4,
        SelectionTypeRecurse = 0x8
    };

    Q_DECLARE_FLAGS(SelectionTypeFlags, SelectionType) DopeSheetSelectionModel(DopeSheet *dopeSheet);
    ~DopeSheetSelectionModel();

    void selectAll();
    void makeDopeSheetKeyframesForKnob(const DSKnobPtr &dsKnob, std::vector<DopeSheetKey> *result);

    void clearKeyframeSelection();
    void makeSelection(const std::vector<DopeSheetKey> &keys,
                       const std::vector<DSNodePtr>& rangeNodes,
                       DopeSheetSelectionModel::SelectionTypeFlags selectionFlags);

    bool isEmpty() const;

    void getCurrentSelection(DopeSheetKeyPtrList* keys, std::vector<DSNodePtr>* nodes) const;

    std::vector<DopeSheetKey> getKeyframesSelectionCopy() const;

    bool hasSingleKeyFrameTimeSelected(double* time) const;

    int getSelectedKeyframesCount() const;

    bool keyframeIsSelected(const DSKnobPtr &dsKnob, const KeyFrame &keyframe) const;

    DopeSheetKeyPtrList::iterator keyframeIsSelected(const DopeSheetKey &key) const;

    bool rangeIsSelected(const DSNodePtr& node) const;

    void emit_keyframeSelectionChanged(bool recurse);

    void onNodeAboutToBeRemoved(const DSNodePtr &removed);

Q_SIGNALS:
    void keyframeSelectionChangedFromModel(bool recurse);

private:

    std::list<DSNodeWPtr>::iterator isRangeNodeSelected(const DSNodePtr& node);
    boost::scoped_ptr<DopeSheetSelectionModelPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

Q_DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::DopeSheetSelectionModel::SelectionTypeFlags)

#endif // DOPESHEET_H
