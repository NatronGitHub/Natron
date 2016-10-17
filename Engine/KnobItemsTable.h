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

#ifndef NATRON_ENGINE_KNOBITEMSTABLE_H
#define NATRON_ENGINE_KNOBITEMSTABLE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <vector>
#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include <QMetaType>
#include <QString>

#include "Engine/Knob.h"

#include "Serialization/KnobSerialization.h"

/**
 * @brief String passed to setColumn to indicate that a column corresponds to the label
 **/
#define kKnobTableItemColumnLabel "KnobTableItemColumnLabel"

NATRON_NAMESPACE_ENTER;

/**
 * @class A KnobTableItem represents one row in an item view. Each column of the row can be the view of a Knob (or only
 * one of its dimensions). An item is identified by a script-name, and has a label that is the name visible on the Gui.
 * The item view can display the content of the label by passing kKnobTableItemColumnLabel
 * to the columnName parameter of setColumn.
 **/
struct KnobTableItemPrivate;
class KnobTableItem
: public NamedKnobHolder
, public SERIALIZATION_NAMESPACE::SerializableObjectBase
, public AnimatingObjectI
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON


public:

    KnobTableItem(const KnobItemsTablePtr& model);

    virtual ~KnobTableItem();

    KnobItemsTablePtr getModel() const;

    /**
     * @brief Returns the script-name of the item as it appears to Python
     **/
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL;
    
    /**
     * @brief Set the script-name of the item as it appears to Python. This function
     * returns false if the name is invalid
     **/
    bool setScriptName(const std::string& name);

    /**
     * @brief Same as getScriptName_mt_safe, except that if the item has a parent, it will recursively preprend the parent name
     * with a '.' separating each hierarchy level, e.g: "item1.child1.childchild1"
     **/
    std::string getFullyQualifiedName() const;

    /**
     * @brief Returns the label of the item as visible in the GUI
     **/
    std::string getLabel() const;

    /**
     * @brief Set the label of the item as visible in the GUI
     **/
    void setLabel(const std::string& label, TableChangeReasonEnum reason);

    /**
     * @brief If true, the item may receive children, otherwise it cannot have children.
     **/
    virtual bool isItemContainer() const = 0;
    
    /**
     * @brief Add a child to the item after any other children
     **/
    void addChild(const KnobTableItemPtr& item, TableChangeReasonEnum reason) {
        insertChild(-1, item, reason);
    }

    /**
     * @brief Inserts the given item as a child at the given index. This function cannot succeed if
     * isItemContainer() returns false. This will remove the item from its previous parent/model 
     * before inserting it as a child. This will emit the childInserted signal.
     **/
    void insertChild(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason);

    /**
     * @brief Removes given child if it exists.
     * @return True upon success, false otherwise.
     * Upon success, the childRemoved signal is emitted.
     **/
    bool removeChild(const KnobTableItemPtr& item, TableChangeReasonEnum reason);

    /**
     * @brief Removes all children. Same as calling removeChild on each children individually.
     **/
    void clearChildren(TableChangeReasonEnum reason);

    /**
     * @brief Returns a vector of all children of the item.
     **/
    std::vector<KnobTableItemPtr> getChildren() const;

    /**
     * @brief If this item is child of another item, this is its parent.
     **/
    KnobTableItemPtr getParent() const;

    /**
     * @brief If this item has a parent, returns the index of this item in the parent's children list.
     * If this item is a top-level item, returns the index of this item in the model top level items list.
     * This function returns -1 if the item is not in a model.
     **/
    int getIndexInParent() const;

    /**
     * @brief Set the content of a column. 
     * @param columnName If this is the script-name of a knob, then the cell represents the given dimension of the knob.
     * Otherwise this can be kKnobTableItemColumnLabel to indicate that the column should display the label.
     * @param dimension If set columnName is the script-name of a knob, dimension is the index of the dimension to represent 
     * of the knob in the case it is multi-dimensional. 
     * If dimension is -1, the column should represent all dimensions (Currently -1 is only supported for KnobColor)
     *
     * Note: if a column if not set to anything, it will be empty in the user interface
     **/
    void setColumn(int col, const std::string& columnName, int dimension);

    /**
     * @brief Return a knob that was previously set by setColumn at the given column col.
     * @param dimensionIndex[out] Set the dimension of the knob the given column should represent.
     **/
    KnobIPtr getColumnKnob(int col, int *dimensionIndex) const;

    /**
     * @brief Return the column name that was previously set by setColumn
     **/
    std::string getColumnName(int col) const;
    
    /**
     * @brief If a column was set to display the label with setColum, then this function will return the column
     * passed to setColumn, otherwise it returns -1
     **/
    int getLabelColumnIndex() const;
    
    /**
     * @brief If a column was set to display the content 
     * of the given dimension of the given knob,
     * then this function will return the column
     * passed to setColumn, otherwise it returns -1.
     **/
    int getKnobColumnIndex(const KnobIPtr& knob, int dimension) const;
    
    /**
     * @brief Returns the row index of the item in the model. Note that the item
     * MUST be in the model, otherwise this function returns -1
     **/
    int getItemRow() const;

    /**
     * @brief Serialization support. May be overlayded to serialize more data structures.
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE;
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE;

#pragma message WARN("KnobTableItem does not support multi-view for user keyframes")
    //////////// Overriden from AnimatingObjectI
    virtual KeyframeDataTypeEnum getKeyFrameDataType() const OVERRIDE FINAL;
    virtual CurvePtr getAnimationCurve(ViewGetSpec idx, DimIdx dimension) const OVERRIDE FINAL;
    virtual bool cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* stringAnimation) OVERRIDE;
    virtual void deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension) OVERRIDE;
    virtual bool warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, bool allowKeysOverlap, std::vector<KeyFrame>* keyframes = 0) OVERRIDE ;
    virtual void removeAnimation(ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void deleteAnimationBeforeTime(double time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void deleteAnimationAfterTime(double time, ViewSetSpec view, DimSpec dimension) OVERRIDE ;
    virtual void setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys = 0) OVERRIDE ;
    virtual bool setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, double time, double left, double right)  OVERRIDE WARN_UNUSED_RETURN;
    virtual bool setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, double time, double derivative, bool isLeft) OVERRIDE WARN_UNUSED_RETURN;
    
    virtual ValueChangedReturnCodeEnum setDoubleValueAtTime(double time, double value, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, KeyFrame* newKey = 0) OVERRIDE ;
    virtual void setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSetSpec view = ViewSetSpec::current(), DimSpec dimension = DimSpec(0), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<KeyFrame>* newKey = 0) OVERRIDE ;
    virtual void setDoubleValueAtTimeAcrossDimensions(double time, const std::vector<double>& values, DimIdx dimensionStartIndex = DimIdx(0), ViewSetSpec view = ViewSetSpec::current(), ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) OVERRIDE ;
    virtual void setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& keysPerDimension, ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited) OVERRIDE ;
    //////////// End from AnimatingObjectI

protected:

    /**
     * @brief Utility function to create a duplicate of a knob that was first registered on the KnobItemsTable
     * with addPerItemKnobMaster.
     **/
    template <typename KNOB_TYPE>
    boost::shared_ptr<KNOB_TYPE> createDuplicateOfTableKnob(const std::string& scriptName)
    {
        return boost::dynamic_pointer_cast<KNOB_TYPE>(createDuplicateOfTableKnobInternal(scriptName));
    }

