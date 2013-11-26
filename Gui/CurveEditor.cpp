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
    , _nameItem(0)
{
    
    QTreeWidgetItem* nameItem = new QTreeWidgetItem(tree);
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
        
        QTreeWidgetItem* knobItem = new QTreeWidgetItem(nameItem);

        knobItem->setText(0,k->getName().c_str());
        CurveGui* knobCurve = NULL;

        if(k->getDimension() == 1){

            knobCurve = curveWidget->createCurve(k->getCurve(),k->getDimensionName(0).c_str());
            if(!k->getCurve()->isAnimated()){
                knobItem->setHidden(true);
            }else{
                hasAtLeast1KnobWithACurveShown = true;
            }
        }else{
            for(int j = 0 ; j < k->getDimension();++j){
                
                QTreeWidgetItem* dimItem = new QTreeWidgetItem(knobItem);
                dimItem->setText(0,k->getDimensionName(j).c_str());
                CurveGui* dimCurve = curveWidget->createCurve(k->getCurve(j),k->getDimensionName(j).c_str());
                NodeCurveEditorElement* elem = new NodeCurveEditorElement(curveWidget,dimItem,dimCurve);
                QObject::connect(kgui,SIGNAL(keyAdded()),elem,SLOT(checkVisibleState()));
                _nodeElements.push_back(elem);
                if(!k->getCurve()->isAnimated()){
                    dimItem->setHidden(true);
                }else{
                    hasAtLeast1KnobWithACurveShown = true;
                }
            }
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
    }else{
        delete nameItem;
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
            _curve->setVisible(true);
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
            _curve->setVisible(false);
        }
    }
}


NodeCurveEditorElement::NodeCurveEditorElement(CurveWidget* curveWidget,QTreeWidgetItem* item,CurveGui* curve):
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
    delete _treeItem;
}

void CurveEditor::centerOn(double bottom,double left,double top,double right){
    _curveWidget->centerOn(left, right, bottom, top);
}

void CurveEditor::centerOn(const RectD& rect){
    centerOn(rect.bottom(),rect.left(),rect.top(),rect.right());
}
