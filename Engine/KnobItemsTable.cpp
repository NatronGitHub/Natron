/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include <sstream> // stringstream

#include <QMutex>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Serialization/KnobTableItemSerialization.h"

NATRON_NAMESPACE_ENTER;

NATRON_NAMESPACE_ANONYMOUS_ENTER

struct ColumnHeader
{
    std::string text;
    std::string iconFilePath;
    std::string tooltip;
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

struct KnobItemsTableCommon
{
    KnobItemsTable::KnobItemsTableTypeEnum type;
    KnobItemsTable::TableSelectionModeEnum selectionMode;
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

    // Used to prevent updates on the master knob
    int masterKnobUpdatesBlocked;

    // track items that were added/removed during the full change of a begin/end selection
    std::set<KnobTableItemPtr> newItemsInSelection, itemsRemovedFromSelection;

    // List of knobs on the holder which controls each knob with the same script-name on each item in the table
    std::list<KnobIWPtr> perItemMasterKnobs;

    std::string pythonPrefix;

    KnobItemsTableCommon()
    : type()
    , selectionMode(KnobItemsTable::eTableSelectionModeExtendedSelection)
    , headers()
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
    , masterKnobUpdatesBlocked(0)
    , newItemsInSelection()
    , itemsRemovedFromSelection()
    , perItemMasterKnobs()
    , pythonPrefix()
    {

    }

};

struct KnobItemsTablePrivate
{
    KnobHolderWPtr holder;
    boost::shared_ptr<KnobItemsTableCommon> common;

    KnobItemsTablePrivate(const KnobHolderPtr& originalHolder, KnobItemsTable::KnobItemsTableTypeEnum type)
    : holder(originalHolder)
    , common(new KnobItemsTableCommon)
    {
        common->type = type;
    }


    void incrementSelectionCounter()
    {
        ++common->beginSelectionCounter;
    }

    bool decrementSelectionCounter()
    {
        if (common->beginSelectionCounter > 0) {
            --common->beginSelectionCounter;
            return true;
        }
        return false;
    }

    void addToSelectionList(const KnobTableItemPtr& item)
    {
        // If the item is already in items removed from the selection, remove it from the other list
        std::set<KnobTableItemPtr>::iterator found = common->itemsRemovedFromSelection.find(item);
        if (found != common->itemsRemovedFromSelection.end() ) {
            common->itemsRemovedFromSelection.erase(found);
        }
        common->newItemsInSelection.insert(item);
    }

    void removeFromSelectionList(const KnobTableItemPtr& item)
    {
        // If the item is already in items added to the selection, remove it from the other list
        std::set<KnobTableItemPtr>::iterator found = common->newItemsInSelection.find(item);
        if (found != common->newItemsInSelection.end() ) {
            common->newItemsInSelection.erase(found);
        }
        
        common->itemsRemovedFromSelection.insert(item);
        
    }

    static KnobTableItemPtr getItemByScriptNameInternal(const std::string& name, const std::string& recurseName,  const std::vector<KnobTableItemPtr>& items);

};

struct ColumnDesc
{
    std::string columnName;

    // If the columnName is the script-name of a knob, we hold a weak ref to the knob for faster access later on
    KnobIWPtr knob;

    // The dimension in the knob
    DimSpec dimensionIndex;
};

typedef std::map<ViewIdx, CurvePtr> PerViewAnimationCurveMap;

struct KnobTableItemCommon
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


    KnobTableItemCommon(const KnobItemsTablePtr& model)
    : parent()
    , children()
    , columns()
    , model(model)
    , scriptName()
    , label()
    , iconFilePath()
    , lock()
    {

    }
};

struct KnobTableItemPrivate
{

    boost::shared_ptr<KnobTableItemCommon> common;

    // List of keyframe times set by the user
    PerViewAnimationCurveMap animationCurves;

    KnobTableItemPrivate(const KnobItemsTablePtr& model)
    : common(new KnobTableItemCommon(model))
    {
        CurvePtr c(new Curve);
        c->setYComponentMovable(false);
        animationCurves[ViewIdx(0)] = c;
    }

    KnobTableItemPrivate(const KnobTableItemPrivate& other)
    : common(other.common)
    {
        for (PerViewAnimationCurveMap::const_iterator it = other.animationCurves.begin(); it!=other.animationCurves.end(); ++it) {
            CurvePtr c(new Curve);
            c->setYComponentMovable(false);
            c->clone(*it->second);
            animationCurves[it->first] = c;
        }
    }

};

KnobItemsTable::KnobItemsTable(const KnobHolderPtr& originalHolder, KnobItemsTableTypeEnum type)
: _imp(new KnobItemsTablePrivate(originalHolder, type))
{
}

KnobItemsTable::~KnobItemsTable()
{

}

void
KnobItemsTable::setSupportsDragAndDrop(bool supports)
{
    _imp->common->supportsDnD = supports;
}

bool
KnobItemsTable::isDragAndDropSupported() const
{
    return _imp->common->supportsDnD;
}

void
KnobItemsTable::setDropSupportsExternalSources(bool supports)
{
    _imp->common->dndSupportsExternalSource = supports;
}

bool
KnobItemsTable::isDropFromExternalSourceSupported() const
{
    return _imp->common->dndSupportsExternalSource;
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
    return _imp->common->type;
}

void
KnobItemsTable::setIconsPath(const std::string& iconPath)
{
    _imp->common->iconsPath = iconPath;
}

const std::string&
KnobItemsTable::getIconsPath() const
{
    return _imp->common->iconsPath;
}

void
KnobItemsTable::setRowsHaveUniformHeight(bool uniform)
{
    _imp->common->uniformRowsHeight = uniform;
}

bool
KnobItemsTable::getRowsHaveUniformHeight() const
{
    return _imp->common->uniformRowsHeight;
}

