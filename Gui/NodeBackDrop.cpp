//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "NodeBackDrop.h"

#include <QVBoxLayout>
#include <QTextDocument>
#include <QGraphicsTextItem>
#include <QGraphicsPolygonItem>

#include "Engine/KnobTypes.h"

#include "Gui/Gui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"

struct NodeBackDropPrivate
{
    
    NodeGraph* graph;
    
    QGraphicsRectItem* header;
    QGraphicsTextItem* name;
    QGraphicsTextItem* label;
    QGraphicsPolygonItem* resizeHandle;
    
    DockablePanel* settingsPanel;
    
    boost::shared_ptr<String_Knob> knobLabel;
    
    NodeBackDropPrivate(NodeGraph* dag)
    : graph(dag)
    , header(0)
    , name(0)
    , label(0)
    , resizeHandle(0)
    , settingsPanel(0)
    , knobLabel()
    {
        
    }
};

NodeBackDrop::NodeBackDrop(const QString& name,bool requestedByLoad,NodeGraph* dag,QVBoxLayout *dockContainer,QGraphicsItem* parent)
: QGraphicsRectItem(parent)
, KnobHolder(dag->getGui()->getApp())
, _imp(new NodeBackDropPrivate(dag))
{
    QString tooltip("The node backdrop is useful to group nodes and identify them in the node graph. You can also "
                    "move all the nodes inside the backdrop.");
    _imp->settingsPanel = new DockablePanel(dag->getGui(), //< pointer to the gui
                                            this, //< pointer to the knob holder (this)
                                            dockContainer, //< pointer to the layout that will contain this settings panel
                                            DockablePanel::FULLY_FEATURED, //< use a fully featured header with editable text
                                            false, //< don't use scroll areas for tabs
                                            name, //< initial name
                                            Qt::convertFromPlainText(tooltip,Qt::WhiteSpaceNormal), //< help tooltip
                                            false, //< no default page
                                            "BackDrop", //< default page name
                                            dockContainer->parentWidget());
    
   
    ///initialize knobs here
    initializeKnobsPublic();
    
    QObject::connect(_imp->settingsPanel,SIGNAL(nameChanged(QString)),this,SLOT(onNameChanged(QString)));
    dockContainer->addWidget(_imp->settingsPanel);
    
    if (!requestedByLoad) {
        _imp->graph->getGui()->putSettingsPanelFirst(_imp->settingsPanel);
        _imp->graph->getGui()->addVisibleDockablePanel(_imp->settingsPanel);
    } else {
        _imp->settingsPanel->setClosed(true);
    }
    
    
    ///initialize knobs gui now
    _imp->settingsPanel->initializeKnobs();
    
    setZValue(-10);
    
    _imp->header = new QGraphicsRectItem(this);
    _imp->header->setZValue(-9);
    
    _imp->name = new QGraphicsTextItem(name,this);
    _imp->name->setDefaultTextColor(QColor(0,0,0,255));
    _imp->name->setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_12));
    _imp->name->setZValue(-8);
    
    
    
    
    _imp->label = new QGraphicsTextItem("",this);
}

NodeBackDrop::~NodeBackDrop()
{
    
}

void NodeBackDrop::initializeKnobs()
{
    _imp->knobLabel = Natron::createKnob<String_Knob>(this, "Label");
    _imp->knobLabel->setAnimationEnabled(false);
    _imp->knobLabel->setAsMultiLine();
    _imp->knobLabel->setUsesRichText(true);
    _imp->knobLabel->setHintToolTip("Text to display on the backdrop.");
}

void NodeBackDrop::onNameChanged(const QString& name)
{
    
}