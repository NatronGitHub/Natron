//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#ifndef NODEBACKDROP_H
#define NODEBACKDROP_H

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <QGraphicsRectItem>

#include "Engine/Knob.h"

class QVBoxLayout;

class String_Knob;

class NodeGraph;
class DockablePanel;

struct NodeBackDropPrivate;
class NodeBackDrop : public QObject, public QGraphicsRectItem, public NamedKnobHolder
{
    
    Q_OBJECT
    
public:
    
    NodeBackDrop(NodeGraph* dag,QGraphicsItem* parent = 0);
    
    void initialize(const QString& name,bool requestedByLoad,QVBoxLayout *dockContainer);
    
    virtual ~NodeBackDrop();
    

    ///We should only ever use these functions that makes it thread safe for the serialization
    void setPos_mt_safe(const QPointF& pos);
    QPointF getPos_mt_safe() const;
    
    boost::shared_ptr<String_Knob> getLabelKnob() const;
    
    ///Mt-safe
    QColor getCurrentColor() const;
    void setCurrentColor(const QColor& color);
    
    ///Mt-safe
    QString getName() const;
    void setName(const QString& str);
    virtual std::string getName_mt_safe() const OVERRIDE FINAL WARN_UNUSED_RETURN;
        
    ///Mt-safe
    void resize(int w,int h);
    void getSize(int& w,int& h) const;
    
    double getHeaderHeight() const;
    
    bool isNearbyHeader(const QPointF& scenePos);
    bool isNearbyResizeHandle(const QPointF& scenePos);
    
    bool isSettingsPanelClosed() const;
    
    DockablePanel* getSettingsPanel() const;
    
    void refreshTextLabelFromKnob();
    
    void slaveTo(NodeBackDrop* master);
    void unslave();
    
    bool isSlave() const;
    
    NodeBackDrop* getMaster() const;
    
    void deactivate();
    void activate();
    
    ///MT-Safe
    bool getIsSelected() const;
    
signals:
    
    void positionChanged();
    
public slots:

    void onNameChanged(const QString& name);
    void onColorChanged(const QColor& color);
    
    void setSettingsPanelClosed(bool closed);
    
    void setSelected(bool selected);
    
    void refreshSlaveMasterLinkPosition();
    
private:
    
    virtual void onKnobValueChanged(KnobI* k,Natron::ValueChangedReason reason,SequenceTime time) OVERRIDE FINAL;
    
    virtual void evaluate(KnobI* /*knob*/,bool /*isSignificant*/,Natron::ValueChangedReason /*reason*/) OVERRIDE FINAL {}
    
    virtual void initializeKnobs() OVERRIDE FINAL;

    
    boost::scoped_ptr<NodeBackDropPrivate> _imp;
    
};

#endif // NODEBACKDROP_H