void
KnobItemsTable::setColumnText(int col, const std::string& text)
{
    ColumnHeader* header = 0;
    if (col < 0 || col >= (int)_imp->common->headers.size()) {
        _imp->common->headers.push_back(ColumnHeader());
        header = &_imp->common->headers.back();
    } else {
        header = &_imp->common->headers[col];
    }
    header->text = text;
}

std::string
KnobItemsTable::getColumnText(int col) const
{
    if (col < 0 || col >= (int)_imp->common->headers.size()) {
        return std::string();
    }
    return _imp->common->headers[col].text;
}

void
KnobItemsTable::setColumnIcon(int col, const std::string& iconFilePath)
{
    ColumnHeader* header = 0;
    if (col < 0 || col >= (int)_imp->common->headers.size()) {
        _imp->common->headers.push_back(ColumnHeader());
        header = &_imp->common->headers.back();
    } else {
        header = &_imp->common->headers[col];
    }
    header->iconFilePath = iconFilePath;
}

std::string
KnobItemsTable::getColumnIcon(int col) const
{
    if (col < 0 || col >= (int)_imp->common->headers.size()) {
        return std::string();
    }
    return _imp->common->headers[col].iconFilePath;
}

void
KnobItemsTable::setColumnTooltip(int col, const std::string& tooltip)
{
    ColumnHeader* header = 0;
    if (col < 0 || col >= (int)_imp->common->headers.size()) {
        _imp->common->headers.push_back(ColumnHeader());
        header = &_imp->common->headers.back();
    } else {
        header = &_imp->common->headers[col];
    }
    header->tooltip = tooltip;
}

std::string
KnobItemsTable::getColumnTooltip(int col) const
{
    if (col < 0 || col >= (int)_imp->common->headers.size()) {
        return std::string();
    }
    return _imp->common->headers[col].tooltip;

}

int
KnobItemsTable::getColumnsCount() const
{
    return (int)_imp->common->headers.size();
}

void
KnobItemsTable::setSelectionMode(TableSelectionModeEnum mode)
{
    _imp->common->selectionMode = mode;
}

KnobItemsTable::TableSelectionModeEnum
KnobItemsTable::getSelectionMode() const
{
    return _imp->common->selectionMode;
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

    if (parent) {
        parent->insertChild(index, item);
    } else {
        int insertIndex;

        {
            QMutexLocker k(&_imp->common->topLevelItemsLock);
            if (index < 0 || index >= (int)_imp->common->topLevelItems.size()) {
                _imp->common->topLevelItems.push_back(item);
                insertIndex = _imp->common->topLevelItems.size() - 1;
            } else {
                std::vector<KnobTableItemPtr>::iterator it = _imp->common->topLevelItems.begin();
                std::advance(it, index);
                _imp->common->topLevelItems.insert(it, item);
                insertIndex = index;
            }
        }
    }

    item->ensureItemInitialized();


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
            QMutexLocker k(&_imp->common->topLevelItemsLock);
            for (std::vector<KnobTableItemPtr>::iterator it = _imp->common->topLevelItems.begin(); it != _imp->common->topLevelItems.end(); ++it) {
                if (*it == item) {
                    _imp->common->topLevelItems.erase(it);
                    removed = true;
                    break;
                }
            }
        }
    }
    if (removed) {
        Q_EMIT itemRemoved(item, reason);

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
        QMutexLocker k(&_imp->common->topLevelItemsLock);
        topLevelItems = _imp->common->topLevelItems;
        _imp->common->topLevelItems.clear();
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
    QMutexLocker k(&_imp->common->topLevelItemsLock);
    return _imp->common->topLevelItems;
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
    QMutexLocker k(&_imp->common->topLevelItemsLock);
    if (_imp->common->type == KnobItemsTable::eKnobItemsTableTypeTable) {
        return _imp->common->topLevelItems;
    } else {
        std::vector<KnobTableItemPtr> ret;
        for (std::size_t i = 0; i < _imp->common->topLevelItems.size(); ++i) {
            appendItemRecursive(_imp->common->topLevelItems[i], &ret);
        }
        return ret;
    }
}

bool
KnobItemsTable::getNumTopLevelItems() const
{
    QMutexLocker k(&_imp->common->topLevelItemsLock);
    return (int)_imp->common->topLevelItems.size();
}

