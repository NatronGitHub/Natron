//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#ifndef MULTIINSTANCEPANEL_H
#define MULTIINSTANCEPANEL_H


#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "Engine/Knob.h"

namespace Natron
{
    class Node;
}
class NodeGui;
class QVBoxLayout;
class QHBoxLayout;
class QItemSelection;
class TableItem;
class Button_Knob;
class Gui;
/**
* @brief This class represents a multi-instance settings panel.
**/
struct MultiInstancePanelPrivate;
class MultiInstancePanel : public QObject, public NamedKnobHolder
{
    
    Q_OBJECT
    
public:
    
    MultiInstancePanel(const boost::shared_ptr<NodeGui>& node);
    
    virtual ~MultiInstancePanel();
    
    void createMultiInstanceGui(QVBoxLayout* layout);
    
    bool isGuiCreated() const;

    void addRow(const boost::shared_ptr<Natron::Node>& node);
    
    void removeRow(int index);
    
    int getNodeIndex(const boost::shared_ptr<Natron::Node>& node) const;
    
    const std::list< std::pair<boost::shared_ptr<Natron::Node>,bool > >& getInstances() const;
    
    virtual std::string getName_mt_safe() const OVERRIDE FINAL;
    
    boost::shared_ptr<Natron::Node> getMainInstance() const;
    
    void getSelectedInstances(std::list<Natron::Node*>* instances) const;

    void resetAllInstances();
    
    boost::shared_ptr<KnobI> getKnobForItem(TableItem* item,int* dimension) const;
    
    Gui* getGui() const;
    
    virtual void setIconForButton(Button_Knob* /*knob*/) {}

    boost::shared_ptr<Natron::Node> createNewInstance();
    
    void selectNode(const boost::shared_ptr<Natron::Node>& node,bool addToSelection);
    
    void selectNodes(const std::list<Natron::Node*>& nodes,bool addToSelection);
    
    void removeNodeFromSelection(const boost::shared_ptr<Natron::Node>& node);
    
    void clearSelection();
    
public slots:

    void onAddButtonClicked();
    
    void onRemoveButtonClicked();
    
    void onSelectAllButtonClicked();
    
    void onSelectionChanged(const QItemSelection& oldSelection,const QItemSelection& newSelection);
    
    void onItemDataChanged(TableItem* item);
    
    void onCheckBoxChecked(bool checked);
    
    void onDeleteKeyPressed();
    
    void onInstanceKnobValueChanged(int dim);
    
    void resetSelectedInstances();
    
    
protected:
    
    virtual void appendExtraGui(QVBoxLayout* /*layout*/) {}
    virtual void appendButtons(QHBoxLayout* /*buttonLayout*/) {}
    
    boost::shared_ptr<Natron::Node> addInstanceInternal();

private:
    
    virtual void onButtonTriggered(Button_Knob* button);
    
    void resetInstances(const std::list<Natron::Node*>& instances);
    
    void removeInstancesInternal();
    
    virtual void evaluate(KnobI* knob,bool isSignificant,Natron::ValueChangedReason reason);
    
    virtual void initializeKnobs() OVERRIDE FINAL;
    
    
    
    boost::scoped_ptr<MultiInstancePanelPrivate> _imp;
    
};

struct TrackerPanelPrivate;
class TrackerPanel : public MultiInstancePanel
{
    Q_OBJECT
    
public:
    
    TrackerPanel(const boost::shared_ptr<NodeGui>& node);
    
    virtual ~TrackerPanel();
   
public slots:
    
    void onAverageTracksButtonClicked();
    
private:
    
    virtual void appendExtraGui(QVBoxLayout* layout) OVERRIDE FINAL;
    virtual void appendButtons(QHBoxLayout* buttonLayout) OVERRIDE FINAL;
    virtual void setIconForButton(Button_Knob* knob) OVERRIDE FINAL;
    
    virtual void onButtonTriggered(Button_Knob* button) OVERRIDE FINAL;
    
    void handleTrackNextAndPrevious(Button_Knob* button,const std::list<Natron::Node*>& selectedInstances);

    boost::scoped_ptr<TrackerPanelPrivate> _imp;
};


#endif // MULTIINSTANCEPANEL_H
