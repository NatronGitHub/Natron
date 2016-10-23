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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "KnobItemsTable.h"

#include <QMutex>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Serialization/KnobTableItemSerialization.h"

NATRON_NAMESPACE_ENTER;

NATRON_NAMESPACE_ANONYMOUS_ENTER

struct ColumnHeader
{
    std::string text;
    std::string iconFilePath;
};

class KnobItemsTablesMetaTypesRegistration
{
public:
    inline KnobItemsTablesMetaTypesRegistration()
    {
        qRegisterMetaType<KnobTableItemPtr>("KnobTableItemPtr");
        qRegisterMetaType<std::list<KnobTableItemPtr> >("std::list<KnobTableItemPtr>");
        qRegisterMetaType<TableChangeReasonEnum>("TableChangeReasonEnum");
    }
};


NATRON_NAMESPACE_ANONYMOUS_EXIT

static KnobItemsTablesMetaTypesRegistration registration;


struct KnobItemsTablePrivate
{
    KnobHolderWPtr holder;
    KnobItemsTable::KnobItemsTableTypeEnum type;
    KnobItemsTable::TableSelectionModeEnum selectionMode;
    int colsCount;
    std::vector<ColumnHeader> headers;
    std::vector<KnobTableItemPtr> topLevelItems;
    std::list<KnobTableItemWPtr> selectedItems;
    std::string iconsPath;
    bool uniformRowsHeight;
    bool supportsDnD;
    bool dndSupportsExternalSource;

    mutable QMutex topLevelItemsLock, selectionLock;

    // Used to bracket changes in selection
    int beginSelectionCounter;

    // Used to prevent nasty recursion in endSelection
    int selectionRecursion;

    // track items that were added/removed during the full change of a begin/end selection
    std::set<KnobTableItemPtr> newItemsInSelection, itemsRemovedFromSelection;

    // List of knobs on the holder which controls each knob with the same script-name on each item in the table
    std::list<KnobIWPtr> perItemMasterKnobs;

    std::string pythonPrefix;

    KnobItemsTablePrivate(const KnobHolderPtr& originalHolder, KnobItemsTable::KnobItemsTableTypeEnum type, int colsCount)
    : holder(originalHolder)
    , type(type)
    , selectionMode(KnobItemsTable::eTableSelectionModeExtendedSelection)
    , colsCount(colsCount)
    , headers(colsCount)
    , topLevelItems()
    , selectedItems()
    , iconsPath()
    , uniformRowsHeight(false)
    , supportsDnD(false)
    , dndSupportsExternalSource(false)
    , topLevelItemsLock()
    , selectionLock(QMutex::Recursive)
    , beginSelectionCounter(0)
    , selectionRecursion(0)
    , newItemsInSelection()
    , itemsRemovedFromSelection()
    , perItemMasterKnobs()
    , pythonPrefix()
    {

    }

    void incrementSelectionCounter()
    {
        ++beginSelectionCounter;
    }

    bool decrementSelectionCounter()
    {
        if (beginSelectionCounter > 0) {
            --beginSelectionCounter;
            return true;
        }
        return false;
    }

    void addToSelectionList(const KnobTableItemPtr& item)
    {
        // If the item is already in items removed from the selection, remove it from the other list
        std::set<KnobTableItemPtr>::iterator found = itemsRemovedFromSelection.find(item);
        if (found != itemsRemovedFromSelection.end() ) {
            itemsRemovedFromSelection.erase(found);
        }
        newItemsInSelection.insert(item);
    }

    void removeFromSelectionList(const KnobTableItemPtr& item)
    {
        // If the item is already in items added to the selection, remove it from the other list
        std::set<KnobTableItemPtr>::iterator found = newItemsInSelection.find(item);
        if (found != newItemsInSelection.end() ) {
            newItemsInSelection.erase(found);
        }
        
        itemsRemovedFromSelection.insert(item);
        
    }

    static KnobTableItemPtr getItemByScriptNameInternal(const std::string& name, const std::string& recurseName,  const std::vector<KnobTableItemPtr>& items);

};

struct ColumnDesc
{
    std::string columnName;

    // If the columnName is the script-name of a knob, we hold a weak ref to the knob for faster access later on
    KnobIWPtr knob;

    // The dimension in the knob or -1
    int dimensionIndex;
};

typedef std::map<ViewIdx, CurvePtr> PerViewAnimationCurveMap;

struct KnobTableItemPrivate
{
    // If we are in a tree, this is a pointer to the parent.
    // If null, the item is considered to be top-level
    KnobTableItemWPtr parent;

    // A list of children. This item holds a strong reference to them
    std::vector<KnobTableItemPtr> children;

    // The columns used by the item. If the column name empty, the column will be empty in the Gui
    std::vector<ColumnDesc> columns;

    // Weak reference to the model
    KnobItemsTableWPtr model;

    // Name and label
    std::string scriptName, label, iconFilePath;

    // Locks all members
    mutable QMutex lock;

    // List of keyframe times set by the user
    PerViewAnimationCurveMap animationCurves;

    KnobTableItemPrivate(const KnobItemsTablePtr& model)
    : parent()
    , children()
    , columns(model->getColumnsCount())
    , model(model)
    , scriptName()
    , label()
    , iconFilePath()
    , lock()
    , animationCurves()
    {
        animationCurves[ViewIdx(0)].reset(new Curve());
    }

    ViewIdx getViewIdxFromGetSpec(ViewGetSpec view) const;
};

KnobItemsTable::KnobItemsTable(const KnobHolderPtr& originalHolder, KnobItemsTableTypeEnum type, int colsCount)
: _imp(new KnobItemsTablePrivate(originalHolder, type, colsCount))
{
}

KnobItemsTable::~KnobItemsTable()
{

}

void
KnobItemsTable::setSupportsDragAndDrop(bool supports)
{
    _imp->supportsDnD = supports;
}

bool
KnobItemsTable::isDragAndDropSupported() const
{
    return _imp->supportsDnD;
}

void
KnobItemsTable::setDropSupportsExternalSources(bool supports)
{
    _imp->dndSupportsExternalSource = supports;
}

bool
KnobItemsTable::isDropFromExternalSourceSupported() const
{
    return _imp->dndSupportsExternalSource;
}

KnobHolderPtr
KnobItemsTable::getOriginalHolder() const
{
    return _imp->holder.lock();
}

NodePtr
KnobItemsTable::getNode() const
{
    KnobHolderPtr holder = _imp->holder.lock();
    if (!holder) {
        return NodePtr();
    }
    EffectInstancePtr isEffect = toEffectInstance(holder);
    if (!isEffect) {
        return NodePtr();
    }
    return isEffect->getNode();
}


KnobItemsTable::KnobItemsTableTypeEnum
KnobItemsTable::getType() const
{
    return _imp->type;
}

void
KnobItemsTable::setIconsPath(const std::string& iconPath)
{
    _imp->iconsPath = iconPath;
}

const std::string&
KnobItemsTable::getIconsPath() const
{
    return _imp->iconsPath;
}

void
KnobItemsTable::setRowsHaveUniformHeight(bool uniform)
{
    _imp->uniformRowsHeight = uniform;
}

bool
KnobItemsTable::getRowsHaveUniformHeight() const
{
    return _imp->uniformRowsHeight;
}

int
KnobItemsTable::getColumnsCount() const
{
    return _imp->colsCount;
}

void
KnobItemsTable::setColumnText(int col, const std::string& text)
{
    if (col < 0 || col >= (int)_imp->headers.size()) {
        return;
    }
    _imp->headers[col].text = text;
}

std::string
KnobItemsTable::getColumnText(int col) const
{
    if (col < 0 || col >= (int)_imp->headers.size()) {
        return std::string();
    }
    return _imp->headers[col].text;
}

void
KnobItemsTable::setColumnIcon(int col, const std::string& iconFilePath)
{
    if (col < 0 || col >= (int)_imp->headers.size()) {
        return;
    }
    _imp->headers[col].iconFilePath = iconFilePath;
}

std::string
KnobItemsTable::getColumnIcon(int col) const
{
    if (col < 0 || col >= (int)_imp->headers.size()) {
        return std::string();
    }
    return _imp->headers[col].iconFilePath;
}