private:

    KnobIPtr createDuplicateOfTableKnobInternal(const std::string& scriptName);


    template <typename T>
    void convertTimeValuePairListToTimeList(const std::list<TimeValuePair<T> >& inList,
                                       std::list<double>* outList)
    {
        for (typename std::list<TimeValuePair<T> >::const_iterator it = inList.begin(); it!=inList.end(); ++it) {
            outList->push_back(it->time);
        }
    }

    ValueChangedReturnCodeEnum setKeyFrame(double time,
                                           ViewSetSpec view,
                                           KeyFrame* newKey);

    ValueChangedReturnCodeEnum setKeyFrameInternal(double time,
                                                   ViewSetSpec view,
                                                   KeyFrame* newKey);

    void deleteAnimationConditional(double time, ViewSetSpec view, bool before);

    void setMultipleKeyFrames(const std::list<double>& keys, ViewSetSpec view,  std::vector<KeyFrame>* newKeys = 0);


protected:

    /**
     * @brief Implement to initialize the knobs of the item and add them to the table by calling setColumn
     **/
    virtual void initializeKnobs() OVERRIDE = 0;

    
    /**
     * @brief Returns what should be the default base-name for the item, e.g "Bezier" or "Brush" etc...
     * The model will then derive from this name to assign a unique script-name to the item.
     **/
    virtual std::string getBaseItemName() const = 0;
    
    /**
     * @brief Callback called when the item is removed from its parent item or from the table if this is a top-level item
     **/
    virtual void onItemRemovedFromParent() {}

