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
#include "Gui/KnobGui.h"


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
    
    QObject::connect(_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
                     this, SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

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

    NodeCurveEditorContext* nodeContext = new NodeCurveEditorContext(_tree,_curveWidget,node);
    _nodes.push_back(nodeContext);

}

void CurveEditor::removeNode(NodeGui *node){
    for(std::list<NodeCurveEditorContext*>::iterator it = _nodes.begin();it!=_nodes.end();++it){
        if((*it)->getNode() == node){
            delete (*it);
            _nodes.erase(it);
            break;
        }
    }
}


NodeCurveEditorContext::NodeCurveEditorContext(QTreeWidget* tree,CurveWidget* curveWidget,NodeGui *node)
    : _node(node)
    , _nodeElements()
    , _nameItem()
{
    
    boost::shared_ptr<QTreeWidgetItem> nameItem(new QTreeWidgetItem(tree));
    nameItem->setText(0,_node->getNode()->getName().c_str());

    QObject::connect(node,SIGNAL(nameChanged(QString)),this,SLOT(onNameChanged(QString)));
    const std::map<Knob*,KnobGui*>& knobs = node->getKnobs();

    bool hasAtLeast1KnobWithACurve = false;
    bool hasAtLeast1KnobWithACurveShown = false;

    for(std::map<Knob*,KnobGui*>::const_iterator it = knobs.begin();it!=knobs.end();++it){
        
        Knob* k = it->first;
        KnobGui* kgui = it->second;
        if(!k->canAnimate()){
            continue;
        }
        
        hasAtLeast1KnobWithACurve = true;
        
        boost::shared_ptr<QTreeWidgetItem> knobItem(new QTreeWidgetItem(nameItem.get()));

        knobItem->setText(0,k->getName().c_str());
        CurveGui* knobCurve = NULL;
        bool hideKnob = true;
        if(k->getDimension() == 1){

            knobCurve = curveWidget->createCurve(k->getCurve(0),k->getDimensionName(0).c_str());
            if(!k->getCurve(0)->isAnimated()){
                knobItem->setHidden(true);
            }else{
                hasAtLeast1KnobWithACurveShown = true;
                hideKnob = false;
            }
        }else{
            for(int j = 0 ; j < k->getDimension();++j){
                
                boost::shared_ptr<QTreeWidgetItem> dimItem(new QTreeWidgetItem(knobItem.get()));
                dimItem->setText(0,k->getDimensionName(j).c_str());
                CurveGui* dimCurve = curveWidget->createCurve(k->getCurve(j),k->getDimensionName(j).c_str());
                NodeCurveEditorElement* elem = new NodeCurveEditorElement(curveWidget,dimItem,dimCurve);
                QObject::connect(kgui,SIGNAL(keyAdded()),elem,SLOT(checkVisibleState()));
                _nodeElements.push_back(elem);
                if(!dimCurve->getInternalCurve()->isAnimated()){
                    dimItem->setHidden(true);
                }else{
                    hasAtLeast1KnobWithACurveShown = true;
                    hideKnob = false;
                }
            }
        }
        if(hideKnob){
            knobItem->setHidden(true);
        }
        NodeCurveEditorElement* elem = new NodeCurveEditorElement(curveWidget,knobItem,knobCurve);
        QObject::connect(kgui,SIGNAL(keyAdded()),elem,SLOT(checkVisibleState()));
        _nodeElements.push_back(elem);
    }
    if(hasAtLeast1KnobWithACurve){
        NodeCurveEditorElement* elem = new NodeCurveEditorElement(curveWidget,nameItem,(CurveGui*)NULL);
        _nodeElements.push_back(elem);
        if(!hasAtLeast1KnobWithACurveShown){
            nameItem->setHidden(true);
        }
        _nameItem = nameItem;
    }


}

NodeCurveEditorContext::~NodeCurveEditorContext() {
    for(U32 i = 0 ; i < _nodeElements.size();++i){
        delete _nodeElements[i];
    }
    _nodeElements.clear();
}

