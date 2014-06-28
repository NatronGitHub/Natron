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

#include <QObject>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "Engine/Knob.h"

namespace Natron
{
    class Node;
}

class QVBoxLayout;
class QHBoxLayout;
class QTableWidgetItem;

/**
* @brief This class represents a multi-instance settings panel.
**/
struct MultiInstancePanelPrivate;
class MultiInstancePanel : public QObject, public KnobHolder
{
    
    Q_OBJECT
    
public:
    
    MultiInstancePanel(const boost::shared_ptr<Natron::Node>& node);
    
    virtual ~MultiInstancePanel();
    
    void createMultiInstanceGui(QVBoxLayout* layout);

public slots:

    void onAddButtonClicked();
    
    void onRemoveButtonClicked();
    
    void onSelectAllButtonClicked();
    
    void onItemDataChanged(QTableWidgetItem* item);
    
protected:
    
    virtual void appendExtraGui(QVBoxLayout* /*layout*/) {}
    virtual void appendButton(QHBoxLayout* /*buttonLayout*/) {}
    
private:
    
    virtual void evaluate(KnobI* /*knob*/,bool /*isSignificant*/) OVERRIDE FINAL {}
    
    virtual void initializeKnobs() OVERRIDE FINAL;
    
    
    
    boost::scoped_ptr<MultiInstancePanelPrivate> _imp;
    
};

class TrackerPanel : public MultiInstancePanel
{
public:
    
    TrackerPanel(const boost::shared_ptr<Natron::Node>& node);
    
    virtual ~TrackerPanel();
    
private:
    
    virtual void appendExtraGui(QVBoxLayout* layout) OVERRIDE FINAL;
    virtual void appendButton(QHBoxLayout* buttonLayout) OVERRIDE FINAL;
};


#endif // MULTIINSTANCEPANEL_H