void
KnobItemsTable::setSelectionMode(TableSelectionModeEnum mode)
{
    _imp->selectionMode = mode;
}

KnobItemsTable::TableSelectionModeEnum
KnobItemsTable::getSelectionMode() const
{
    return _imp->selectionMode;
}

void
KnobItemsTable::addItem(const KnobTableItemPtr& item, const KnobTableItemPtr& parent, TableChangeReasonEnum reason)
{
    insertItem(-1, item, parent, reason);
}

void
KnobItemsTable::insertItem(int index, const KnobTableItemPtr& item, const KnobTableItemPtr& parent, TableChangeReasonEnum reason)
{
    if (!item) {
        return;
    }
    // The item must have been constructed with this model in parameter
    assert(item->getModel().get() == this);

    removeItem(item, reason);

    item->ensureItemInitialized();

    if (parent) {
        parent->insertChild(index, item);
    } else {
        int insertIndex;

        {
            QMutexLocker k(&_imp->topLevelItemsLock);
            if (index < 0 || index >= (int)_imp->topLevelItems.size()) {
                _imp->topLevelItems.push_back(item);
                insertIndex = _imp->topLevelItems.size() - 1;
            } else {
                std::vector<KnobTableItemPtr>::iterator it = _imp->topLevelItems.begin();
                std::advance(it, index);
                _imp->topLevelItems.insert(it, item);
                insertIndex = index;
            }
        }
    }
    
    if (!getPythonPrefix().empty()) {
        declareItemAsPythonField(item);
    }

    item->onItemInsertedInModel_recursive();

    Q_EMIT itemInserted(index, item, reason);
    
}


void
KnobItemsTable::removeItem(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    KnobTableItemPtr parent = item->getParent();
    bool removed = false;

    if (parent) {
        removed = parent->removeChild(item);
    } else {
        {
            QMutexLocker k(&_imp->topLevelItemsLock);
            for (std::vector<KnobTableItemPtr>::iterator it = _imp->topLevelItems.begin(); it != _imp->topLevelItems.end(); ++it) {
                if (*it == item) {
                    _imp->topLevelItems.erase(it);
                    removed = true;
                    break;
                }
            }
        }
        if (removed) {
            Q_EMIT itemRemoved(item, reason);
        }
    }
    if (removed) {
        if (!getPythonPrefix().empty()) {
            removeItemAsPythonField(item);
        }
        item->onItemRemovedFromModel_recursive();
    }
}

void
KnobItemsTable::resetModel(TableChangeReasonEnum reason)
{
    std::vector<KnobTableItemPtr> topLevelItems;
    {
        QMutexLocker k(&_imp->topLevelItemsLock);
        topLevelItems = _imp->topLevelItems;
        _imp->topLevelItems.clear();
    }
    for (std::size_t i = 0; i < topLevelItems.size(); ++i) {
        if (!getPythonPrefix().empty()) {
            removeItemAsPythonField(topLevelItems[i]);
        }

        Q_EMIT itemRemoved(topLevelItems[i], reason);
        topLevelItems[i]->onItemRemovedFromModel_recursive();
    }
    onModelReset();
}

std::vector<KnobTableItemPtr>
KnobItemsTable::getTopLevelItems() const
{
    QMutexLocker k(&_imp->topLevelItemsLock);
    return _imp->topLevelItems;
}

static void appendItemRecursive(const KnobTableItemPtr& item, std::vector<KnobTableItemPtr>* allItems)
{
    allItems->push_back(item);

    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        appendItemRecursive(children[i], allItems);
    }
}

std::vector<KnobTableItemPtr>
KnobItemsTable::getAllItems() const
{
    QMutexLocker k(&_imp->topLevelItemsLock);
    if (_imp->type == KnobItemsTable::eKnobItemsTableTypeTable) {
        return _imp->topLevelItems;
    } else {
        std::vector<KnobTableItemPtr> ret;
        for (std::size_t i = 0; i < _imp->topLevelItems.size(); ++i) {
            appendItemRecursive(_imp->topLevelItems[i], &ret);
        }
        return ret;
    }
}

bool
KnobItemsTable::getNumTopLevelItems() const
{
    QMutexLocker k(&_imp->topLevelItemsLock);
    return (int)_imp->topLevelItems.size();
}

KnobTableItemPtr
KnobItemsTable::getTopLevelItem(int index) const
{
    QMutexLocker k(&_imp->topLevelItemsLock);
    if (index < 0 || index >= (int)_imp->topLevelItems.size()) {
        return KnobTableItemPtr();
    }
    return _imp->topLevelItems[index];
}

std::string
KnobItemsTable::generateUniqueName(const KnobTableItemPtr& item, const std::string& baseName) const
{
    int no = 1;
    bool foundItem;
    std::string name;
    
    do {
        std::stringstream ss;
        ss << baseName;
        ss << no;
        name = ss.str();

        foundItem = false;
        KnobTableItemPtr parent = item->getParent();
        if (parent) {
            foundItem = (bool)parent->getChildItemByScriptName(name);
        } else {
            KnobItemsTablePtr model = item->getModel();
            if (model) {
                foundItem = (bool)model->getTopLevelItemByScriptName(name);
            }
        }
        ++no;
    } while (foundItem);
    
    return name;

}

KnobTableItem::KnobTableItem(const KnobItemsTablePtr& model)
: NamedKnobHolder(model->getOriginalHolder()->getApp())
, _imp(new KnobTableItemPrivate(model))
{

}

KnobTableItem::~KnobTableItem()
{
   
}

KnobItemsTablePtr
KnobTableItem::getModel() const
{
    return _imp->model.lock();
}

void
KnobTableItem::copyItem(const KnobTableItemPtr& other)
{
    const std::vector<KnobIPtr>& otherKnobs = other->getKnobs();
    const std::vector<KnobIPtr>& thisKnobs = getKnobs();
    if (thisKnobs.size() != otherKnobs.size()) {
        return;
    }
    KnobsVec::const_iterator it = thisKnobs.begin();
    KnobsVec::const_iterator otherIt = otherKnobs.begin();
    for (; it != thisKnobs.end(); ++it, ++otherIt) {
        (*it)->copyKnob(*otherIt);
    }

}

void
KnobTableItem::onItemRemovedFromModel_recursive()
{
    onItemRemovedFromModel();
    std::vector<KnobTableItemPtr> children = getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        children[i]->onItemRemovedFromModel_recursive();
    }
}

void
KnobTableItem::onItemInsertedInModel_recursive()
{
    onItemInsertedInModel();
    std::vector<KnobTableItemPtr> children = getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        children[i]->onItemInsertedInModel_recursive();
    }
}

void
KnobTableItem::onSignificantEvaluateAboutToBeCalled(const KnobIPtr& knob, ValueChangedReasonEnum reason, DimSpec dimension, double time, ViewSetSpec view)
{

    KnobItemsTablePtr model = getModel();
    if (model) {
        NodePtr node = model->getNode();
        if ( !node || !node->isNodeCreated() ) {
            return;
        }
        node->getEffectInstance()->abortAnyEvaluation();
    }
    
    
    if (knob) {
        // This will also invalidate this hash cache
        knob->invalidateHashCache();
    } else {
        invalidateHashCache();
    }

    bool isMT = QThread::currentThread() == qApp->thread();
    if ( isMT && ( !knob || knob->getEvaluateOnChange() ) ) {
        getApp()->triggerAutoSave();
    }

    Q_UNUSED(reason);
    Q_UNUSED(dimension);
    Q_UNUSED(time);
    Q_UNUSED(view);

}

void
KnobTableItem::evaluate(bool isSignificant, bool refreshMetadatas)
{
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return;
    }
    NodePtr node = model->getNode();
    if (!node) {
        return;
    }
    node->getEffectInstance()->evaluate(isSignificant, refreshMetadatas);
}

void
KnobTableItem::setLabel(const std::string& label, TableChangeReasonEnum reason)
{
    bool changed;
    {
        QMutexLocker k(&_imp->lock);
        changed = _imp->label != label;
        if (changed) {
            _imp->label = label;
        }
    }
    if (changed) {
        Q_EMIT labelChanged(QString::fromUtf8(label.c_str()), reason);
    }
}

