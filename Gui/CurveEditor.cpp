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

#include "CurveEditor.h"

#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QHeaderView>

#include "Engine/Knob.h"
#include "Engine/Node.h"

#include "Gui/CurveWidget.h"
#include "Gui/NodeGui.h"


using std::make_pair;
using std::cout;
using std::endl;

CurveEditor::CurveEditor(QWidget *parent)
    : QWidget(parent)
    , _nodes()
    , _mainLayout(NULL)
    , _splitter(NULL)
    , _curveWidget(NULL)
    , _tree(NULL)
{
    _mainLayout = new QHBoxLayout(this);
    setLayout(_mainLayout);
    _mainLayout->setContentsMargins(0,0,0,0);
    _mainLayout->setSpacing(0);

    _splitter = new QSplitter(Qt::Horizontal,this);

    _curveWidget = new CurveWidget(_splitter);

    _tree = new QTreeWidget(_splitter);
    _tree->setColumnCount(1);
    _tree->header()->close();

    _splitter->addWidget(_tree);
    _splitter->addWidget(_curveWidget);


    _mainLayout->addWidget(_splitter);

}


void CurveEditor::addNode(NodeGui* node){

    const std::vector<boost::shared_ptr<Knob> >& knobs = node->getNode()->getKnobs();
    if(knobs.empty()){
        return;
    }
    bool hasKnobsAnimating = false;
    for(U32 i = 0;i < knobs.size();++i){
        if(knobs[i]->canAnimate()){
            hasKnobsAnimating = true;
            break;
        }
    }
    if(!hasKnobsAnimating){
        return;
    }

    NodeCurveEditorContext* nodeContext = new NodeCurveEditorContext(_tree,node);
    const NodeCurveEditorContext::Elements& elems = nodeContext->getElements();
    for(U32 i = 0 ; i < elems.size();++i){
        if(elems[i].second){
            _curveWidget->addCurve(elems[i].second);
        }
    }
    _nodes.push_back(nodeContext);

}

void CurveEditor::removeNode(NodeGui *node){
    for(std::list<NodeCurveEditorContext*>::iterator it = _nodes.begin();it!=_nodes.end();++it){
        if((*it)->getNode() == node){
            const NodeCurveEditorContext::Elements& elems = (*it)->getElements();
            for(U32 i = 0 ; i < elems.size();++i){
                if(elems[i].second){
                    _curveWidget->removeCurve(elems[i].second);
                }
                delete elems[i].first;
            }
            delete (*it);
            _nodes.erase(it);
            break;
        }
    }
}


NodeCurveEditorContext::NodeCurveEditorContext(QTreeWidget* tree,NodeGui *node)
    : _node(node)
    , _nodeElements()
{
    
    QTreeWidgetItem* nameItem = new QTreeWidgetItem(tree);
    nameItem->setText(0,_node->getNode()->getName().c_str());

    QObject::connect(node,SIGNAL(nameChanged(QString)),this,SLOT(onNameChanged(QString)));
    const std::vector<boost::shared_ptr<Knob> >& knobs = node->getNode()->getKnobs();

    bool hasAtLeast1KnobWithACurve = false;
    bool hasAtLeast1KnobWithACurveShown = false;

    for(U32 i = 0; i < knobs.size();++i){
        
        boost::shared_ptr<Knob> k = knobs[i];
        
        if(!k->canAnimate()){
            continue;
        }
        
        hasAtLeast1KnobWithACurve = true;
        
        QTreeWidgetItem* knobItem = new QTreeWidgetItem(nameItem);

        knobItem->setText(0,k->getName().c_str());
        boost::shared_ptr<CurveGui> knobCurve;

        if(k->getDimension() == 1){

            knobCurve.reset(new CurveGui(k->getCurve(),k->getDimensionName(0).c_str(),QColor(255,255,255),2));
            if(!k->getCurve().isAnimated()){
                knobItem->setHidden(true);
            }else{
                hasAtLeast1KnobWithACurveShown = true;
            }
        }else{
            for(int j = 0 ; j < k->getDimension();++j){
                
                QTreeWidgetItem* dimItem = new QTreeWidgetItem(knobItem);
                dimItem->setText(0,k->getDimensionName(j).c_str());
                boost::shared_ptr<CurveGui> dimCurve(new CurveGui(k->getCurve(j),k->getDimensionName(j).c_str(),QColor(255,255,255),2));
                _nodeElements.push_back(make_pair(dimItem,dimCurve));
                if(!k->getCurve().isAnimated()){
                    dimItem->setHidden(true);
                }else{
                    hasAtLeast1KnobWithACurveShown = true;
                }
            }
        }
        _nodeElements.push_back(make_pair(knobItem,knobCurve));
    }
    if(hasAtLeast1KnobWithACurve){
        _nodeElements.push_back(make_pair(nameItem,boost::shared_ptr<CurveGui>()));
        if(!hasAtLeast1KnobWithACurveShown){
            nameItem->setHidden(true);
        }
    }else{
        delete nameItem;
    }


}

NodeCurveEditorContext::~NodeCurveEditorContext() { _nodeElements.clear(); }

void NodeCurveEditorContext::onNameChanged(const QString& name){
    _nodeElements.front().first->setText(0,name);
}

void CurveEditor::centerOn(double bottom,double left,double top,double right){
    _curveWidget->centerOn(left, right, bottom, top);
}

void CurveEditor::centerOn(const RectD& rect){
    centerOn(rect.bottom(),rect.left(),rect.top(),rect.right());
}