KnobTableItemPtr
KnobItemsTable::getTopLevelItem(int index) const
{
    QMutexLocker k(&_imp->common->topLevelItemsLock);
    if (index < 0 || index >= (int)_imp->common->topLevelItems.size()) {
        return KnobTableItemPtr();
    }
    return _imp->common->topLevelItems[index];
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
, AnimatingObjectI()
, _imp(new KnobTableItemPrivate(model))
{

}


KnobTableItem::KnobTableItem(const KnobTableItemPtr& other, const FrameViewRenderKey& key)
: NamedKnobHolder(other, key)
, AnimatingObjectI(other, key)
, _imp(new KnobTableItemPrivate(*other->_imp))
{

}

KnobTableItem::~KnobTableItem()
{
   
}


KnobItemsTablePtr
KnobTableItem::getModel() const
{
    return _imp->common->model.lock();
}

void
KnobTableItem::copyItem(const KnobTableItem& other)
{
    std::vector<KnobIPtr> otherKnobs = other.getKnobs_mt_safe();
    std::vector<KnobIPtr> thisKnobs = getKnobs_mt_safe();
    if (thisKnobs.size() != otherKnobs.size()) {
        return;
    }
    KnobsVec::const_iterator it = thisKnobs.begin();
    KnobsVec::const_iterator otherIt = otherKnobs.begin();
    for (; it != thisKnobs.end(); ++it, ++otherIt) {
        (*it)->copyKnob(*otherIt);
    }
    {
        QMutexLocker k(&_imp->common->lock);
        _imp->animationCurves.clear();
        for (PerViewAnimationCurveMap::const_iterator it = other._imp->animationCurves.begin(); it != other._imp->animationCurves.end(); ++it) {
            CurvePtr& c = _imp->animationCurves[it->first];
            c.reset(new Curve());
            c->setYComponentMovable(false);
            c->clone(*it->second);
        }
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
KnobTableItem::evaluate(bool isSignificant, bool refreshMetadatas)
{
    // If the item is not part of the model, do nothing
    if (getIndexInParent() == -1) {
        return;
    }
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return;
    }
    NodePtr node = model->getNode();
    if (!node) {
        return;
    }

    // Evaluate the node itself
    node->getEffectInstance()->evaluate(isSignificant, refreshMetadatas);
}

void
KnobTableItem::setLabel(const std::string& label, TableChangeReasonEnum reason)
{
    bool changed;
    {
        QMutexLocker k(&_imp->common->lock);
        changed = _imp->common->label != label;
        if (changed) {
            _imp->common->label = label;
        }
    }
    if (changed) {
        Q_EMIT labelChanged(QString::fromUtf8(label.c_str()), reason);
        evaluate(false, false);
    }
}

void
KnobTableItem::setIconLabelFilePath(const std::string& iconFilePath, TableChangeReasonEnum reason)
{
    bool changed;
    {
        QMutexLocker k(&_imp->common->lock);
        changed = _imp->common->iconFilePath != iconFilePath;
        if (changed) {
            _imp->common->iconFilePath = iconFilePath;
        }
    }
    if (changed) {
        Q_EMIT labelIconChanged(reason);
    }
}


std::string
KnobTableItem::getIconLabelFilePath() const
{
    QMutexLocker k(&_imp->common->lock);
    return _imp->common->iconFilePath;
}

std::string
KnobTableItem::getLabel() const
{
    QMutexLocker k(&_imp->common->lock);
    return _imp->common->label;
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
        QMutexLocker l(&_imp->common->lock);
        currentName = _imp->common->scriptName;
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
        QMutexLocker l(&_imp->common->lock);
        _imp->common->scriptName = nameToSet;
    }

    model->declareItemAsPythonField(thisShared);
    
}

std::string
KnobTableItem::getScriptName_mt_safe() const
{
    QMutexLocker k(&_imp->common->lock);
    return _imp->common->scriptName;
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

    assert(_imp->common->model.lock()->getType() == KnobItemsTable::eKnobItemsTableTypeTree);

    int insertedIndex;

    {
        QMutexLocker k(&_imp->common->lock);
        if (index < 0 || index >= (int)_imp->common->children.size()) {
            _imp->common->children.push_back(item);
            insertedIndex = _imp->common->children.size() - 1;
        } else {
            std::vector<KnobTableItemPtr>::iterator it = _imp->common->children.begin();
            std::advance(it, index);
            _imp->common->children.insert(it, item);
            insertedIndex = index;
        }
    }

    {
        QMutexLocker k(&item->_imp->common->lock);
        item->_imp->common->parent = toKnobTableItem(shared_from_this());
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
        QMutexLocker k(&_imp->common->lock);
        for (std::vector<KnobTableItemPtr>::iterator it = _imp->common->children.begin(); it != _imp->common->children.end(); ++it) {
            if (*it == item) {
                _imp->common->children.erase(it);
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
    if (getIndexInParent() != -1) {
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
    }
    
    initializeKnobsPublic();


}

KnobTableItemPtr
KnobTableItem::getParent() const
{
    QMutexLocker k(&_imp->common->lock);
    return _imp->common->parent.lock();
}

int
KnobTableItem::getIndexInParent() const
{
    KnobTableItemPtr parent = getParent();
    int found = -1;
    if (parent) {
        QMutexLocker k(&parent->_imp->common->lock);
        for (std::size_t i = 0; i < parent->_imp->common->children.size(); ++i) {
            if (parent->_imp->common->children[i].get() == this) {
                found = i;
                break;
            }
        }

    } else {
        KnobItemsTablePtr table = _imp->common->model.lock();
        assert(table);
        QMutexLocker k(&table->_imp->common->topLevelItemsLock);
        for (std::size_t i = 0; i < table->_imp->common->topLevelItems.size(); ++i) {
            if (table->_imp->common->topLevelItems[i].get() == this) {
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
    QMutexLocker k(&_imp->common->lock);
    return _imp->common->children;
}

KnobTableItemPtr
KnobTableItem::getChild(int index) const
{
    QMutexLocker k(&_imp->common->lock);
    if (index < 0 || index >= (int)_imp->common->children.size()) {
        return KnobTableItemPtr();
    }
    return _imp->common->children[index];
}

int
KnobTableItem::getColumnsCount() const
{
    return (int)_imp->common->columns.size();
}

void
KnobTableItem::addColumn(const std::string& columnName, DimSpec dimension)
{
    setColumn(-1, columnName, dimension);
}

void
KnobTableItem::setColumn(int col, const std::string& columnName, DimSpec dimension)
{
    QMutexLocker k(&_imp->common->lock);
    ColumnDesc* column = 0;
    if (col < 0 || col >= (int)_imp->common->columns.size()) {
        _imp->common->columns.push_back(ColumnDesc());
        column = &_imp->common->columns.back();
    } else {
        column = &_imp->common->columns[col];
    }
    if (columnName.empty()) {
        return;
    }
    if (columnName != kKnobTableItemColumnLabel) {
        KnobIPtr knob = getKnobByName(columnName);
        if (knob) {
            // Prevent-auto dimension switching for table items knobs.
            knob->setCanAutoFoldDimensions(false);
            column->knob = knob;
            column->columnName = columnName;
        }
    } else {
        column->columnName = columnName;
    }

    column->dimensionIndex = dimension;
}

KnobIPtr
KnobTableItem::getColumnKnob(int col, DimSpec *dimensionIndex) const
{
    QMutexLocker k(&_imp->common->lock);
    if (col < 0 || col >= (int)_imp->common->columns.size()) {
        return KnobIPtr();
    }
    *dimensionIndex = _imp->common->columns[col].dimensionIndex;
    return  _imp->common->columns[col].knob.lock();
}

std::string
KnobTableItem::getColumnName(int col) const
{
    QMutexLocker k(&_imp->common->lock);
    if (col < 0 || col >= (int)_imp->common->columns.size()) {
        return std::string();
    }
    return  _imp->common->columns[col].columnName;
}

int
KnobTableItem::getLabelColumnIndex() const
{
    QMutexLocker k(&_imp->common->lock);
    for (std::size_t i = 0; i < _imp->common->columns.size(); ++i) {
        if (_imp->common->columns[i].columnName == kKnobTableItemColumnLabel) {
            return i;
        }
    }
    return -1;
}

int
KnobTableItem::getKnobColumnIndex(const KnobIPtr& knob, int dimension) const
{
    QMutexLocker k(&_imp->common->lock);
    for (std::size_t i = 0; i < _imp->common->columns.size(); ++i) {
        if (_imp->common->columns[i].knob.lock() == knob && (_imp->common->columns[i].dimensionIndex == -1 || _imp->common->columns[i].dimensionIndex == dimension)) {
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

    // The item must be in the siblings vector, otherwise it is considered not in the model.
    if( found == siblings.end() ){
        return KnobTableItemPtr();
    }

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

    serialization->verbatimTag = getSerializationClassName();

    std::vector<std::string> projectViewNames = getApp()->getProject()->getProjectViewNames();
    {
        QMutexLocker k(&_imp->common->lock);
        serialization->scriptName = _imp->common->scriptName;
        serialization->label = _imp->common->label;

        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it != _imp->animationCurves.end(); ++it) {
            std::string viewName;
            if (it->first >= 0 && it->first < (int)projectViewNames.size()) {
                viewName = projectViewNames[it->first];
            } else {
                viewName = "Main";
            }
            SERIALIZATION_NAMESPACE::CurveSerialization c;
            it->second->toSerialization(&c);
            if (!c.keys.empty()) {
                serialization->animationCurves[viewName] = c;
            }
        }
    }

    KnobsVec knobs = getKnobs_mt_safe();
    for (std::size_t i = 0; i < knobs.size(); ++i) {

        bool hasExpr = false;
        {
            std::list<ViewIdx> views = knobs[i]->getViewsList();
            for (int d = 0; d < knobs[i]->getNDimensions(); ++d) {
                for (std::list<ViewIdx>::const_iterator itV = views.begin(); itV != views.end(); ++itV) {
                    if (!knobs[i]->getExpression(DimIdx(d), *itV).empty()) {
                        hasExpr = true;
                        break;
                    }
                    KnobDimViewKey linkData;
                    if (knobs[i]->getSharingMaster(DimIdx(d), *itV, &linkData)) {
                        hasExpr = true;
                        break;
                    }
                }
                if (hasExpr) {
                    break;
                }
            }
        }

        if (!knobs[i]->getIsPersistent() && !hasExpr) {
            continue;
        }
        KnobGroupPtr isGroup = toKnobGroup(knobs[i]);
        KnobPagePtr isPage = toKnobPage(knobs[i]);
        if (isGroup || isPage) {
            continue;
        }

        if (!knobs[i]->hasModifications() && !knobs[i]->hasDefaultValueChanged() && !hasExpr) {
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
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s = getModel()->createSerializationFromItem(children[i]);
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

    if (serialization->verbatimTag != getSerializationClassName()) {
        return;
    }
    std::vector<std::string> projectViewNames = getApp()->getProject()->getProjectViewNames();
    {
        QMutexLocker k(&_imp->common->lock);
        _imp->common->label = serialization->label;
        _imp->common->scriptName = serialization->scriptName;
        for (std::map<std::string, SERIALIZATION_NAMESPACE::CurveSerialization>::const_iterator it = serialization->animationCurves.begin(); it != serialization->animationCurves.end(); ++it) {
            ViewIdx view_i(0);
            getApp()->getProject()->getViewIndex(projectViewNames, it->first, &view_i);
            CurvePtr& c = _imp->animationCurves[view_i];
            if (!c) {
                c.reset(new Curve);
                c->setYComponentMovable(false);
            }
            c->fromSerialization(it->second);
        }
    }

    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = serialization->knobs.begin(); it != serialization->knobs.end(); ++it) {
        KnobIPtr foundKnob = getKnobByName((*it)->getName());
        if (!foundKnob) {
            continue;
        }
        foundKnob->fromSerialization(**it);
    }

    assert(_imp->common->children.empty());

    KnobItemsTablePtr model = _imp->common->model.lock();
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
    QMutexLocker k(&_imp->common->selectionLock);
    for (std::list<KnobTableItemWPtr>::const_iterator it = _imp->common->selectedItems.begin(); it != _imp->common->selectedItems.end(); ++it) {
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
    QMutexLocker k(&_imp->common->selectionLock);
    for (std::list<KnobTableItemWPtr>::const_iterator it = _imp->common->selectedItems.begin(); it != _imp->common->selectedItems.end(); ++it) {
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
    QMutexLocker k(&_imp->common->selectionLock);
    _imp->incrementSelectionCounter();

}

void
KnobItemsTable::endEditSelection(TableChangeReasonEnum reason)
{
    bool doEnd = false;
    {
        QMutexLocker k(&_imp->common->selectionLock);
        if (_imp->decrementSelectionCounter()) {

            if (_imp->common->beginSelectionCounter == 0) {
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
        QMutexLocker k(&_imp->common->selectionLock);

        if (!_imp->common->beginSelectionCounter) {
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }

        for (std::list<KnobTableItemPtr >::const_iterator it = items.begin(); it != items.end(); ++it) {
            // Only add items to the selection that are in the model
            if ((*it)->getIndexInParent() != -1) {
                _imp->addToSelectionList(*it);
            }
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
    // Only add items to the selection that are in the model
    if (item->getIndexInParent() == -1) {
        return;
    }
    std::list<KnobTableItemPtr > items;
    items.push_back(item);
    addToSelection(items, reason);
}

void
KnobItemsTable::removeFromSelection(const std::list<KnobTableItemPtr >& items, TableChangeReasonEnum reason)
{
    bool hasCalledBegin = false;

    {
        QMutexLocker k(&_imp->common->selectionLock);

        if (!_imp->common->beginSelectionCounter) {
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
        QMutexLocker k(&_imp->common->topLevelItemsLock);
        items = _imp->common->topLevelItems;
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
        QMutexLocker k(&_imp->common->selectionLock);
        selection = _imp->common->selectedItems;
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

bool
KnobItemsTable::isPerItemKnobMaster(const KnobIPtr& masterKnob)
{
    for (std::list<KnobIWPtr>::const_iterator it = _imp->common->perItemMasterKnobs.begin(); it!=_imp->common->perItemMasterKnobs.end(); ++it) {
        if (it->lock() == masterKnob) {
            return true;
        }
    }
    return false;
}

void
KnobItemsTable::addPerItemKnobMaster(const KnobIPtr& masterKnob)
{
    if (!masterKnob) {
        return;
    }

    // You cannot add knobs that do not belong to the node
    assert(masterKnob->getHolder() == getNode()->getEffectInstance());

    masterKnob->setEnabled(false);
    masterKnob->setIsPersistent(false);

    _imp->common->perItemMasterKnobs.push_back(masterKnob);

    QObject::connect(masterKnob->getSignalSlotHandler().get(), SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onMasterKnobValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), Qt::UniqueConnection);
}

void
KnobItemsTable::onMasterKnobValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum reason)
{
    KnobSignalSlotHandler* handler = dynamic_cast<KnobSignalSlotHandler*>(sender());
    if (!handler) {
        return;
    }
    KnobIPtr masterKnob = handler->getKnob();
    if (!masterKnob) {
        return;
    }
    if (reason != eValueChangedReasonTimeChanged) {
        QMutexLocker k(&_imp->common->selectionLock);

        // We may be in endSelection() when we  copy the masterKnob from the selection. In that case we don't want to recurse
        // again or we would hit an infinite loop.
        if (_imp->common->masterKnobUpdatesBlocked > 0) {
            return;
        }
        for (std::list<KnobTableItemWPtr>::iterator it2 = _imp->common->selectedItems.begin(); it2!= _imp->common->selectedItems.end(); ++it2) {
            KnobTableItemPtr item = it2->lock();
            if (!item) {
                continue;
            }
            KnobIPtr foundItemKnob = item->getKnobByName(masterKnob->getName());
            if (foundItemKnob && foundItemKnob->isEnabled()) {
                foundItemKnob->copyKnob(masterKnob);
            }
        }
    }
}

void
KnobItemsTable::onSelectionKnobValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum reason)
{
    KnobSignalSlotHandler* handler = dynamic_cast<KnobSignalSlotHandler*>(sender());
    if (!handler) {
        return;
    }
    KnobIPtr itemKnob = handler->getKnob();
    if (!itemKnob) {
        return;
    }

    if (reason == eValueChangedReasonTimeChanged) {
        return;
    }

    // If a selected item knob changes, update the master knob unless multiple knobs are selected
    // Prevent the onMasterKnobValueChanged() to have any effect so that other items knobs are left
    // untouched.
    ++_imp->common->masterKnobUpdatesBlocked;
    for (std::list<KnobIWPtr>::const_iterator it = _imp->common->perItemMasterKnobs.begin(); it != _imp->common->perItemMasterKnobs.end(); ++it) {
        KnobIPtr masterKnob = it->lock();
        if (!masterKnob) {
            continue;
        }
        if (masterKnob->getName() != itemKnob->getName()) {
            continue;
        }
        masterKnob->copyKnob(itemKnob);
    }
    --_imp->common->masterKnobUpdatesBlocked;
}

void
KnobItemsTable::endSelection(TableChangeReasonEnum reason)
{

    std::list<KnobTableItemPtr> itemsAdded, itemsRemoved;
    {
        // Avoid recursions
        QMutexLocker k(&_imp->common->selectionLock);
        if (_imp->common->selectionRecursion > 0) {
            _imp->common->itemsRemovedFromSelection.clear();
            _imp->common->newItemsInSelection.clear();
            return;
        }
        if ( _imp->common->itemsRemovedFromSelection.empty() && _imp->common->newItemsInSelection.empty() ) {
            return;
        }

        ++_imp->common->selectionRecursion;

        // Remove from selection and unslave from master knobs
        for (std::set<KnobTableItemPtr>::const_iterator it = _imp->common->itemsRemovedFromSelection.begin(); it != _imp->common->itemsRemovedFromSelection.end(); ++it) {
            for (std::list<KnobTableItemWPtr>::iterator it2 = _imp->common->selectedItems.begin(); it2!= _imp->common->selectedItems.end(); ++it2) {
                if ( it2->lock() == *it) {
                    itemsRemoved.push_back(*it);
                    _imp->common->selectedItems.erase(it2);
                    break;
                }

            }

        }

        // Add to selection (if not already selected) and slave to master knobs
        for (std::set<KnobTableItemPtr>::const_iterator it = _imp->common->newItemsInSelection.begin(); it != _imp->common->newItemsInSelection.end(); ++it) {
            bool found = false;
            for (std::list<KnobTableItemWPtr>::iterator it2 = _imp->common->selectedItems.begin(); it2!= _imp->common->selectedItems.end(); ++it2) {
                if ( it2->lock() == *it) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                itemsAdded.push_back(*it);
                _imp->common->selectedItems.push_back(*it);
            }
        }


        // For each master knobs, look in selected items for a knob with the same script-name. If found, then slave the item's
        // knob to the master knob.
        for (std::list<KnobIWPtr>::iterator it = _imp->common->perItemMasterKnobs.begin(); it != _imp->common->perItemMasterKnobs.end(); ++it) {
            KnobIPtr masterKnob = it->lock();
            if (!masterKnob) {
                continue;
            }

            // Get the number of selected items with the given knob
            int nItemsWithKnob = 0;
            for (std::list<KnobTableItemWPtr>::iterator it2 = _imp->common->selectedItems.begin(); it2!= _imp->common->selectedItems.end(); ++it2) {
                KnobTableItemPtr item = it2->lock();
                if (!item) {
                    continue;
                }
                KnobIPtr itemKnob = item->getKnobByName(masterKnob->getName());
                if (itemKnob && itemKnob->isEnabled()) {
                    ++nItemsWithKnob;
                }
            }

            // The master knob is enabled as long as the selection contains at least 1 item with the given knob.
            masterKnob->setEnabled(nItemsWithKnob > 0);

            // The master knob is set "dirty" if multiple selected items have the given knob, to indicate to the user
            // that modifying the master knob modifies all selected items.
            masterKnob->setKnobSelectedMultipleTimes(nItemsWithKnob > 1);

            // We need this otherwise the copyKnob() function would emit the value changed signal of the master knob
            // which would in turn call onMasterKnobValueChanged and recurse.
            ++_imp->common->masterKnobUpdatesBlocked;

            for (std::set<KnobTableItemPtr>::const_iterator it = _imp->common->newItemsInSelection.begin(); it != _imp->common->newItemsInSelection.end(); ++it) {
                KnobIPtr itemKnob = (*it)->getKnobByName(masterKnob->getName());
                if (!itemKnob) {
                    continue;
                }
                // Ensure the master knob reflects the state of the last item that was added to the selection.
                // We ensure the state change does not affect already slaved knobs otherwise all items would
                // get the value of the item we are copying.
                masterKnob->copyKnob(itemKnob);

                // Connect the item knob's value changed signal so that we can refresh the master knob when it changes
                KnobSignalSlotHandler* handler = itemKnob->getSignalSlotHandler().get();
                if (handler) {
                    QObject::connect(handler, SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onSelectionKnobValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), Qt::UniqueConnection);
                }
            }

            for (std::set<KnobTableItemPtr>::const_iterator it = _imp->common->itemsRemovedFromSelection.begin(); it != _imp->common->itemsRemovedFromSelection.end(); ++it) {
                KnobIPtr itemKnob = (*it)->getKnobByName(masterKnob->getName());
                if (!itemKnob) {
                    continue;
                }

                // Disconnect previous connection established above.
                KnobSignalSlotHandler* handler = itemKnob->getSignalSlotHandler().get();
                if (handler) {
                    QObject::disconnect(handler, SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onSelectionKnobValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum)));
                }
            }

            --_imp->common->masterKnobUpdatesBlocked;

        } // for all master knobs


        _imp->common->itemsRemovedFromSelection.clear();
        _imp->common->newItemsInSelection.clear();

    } //  QMutexLocker k(&_imp->selectionLock);

    Q_EMIT selectionChanged(itemsAdded, itemsRemoved, reason);

    // Run the Python callback if it is set
    NodePtr node = getNode();
    if (node) {
        node->runAfterTableItemsSelectionChangedCallback(itemsRemoved, itemsAdded, reason);
    }

    QMutexLocker k(&_imp->common->selectionLock);
    --_imp->common->selectionRecursion;

} // endSelection


void
KnobItemsTable::declareItemsToPython()
{
    assert(QThread::currentThread() == qApp->thread());

    _imp->common->pythonPrefix = getTablePythonPrefix();

    NodePtr node = getNode();
    if (!node) {
        return;
    }
    std::string appID = node->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string script = nodeFullName + "." + _imp->common->pythonPrefix + " = " + nodeFullName + ".getItemsTable()\n";
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
    return _imp->common->pythonPrefix;
}

void
KnobItemsTable::removeItemAsPythonField(const KnobTableItemPtr& item)
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    if (_imp->common->pythonPrefix.empty()) {
        return;
    }
    std::string appID = node->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::stringstream ss;
    ss << "del " << nodeFullName << "." << _imp->common->pythonPrefix << "." << item->getFullyQualifiedName() << "\n";
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
    if (_imp->common->pythonPrefix.empty()) {
        return;
    }
    std::string appID = node->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string err;
    std::string itemName = item->getFullyQualifiedName();
    std::stringstream ss;

    ss << nodeFullName << "." << _imp->common->pythonPrefix << "." << itemName << " = ";
    ss << nodeFullName << "." << _imp->common->pythonPrefix << ".getItemByFullyQualifiedScriptName(\"" << itemName + "\")\n";


    // Declare its knobs
    const KnobsVec& knobs = item->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss << nodeFullName << "." << _imp->common->pythonPrefix << "." << itemName << "." << (*it)->getName() << " = ";
        ss << nodeFullName << "." << _imp->common->pythonPrefix << "." << itemName << ".getParam(\"" << (*it)->getName() << "\")\n";
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
    for (std::list<KnobIWPtr>::const_iterator it = _imp->common->perItemMasterKnobs.begin(); it!=_imp->common->perItemMasterKnobs.end(); ++it) {
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
    KnobIPtr ret = masterKnob->createDuplicateOnHolder(item, KnobPagePtr(), KnobGroupPtr(), -1 /*index in parent*/, KnobI::eDuplicateKnobTypeCopy /*dupType*/, paramName, masterKnob->getLabel(), masterKnob->getHintToolTip(), false /*refreshParamsGui*/, false /*isUserKnob*/);
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


bool
KnobTableItem::splitView(ViewIdx view)
{
    if (!AnimatingObjectI::splitView(view)) {
        return false;
    }

    {
        QMutexLocker k(&_imp->common->lock);

        CurvePtr& foundCurve = _imp->animationCurves[view];
        if (!foundCurve) {
            foundCurve.reset(new Curve);
            foundCurve->setYComponentMovable(false);
        }
    }
    Q_EMIT availableViewsChanged();
    return true;

} // splitView

bool
KnobTableItem::unSplitView(ViewIdx view)
{
    if (!AnimatingObjectI::unSplitView(view)) {
        return false;
    }
    {
        QMutexLocker k(&_imp->common->lock);
        PerViewAnimationCurveMap::iterator foundView = _imp->animationCurves.find(view);
        if (foundView != _imp->animationCurves.end()) {
            _imp->animationCurves.erase(foundView);
        }
    }
    Q_EMIT availableViewsChanged();
    return true;
} // unSplitView

ViewIdx
KnobTableItem::getCurrentRenderView() const
{
    KnobItemsTablePtr m = _imp->common->model.lock();
    if (!m) {
        return ViewIdx(0);
    }
    return m->getNode()->getEffectInstance()->getCurrentRenderView();
}


CurvePtr
KnobTableItem::getAnimationCurve(ViewIdx idx, DimIdx /*dimension*/) const
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(idx);
    PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
    if (foundView != _imp->animationCurves.end()) {
        return foundView->second;
    }
    return CurvePtr();
}

ValueChangedReturnCodeEnum
KnobTableItem::setKeyFrameInternal(TimeValue time,
                                  ViewSetSpec view,
                                  KeyFrame* newKey)
{
    // Private - should not lock
    assert(!_imp->common->lock.tryLock());

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
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
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
KnobTableItem::setKeyFrame(TimeValue time,
                           ViewSetSpec view,
                           KeyFrame* newKey)
{
    ValueChangedReturnCodeEnum ret;
    {
        QMutexLocker k(&_imp->common->lock);
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
        QMutexLocker k(&_imp->common->lock);
        for (std::list<double>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            ValueChangedReturnCodeEnum ret = setKeyFrameInternal(TimeValue(*it), view, newKeys ? &key : 0);
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
KnobTableItem::deleteAnimationConditional(TimeValue time, ViewSetSpec view, bool before)
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
                    onKeyFrameRemoved(TimeValue(*it2), it->first);
                }
                std::list<double> keysAdded;
                Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, it->first);
            }
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
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
                    onKeyFrameRemoved(TimeValue(*it2), it->first);
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
        QMutexLocker k(&_imp->common->lock);
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
            onKeyFrameSet(TimeValue(*it), view);
        }
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(TimeValue(*it), view);
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
        QMutexLocker k(&_imp->common->lock);
        try {
            for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
                curve->removeKeyFrameWithTime(TimeValue(*it));
                keysRemoved.push_back(*it);
            }
        } catch (const std::exception & /*e*/) {
        }
    }
    if (!keysRemoved.empty()) {
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(TimeValue(*it), view);
        }
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
}

void
KnobTableItem::deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/)
{
    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            deleteValuesAtTimeInternal(times, it->first, it->second);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
        PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
        if (foundView != _imp->animationCurves.end()) {
            deleteValuesAtTimeInternal(times, view_i, foundView->second);
        }
    }
}

bool
KnobTableItem::warpValuesAtTimeInternal(const std::list<double>& times, ViewIdx view, const CurvePtr& curve, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes)
{

    std::list<double> keysAdded, keysRemoved;
    {
        QMutexLocker k(&_imp->common->lock);
        std::vector<KeyFrame> newKeys;
        if ( !curve->transformKeyframesValueAndTime(times, warp, keyframes, &keysAdded, &keysRemoved) ) {
            return false;
        }

    }
    if (!keysAdded.empty() || !keysRemoved.empty()) {
        for (std::list<double>::const_iterator it = keysAdded.begin(); it!=keysAdded.end(); ++it) {
            onKeyFrameSet(TimeValue(*it), view);
        }
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(TimeValue(*it), view);
        }

        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
    return true;
}


bool
KnobTableItem::warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec /*dimension*/, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes)
{
    bool ok = false;
    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            ok |= warpValuesAtTimeInternal(times, it->first, it->second, warp, keyframes);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
        PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
        if (foundView != _imp->animationCurves.end()) {
            ok |= warpValuesAtTimeInternal(times, view_i, foundView->second, warp, keyframes);
        }
    }
    return ok;
}

void
KnobTableItem::removeAnimationInternal(ViewIdx view, const CurvePtr& curve)
{
    std::list<double> keysAdded, keysRemoved;
    {
        QMutexLocker k(&_imp->common->lock);

        KeyFrameSet keys = curve->getKeyFrames_mt_safe();
        for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            keysRemoved.push_back(it->getTime());
        }
        curve->clearKeyFrames();
    }
    if (!keysRemoved.empty()) {
        for (std::list<double>::const_iterator it = keysRemoved.begin(); it!=keysRemoved.end(); ++it) {
            onKeyFrameRemoved(TimeValue(*it), view);
        }

        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
}


void
KnobTableItem::removeAnimation(ViewSetSpec view, DimSpec /*dimensions*/, ValueChangedReasonEnum /*reason*/)
{

    if (view.isAll()) {
        for (PerViewAnimationCurveMap::const_iterator it = _imp->animationCurves.begin(); it!=_imp->animationCurves.end(); ++it) {
            removeAnimationInternal(it->first, it->second);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view.value()));
        PerViewAnimationCurveMap::const_iterator foundView = _imp->animationCurves.find(view_i);
        if (foundView != _imp->animationCurves.end()) {
            removeAnimationInternal(view_i, foundView->second);
        }
    }

}

void
KnobTableItem::deleteAnimationBeforeTime(TimeValue time, ViewSetSpec view, DimSpec /*dimension*/)
{
    deleteAnimationConditional(time, view, true);
}

void
KnobTableItem::deleteAnimationAfterTime(TimeValue time, ViewSetSpec view, DimSpec /*dimension*/)
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
KnobTableItem::setLeftAndRightDerivativesAtTime(ViewSetSpec /*view*/, DimSpec /*dimension*/, TimeValue /*time*/, double /*left*/, double /*right*/)
{
    // user keyframes should always have linear interpolation
    return false;
}

bool
KnobTableItem::setDerivativeAtTime(ViewSetSpec /*view*/, DimSpec /*dimension*/, TimeValue /*time*/, double /*derivative*/, bool /*isLeft*/)
{
    // user keyframes should always have linear interpolation
    return false;
}


ValueChangedReturnCodeEnum
KnobTableItem::setDoubleValueAtTime(TimeValue time, double /*value*/, ViewSetSpec view, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* newKey)
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
KnobTableItem::setDoubleValueAtTimeAcrossDimensions(TimeValue time, const std::vector<double>& values, DimIdx /*dimensionStartIndex*/, ViewSetSpec view, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* retCodes)
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
KnobTableItem::setKeyFrameRecursively(TimeValue time, ViewSetSpec view)
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
KnobItemsTable::setMasterKeyframeOnSelectedItems(TimeValue time, ViewSetSpec view)
{
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        (*it)->setKeyFrameRecursively(time, view);
    }
}

static void removeKeyFrameRecursively(const KnobTableItemPtr& item, TimeValue time, ViewSetSpec view)
{
    if (item->getCanAnimateUserKeyframes()) {
        item->deleteValueAtTime(time, view, DimSpec::all(), eValueChangedReasonUserEdited);
    }
    if (item->isItemContainer()) {
        std::vector<KnobTableItemPtr> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            removeKeyFrameRecursively(children[i], time, view);
        }
    }
}

void
KnobItemsTable::removeMasterKeyframeOnSelectedItems(TimeValue time, ViewSetSpec view)
{
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        removeKeyFrameRecursively(*it, time, view);
    }
}

static void removeAnimationRecursively(const KnobTableItemPtr& item, ViewSetSpec view)
{
    if (item->getCanAnimateUserKeyframes()) {
        item->removeAnimation(view, DimSpec::all(), eValueChangedReasonUserEdited);
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
KnobTableItem::getMasterKeyframesCount(ViewIdx view) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return 0;
    }
    return curve->getKeyFramesCount();
}

bool
KnobTableItem::getMasterKeyframe(int index, ViewIdx view, KeyFrame* k) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return false;
    }
    return curve->getKeyFrameWithIndex(index, k);
}