void NodeCurveEditorContext::onNameChanged(const QString& name){
    _nameItem->setText(0,name);
}

void NodeCurveEditorElement::checkVisibleState(){
    if(!_curve)
        return;
    int i = _curve->getInternalCurve()->getControlPointsCount();
    if(i > 1){
        if(!_curveDisplayed){
            _curveDisplayed = true;
            _curve->setVisibleAndRefresh(true);
            _treeItem->setHidden(false);
            _treeItem->parent()->setHidden(false);
            _treeItem->parent()->setExpanded(true);
            if(_treeItem->parent()->parent()){
                _treeItem->parent()->parent()->setHidden(false);
                _treeItem->parent()->parent()->setExpanded(true);
            }
        }
    }else{
        if(_curveDisplayed){
            _curveDisplayed = false;
            _treeItem->setHidden(true);
            _treeItem->parent()->setHidden(true);
            _treeItem->parent()->setExpanded(false);
            if(_treeItem->parent()->parent()){
                _treeItem->parent()->parent()->setHidden(true);
                _treeItem->parent()->parent()->setExpanded(false);
            }
            _curve->setVisibleAndRefresh(false);
        }
    }
}


NodeCurveEditorElement::NodeCurveEditorElement(CurveWidget* curveWidget,boost::shared_ptr<QTreeWidgetItem> item,CurveGui* curve):
    _treeItem(item)
  ,_curve(curve)
  ,_curveDisplayed(false)
  ,_curveWidget(curveWidget)
{
    if(curve){
        if(curve->getInternalCurve()->getControlPointsCount() > 1){
            _curveDisplayed = true;
        }
    }
}

NodeCurveEditorElement::~NodeCurveEditorElement(){
    _curveWidget->removeCurve(_curve);
}

void CurveEditor::centerOn(const std::vector<boost::shared_ptr<Curve> >& curves){
    
    // find the curve's gui
    std::vector<CurveGui*> curvesGuis;
    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        const NodeCurveEditorContext::Elements& elems = (*it)->getElements();
        for (U32 i = 0; i < elems.size(); ++i) {
            CurveGui* curve = elems[i]->getCurve();
            if (curve) {
                std::vector<boost::shared_ptr<Curve> >::const_iterator found =
                std::find(curves.begin(), curves.end(), curve->getInternalCurve());
                if(found != curves.end()){
                    curvesGuis.push_back(curve);
                    elems[i]->getTreeItem()->setSelected(true);
                }else{
                    elems[i]->getTreeItem()->setSelected(false);
                }
            }else{
                elems[i]->getTreeItem()->setSelected(false);
            }
        }
    }
    _curveWidget->centerOn(curvesGuis);
    _curveWidget->showCurvesAndHideOthers(curvesGuis);
    
}


void CurveEditor::recursiveSelect(QTreeWidgetItem* cur,std::vector<CurveGui*> *curves){
    if(!cur){
        return;
    }
    cur->setSelected(true);
    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        const NodeCurveEditorContext::Elements& elems = (*it)->getElements();
        for (U32 i = 0; i < elems.size(); ++i) {
            if(elems[i]->getTreeItem() && cur == elems[i]->getTreeItem().get()){
                CurveGui* curve = elems[i]->getCurve();
                if (curve && curve->getInternalCurve()->isAnimated()) {
                    curves->push_back(curve);
                }
            }
        }
    }
    for (int j = 0; j < cur->childCount(); ++j) {
        recursiveSelect(cur->child(j),curves);
    }
}

static void recursiveDeselect(QTreeWidgetItem* current){
    current->setSelected(false);
    for (int j = 0; j < current->childCount(); ++j) {
        recursiveDeselect(current->child(j));
    }
}

void CurveEditor::onCurrentItemChanged(QTreeWidgetItem* current,QTreeWidgetItem* previous){
    std::vector<CurveGui*> curves;
    if(previous){
        recursiveDeselect(previous);
    }
    recursiveSelect(current,&curves);
    
    _curveWidget->centerOn(curves);
    _curveWidget->showCurvesAndHideOthers(curves);
    
}