Q_SIGNALS:

    void labelChanged(QString, TableChangeReasonEnum);
    void childRemoved(KnobTableItemPtr, TableChangeReasonEnum);
    void childInserted(int index, KnobTableItemPtr, TableChangeReasonEnum);
    void curveAnimationChanged(std::list<double> added, std::list<double> removed, ViewIdx view);

private:

    friend class KnobItemsTable;
    boost::scoped_ptr<KnobTableItemPrivate> _imp;
};

inline
KnobTableItemPtr toKnobTableItem(const KnobHolderPtr& holder)
{
    return boost::dynamic_pointer_cast<KnobTableItem>(holder);
}

/**
 * @class KnobItemsTable A class abstracting an item-view which can be either represented as a tree or table.
 * each item in the table corresponds to a row. If the model is set to be a tree, you may add children to items to
 * make-up an item hierarchy. If an item does not have a parent, it is considered a top-level item of the model. 
 * For example, in the Tracker node all tracks are top-level items of the model.
 **/
struct KnobItemsTablePrivate;
class KnobItemsTable : public QObject, public boost::enable_shared_from_this<KnobItemsTable>
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum KnobItemsTableTypeEnum
    {
        // Each item takes up a full row
        eKnobItemsTableTypeTable,

        // Items may be nested with parents as a tree
        eKnobItemsTableTypeTree
    };

    // See QAbstractItemView::SelectionMode
    enum TableSelectionModeEnum
    {
        eTableSelectionModeNoSelection,
        eTableSelectionModeSingleSelection,
        eTableSelectionModeMultiSelection,
        eTableSelectionModeExtendedSelection,
        eTableSelectionModeContiguousSelection
    };
    