bool
KnobTableItem::getHasMasterKeyframe(TimeValue time, ViewIdx view) const
{
    CurvePtr curve = getAnimationCurve(view, DimIdx(0));
    if (!curve) {
        return -1;
    }
    KeyFrame k;
    return curve->getKeyFrameWithTime(time, &k);
}

void
KnobTableItem::getMasterKeyFrameTimes(ViewIdx view, std::set<double>* times) const
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
KnobTableItem::hasMasterKeyframeAtTime(TimeValue time, ViewIdx view) const
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
                                  TimeValue time,
                                  ViewIdx view,
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
KnobTableItem::getPreviousMasterKeyframeTime(TimeValue time, ViewIdx view) const
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
KnobTableItem::getNextMasterKeyframeTime(TimeValue time, ViewIdx view) const
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
KnobItemsTable::goToPreviousMasterKeyframe(ViewIdx view)
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    TimeValue time = TimeValue(node->getApp()->getTimeLine()->currentFrame());
    double minTime = -std::numeric_limits<double>::infinity();
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        findOutNearestKeyframeRecursively(*it, true, time, view, &minTime);
    }
    if (minTime != -std::numeric_limits<double>::infinity()) {
        node->getApp()->setLastViewerUsingTimeline( NodePtr() );
        node->getApp()->getTimeLine()->seekFrame(minTime, false, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);
    }
}

