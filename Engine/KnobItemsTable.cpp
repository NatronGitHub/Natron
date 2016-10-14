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

    void linkItemKnobsToGuiKnobs(const KnobTableItemPtr& markers, bool multipleItemsSelected, bool slave);

};

struct ColumnDesc
{
    std::string columnName;

    // If the columnName is the script-name of a knob, we hold a weak ref to the knob for faster access later on
    KnobIWPtr knob;

    // The dimension in the knob or -1
    int dimensionIndex;
};


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
    std::string scriptName, label;

    // Locks all members
    mutable QMutex lock;

    // List of keyframe times set by the user
    CurvePtr animation;

    KnobTableItemPrivate(const KnobItemsTablePtr& model)
    : parent()
    , children()
    , columns(model->getColumnsCount())
    , model(model)
    , scriptName()
    , label()
    , lock()
    , animation(new Curve)
    {
        
    }
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
KnobItemsTable::addTopLevelItem(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    insertTopLevelItem(-1, item, reason);
}

void
KnobItemsTable::insertTopLevelItem(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    
    removeItem(item, reason);
    
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

    if (!getPythonPrefix().empty()) {
        declareItemAsPythonField(item);
    }

    Q_EMIT topLevelItemInserted(index, item, reason);
    
}

std::vector<KnobTableItemPtr>
KnobItemsTable::getTopLevelItems() const
{
    QMutexLocker k(&_imp->topLevelItemsLock);
    return _imp->topLevelItems;
}

void
KnobItemsTable::removeItem(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    KnobTableItemPtr parent = item->getParent();
    if (parent) {
        parent->removeChild(item, reason);
    } else {
        bool removed = false;
        if (!getPythonPrefix().empty()) {
            removeItemAsPythonField(item);
        }
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
            Q_EMIT topLevelItemRemoved(item, reason);
        }
    }
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
KnobTableItem::setLabel(const std::string& label, TableChangeReasonEnum reason)
{
    {
        QMutexLocker k(&_imp->lock);
        _imp->label = label;
    }
    Q_EMIT labelChanged(QString::fromUtf8(label.c_str()), reason);
}

std::string
KnobTableItem::getLabel() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->label;
}