void
KnobTableItem::setIconLabelFilePath(const std::string& iconFilePath, TableChangeReasonEnum reason)
{
    bool changed;
    {
        QMutexLocker k(&_imp->lock);
        changed = _imp->iconFilePath != iconFilePath;
        if (changed) {
            _imp->iconFilePath = iconFilePath;
        }
    }
    if (changed) {
        Q_EMIT labelIconChanged(reason);
    }
}


std::string
KnobTableItem::getIconLabelFilePath() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->iconFilePath;
}

std::string
KnobTableItem::getLabel() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->label;
}


void
KnobTableItem::setScriptName(const std::string& name)
{

    KnobItemsTablePtr model = getModel();
    if (!model) {
        return;
    }

    KnobTableItemPtr thisShared = toKnobTableItem(shared_from_this());
    std::string nameToSet = name;
    if ( nameToSet.empty() ) {
        std::string baseName = getBaseItemName();
        if (baseName.empty()) {
            baseName = "Item";
        }
        nameToSet = model->generateUniqueName(thisShared, baseName);
    }

    // Ensure the name is not empty so far

    std::string currentName;
    {
        QMutexLocker l(&_imp->lock);
        currentName = _imp->scriptName;
    }

    // Make sure the script-name is Python compliant
    nameToSet = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(nameToSet);

    // The name can be empty if the user passed a string with only non python non compliant characters
    if (nameToSet.empty()) {
        std::string baseName = getBaseItemName();
        if (baseName.empty()) {
            baseName = "Item";
        }
        nameToSet = model->generateUniqueName(thisShared, baseName);
    }

    // Nothing changed
    if ( nameToSet == currentName) {
        return;
    }

    // Check that it is unique in the model
    {
        int no = 1;
        bool foundItem;
        std::string tmpName;

        do {
            std::stringstream ss;
            ss << nameToSet;
            if (no > 1) {
                ss << no;
            }
            tmpName = ss.str();
            KnobTableItemPtr existingItem;
            KnobTableItemPtr parent = getParent();
            if (parent) {
                existingItem = parent->getChildItemByScriptName(tmpName);
            } else {
                existingItem = model->getTopLevelItemByScriptName(tmpName);
            }

            if ( existingItem && existingItem.get() != this ) {
                foundItem = true;
            } else {
                foundItem = false;
            }
            ++no;
        } while (foundItem);
        nameToSet = tmpName;
    }


    if (!currentName.empty()) {
        model->removeItemAsPythonField(thisShared);
    }

    {
        QMutexLocker l(&_imp->lock);
        _imp->scriptName = nameToSet;
    }

    model->declareItemAsPythonField(thisShared);
    
}

std::string
KnobTableItem::getScriptName_mt_safe() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->scriptName;
}

static void
getScriptNameRecursive(const KnobTableItemPtr& item,
                       std::string* scriptName)
{
    scriptName->insert(0, ".");
    scriptName->insert(0, item->getScriptName_mt_safe());
    KnobTableItemPtr parent = item->getParent();
    if (parent) {
        getScriptNameRecursive(parent, scriptName);
    }
}

std::string
KnobTableItem::getFullyQualifiedName() const
{
    std::string name = getScriptName_mt_safe();
    KnobTableItemPtr parent = getParent();
    if (parent) {
        getScriptNameRecursive(parent, &name);
    }

    return name;
}

void
KnobTableItem::insertChild(int index, const KnobTableItemPtr& item)
{
    if (!item || !isItemContainer()) {
        return;
    }

    assert(_imp->model.lock()->getType() == KnobItemsTable::eKnobItemsTableTypeTree);

    int insertedIndex;

    {
        QMutexLocker k(&_imp->lock);
        if (index < 0 || index >= (int)_imp->children.size()) {
            _imp->children.push_back(item);
            insertedIndex = _imp->children.size() - 1;
        } else {
            std::vector<KnobTableItemPtr>::iterator it = _imp->children.begin();
            std::advance(it, index);
            _imp->children.insert(it, item);
            insertedIndex = index;
        }
    }

    {
        QMutexLocker k(&item->_imp->lock);
        item->_imp->parent = toKnobTableItem(shared_from_this());
    }

}

bool
KnobTableItem::removeChild(const KnobTableItemPtr& item)
{
    if (!isItemContainer()) {
        return false;
    }
    bool removed = false;
    KnobItemsTablePtr model = getModel();
    if (!model->getPythonPrefix().empty()) {
        model->removeItemAsPythonField(item);
    }
    {
        QMutexLocker k(&_imp->lock);
        for (std::vector<KnobTableItemPtr>::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
            if (*it == item) {
                _imp->children.erase(it);
                removed = true;
                break;
            }
        }
    }
    return removed;
}

void
KnobTableItem::ensureItemInitialized()
{
    KnobItemsTablePtr model = getModel();
    assert(model);
    if (!model) {
        return;
    }
    std::string curName = getScriptName_mt_safe();
    if (curName.empty()) {
        KnobTableItemPtr thisShared = toKnobTableItem(shared_from_this());
        // No script-name: generate one
        curName = model->generateUniqueName(thisShared, getBaseItemName());
    }
    setScriptName(curName);
    if (getLabel().empty()) {
        setLabel(curName, eTableChangeReasonInternal);
    }

    initializeKnobsPublic();


}

KnobTableItemPtr
KnobTableItem::getParent() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->parent.lock();
}

int
KnobTableItem::getIndexInParent() const
{
    KnobTableItemPtr parent = getParent();
    int found = -1;
    if (parent) {
        QMutexLocker k(&parent->_imp->lock);
        for (std::size_t i = 0; i < parent->_imp->children.size(); ++i) {
            if (parent->_imp->children[i].get() == this) {
                found = i;
                break;
            }
        }

    } else {
        KnobItemsTablePtr table = _imp->model.lock();
        assert(table);
        QMutexLocker k(&table->_imp->topLevelItemsLock);
        for (std::size_t i = 0; i < table->_imp->topLevelItems.size(); ++i) {
            if (table->_imp->topLevelItems[i].get() == this) {
                found = i;
                break;
            }
        }
    }
    return found;
}

std::vector<KnobTableItemPtr>
KnobTableItem::getChildren() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->children;
}

KnobTableItemPtr
KnobTableItem::getChild(int index) const
{
    QMutexLocker k(&_imp->lock);
    if (index < 0 || index >= (int)_imp->children.size()) {
        return KnobTableItemPtr();
    }
    return _imp->children[index];
}

void
KnobTableItem::setColumn(int col, const std::string& columnName, int dimension)
{
    QMutexLocker k(&_imp->lock);
    if (col < 0 || col >= (int)_imp->columns.size()) {
        return;
    }
    if (columnName != kKnobTableItemColumnLabel) {
        _imp->columns[col].knob = getKnobByName(columnName);
        assert(_imp->columns[col].knob.lock());
    }
    _imp->columns[col].columnName = columnName;
    _imp->columns[col].dimensionIndex = dimension;
}

KnobIPtr
KnobTableItem::getColumnKnob(int col, int *dimensionIndex) const
{
    QMutexLocker k(&_imp->lock);
    if (col < 0 || col >= (int)_imp->columns.size()) {
        return KnobIPtr();
    }
    *dimensionIndex = _imp->columns[col].dimensionIndex;
    return  _imp->columns[col].knob.lock();
}

std::string
KnobTableItem::getColumnName(int col) const
{
    QMutexLocker k(&_imp->lock);
    if (col < 0 || col >= (int)_imp->columns.size()) {
        return std::string();
    }
    return  _imp->columns[col].columnName;
}

int
KnobTableItem::getLabelColumnIndex() const
{
    QMutexLocker k(&_imp->lock);
    for (std::size_t i = 0; i < _imp->columns.size(); ++i) {
        if (_imp->columns[i].columnName == kKnobTableItemColumnLabel) {
            return i;
        }
    }
    return -1;
}

int
KnobTableItem::getKnobColumnIndex(const KnobIPtr& knob, int dimension) const
{
    QMutexLocker k(&_imp->lock);
    for (std::size_t i = 0; i < _imp->columns.size(); ++i) {
        if (_imp->columns[i].knob.lock() == knob && (_imp->columns[i].dimensionIndex == -1 || _imp->columns[i].dimensionIndex == dimension)) {
            return i;
        }
    }
    return -1;
}