public:
    
    KnobItemsTable(const KnobHolderPtr& originalHolder, KnobItemsTableTypeEnum type, int colsCount);


    virtual ~KnobItemsTable();

    /**
     * @brief Returns the knobs holder to which this table belongs to
     **/
    KnobHolderPtr getOriginalHolder() const;

    /**
     * @brief Returns the node corresponding to the holder
     **/
    NodePtr getNode() const;

    /**
     * Is it a tree or table ? E.g: Roto has a tree, and tracker has a Table
     * When a tree, items may have children
     **/
    KnobItemsTableTypeEnum getType() const;
    
    /**
     * @brief Return a identifier for the table so that when the table is serialize we don't attempt to 
     * deserialize it onto another table type. E.G: we don't want the user to drop the content of a tracker table
     * onto an item of the Roto
     **/
    virtual std::string getTableIdentifier() const = 0;

    /**
     * @brief Tells whether you implement the drag and drop handlers. If not the GUI will not support it
     **/
    void setSupportsDragAndDrop(bool supports);
    bool isDragAndDropSupported() const;

    /**
     * @brief If D&D is supported, tells whether dropping an item that is from another view should be possible
     **/
    void setDropSupportsExternalSources(bool supports);
    bool isDropFromExternalSourceSupported() const;

    /**
     * @brief The path where to look for icons
     **/
    void setIconsPath(const std::string& iconPath);
    const std::string& getIconsPath() const;

    /**
     * @brief This property holds whether all items have the same height.
     * This property should only be set to true if it is guaranteed that all items in the view has the same height.
     * This enables the view to do some optimizations.
     **/
    void setRowsHaveUniformHeight(bool uniform);
    bool getRowsHaveUniformHeight() const;

    /**
     * @brief Returns the number of columns
     **/
    int getColumnsCount() const;

    /**
     * @brief Set the column header text.
     **/
    void setColumnText(int col, const std::string& text);
    std::string getColumnText(int col) const;

    /**
     * @brief Set the column header icon.
     **/
    void setColumnIcon(int col, const std::string& iconFilePath);
    std::string getColumnIcon(int col) const;

    /**
     * @brief Set the selection mode. By default selection is eTableSelectionModeExtendedSelection
     **/
    void setSelectionMode(TableSelectionModeEnum mode);
    TableSelectionModeEnum getSelectionMode() const;

    /**
     * @brief Add/Insert a top-level item with no parent.
     **/
    void addTopLevelItem(const KnobTableItemPtr& item, TableChangeReasonEnum reason);
    void insertTopLevelItem(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason);

    std::vector<KnobTableItemPtr> getTopLevelItems() const;

    /**
     * @brief Remove the item from the model. The model will no longer hold a strong
     * reference to the item and may be deleted afterwards.
     **/
    void removeItem(const KnobTableItemPtr& item, TableChangeReasonEnum reason);

    /**
     * @brief  Create an item from a serialization.
     * The item will not yet be part of the model and must be added either as a top level item with addTopLevelItem
     * or to another item with addChild
     **/
    virtual KnobTableItemPtr createItemFromSerialization(const SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr& data) = 0;

    /**
     * @brief Attempts to find an existing item in the model by its script-name.
     **/
    KnobTableItemPtr getItemByScriptName(const std::string& scriptName) const;

    /**
     * @brief Returns true if the given item is selected
     **/
    bool isItemSelected(const KnobTableItemPtr& item) const;

    /**
     * @brief Get current selection
     **/
    std::list<KnobTableItemPtr> getSelectedItems() const;

    /**
     * @brief Selection support. If possible, bracket any change to selection to beginEditSelection/endEditSelection to avoid potential
     * costly redraws.
     **/
    void beginEditSelection();
    void endEditSelection(TableChangeReasonEnum reason);
    void addToSelection(const std::list<KnobTableItemPtr >& items, TableChangeReasonEnum reason);
    void addToSelection(const KnobTableItemPtr& item, TableChangeReasonEnum reason);
    void removeFromSelection(const std::list<KnobTableItemPtr >& items, TableChangeReasonEnum reason);
    void removeFromSelection(const KnobTableItemPtr& item, TableChangeReasonEnum reason);
    void clearSelection(TableChangeReasonEnum reason);
    void selectAll(TableChangeReasonEnum reason);

    /**
     * @brief Add a master knob that is visible on the main user interface of the node (outside of the table) onto which
     * a corresponding item's knob with the same script-name in the table should slave to. For example in the Roto node settings panel
     * all items are slaved to knobs in the panels such as opacity, color etc...
     **/
    void addPerItemKnobMaster(const KnobIPtr& masterKnob);

    /**
     * @brief Call declareItemAsPythonField on all items in the model that are attributes of the given python prefix.
     * The pythonPrefix will be an attribute of the node object itself.
     **/
    void declareItemsToPython(const std::string& pythonPrefix);

    /**
     * @brief Returns the python prefix under which each item is that has previously been set by declareItemsToPython.
     * Items will be of the following form "app.Roto1.roto.Layer1.bezier1"
     **/
    const std::string& getPythonPrefix() const;

    /**
     * @brief Remove the item from Python
     **/
    void removeItemAsPythonField(const KnobTableItemPtr& item);

    /**
     * @brief Add the item to Python
     **/
    void declareItemAsPythonField(const KnobTableItemPtr& item);


Q_SIGNALS:

    void selectionChanged(std::list<KnobTableItemPtr> addedToSelection, std::list<KnobTableItemPtr> removedFromSelection, TableChangeReasonEnum reason);
    void topLevelItemRemoved(KnobTableItemPtr, TableChangeReasonEnum);
    void topLevelItemInserted(int index, KnobTableItemPtr, TableChangeReasonEnum);

protected:
    
    std::string generateUniqueName(const std::string& name) const;

private:

    KnobIPtr createMasterKnobDuplicateOnItem(const KnobTableItemPtr& item, const std::string& paramName);

    void endSelection(TableChangeReasonEnum reason);

    friend class KnobTableItem;
    boost::scoped_ptr<KnobItemsTablePrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

Q_DECLARE_METATYPE(NATRON_NAMESPACE::KnobTableItemPtr)
Q_DECLARE_METATYPE(std::list<NATRON_NAMESPACE::KnobTableItemPtr>)
Q_DECLARE_METATYPE(NATRON_NAMESPACE::TableChangeReasonEnum)


#endif // NATRON_ENGINE_KNOBITEMSTABLE_H
