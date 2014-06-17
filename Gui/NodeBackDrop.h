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


class NodeGraph;

struct NodeBackDropPrivate;
class NodeBackDrop : public QObject, public QGraphicsRectItem, public KnobHolder
{
    
    Q_OBJECT
    
public:
    
    NodeBackDrop(const QString& name,bool requestedByLoad,NodeGraph* dag,QVBoxLayout *dockContainer,QGraphicsItem* parent = 0);
    
    virtual ~NodeBackDrop();
    
public slots:

    void onNameChanged(const QString& name);
    
private:
    
    virtual void initializeKnobs() OVERRIDE FINAL;

    
    boost::scoped_ptr<NodeBackDropPrivate> _imp;
    
};

#endif // NODEBACKDROP_H