int
KnobTableItem::getItemRow() const
{
    int indexInParent = getIndexInParent();
    
    // If this fails here, that means the item is not yet in the model.
    if (indexInParent != -1) {
        return indexInParent;
    }
    
    int rowI = indexInParent;
    KnobTableItemPtr parent = getParent();
    while (parent) {
        indexInParent = parent->getIndexInParent();
        parent = parent->getParent();
        rowI += indexInParent;
        
        // Add one because the parent itself counts, here is a simple example:
        //
        //  Item1                       --> row 0
        //      ChildLevel1             --> row 1
        //          ChildLevel2         --> row 2
        //              ChildLevel3_1   --> row 3
        //              ChildLevel3_2   --> row 4
        rowI += 1;
    }
    return rowI;
}

int
KnobTableItem::getHierarchyLevel() const
{
    int level = 0;
    KnobTableItemPtr parent = getParent();
    while (parent) {
        ++level;
        parent = parent->getParent();
    }
    return level;
}

static KnobTableItemPtr
getNextNonContainerItemInternal(const std::vector<KnobTableItemPtr>& siblings, const KnobTableItemConstPtr& item)
{

    if ( !item || siblings.empty() ) {
        return KnobTableItemPtr();
    }
    std::vector<KnobTableItemPtr>::const_iterator found = siblings.end();
    for (std::vector<KnobTableItemPtr>::const_iterator it = siblings.begin(); it != siblings.end(); ++it) {
        if (*it == item) {
            found = it;
            break;
        }
    }

    // The item must be in the siblings vector
    assert( found != siblings.end() );

    if ( found != siblings.end() ) {
        // If there's a next items in the siblings, return it
        ++found;
        if ( found != siblings.end() ) {
            return *found;
        }
    }

    // Item was still not found, find in great parent layer
    KnobTableItemPtr parentItem;
    KnobTableItemPtr greatParentItem;
    {
        parentItem = item->getParent();
        if (!parentItem) {
            return KnobTableItemPtr();
        }
        greatParentItem = parentItem->getParent();
        if (!greatParentItem) {
            return KnobTableItemPtr();
        }
    }
    std::vector<KnobTableItemPtr> greatParentItems = greatParentItem->getChildren();

    return getNextNonContainerItemInternal(greatParentItems, parentItem);
}

KnobTableItemPtr
KnobTableItem::getNextNonContainerItem() const
{
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return KnobTableItemPtr();
    }
    std::vector<KnobTableItemPtr> siblings;
    KnobTableItemPtr parentItem = getParent();
    if (!parentItem) {
        siblings = model->getTopLevelItems();
    } else {
        siblings = parentItem->getChildren();
    }
    KnobTableItemConstPtr thisShared = toKnobTableItem(shared_from_this());
    return getNextNonContainerItemInternal(siblings, thisShared);
}

SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr
KnobItemsTable::createSerializationFromItem(const KnobTableItemPtr& item)
{
    SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
    item->toSerialization(s.get());
    return s;
}

void
KnobTableItem::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase)
{
    SERIALIZATION_NAMESPACE::KnobTableItemSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::KnobTableItemSerialization*>(serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }


    {
        QMutexLocker k(&_imp->lock);
        serialization->scriptName = _imp->scriptName;
        serialization->label = _imp->label;
    }

    KnobsVec knobs = getKnobs_mt_safe();
    for (std::size_t i = 0; i < knobs.size(); ++i) {

        if (!knobs[i]->getIsPersistent()) {
            continue;
        }
        KnobGroupPtr isGroup = toKnobGroup(knobs[i]);
        KnobPagePtr isPage = toKnobPage(knobs[i]);
        if (isGroup || isPage) {
            continue;
        }

        if (!knobs[i]->hasModifications()) {
            continue;
        }


        SERIALIZATION_NAMESPACE::KnobSerializationPtr newKnobSer( new SERIALIZATION_NAMESPACE::KnobSerialization );
        knobs[i]->toSerialization(newKnobSer.get());
        if (newKnobSer->_mustSerialize) {
            serialization->knobs.push_back(newKnobSer);
        }
        
    }

    // Recurse
    std::vector<KnobTableItemPtr> children = getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
        children[i]->toSerialization(s.get());
        serialization->children.push_back(s);
    }
}


void
KnobTableItem::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase)
{
    const SERIALIZATION_NAMESPACE::KnobTableItemSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::KnobTableItemSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    {
        QMutexLocker k(&_imp->lock);
        _imp->label = serialization->label;
        _imp->scriptName = serialization->scriptName;
    }

    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = serialization->knobs.begin(); it != serialization->knobs.end(); ++it) {
        KnobIPtr foundKnob = getKnobByName((*it)->getName());
        if (!foundKnob) {
            continue;
        }
        foundKnob->fromSerialization(**it);
    }

    assert(_imp->children.empty());

    KnobItemsTablePtr model = _imp->model.lock();
    KnobTableItemPtr thisShared = toKnobTableItem(shared_from_this());
    for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = serialization->children.begin(); it!=serialization->children.end(); ++it) {
        KnobTableItemPtr child = model->createItemFromSerialization(*it);
        if (child) {
            model->addItem(child, thisShared, eTableChangeReasonInternal);
        }
    }
}

static void
getItemNameAndRemainder_LeftToRight(const std::string& fullySpecifiedName,
                                    std::string& name,
                                    std::string& remainder)
{
    std::size_t foundDot = fullySpecifiedName.find_first_of('.');

    if (foundDot != std::string::npos) {
        name = fullySpecifiedName.substr(0, foundDot);
        if ( foundDot + 1 < fullySpecifiedName.size() ) {
            remainder = fullySpecifiedName.substr(foundDot + 1, std::string::npos);
        }
    } else {
        name = fullySpecifiedName;
    }
}

KnobTableItemPtr
KnobItemsTablePrivate::getItemByScriptNameInternal(const std::string& name, const std::string& recurseName, const std::vector<KnobTableItemPtr>& items) 
{
    for (std::vector<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        if ( (*it)->getScriptName_mt_safe() == name ) {
            if ( !recurseName.empty() ) {
                if ((*it)->isItemContainer()) {
                    std::string toFind;
                    std::string subrecurseName;
                    getItemNameAndRemainder_LeftToRight(recurseName, toFind, subrecurseName);
                    return getItemByScriptNameInternal(toFind, subrecurseName, (*it)->getChildren());
                }
            } else {
                return *it;
            }
        }
    }
    return KnobTableItemPtr();
}

KnobTableItemPtr
KnobItemsTable::getItemByFullyQualifiedScriptName(const std::string& fullyQualifiedScriptNa) const
{
    std::string toFind;
    std::string recurseName;
    getItemNameAndRemainder_LeftToRight(fullyQualifiedScriptNa, toFind, recurseName);

    std::vector<KnobTableItemPtr> topLevel = getTopLevelItems();
    return KnobItemsTablePrivate::getItemByScriptNameInternal(toFind, recurseName, topLevel);
}

KnobTableItemPtr
KnobItemsTable::getTopLevelItemByScriptName(const std::string& scriptName) const
{
    std::vector<KnobTableItemPtr> topLevel = getTopLevelItems();
    return KnobItemsTablePrivate::getItemByScriptNameInternal(scriptName, std::string(), topLevel);
}

KnobTableItemPtr
KnobTableItem::getChildItemByScriptName(const std::string& scriptName) const
{
    std::vector<KnobTableItemPtr> children = getChildren();
    return KnobItemsTablePrivate::getItemByScriptNameInternal(scriptName, std::string(), children);
}

bool
KnobItemsTable::isItemSelected(const KnobTableItemPtr& item) const
{
    QMutexLocker k(&_imp->selectionLock);
    for (std::list<KnobTableItemWPtr>::const_iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        if (it->lock() == item) {
            return true;
        }
    }
    return false;
}

std::list<KnobTableItemPtr>
KnobItemsTable::getSelectedItems() const
{
    std::list<KnobTableItemPtr> ret;
    QMutexLocker k(&_imp->selectionLock);
    for (std::list<KnobTableItemWPtr>::const_iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        KnobTableItemPtr i = it->lock();
        if (!i) {
            continue;
        }
        ret.push_back(i);
    }
    return ret;
}


