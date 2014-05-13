//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef ROTOPANEL_H
#define ROTOPANEL_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <QWidget>

#include "Global/GlobalDefines.h"

class QTreeWidgetItem;
class QWidget;
class NodeGui;
class RotoItem;
struct RotoPanelPrivate;
class RotoPanel : public QWidget
{
    
    Q_OBJECT
    
public:
    
    RotoPanel(NodeGui* n,QWidget* parent = 0);
    
    virtual ~RotoPanel();
    
    void onTreeOutOfFocusEvent();
    
    
public slots:
    
    void onGoToPrevKeyframeButtonClicked();
    
    void onGoToNextKeyframeButtonClicked();
    
    void onAddKeyframeButtonClicked();
    
    void onRemoveKeyframeButtonClicked();
    
    void onAddLayerButtonClicked();
    
    void onRemoveItemButtonClicked();
    
    ///This gets called when the selection changes internally in the RotoContext
    void onSelectionChanged(int reason);
    
    void onSelectedBezierKeyframeSet(int time);
    
    void onSelectedBezierKeyframeRemoved(int time);
    
    void onTimeChanged(SequenceTime time,int reason);
    
    ///A new item has been created internally
    void onItemInserted();
    
    ///An item was removed by the user
    void onItemRemoved(RotoItem* item);
    
    ///An item had its inverted state changed
    void onRotoItemInversionChanged();
    
    ///The user changed the current item in the tree
    void onCurrentItemChanged(QTreeWidgetItem* current,QTreeWidgetItem* previous);
    
    ///An item content was changed
    void onItemChanged(QTreeWidgetItem* item,int column);
    
    ///An item was clicked
    void onItemClicked(QTreeWidgetItem* item,int column);
    
    ///An item was double clicked
    void onItemDoubleClicked(QTreeWidgetItem* item,int column);
    
    ///Connected to QApplication::focusChanged to deal with an issue of the QTreeWidget
    void onFocusChanged(QWidget* old,QWidget*);
    
    ///This gets called when the selection in the tree has changed
    void onItemSelectionChanged();

private:
    
    void onSelectionChangedInternal();
    
    boost::scoped_ptr<RotoPanelPrivate> _imp;
};


#endif // ROTOPANEL_H
