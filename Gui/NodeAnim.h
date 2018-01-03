/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_NODEANIM_H
#define NATRON_GUI_NODEANIM_H


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include <QObject>

#include "Gui/AnimItemBase.h"
#include "Global/GlobalDefines.h"
NATRON_NAMESPACE_ENTER

class NodeAnimPrivate;
class NodeAnim : public QObject,  public boost::enable_shared_from_this<NodeAnim>, public KnobsHolderAnimBase
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON


    NodeAnim(const AnimationModulePtr& model,
             const NodeGuiPtr &nodeGui);

public:

    static NodeAnimPtr create(const AnimationModulePtr& model,
                              AnimatedItemTypeEnum itemType,
                              const NodeGuiPtr &nodeGui)
    {
        NodeAnimPtr ret(new NodeAnim(model, nodeGui));
        ret->initialize(itemType);
        return ret;
    }


    virtual ~NodeAnim();

    QTreeWidgetItem * getTreeItem() const;

    AnimationModulePtr getModel() const;


    NodeGuiPtr getNodeGui() const;
    NodePtr getInternalNode() const;

    virtual const std::vector<KnobAnimPtr>& getKnobs() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    const std::vector<TableItemAnimPtr>& getTopLevelItems() const;

    AnimatedItemTypeEnum getItemType() const;

    bool isTimeNode() const;

    bool isRangeDrawingEnabled() const;

    bool canContainOtherNodeContexts() const;
    bool containsNodeContext() const;

    virtual void refreshVisibility() OVERRIDE FINAL;

    void refreshFrameRange();

    RangeD getFrameRange() const;

    TableItemAnimPtr findTableItem(const KnobTableItemPtr& item) const;


public Q_SLOTS:

    void onFrameRangeKnobChanged();

    void onNodeLabelChanged(const QString &oldName, const QString& newName);

    void onTableItemRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum);
    
    void onTableItemInserted(int index, const KnobTableItemPtr& item, TableChangeReasonEnum);

private:


    void initialize(AnimatedItemTypeEnum itemType);

    void initializeKnobsAnim();
    
    void initializeTableItems();
    
    boost::scoped_ptr<NodeAnimPrivate> _imp;
};

inline NodeAnimPtr toNodeAnim(const KnobsHolderAnimBasePtr& p)
{
    return boost::dynamic_pointer_cast<NodeAnim>(p);
}


NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_NODEANIM_H