void
KnobItemsTable::beginEditSelection()
{
    QMutexLocker k(&_imp->selectionLock);
    _imp->incrementSelectionCounter();

}

void
KnobItemsTable::endEditSelection(TableChangeReasonEnum reason)
{
    bool doEnd = false;
    {
        QMutexLocker k(&_imp->selectionLock);
        if (_imp->decrementSelectionCounter()) {

            if (_imp->beginSelectionCounter == 0) {
                doEnd = true;
            }
        }
    }

    if (doEnd) {
        endSelection(reason);
    }
}

void
KnobItemsTable::addToSelection(const std::list<KnobTableItemPtr >& items, TableChangeReasonEnum reason)
{
    bool hasCalledBegin = false;
    {
        QMutexLocker k(&_imp->selectionLock);

        if (!_imp->beginSelectionCounter) {
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }

        for (std::list<KnobTableItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
            _imp->addToSelectionList(*it);
        }

        if (hasCalledBegin) {
            _imp->decrementSelectionCounter();
        }
    }

    if (hasCalledBegin) {
        endSelection(reason);
    }

}

void
KnobItemsTable::addToSelection(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    std::list<KnobTableItemPtr > items;
    items.push_back(item);
    addToSelection(items, reason);
}

void
KnobItemsTable::removeFromSelection(const std::list<KnobTableItemPtr >& items, TableChangeReasonEnum reason)
{
    bool hasCalledBegin = false;

    {
        QMutexLocker k(&_imp->selectionLock);

        if (!_imp->beginSelectionCounter) {
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }

        for (std::list<KnobTableItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
            _imp->removeFromSelectionList(*it);
        }

        if (hasCalledBegin) {
            _imp->decrementSelectionCounter();
        }
    }

    if (hasCalledBegin) {
        endSelection(reason);
    }
}

void
KnobItemsTable::removeFromSelection(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    std::list<KnobTableItemPtr > items;
    items.push_back(item);
    removeFromSelection(items, reason);
}

void
KnobItemsTable::clearSelection(TableChangeReasonEnum reason)
{
    std::list<KnobTableItemPtr > items = getSelectedItems();
    if ( items.empty() ) {
        return;
    }
    removeFromSelection(items, reason);
}

static void addToSelectionRecursive(const KnobTableItemPtr& item, TableChangeReasonEnum reason, KnobItemsTable* table)
{
    table->addToSelection(item, reason);
    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        addToSelectionRecursive(children[i], reason, table);
    }
}

void
KnobItemsTable::selectAll(TableChangeReasonEnum reason)
{
    beginEditSelection();
    std::vector<KnobTableItemPtr> items;
    {
        QMutexLocker k(&_imp->topLevelItemsLock);
        items = _imp->topLevelItems;
    }
    for (std::vector<KnobTableItemPtr>::iterator it = items.begin(); it != items.end(); ++it) {
        addToSelectionRecursive(*it, reason, this);
    }
    endEditSelection(reason);
}

KnobTableItemPtr
KnobItemsTable::findDeepestSelectedItemContainer() const
{
    int minLevel = -1;
    KnobTableItemPtr minContainer;

    std::list<KnobTableItemWPtr> selection;
    {
        QMutexLocker k(&_imp->selectionLock);
        selection = _imp->selectedItems;
    }

    for (std::list<KnobTableItemWPtr>::const_iterator it = selection.begin(); it != selection.end(); ++it) {
        KnobTableItemPtr item = it->lock();
        if (!item) {
            continue;
        }
        int lvl = item->getHierarchyLevel();
        if (lvl > minLevel) {
            if (item->isItemContainer()) {
                minContainer = item;
            } else {
                minContainer = item->getParent();
            }
            minLevel = lvl;
        }
    }
    return minContainer;
}

void
KnobItemsTable::addPerItemKnobMaster(const KnobIPtr& masterKnob)
{
    if (!masterKnob) {
        return;
    }
    
    masterKnob->setEnabled(false);
    masterKnob->setIsPersistent(false);

    _imp->perItemMasterKnobs.push_back(masterKnob);
}

void
KnobItemsTable::endSelection(TableChangeReasonEnum reason)
{

    std::list<KnobTableItemPtr> itemsAdded, itemsRemoved;
    {
        // Avoid recursions
        QMutexLocker k(&_imp->selectionLock);
        if (_imp->selectionRecursion > 0) {
            _imp->itemsRemovedFromSelection.clear();
            _imp->newItemsInSelection.clear();
            return;
        }
        if ( _imp->itemsRemovedFromSelection.empty() && _imp->newItemsInSelection.empty() ) {
            return;
        }

        ++_imp->selectionRecursion;

        // Remove from selection and unslave from master knobs
        for (std::set<KnobTableItemPtr>::const_iterator it = _imp->itemsRemovedFromSelection.begin(); it != _imp->itemsRemovedFromSelection.end(); ++it) {
            for (std::list<KnobTableItemWPtr>::iterator it2 = _imp->selectedItems.begin(); it2!= _imp->selectedItems.end(); ++it2) {
                if ( it2->lock() == *it) {
                    itemsRemoved.push_back(*it);
                    _imp->selectedItems.erase(it2);
                    break;
                }

            }

        }

        // Add to selection and slave to master knobs
        for (std::set<KnobTableItemPtr>::const_iterator it = _imp->newItemsInSelection.begin(); it != _imp->newItemsInSelection.end(); ++it) {
            bool found = false;
            for (std::list<KnobTableItemWPtr>::iterator it2 = _imp->selectedItems.begin(); it2!= _imp->selectedItems.end(); ++it2) {
                if ( it2->lock() == *it) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                itemsAdded.push_back(*it);
                _imp->selectedItems.push_back(*it);
            }
        }



        for (std::list<KnobIWPtr>::iterator it = _imp->perItemMasterKnobs.begin(); it != _imp->perItemMasterKnobs.end(); ++it) {
            KnobIPtr masterKnob = it->lock();
            if (!masterKnob) {
                continue;
            }
            int nItemsWithKnob = 0;
            for (std::list<KnobTableItemWPtr>::iterator it2 = _imp->selectedItems.begin(); it2!= _imp->selectedItems.end(); ++it2) {
                KnobTableItemPtr item = it2->lock();
                if (!item) {
                    continue;
                }
                KnobIPtr itemKnob = item->getKnobByName(masterKnob->getName());
                if (itemKnob) {
                    ++nItemsWithKnob;
                }
            }

            masterKnob->setEnabled(nItemsWithKnob > 0);
            masterKnob->setDirty(nItemsWithKnob > 1);

            for (std::set<KnobTableItemPtr>::const_iterator it = _imp->newItemsInSelection.begin(); it != _imp->newItemsInSelection.end(); ++it) {
                KnobIPtr itemKnob = (*it)->getKnobByName(masterKnob->getName());
                if (!itemKnob) {
                    continue;
                }
                // Ensure the master knob reflects the state of the last item that was added to the selection.
                // We ensure the state change does not affect already slaved knobs otherwise all items would
                // get the value of the item we are copying.
                masterKnob->blockListenersNotification();
                masterKnob->copyKnob(itemKnob);
                masterKnob->unblockListenersNotification();


                // Slave item knob to master knob
                itemKnob->slaveTo(masterKnob);

            }
            for (std::set<KnobTableItemPtr>::const_iterator it = _imp->itemsRemovedFromSelection.begin(); it != _imp->itemsRemovedFromSelection.end(); ++it) {
                KnobIPtr itemKnob = (*it)->getKnobByName(masterKnob->getName());
                if (!itemKnob) {
                    continue;
                }

                // Unslave from master knob, copy state only if a single item is selected
                itemKnob->unSlave(DimSpec::all(), ViewSetSpec::all(), nItemsWithKnob <= 1 /*copyState*/);
            }

        } // for all master knobs


        _imp->itemsRemovedFromSelection.clear();
        _imp->newItemsInSelection.clear();

    } //  QMutexLocker k(&_imp->selectionLock);

    Q_EMIT selectionChanged(itemsAdded, itemsRemoved, reason);

    // Run the Python callback if it is set
    NodePtr node = getNode();
    if (node) {
        node->runAfterTableItemsSelectionChangedCallback(itemsRemoved, itemsAdded, reason);
    }

    QMutexLocker k(&_imp->selectionLock);
    --_imp->selectionRecursion;

} // endSelection