bool
KnobTableItem::setScriptName(const std::string& name)
{

    if ( name.empty() ) {
        return false;
    }

    std::string currentName;
    {
        QMutexLocker l(&_imp->lock);
        currentName = _imp->scriptName;
    }

    // Make sure the script-name is Python compliant
    std::string cpy = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(name);

    if ( cpy.empty() || cpy == currentName) {
        return false;
    }

    KnobItemsTablePtr model = getModel();
    KnobTableItemPtr existingItem = model->getItemByScriptName(name);
    if ( existingItem && (existingItem.get() != this) ) {
        return false;
    }

    KnobTableItemPtr thisShared = toKnobTableItem(shared_from_this());
    if (!currentName.empty()) {
        model->removeItemAsPythonField(thisShared);
    }

    {
        QMutexLocker l(&_imp->lock);
        _imp->scriptName = cpy;
    }

    model->declareItemAsPythonField(thisShared);
    
    return true;
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
KnobTableItem::insertChild(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    if (!isItemContainer()) {
        return;
    }
    assert(_imp->model.lock()->getType() == KnobItemsTable::eKnobItemsTableTypeTree);

    item->getModel()->removeItem(item, reason);

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

    KnobItemsTablePtr model = getModel();
    if (!model->getPythonPrefix().empty()) {
        model->declareItemAsPythonField(item);
    }

    {
        QMutexLocker k(&item->_imp->lock);
        item->_imp->parent = toKnobTableItem(shared_from_this());
    }


    Q_EMIT childInserted(insertedIndex, item, reason);
}

void
KnobTableItem::removeChild(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    if (!isItemContainer()) {
        return;
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
    if (removed) {
        Q_EMIT childRemoved(item, reason);
    }
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
KnobTableItem::clearChildren(TableChangeReasonEnum reason)
{
    std::vector<KnobTableItemPtr> children = getChildren();
    for (std::vector<KnobTableItemPtr>::iterator it = children.begin(); it!=children.end(); ++it) {
        Q_EMIT childRemoved(*it, reason);
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

    clearChildren(eTableChangeReasonInternal);

    KnobItemsTablePtr model = _imp->model.lock();
    for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = serialization->children.begin(); it!=serialization->children.end(); ++it) {
        KnobTableItemPtr child = model->createItemFromSerialization(*it);
        if (child) {
            _imp->children.push_back(child);
        }
    }
}

static KnobTableItemPtr
getItemByScriptNameInternal(const std::string& scriptName, const std::vector<KnobTableItemPtr>& items)
{
    for (std::vector<KnobTableItemPtr>::const_iterator it = items.begin(); it!=items.end(); ++it) {
        if ((*it)->getScriptName_mt_safe() == scriptName) {
            return *it;
        }
        // Recurse
        std::vector<KnobTableItemPtr> children = (*it)->getChildren();
        if (!children.empty()) {
            KnobTableItemPtr childrenRet = getItemByScriptNameInternal(scriptName, children);
            if (childrenRet) {
                return childrenRet;
            }
        }
    }
    return KnobTableItemPtr();
}

KnobTableItemPtr
KnobItemsTable::getItemByScriptName(const std::string& scriptName) const
{
    std::vector<KnobTableItemPtr> topLevel = getTopLevelItems();
    return getItemByScriptNameInternal(scriptName, topLevel);
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
KnobItemsTablePrivate::linkItemKnobsToGuiKnobs(const KnobTableItemPtr& item, bool multipleItemsSelected, bool slave)
{
    if (perItemMasterKnobs.empty()) {
        return;
    }
    const KnobsVec& itemKnobs = item->getKnobs();
    for (KnobsVec::const_iterator it = itemKnobs.begin(); it != itemKnobs.end(); ++it) {
        // Find the knob in the TrackerContext knobs
        KnobIPtr found;
        for (std::list<KnobIWPtr >::const_iterator it2 = perItemMasterKnobs.begin(); it2 != perItemMasterKnobs.end(); ++it2) {
            KnobIPtr k = it2->lock();
            if ( k->getName() == (*it)->getName() ) {
                found = k;
                break;
            }
        }

        if (!found) {
            continue;
        }

        //Clone current state only for the last marker
        found->blockListenersNotification();
        found->copyKnob(*it);
        found->unblockListenersNotification();

        //Slave internal knobs

        if (slave) {
            (*it)->slaveTo(found);
        } else {
            (*it)->unSlave(DimSpec::all(), ViewSetSpec::all(), !multipleItemsSelected);
        }

    }

}

void
KnobItemsTable::endSelection(TableChangeReasonEnum reason)
{

    std::list<KnobTableItemPtr> itemsAdded, itemsRemoved;
    {
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



        // Slave newly selected knobs
        bool selectionIsDirty = _imp->selectedItems.size() > 1;
        bool selectionEmpty = _imp->selectedItems.empty();


        for (std::set<KnobTableItemPtr>::const_iterator it = _imp->newItemsInSelection.begin(); it != _imp->newItemsInSelection.end(); ++it) {
            _imp->linkItemKnobsToGuiKnobs(*it, selectionIsDirty, true);
        }
        for (std::set<KnobTableItemPtr>::const_iterator it = _imp->itemsRemovedFromSelection.begin(); it != _imp->itemsRemovedFromSelection.end(); ++it) {
            _imp->linkItemKnobsToGuiKnobs(*it, selectionIsDirty, false);
        }

        _imp->itemsRemovedFromSelection.clear();
        _imp->newItemsInSelection.clear();


        for (std::list<KnobIWPtr>::iterator it = _imp->perItemMasterKnobs.begin(); it != _imp->perItemMasterKnobs.end(); ++it) {
            KnobIPtr k = it->lock();
            if (!k) {
                continue;
            }
            k->setEnabled(!selectionEmpty);
            k->setDirty(selectionIsDirty);
        }
    } //  QMutexLocker k(&_imp->selectionLock);

    Q_EMIT selectionChanged(itemsAdded, itemsRemoved, reason);

    QMutexLocker k(&_imp->selectionLock);
    --_imp->selectionRecursion;

}


void
KnobItemsTable::declareItemsToPython(const std::string& pythonPrefix)
{
    assert(QThread::currentThread() == qApp->thread());

    _imp->pythonPrefix = pythonPrefix;
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


//////// Animation implementation
AnimatingObjectI::KeyframeDataTypeEnum
KnobTableItem::getKeyFrameDataType() const
{
    return eKeyframeDataTypeNone;
}

CurvePtr
KnobTableItem::getAnimationCurve(ViewGetSpec /*idx*/, DimIdx /*dimension*/) const
{
    return _imp->animation;
}

ValueChangedReturnCodeEnum
KnobTableItem::setKeyFrameInternal(double time,
                                  ViewSetSpec /*view*/,
                                  KeyFrame* newKey)
{
    // Private - should not lock
    assert(!_imp->lock.tryLock());

    if (newKey) {
        newKey->setTime(time);

        // Value is not used by this class
        newKey->setValue(0);
    }

    KeyFrame existingKey;
    ValueChangedReturnCodeEnum ret;
    bool hasExistingKey = _imp->animation->getKeyFrameWithTime(time, &existingKey);
    if (hasExistingKey) {
        if (newKey) {
            *newKey = existingKey;
        }
        ret = eValueChangedReturnCodeNothingChanged;
    } else {
        KeyFrame k(time, 0);
        k.setInterpolation(eKeyframeTypeLinear);
        if (newKey) {
            *newKey = k;
        }
        bool newKeyFrame = _imp->animation->addKeyFrame(k);
        if (newKeyFrame) {
            ret = eValueChangedReturnCodeKeyframeAdded;
        } else {
            ret = eValueChangedReturnCodeNothingChanged;
        }
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
KnobTableItem::deleteAnimationConditional(double time, ViewSetSpec /*view*/, bool before)
{
    std::list<double> keysRemoved;
    {
        QMutexLocker k(&_imp->lock);
        if (before) {
            _imp->animation->removeKeyFramesBeforeTime(time, &keysRemoved);
        } else {
            _imp->animation->removeKeyFramesAfterTime(time, &keysRemoved);
        }

    }
    if (!keysRemoved.empty()) {
        std::list<double> keysAdded;
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
}

bool
KnobTableItem::cloneCurve(ViewIdx /*view*/, DimIdx /*dimension*/, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* /*stringAnimation*/)
{
    std::list<double> keysAdded, keysRemoved;
    bool hasChanged;
    {
        QMutexLocker k(&_imp->lock);
        KeyFrameSet oldKeys = _imp->animation->getKeyFrames_mt_safe();
        hasChanged = _imp->animation->cloneAndCheckIfChanged(curve, offset, range);
        if (hasChanged) {
            KeyFrameSet newKeys = _imp->animation->getKeyFrames_mt_safe();


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
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
    return hasChanged;
}

void
KnobTableItem::deleteValuesAtTime(const std::list<double>& times, ViewSetSpec /*view*/, DimSpec /*dimension*/)
{
    
    std::list<double> keysRemoved, keysAdded;
    {
        QMutexLocker k(&_imp->lock);
        try {
            for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
                _imp->animation->removeKeyFrameWithTime(*it);
                keysRemoved.push_back(*it);
            }
        } catch (const std::exception & /*e*/) {
        }
    }
    if (!keysRemoved.empty()) {
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
}

bool
KnobTableItem::warpValuesAtTime(const std::list<double>& times, ViewSetSpec /*view*/,  DimSpec /*dimension*/, const Curve::KeyFrameWarp& warp, bool allowKeysOverlap, std::vector<KeyFrame>* keyframes)
{
    std::list<double> keysAdded, keysRemoved;
    {
        QMutexLocker k(&_imp->lock);
        std::vector<KeyFrame> newKeys;
        if ( !_imp->animation->transformKeyframesValueAndTime(times, warp, allowKeysOverlap, keyframes, &keysAdded, &keysAdded) ) {
            return false;
        }

    }
    if (!keysAdded.empty() || !keysRemoved.empty()) {
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
    }
    return true;
}

void
KnobTableItem::removeAnimation(ViewSetSpec /*view*/, DimSpec /*dimensions*/)
{
    std::list<double> keysAdded, keysRemoved;
    {
        QMutexLocker k(&_imp->lock);

        KeyFrameSet keys = _imp->animation->getKeyFrames_mt_safe();
        for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            keysRemoved.push_back(it->getTime());
        }
        _imp->animation->clearKeyFrames();
    }
    if (!keysRemoved.empty()) {
        Q_EMIT curveAnimationChanged(keysAdded, keysRemoved, ViewIdx(0));
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


NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_KnobItemsTable.cpp"