void
KnobItemsTable::goToNextMasterKeyframe(ViewIdx view)
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    TimeValue time = TimeValue(node->getApp()->getTimeLine()->currentFrame());
    double minTime = std::numeric_limits<double>::infinity();
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        findOutNearestKeyframeRecursively(*it, false, time, view, &minTime);
    }
    if (minTime != std::numeric_limits<double>::infinity()) {
        node->getApp()->setLastViewerUsingTimeline( NodePtr() );
        node->getApp()->getTimeLine()->seekFrame(minTime, false, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);
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

    assert(_imp->common->topLevelItems.empty());

    for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = serialization->items.begin(); it != serialization->items.end(); ++it) {
        KnobTableItemPtr item = createItemFromSerialization(*it);
        if (item) {
            addItem(item, KnobTableItemPtr(), eTableChangeReasonInternal);
        }
    }
}

void
KnobTableItem::refreshAfterTimeChange(bool isPlayback, TimeValue time)
{
    // Since the same  knob may appear across multiple columns, ensure it is refreshed once
    std::set<KnobIPtr> updatedKnobs;

    QMutexLocker k(&_imp->common->lock);
    for (std::size_t i = 0; i < _imp->common->columns.size(); ++i) {
        KnobIPtr colKnob = _imp->common->columns[i].knob.lock();
        if (colKnob) {
            std::pair<std::set<KnobIPtr>::iterator, bool> ok = updatedKnobs.insert(colKnob);
            if (ok.second) {
                colKnob->onTimeChanged(isPlayback, time);
            }
        }
    }
    for (std::vector<KnobTableItemPtr>::const_iterator it = _imp->common->children.begin(); it != _imp->common->children.end(); ++it) {
        (*it)->refreshAfterTimeChange(isPlayback, time);
    }
}

void
KnobItemsTable::refreshAfterTimeChange(bool isPlayback, TimeValue time)
{
    QMutexLocker k(&_imp->common->topLevelItemsLock);
    for (std::size_t i = 0; i < _imp->common->topLevelItems.size(); ++i) {
        _imp->common->topLevelItems[i]->refreshAfterTimeChange(isPlayback, time);
    }
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_KnobItemsTable.cpp"