void
KnobItemsTable::declareItemsToPython()
{
    assert(QThread::currentThread() == qApp->thread());

    _imp->pythonPrefix = getTablePythonPrefix();

    NodePtr node = getNode();
    if (!node) {
        return;
    }
    std::string appID = node->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = nodeFullName + "." + _imp->pythonPrefix + " = " + nodeFullName + ".getItemsTable()\n";
    if ( !appPTR->isBackground() ) {
        node->getApp()->printAutoDeclaredVariable(script);
    }
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("KnobItemsTable::declareItemsToPython(): interpretPythonScript(" + script + ") failed!");
    }


    std::vector<KnobTableItemPtr> items = getTopLevelItems();
    for (std::vector< KnobTableItemPtr >::iterator it = items.begin(); it != items.end(); ++it) {
        declareItemAsPythonField(*it);
    }
}

const std::string&
KnobItemsTable::getPythonPrefix() const
{
    return _imp->pythonPrefix;
}

void
KnobItemsTable::removeItemAsPythonField(const KnobTableItemPtr& item)
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    if (_imp->pythonPrefix.empty()) {
        return;
    }
    std::string appID = node->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::stringstream ss;
    ss << "del " << nodeFullName << "." << _imp->pythonPrefix << "." << item->getFullyQualifiedName() << "\n";
    std::string script =  ss.str();

    if ( !appPTR->isBackground() ) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        getNode()->getApp()->appendToScriptEditor(err);
    }

    // No need to remove children since we removed the parent attribute
}

void
KnobItemsTable::declareItemAsPythonField(const KnobTableItemPtr& item)
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    if (_imp->pythonPrefix.empty()) {
        return;
    }
    std::string appID = node->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string itemName = item->getFullyQualifiedName();
    std::stringstream ss;

    ss << nodeFullName << "." << _imp->pythonPrefix << "." << itemName << " = ";
    ss << nodeFullName << "." << _imp->pythonPrefix << ".getTrackByName(\"" << itemName + "\")\n";


    // Declare its knobs
    const KnobsVec& knobs = item->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss << nodeFullName << "." << _imp->pythonPrefix << "." << itemName << "." << (*it)->getName() << " = ";
        ss << nodeFullName << "." << _imp->pythonPrefix << "." << itemName << ".getParam(\"" << (*it)->getName() << "\")\n";
    }
    std::string script = ss.str();
    if ( !appPTR->isBackground() ) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        getNode()->getApp()->appendToScriptEditor(err);
    }


    // Declare children recursively
    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        declareItemAsPythonField(children[i]);
    }

}

KnobIPtr
KnobTableItem::createDuplicateOfTableKnobInternal(const std::string& scriptName)
{
    KnobTableItemPtr thisItem = boost::dynamic_pointer_cast<KnobTableItem>(shared_from_this());
    return getModel()->createMasterKnobDuplicateOnItem(thisItem, scriptName);
}

KnobIPtr
KnobItemsTable::createMasterKnobDuplicateOnItem(const KnobTableItemPtr& item, const std::string& paramName)
{
    KnobIPtr masterKnob;
    for (std::list<KnobIWPtr>::const_iterator it = _imp->perItemMasterKnobs.begin(); it!=_imp->perItemMasterKnobs.end(); ++it) {
        KnobIPtr knob = it->lock();
        if (!knob) {
            continue;
        }
        if (knob->getName() == paramName) {
            masterKnob = knob;
            break;
        }
    }
    if (!masterKnob) {
        assert(false);
        return KnobIPtr();
    }
    KnobIPtr ret = masterKnob->createDuplicateOnHolder(item, KnobPagePtr(), KnobGroupPtr(), -1, true, paramName, masterKnob->getLabel(), masterKnob->getHintToolTip(), false /*refreshParamsGui*/, false /*isUserKnob*/);
    if (ret) {
        // Set back persistence to true on the item knob
        ret->setIsPersistent(true);
        ret->setEnabled(true);
    }
    return ret;
}

//////// Animation implementation
AnimatingObjectI::KeyframeDataTypeEnum
KnobTableItem::getKeyFrameDataType() const
{
    return eKeyframeDataTypeNone;
}


void
KnobTableItem::splitView(ViewIdx view)
{
    {
        QMutexLocker k(&_imp->lock);
        CurvePtr& foundCurve = _imp->animationCurves[view];
        if (!foundCurve) {
            foundCurve.reset(new Curve);
        }
    }

} // splitView

void
KnobTableItem::unSplitView(ViewIdx view)
{
    {
        QMutexLocker k(&_imp->lock);
        PerViewAnimationCurveMap::iterator foundView = _imp->animationCurves.find(view);
        if (foundView != _imp->animationCurves.end()) {
            _imp->animationCurves.erase(foundView);
        }
    }
} // unSplitView

std::list<ViewIdx>
KnobTableItem::getViewsList() const
{
    std::list<ViewIdx> ret;
    QMutexLocker k(&_imp->lock);
    for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it != _imp->animationCurves.end(); ++it) {
        ret.push_back(it->first);
    }
    return ret;
}

ViewIdx
KnobTableItem::getViewIdxFromGetSpec(ViewGetSpec view) const
{
    return _imp->getViewIdxFromGetSpec(view);
}

ViewIdx
KnobTableItemPrivate::getViewIdxFromGetSpec(ViewGetSpec view) const
{
    ViewIdx view_i(view);

    if (view.isCurrent()) {
        KnobItemsTablePtr m = model.lock();
        if (!m) {
            view_i = ViewIdx(0);
        } else {
            view_i = m->getNode()->getEffectInstance()->getCurrentView();
        }
    }
    return view_i;
}

CurvePtr
KnobTableItem::getAnimationCurve(ViewGetSpec idx, DimIdx /*dimension*/) const
{
    ViewIdx view_i = _imp->getViewIdxFromGetSpec(idx);
    PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
    if (foundView != _imp->animationCurves.end()) {
        return foundView->second;
    }
    return CurvePtr();
}

ValueChangedReturnCodeEnum
KnobTableItem::setKeyFrameInternal(double time,
                                  ViewSetSpec view,
                                  KeyFrame* newKey)
{
    // Private - should not lock
    assert(!_imp->lock.tryLock());

    if (newKey) {
        newKey->setTime(time);

        // Value is not used by this class
        newKey->setValue(0);
    }

    KeyFrame k(time, 0.);
    k.setInterpolation(eKeyframeTypeLinear);
    if (newKey) {
        *newKey = k;
    }

    ValueChangedReturnCodeEnum ret = eValueChangedReturnCodeNothingChanged;
    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            ret = it->second->setOrAddKeyframe(k);
        }
    } else {
        ViewIdx view_i = _imp->getViewIdxFromGetSpec(ViewGetSpec(view.value()));
        PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.find(view_i);
        if (it != _imp->animationCurves.end()) {
            ret = it->second->setOrAddKeyframe(k);
        }
    }
    if (ret == eValueChangedReturnCodeKeyframeAdded) {
        onKeyFrameSet(time, view);
    }
    return ret;
}

ValueChangedReturnCodeEnum
KnobTableItem::setKeyFrame(double time,
                           ViewSetSpec view,
                           KeyFrame* newKey)
{
    ValueChangedReturnCodeEnum ret;
    {
        QMutexLocker k(&_imp->lock);
        ret = setKeyFrameInternal(time, view, newKey);
    }
    if (ret == eValueChangedReturnCodeKeyframeAdded) {
        std::list<double> added,removed;
        added.push_back(time);
        Q_EMIT curveAnimationChanged(added, removed, ViewIdx(0));
    }
    return ret;
}

void
KnobTableItem::setMultipleKeyFrames(const std::list<double>& keys, ViewSetSpec view,  std::vector<KeyFrame>* newKeys)
{
    if (keys.empty()) {
        return;
    }
    if (newKeys) {
        newKeys->clear();
    }
    std::list<double> added,removed;
    KeyFrame key;
    {
        QMutexLocker k(&_imp->lock);
        for (std::list<double>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            ValueChangedReturnCodeEnum ret = setKeyFrameInternal(*it, view, newKeys ? &key : 0);
            if (ret == eValueChangedReturnCodeKeyframeAdded) {
                added.push_back(*it);
            }
            if (newKeys) {
                newKeys->push_back(key);
            }
        }
    }
    if (!added.empty()) {
        Q_EMIT curveAnimationChanged(added, removed, ViewIdx(0));
    }
}

void
KnobTableItem::deleteAnimationConditional(double time, ViewSetSpec view, bool before)
{

    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            std::list<double> keysRemoved;
            if (before) {
                it->second->removeKeyFramesBeforeTime(time, &keysRemoved);
            } else {
                it->second->removeKeyFramesAfterTime(time, &keysRemoved);
            }
            if (!keysRemoved.empty()) {
                for (std::list<double>::const_iterator it2 = keysRemoved.begin(); it2!=keysRemoved.end(); ++it2) {
                    onKeyFrameRemoved(*it2, it->first);
                }
                std::list<double> keysAdded;
                Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, it->first);
            }
        }
    } else {
        ViewIdx view_i = _imp->getViewIdxFromGetSpec(ViewGetSpec(view.value()));
        PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.find(view_i);
        if (it != _imp->animationCurves.end()) {
            std::list<double> keysRemoved;
            if (before) {
                it->second->removeKeyFramesBeforeTime(time, &keysRemoved);
            } else {
                it->second->removeKeyFramesAfterTime(time, &keysRemoved);
            }
            if (!keysRemoved.empty()) {
                for (std::list<double>::const_iterator it2 = keysRemoved.begin(); it2!=keysRemoved.end(); ++it2) {
                    onKeyFrameRemoved(*it2, it->first);
                }
                std::list<double> keysAdded;
                Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, it->first);
            }
        }
    }

} // deleteAnimationConditional

bool
KnobTableItem::cloneCurve(ViewIdx view, DimIdx /*dimension*/, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* /*stringAnimation*/)
{
    std::list<double> keysAdded, keysRemoved;
    bool hasChanged;
    {
        QMutexLocker k(&_imp->lock);
        CurvePtr thisCurve = getAnimationCurve(view, DimIdx(0));
        if (!thisCurve) {
            return false;
        }

        KeyFrameSet oldKeys = thisCurve->getKeyFrames_mt_safe();
        hasChanged = thisCurve->cloneAndCheckIfChanged(curve, offset, range);
        if (hasChanged) {
            KeyFrameSet newKeys = thisCurve->getKeyFrames_mt_safe();


            for (KeyFrameSet::iterator it = newKeys.begin(); it != newKeys.end(); ++it) {
                KeyFrameSet::iterator foundInOldKeys = Curve::findWithTime(oldKeys, oldKeys.end(), it->getTime());
                if (foundInOldKeys == oldKeys.end()) {
                    keysAdded.push_back(it->getTime());
                } else {
                    oldKeys.erase(foundInOldKeys);
                }
            }

            for (KeyFrameSet::iterator it = oldKeys.begin(); it != oldKeys.end(); ++it) {
                KeyFrameSet::iterator foundInNextKeys = Curve::findWithTime(newKeys, newKeys.end(), it->getTime());
                if (foundInNextKeys == newKeys.end()) {
                    keysRemoved.push_back(it->getTime());
                }
            }

        }
    }

    if (!keysAdded.empty() || !keysRemoved.empty()) {
        for (std::list<double>::const_iterator it = keysAdded.begin(); it!=keysAdded.end(); ++it) {
            onKeyFrameSet(*it, view);
        }
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(*it, view);
        }
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
    return hasChanged;
} // cloneCurve

void
KnobTableItem::deleteValuesAtTimeInternal(const std::list<double>& times, ViewIdx view, const CurvePtr& curve)
{
    std::list<double> keysRemoved, keysAdded;

    {
        QMutexLocker k(&_imp->lock);
        try {
            for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
                curve->removeKeyFrameWithTime(*it);
                keysRemoved.push_back(*it);
            }
        } catch (const std::exception & /*e*/) {
        }
    }
    if (!keysRemoved.empty()) {
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(*it, view);
        }
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
}

void
KnobTableItem::deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec /*dimension*/)
{
    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            deleteValuesAtTimeInternal(times, it->first, it->second);
        }
    } else {
        ViewIdx view_i = _imp->getViewIdxFromGetSpec(ViewGetSpec(view.value()));
        PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
        if (foundView != _imp->animationCurves.end()) {
            deleteValuesAtTimeInternal(times, view_i, foundView->second);
        }
    }
}

bool
KnobTableItem::warpValuesAtTimeInternal(const std::list<double>& times, ViewIdx view, const CurvePtr& curve, const Curve::KeyFrameWarp& warp, bool allowKeysOverlap, std::vector<KeyFrame>* keyframes)
{

    std::list<double> keysAdded, keysRemoved;
    {
        QMutexLocker k(&_imp->lock);
        std::vector<KeyFrame> newKeys;
        if ( !curve->transformKeyframesValueAndTime(times, warp, allowKeysOverlap, keyframes, &keysAdded, &keysAdded) ) {
            return false;
        }

    }
    if (!keysAdded.empty() || !keysRemoved.empty()) {
        for (std::list<double>::const_iterator it = keysAdded.begin(); it!=keysAdded.end(); ++it) {
            onKeyFrameSet(*it, view);
        }
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(*it, view);
        }

        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
    return true;
}


bool
KnobTableItem::warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec /*dimension*/, const Curve::KeyFrameWarp& warp, bool allowKeysOverlap, std::vector<KeyFrame>* keyframes)
{
    bool ok = false;
    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            ok |= warpValuesAtTimeInternal(times, it->first, it->second, warp, allowKeysOverlap, keyframes);
        }
    } else {
        ViewIdx view_i = _imp->getViewIdxFromGetSpec(ViewGetSpec(view.value()));
        PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
        if (foundView != _imp->animationCurves.end()) {
            ok |= warpValuesAtTimeInternal(times, view_i, foundView->second, warp, allowKeysOverlap, keyframes);
        }
    }
    return ok;
}

void
KnobTableItem::removeAnimationInternal(ViewIdx view, const CurvePtr& curve)
{
    std::list<double> keysAdded, keysRemoved;
    {
        QMutexLocker k(&_imp->lock);

        KeyFrameSet keys = curve->getKeyFrames_mt_safe();
        for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            keysRemoved.push_back(it->getTime());
        }
        curve->clearKeyFrames();
    }
    if (!keysRemoved.empty()) {
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(*it, view);
        }

        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
}


void
KnobTableItem::removeAnimation(ViewSetSpec view, DimSpec /*dimensions*/)
{

    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            removeAnimationInternal(it->first, it->second);
        }
    } else {
        ViewIdx view_i = _imp->getViewIdxFromGetSpec(ViewGetSpec(view.value()));
        PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
        if (foundView != _imp->animationCurves.end()) {
            removeAnimationInternal(view_i, foundView->second);
        }
    }

}

void
KnobTableItem::deleteAnimationBeforeTime(double time, ViewSetSpec view, DimSpec /*dimension*/)
{
    deleteAnimationConditional(time, view, true);
}

void
KnobTableItem::deleteAnimationAfterTime(double time, ViewSetSpec view, DimSpec /*dimension*/)
{
    deleteAnimationConditional(time, view, false);
}

void
KnobTableItem::setInterpolationAtTimes(ViewSetSpec /*view*/, DimSpec /*dimension*/, const std::list<double>& /*times*/, KeyframeTypeEnum /*interpolation*/, std::vector<KeyFrame>* /*newKeys*/)
{
    // user keyframes should always have linear interpolation
    return;
}

bool
KnobTableItem::setLeftAndRightDerivativesAtTime(ViewSetSpec /*view*/, DimSpec /*dimension*/, double /*time*/, double /*left*/, double /*right*/)
{
    // user keyframes should always have linear interpolation
    return false;
}

bool
KnobTableItem::setDerivativeAtTime(ViewSetSpec /*view*/, DimSpec /*dimension*/, double /*time*/, double /*derivative*/, bool /*isLeft*/)
{
    // user keyframes should always have linear interpolation
    return false;
}


ValueChangedReturnCodeEnum
KnobTableItem::setDoubleValueAtTime(double time, double /*value*/, ViewSetSpec view, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* newKey)
{
    return setKeyFrame(time, view, newKey);
}

void
KnobTableItem::setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSetSpec view, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* newKey)
{
    std::list<double> keyTimes;
    convertTimeValuePairListToTimeList(keys, &keyTimes);
    setMultipleKeyFrames(keyTimes, view, newKey);
}

void
KnobTableItem::setDoubleValueAtTimeAcrossDimensions(double time, const std::vector<double>& values, DimIdx /*dimensionStartIndex*/, ViewSetSpec view, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* retCodes)
{
    if (values.empty()) {
        return;
    }
    ValueChangedReturnCodeEnum ret = setKeyFrame(time, view, 0);
    if (retCodes) {
        retCodes->push_back(ret);
    }
}

void
KnobTableItem::setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& keysPerDimension, ValueChangedReasonEnum /*reason*/)
{
    if (keysPerDimension.empty()) {
        return;
    }
    for (std::size_t i = 0; i < keysPerDimension.size();++i) {
        std::list<double> keyTimes;
        convertTimeValuePairListToTimeList(keysPerDimension[i].second, &keyTimes);
        setMultipleKeyFrames(keyTimes, ViewSetSpec::all(), 0);
    }

}

void
KnobTableItem::setKeyFrameRecursively(double time, ViewSetSpec view)
{
    if (getCanAnimateUserKeyframes()) {
        setKeyFrame(time, view, 0);
    }
    if (isItemContainer()) {
        std::vector<KnobTableItemPtr> children = getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            children[i]->setKeyFrameRecursively(time, view);
        }
    }
}

void
KnobItemsTable::setMasterKeyframeOnSelectedItems(double time, ViewSetSpec view)
{
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        (*it)->setKeyFrameRecursively(time, view);
    }
}

static void removeKeyFrameRecursively(const KnobTableItemPtr& item, double time, ViewSetSpec view)
{
    if (item->getCanAnimateUserKeyframes()) {
        item->deleteValueAtTime(time, view, DimSpec::all());
    }
    if (item->isItemContainer()) {
        std::vector<KnobTableItemPtr> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            removeKeyFrameRecursively(children[i], time, view);
        }
    }
}

void
KnobItemsTable::removeMasterKeyframeOnSelectedItems(double time, ViewSetSpec view)
{
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        removeKeyFrameRecursively(*it, time, view);
    }
}

static void removeAnimationRecursively(const KnobTableItemPtr& item, ViewSetSpec view)
{
    if (item->getCanAnimateUserKeyframes()) {
        item->removeAnimation(view, DimSpec::all());
    }
    if (item->isItemContainer()) {
        std::vector<KnobTableItemPtr> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            removeAnimationRecursively(children[i], view);
        }
    }
}

void
KnobItemsTable::removeAnimationOnSelectedItems(ViewSetSpec view)
{
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        removeAnimationRecursively(*it,  view);
    }
}

int
KnobTableItem::getMasterKeyframesCount(ViewGetSpec view) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return 0;
    }
    return curve->getKeyFramesCount();
}

bool
KnobTableItem::getMasterKeyframe(int index, ViewGetSpec view, KeyFrame* k) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return false;
    }
    return curve->getKeyFrameWithIndex(index, k);
}

bool
KnobTableItem::getHasMasterKeyframe(double time, ViewGetSpec view) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return -1;
    }
    KeyFrame k;
    return curve->getKeyFrameWithTime(time, &k);
}

void
KnobTableItem::getMasterKeyFrameTimes(ViewGetSpec view, std::set<double>* times) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve || !times) {
        return;
    }
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        times->insert(it->getTime());
    }
}

bool
KnobTableItem::hasMasterKeyframeAtTime(double time, ViewGetSpec view) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return false;
    }
    KeyFrame k;
    return curve->getKeyFrameWithTime(time, &k);
}

static void
findOutNearestKeyframeRecursively(const KnobTableItemPtr& item,
                                  bool previous,
                                  double time,
                                  ViewGetSpec view,
                                  double* nearest)
{

    if (item->isItemContainer()) {
        std::vector<KnobTableItemPtr> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            findOutNearestKeyframeRecursively(children[i], previous, time, view, nearest);
        }
    } else if (item->getCanAnimateUserKeyframes()) {
        if (previous) {
            int t = item->getPreviousMasterKeyframeTime(time, view);
            if ( (t != -std::numeric_limits<double>::infinity()) && (t > *nearest) ) {
                *nearest = t;
            }
        } else {
            int t = item->getNextMasterKeyframeTime(time, view);
            if ( (t != std::numeric_limits<double>::infinity()) && (t < *nearest) ) {
                *nearest = t;
            }
        }

    }

}

double
KnobTableItem::getPreviousMasterKeyframeTime(double time, ViewGetSpec view) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return 0;
    }
    KeyFrame k;
    if (!curve->getPreviousKeyframeTime(time, &k)) {
        return -std::numeric_limits<double>::infinity();
    }
    return k.getTime();
}

double
KnobTableItem::getNextMasterKeyframeTime(double time, ViewGetSpec view) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return 0;
    }
    KeyFrame k;
    if (!curve->getNextKeyframeTime(time, &k)) {
        return std::numeric_limits<double>::infinity();
    }
    return k.getTime();
}

void
KnobItemsTable::goToPreviousMasterKeyframe(ViewGetSpec view)
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    double time = node->getApp()->getTimeLine()->currentFrame();
    double minTime = -std::numeric_limits<double>::infinity();
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        findOutNearestKeyframeRecursively(*it, true, time, view, &minTime);
    }
    if (minTime != -std::numeric_limits<double>::infinity()) {
        node->getApp()->setLastViewerUsingTimeline( NodePtr() );
        node->getApp()->getTimeLine()->seekFrame(minTime, false, OutputEffectInstancePtr(), eTimelineChangeReasonOtherSeek);
    }
}

void
KnobItemsTable::goToNextMasterKeyframe(ViewGetSpec view)
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    double time = node->getApp()->getTimeLine()->currentFrame();
    double minTime = std::numeric_limits<double>::infinity();
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        findOutNearestKeyframeRecursively(*it, false, time, view, &minTime);
    }
    if (minTime != std::numeric_limits<double>::infinity()) {
        node->getApp()->setLastViewerUsingTimeline( NodePtr() );
        node->getApp()->getTimeLine()->seekFrame(minTime, false, OutputEffectInstancePtr(), eTimelineChangeReasonOtherSeek);
    }
}

static bool hasAnimationRecursive(const std::vector<KnobTableItemPtr>& items)
{
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i]->isItemContainer()) {
            bool subRet = hasAnimationRecursive(items[i]->getChildren());
            if (subRet) {
                return true;
            }
        } else {
            std::list<ViewIdx> views;
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                if (items[i]->getMasterKeyframesCount(*it) > 1) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool
KnobItemsTable::hasAnimation() const
{
    std::vector<KnobTableItemPtr> items = getTopLevelItems();
    return hasAnimationRecursive(items);
}

void
KnobItemsTable::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase)
{
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::KnobItemsTableSerialization*>(serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    std::vector<KnobTableItemPtr> topLevelItems = getTopLevelItems();
    for (std::vector<KnobTableItemPtr>::const_iterator it = topLevelItems.begin(); it != topLevelItems.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s = createSerializationFromItem(*it);
        if (s) {
            serialization->items.push_back(s);
        }
    }
    

}

void
KnobItemsTable::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase)
{

    const SERIALIZATION_NAMESPACE::KnobItemsTableSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::KnobItemsTableSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    assert(_imp->topLevelItems.empty());

    for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = serialization->items.begin(); it != serialization->items.end(); ++it) {
        KnobTableItemPtr item = createItemFromSerialization(*it);
        if (item) {
            addItem(item, KnobTableItemPtr(), eTableChangeReasonInternal);
        }
    }
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_KnobItemsTable.cpp"
