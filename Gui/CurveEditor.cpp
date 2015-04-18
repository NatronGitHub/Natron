//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "CurveEditor.h"

#include <utility>

#include <QApplication>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QHeaderView>
#include <QUndoStack> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QAction>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
CLANG_DIAG_ON(unused-private-field)

#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/Node.h"
#include "Engine/KnobFile.h"
#include "Engine/RotoContext.h"
#include "Engine/EffectInstance.h"
#include "Engine/TimeLine.h"

#include "Gui/CurveWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Gui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"

using std::make_pair;
using std::cout;
using std::endl;
using Natron::Label;


struct CurveEditorPrivate
{
    
    Gui* gui;
    
    std::list<NodeCurveEditorContext*> nodes;
    std::list<RotoCurveEditorContext*> rotos;
    QVBoxLayout* mainLayout;
    QSplitter* splitter;
    CurveWidget* curveWidget;
    
    QTreeWidget* tree;
    QWidget* filterContainer;
    QHBoxLayout* filterLayout;
    Natron::Label* filterLabel;
    LineEdit* filterEdit;
    QWidget* leftPaneContainer;
    QVBoxLayout* leftPaneLayout;
    
    boost::scoped_ptr<QUndoStack> undoStack;
    QAction* undoAction,*redoAction;
    
    QWidget* expressionContainer;
    QHBoxLayout* expressionLayout;
    Natron::Label* knobLabel;
    LineEdit* knobLineEdit;
    Natron::Label* resultLabel;
    
    KnobCurveGui* selectedKnobCurve;
    
    CurveEditorPrivate(Gui* gui)
    : gui(gui)
    , nodes()
    , rotos()
    , mainLayout(0)
    , splitter(0)
    , curveWidget(0)
    , tree(0)
    , filterContainer(0)
    , filterLayout(0)
    , filterLabel(0)
    , filterEdit(0)
    , leftPaneContainer(0)
    , leftPaneLayout(0)
    , undoStack(new QUndoStack)
    , undoAction(0)
    , redoAction(0)
    , expressionContainer(0)
    , expressionLayout(0)
    , knobLabel(0)
    , knobLineEdit(0)
    , resultLabel(0)
    , selectedKnobCurve(0)
    {
        
    }
};

CurveEditor::CurveEditor(Gui* gui,
                         boost::shared_ptr<TimeLine> timeline,
                         QWidget *parent)
: QWidget(parent)
, CurveSelection()
, ScriptObject()
, _imp(new CurveEditorPrivate(gui))
{
    setObjectName("CurveEditor");
    _imp->undoAction = _imp->undoStack->createUndoAction( this,tr("&Undo") );
    _imp->undoAction->setShortcuts(QKeySequence::Undo);
    _imp->redoAction = _imp->undoStack->createRedoAction( this,tr("&Redo") );
    _imp->redoAction->setShortcuts(QKeySequence::Redo);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0,0,0,0);
    _imp->mainLayout->setSpacing(0);

    _imp->splitter = new QSplitter(Qt::Horizontal,this);
    _imp->splitter->setObjectName("CurveEditorSplitter");

    _imp->curveWidget = new CurveWidget(gui,this, timeline,_imp->splitter);
    _imp->curveWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    _imp->leftPaneContainer = new QWidget(_imp->splitter);
    _imp->leftPaneLayout = new QVBoxLayout(_imp->leftPaneContainer);
    _imp->leftPaneLayout->setContentsMargins(0, 0, 0, 0);
    _imp->filterContainer = new QWidget(_imp->leftPaneContainer);
    
    _imp->filterLayout = new QHBoxLayout(_imp->filterContainer);
    _imp->filterLayout->setContentsMargins(0, 0, 0, 0);
    
    QString filterTt = tr("Show in the curve editor only nodes containing the following filter");
    _imp->filterLabel = new Label("Filter:",_imp->filterContainer);
    _imp->filterLabel->setToolTip(filterTt);
    _imp->filterEdit = new LineEdit(_imp->filterContainer);
    _imp->filterEdit->setToolTip(filterTt);
    QObject::connect(_imp->filterEdit, SIGNAL(textChanged(QString)), this, SLOT(onFilterTextChanged(QString)));
    _imp->filterLayout->addWidget(_imp->filterLabel);
    _imp->filterLayout->addWidget(_imp->filterEdit);
    
    _imp->leftPaneLayout->addWidget(_imp->filterContainer);
    
    _imp->tree = new QTreeWidget(_imp->leftPaneContainer);
    _imp->tree->setObjectName("tree");
    _imp->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _imp->tree->setColumnCount(1);
    _imp->tree->header()->close();
    _imp->tree->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);

    _imp->leftPaneLayout->addWidget(_imp->tree);

    
    _imp->splitter->addWidget(_imp->leftPaneContainer);
    _imp->splitter->addWidget(_imp->curveWidget);


    _imp->mainLayout->addWidget(_imp->splitter);
    
    _imp->expressionContainer = new QWidget(this);
    _imp->expressionLayout = new QHBoxLayout(_imp->expressionContainer);
    _imp->expressionLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->knobLabel = new Natron::Label(_imp->expressionContainer);
    _imp->knobLabel->setAltered(true);
    _imp->knobLabel->setText(tr("No curve selected"));
    _imp->knobLineEdit = new LineEdit(_imp->expressionContainer);
    QObject::connect(_imp->knobLineEdit, SIGNAL(editingFinished()), this, SLOT(onExprLineEditFinished()));
    _imp->resultLabel = new Natron::Label(_imp->expressionContainer);
    _imp->resultLabel->setAltered(true);
    _imp->resultLabel->setText("= ");
    _imp->knobLineEdit->setReadOnly(true);
    
    _imp->expressionLayout->addWidget(_imp->knobLabel);
    _imp->expressionLayout->addWidget(_imp->knobLineEdit);
    _imp->expressionLayout->addWidget(_imp->resultLabel);
    
    _imp->mainLayout->addWidget(_imp->expressionContainer);
    
    QObject::connect( _imp->tree, SIGNAL( itemSelectionChanged() ),
                      this, SLOT( onItemSelectionChanged() ) );
    QObject::connect( _imp->tree, SIGNAL( itemDoubleClicked(QTreeWidgetItem*,int) ),
                     this, SLOT( onItemDoubleClicked(QTreeWidgetItem*,int) ) );
}

CurveEditor::~CurveEditor()
{
    for (std::list<NodeCurveEditorContext*>::iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        delete (*it);
    }
    _imp->nodes.clear();
    
    for (std::list<RotoCurveEditorContext*>::iterator it = _imp->rotos.begin(); it != _imp->rotos.end(); ++it) {
        delete *it;
    }
    _imp->rotos.clear();
}

void
CurveEditor::onFilterTextChanged(const QString& filter)
{
    for (std::list<NodeCurveEditorContext*>::iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        if (filter.isEmpty() ||
            QString((*it)->getNode()->getNode()->getLabel().c_str()).contains(filter,Qt::CaseInsensitive)) {
            (*it)->setVisible(true);
        } else {
            (*it)->setVisible(false);
        }
    }
    
    for (std::list<RotoCurveEditorContext*>::iterator it = _imp->rotos.begin(); it != _imp->rotos.end(); ++it) {
        if (filter.isEmpty() ||
            QString((*it)->getNode()->getNode()->getLabel().c_str()).contains(filter,Qt::CaseInsensitive)) {
            (*it)->setVisible(true);
        } else {
            (*it)->setVisible(false);
        }
    }
}


void
CurveEditor::recursiveSelect(QTreeWidgetItem* cur,
                             std::vector<CurveGui*> *curves,bool inspectRotos)
{
    if (!cur) {
        return;
    }
    if (!cur->isSelected()) {
        cur->setSelected(true);
    }
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        NodeCurveEditorElement* elem = (*it)->findElement(cur);
        if (elem && !elem->getTreeItem()->isHidden()) {
            CurveGui* curve = elem->getCurve();
            if ( curve /*&& curve->getInternalCurve()->isAnimated()*/ ) {
                curves->push_back(curve);
            }
            break;
        }
    }
    for (int j = 0; j < cur->childCount(); ++j) {
        recursiveSelect(cur->child(j),curves,false);
    }
    
    if (inspectRotos) {
        for (std::list<RotoCurveEditorContext*>::const_iterator it = _imp->rotos.begin(); it != _imp->rotos.end(); ++it) {
            (*it)->recursiveSelectRoto(cur,curves);
            if (curves->size() > 0) {
                break;
            }
        }
    }
    
}

std::pair<QAction*,QAction*> CurveEditor::getUndoRedoActions() const
{
    return std::make_pair(_imp->undoAction,_imp->redoAction);
}

void
CurveEditor::addNode(boost::shared_ptr<NodeGui> node)
{
    const std::vector<boost::shared_ptr<KnobI> > & knobs = node->getNode()->getKnobs();

    if ( knobs.empty() || !node->getSettingPanel() ) {
        return;
    }
    bool hasKnobsAnimating = false;
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->canAnimate() ) {
            hasKnobsAnimating = true;
            break;
        }
    }
    if (!hasKnobsAnimating) {
        return;
    }
    
    boost::shared_ptr<RotoContext> roto = node->getNode()->getRotoContext();
    if (!roto) {
        NodeCurveEditorContext* nodeContext = new NodeCurveEditorContext(_imp->tree,this,node);
        _imp->nodes.push_back(nodeContext);
    } else {
        RotoCurveEditorContext* rotoEditorCtx = new RotoCurveEditorContext(this,_imp->tree,node);
        _imp->rotos.push_back(rotoEditorCtx);
    }
}

void
CurveEditor::removeNode(NodeGui* node)
{
    boost::shared_ptr<RotoContext> roto = node->getNode()->getRotoContext();
    if (!roto) {
        for (std::list<NodeCurveEditorContext*>::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ( (*it)->getNode().get() == node ) {

                const NodeCurveEditorContext::Elements& elems = (*it)->getElements();
                for (NodeCurveEditorContext::Elements::const_iterator it2 = elems.begin(); it2 != elems.end(); ++it2) {
                    if ((*it2)->getCurve() == _imp->selectedKnobCurve) {
                        _imp->selectedKnobCurve = 0;
                        break;
                    }
                }
                delete (*it);
                _imp->nodes.erase(it);
                break;
            }
        }
    } else {
        for (std::list<RotoCurveEditorContext*>::iterator it = _imp->rotos.begin(); it != _imp->rotos.end(); ++it) {
            if ((*it)->getNode().get() == node) {
                delete *it;
                _imp->rotos.erase(it);
                break;
            }
        }
    }
    _imp->curveWidget->centerOn(-10,500,-10,10);
}

static void createElementsForKnob(QTreeWidgetItem* parent,KnobGui* kgui,boost::shared_ptr<KnobI> k,
                                  CurveEditor* curveEditor,QTreeWidget* tree,const boost::shared_ptr<RotoContext>& rotoctx,
                                  std::list<NodeCurveEditorElement*>& elements,bool* hasCurveVisible)
{
    if (k) {
        assert(!kgui);
    } else {
        assert(kgui && !k);
        k = kgui->getKnob();
    }
    
    if ( !k->canAnimate() || !k->isAnimationEnabled() ) {
        return;
    }
    
    CurveWidget* curveWidget = curveEditor->getCurveWidget();

    KnobHelper* helper = dynamic_cast<KnobHelper*>( k.get() );
    assert(helper);
    if (!helper) {
        // coverity[dead_error_line]
        return;
    }
    boost::shared_ptr<KnobSignalSlotHandler> handler = helper->getSignalSlotHandler();
    
    if (kgui) {
        QObject::connect( kgui,SIGNAL( keyFrameSet() ),curveWidget,SLOT( onCurveChanged() ) );
        QObject::connect( kgui,SIGNAL( keyFrameRemoved() ),curveWidget,SLOT( onCurveChanged() ) );
        QObject::connect( kgui, SIGNAL( keyInterpolationChanged() ),curveWidget, SLOT( refreshDisplayedTangents() ) );
        QObject::connect( kgui, SIGNAL( keyInterpolationChanged() ),curveWidget, SLOT( refreshDisplayedTangents() ) );
        QObject::connect( kgui, SIGNAL( refreshCurveEditor() ),curveWidget, SLOT( onCurveChanged() ) );
    }
    
    QTreeWidgetItem* knobItem = new QTreeWidgetItem(parent);
    knobItem->setExpanded(true);
    knobItem->setText( 0,k->getDescription().c_str() );
    
    CurveGui* knobCurve = NULL;
    bool hideKnob = true;
    
    if (k->getDimension() == 1) {
                
        if (kgui) {
            knobCurve = new KnobCurveGui(curveWidget,kgui->getCurve(0),kgui,0,k->getDescription().c_str(),QColor(255,255,255),1.);
        } else {
            
            knobCurve = new KnobCurveGui(curveWidget,k->getCurve(0,true),k,rotoctx,0,k->getDescription().c_str(),QColor(255,255,255),1.);
        }
        curveWidget->addCurveAndSetColor(knobCurve);
        
        if ( !k->getCurve(0)->isAnimated() ) {
            knobItem->setHidden(true);
        } else {
            *hasCurveVisible = true;
            hideKnob = false;
        }
        knobCurve->setVisible(false);
    } else {
        for (int j = 0; j < k->getDimension(); ++j) {
            QTreeWidgetItem* dimItem = new QTreeWidgetItem(knobItem);
            dimItem->setExpanded(true);
            dimItem->setText( 0,k->getDimensionName(j).c_str() );
            QString curveName = QString( k->getDescription().c_str() ) + "." + QString( k->getDimensionName(j).c_str() );
            
            NodeCurveEditorElement* elem;
            KnobCurveGui* dimCurve;
            if (kgui) {
                dimCurve = new KnobCurveGui(curveWidget,kgui->getCurve(j),kgui,j,curveName,QColor(255,255,255),1.);
                elem = new NodeCurveEditorElement(tree,curveEditor,kgui,j,dimItem,dimCurve);
            } else {
                dimCurve = new KnobCurveGui(curveWidget,k->getCurve(j,true),k,rotoctx,j,curveName,QColor(255,255,255),1.);
                elem = new NodeCurveEditorElement(tree,curveEditor,k,j,dimItem,dimCurve);
            }
            curveWidget->addCurveAndSetColor(dimCurve);
            
            elements.push_back(elem);
            std::string expr = k->getExpression(j);
            if ( !dimCurve->getInternalCurve()->isAnimated() && expr.empty() ) {
                dimItem->setHidden(true);
            } else {
                *hasCurveVisible = true;
                hideKnob = false;
            }
            dimCurve->setVisible(false);
        }
    }
    
    
    if (hideKnob) {
        knobItem->setHidden(true);
    }
    NodeCurveEditorElement* elem ;
    if (kgui) {
        elem = new NodeCurveEditorElement(tree,curveEditor,kgui,0,knobItem,knobCurve);
    } else {
        elem = new NodeCurveEditorElement(tree,curveEditor,k,0,knobItem,knobCurve);
    }
    elements.push_back(elem);
    
}

NodeCurveEditorContext::NodeCurveEditorContext(QTreeWidget* tree,
                                               CurveEditor* curveWidget,
                                               const boost::shared_ptr<NodeGui> &node)
    : _node(node)
      , _nodeElements()
      , _nameItem(0)
{
    QTreeWidgetItem* nameItem = new QTreeWidgetItem(tree);
    nameItem->setExpanded(true);
    nameItem->setText( 0,_node->getNode()->getLabel().c_str() );

    QObject::connect( node->getNode().get(),SIGNAL( labelChanged(QString) ),this,SLOT( onNameChanged(QString) ) );

    const std::map<boost::weak_ptr<KnobI>,KnobGui*> & knobs = node->getKnobs();

    bool hasAtLeast1KnobWithACurveShown = false;

    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        createElementsForKnob(nameItem, it->second,boost::shared_ptr<KnobI>() ,
                              curveWidget, tree,boost::shared_ptr<RotoContext>(), _nodeElements, &hasAtLeast1KnobWithACurveShown);
        
    }
    
    if (_nodeElements.size() > 0) {
        NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,(KnobGui*)NULL,-1,
                                                                  nameItem,(CurveGui*)NULL);
        _nodeElements.push_back(elem);
        if (!hasAtLeast1KnobWithACurveShown) {
            nameItem->setHidden(true);
        }
    } else {
        nameItem->setHidden(true);
    }
    _nameItem = nameItem;

}

NodeCurveEditorContext::~NodeCurveEditorContext()
{
    delete _nameItem;
    for (Elements::iterator it = _nodeElements.begin(); it!=_nodeElements.end();++it) {
        delete *it;
    }
    _nodeElements.clear();
}

bool
NodeCurveEditorContext::isVisible() const
{
    return !_nameItem->isHidden();
}

void
NodeCurveEditorContext::setVisible(bool visible)
{
    for (Elements::iterator it = _nodeElements.begin(); it != _nodeElements.end(); ++it) {
        if (visible) {
            (*it)->checkVisibleState(false);
        } else {
            (*it)->setVisible(false);
        }
    }
}

void
NodeCurveEditorContext::onNameChanged(const QString & name)
{
    _nameItem->setText(0,name);
}

static void
checkIfHiddenRecursivly(QTreeWidget* tree,
                        QTreeWidgetItem* item)
{
    bool areAllChildrenHidden = true;

    for (int i = 0; i <  item->childCount(); ++i) {
        if ( !item->child(i)->isHidden() ) {
            areAllChildrenHidden = false;
            break;
        }
    }
    if (areAllChildrenHidden) {
        item->setHidden(true);
        //item->setExpanded(false);
    }
    bool isTopLvl = false;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        if (tree->topLevelItem(i) == item) {
            isTopLvl = true;
            break;
        }
    }
    if (!isTopLvl) {
        checkIfHiddenRecursivly( tree,item->parent() );
    }
}

boost::shared_ptr<KnobI>
NodeCurveEditorElement::getInternalKnob() const
{
    return _knob ? _knob->getKnob() : _internalKnob;
}

void
NodeCurveEditorElement::setVisible(bool visible)
{
    if (visible) {
        //show the item
        if (!_curveDisplayed) {
            _curveDisplayed = true;
            _treeItem->setHidden(false);
            _treeItem->parent()->setHidden(false);
            _treeItem->parent()->setExpanded(true);
            if ( _treeItem->parent()->parent() ) {
                _treeItem->parent()->parent()->setHidden(false);
                _treeItem->parent()->parent()->setExpanded(true);
            }
        }
    } else {

        if (_curveWidget->getSelectedCurve() == _curve) {
            _curveWidget->setSelectedCurve(NULL);
        }
        //hide the item
        //hiding is a bit more complex because we do not always hide the parent too,it also
        // depends on the item's siblings visibility
        if (_curveDisplayed) {
            _curveDisplayed = false;
            _treeItem->setHidden(true);
            checkIfHiddenRecursivly( _treeWidget, _treeItem->parent() );
            _curve->setVisibleAndRefresh(false);
        }
    }
}

void
NodeCurveEditorElement::checkVisibleState(bool autoSelectOnShow)
{
    if (!_curve) {
        return;
    }
    
    boost::shared_ptr<Curve> curve =  _curve->getInternalCurve() ;
    std::string expr = _knob ? _knob->getKnob()->getExpression(_dimension) : std::string();
    // even when there is only one keyframe, there may be tangents!
    if (curve && (curve->getKeyFramesCount() > 0 || !expr.empty())) {
        
        setVisible(true);
        
        if (autoSelectOnShow) {
            QList<QTreeWidgetItem*> selectedItems = _treeWidget->selectedItems();
            bool wasEmpty = false;
            
            ///if there was no selection so far, select this item and its parents
            if ( selectedItems.empty() ) {
                _treeItem->setSelected(true);
                if ( _treeItem->parent() ) {
                    _treeItem->parent()->setSelected(true);
                }
                if ( _treeItem->parent()->parent() ) {
                    _treeItem->parent()->parent()->setSelected(true);
                }
                wasEmpty = true;
            } else {
                for (int i = 0; i < selectedItems.size(); ++i) {
                    if ( ( selectedItems.at(i) == _treeItem->parent() ) ||
                        ( selectedItems.at(i) == _treeItem->parent()->parent() ) ) {
                        _treeItem->setSelected(true);
                    }
                }
            }
            
            if ( _treeItem->isSelected() ) {
                
                CurveWidget* cw = _curveWidget->getCurveWidget();
                std::vector<CurveGui*> curves;
                cw->getVisibleCurves(&curves);
                curves.push_back(_curve);
                cw->showCurvesAndHideOthers(curves);
                if (wasEmpty) {
                    cw->centerOn(curves);
                }
            }
        }
    } else {
        setVisible(false);
    }

}

void
NodeCurveEditorElement::checkVisibleState()
{
    checkVisibleState(true);
} // checkVisibleState

void
NodeCurveEditorElement::onExpressionChanged()
{
    if (_curveWidget->getSelectedCurve() == _curve) {
        _curveWidget->refreshCurrentExpression();
    }
    checkVisibleState();
}

NodeCurveEditorElement::NodeCurveEditorElement(QTreeWidget *tree,
                                               CurveEditor* curveWidget,
                                               KnobGui *knob,
                                               int dimension,
                                               QTreeWidgetItem* item,
                                               CurveGui* curve)
    : _treeItem(item)
      ,_curve(curve)
      ,_curveDisplayed(false)
      ,_curveWidget(curveWidget)
      ,_treeWidget(tree)
      ,_knob(knob)
      ,_internalKnob()
      ,_dimension(dimension)
{
    if (knob) {
        QObject::connect( knob,SIGNAL( keyFrameSet() ),this,SLOT( checkVisibleState() ) );
        QObject::connect( knob,SIGNAL( keyFrameRemoved() ),this,SLOT( checkVisibleState() ) );
        QObject::connect( knob,SIGNAL( expressionChanged() ),this,SLOT( onExpressionChanged() ) );
    }
    if (curve) {
        // even when there is only one keyframe, there may be tangents!
        if (curve->getInternalCurve()->getKeyFramesCount() > 0) {
            _curveDisplayed = true;
        }
    } else {
        _dimension = -1; //set dimension to be meaningless
    }
}

NodeCurveEditorElement::NodeCurveEditorElement(QTreeWidget *tree,
                                               CurveEditor* curveWidget,
                                               const boost::shared_ptr<KnobI>& internalKnob,
                                               int dimension,
                                               QTreeWidgetItem* item,
                                               CurveGui* curve)
: _treeItem(item)
,_curve(curve)
,_curveDisplayed(false)
,_curveWidget(curveWidget)
,_treeWidget(tree)
,_knob(0)
,_internalKnob(internalKnob)
,_dimension(dimension)
{
    if (internalKnob) {
        boost::shared_ptr<KnobSignalSlotHandler> handler = internalKnob->getSignalSlotHandler();
        QObject::connect( handler.get(),SIGNAL( keyFrameSet(SequenceTime,int,int,bool) ),this,SLOT( checkVisibleState() ) );
        QObject::connect( handler.get(),SIGNAL( keyFrameRemoved(SequenceTime,int,int) ),this,SLOT( checkVisibleState() ) );
        QObject::connect( handler.get(),SIGNAL( animationRemoved(int) ),this,SLOT( checkVisibleState() ) );
    }
    if (curve) {
        // even when there is only one keyframe, there may be tangents!
        if (curve->getInternalCurve()->getKeyFramesCount() > 0) {
            _curveDisplayed = true;
        }
    } else {
        _dimension = -1; //set dimension to be meaningless
    }
}

NodeCurveEditorElement::~NodeCurveEditorElement()
{
    _curveWidget->getCurveWidget()->removeCurve(_curve);
}

void
CurveEditor::centerOn(const std::vector<boost::shared_ptr<Curve> > & curves)
{
    // find the curve's gui
    std::vector<CurveGui*> curvesGuis;

    for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        const NodeCurveEditorContext::Elements & elems = (*it)->getElements();
        for (NodeCurveEditorContext::Elements::const_iterator it2 = elems.begin() ; it2 != elems.end(); ++it2) {
            CurveGui* curve = (*it2)->getCurve();
            if (curve) {
                std::vector<boost::shared_ptr<Curve> >::const_iterator found =
                    std::find( curves.begin(), curves.end(), curve->getInternalCurve() );
                if ( found != curves.end() ) {
                    curvesGuis.push_back(curve);
                    (*it2)->getTreeItem()->setSelected(true);
                } else {
                    (*it2)->getTreeItem()->setSelected(false);
                }
            } else {
                (*it2)->getTreeItem()->setSelected(false);
            }
        }
    }
    _imp->curveWidget->centerOn(curvesGuis);
    _imp->curveWidget->showCurvesAndHideOthers(curvesGuis);
}

void
CurveEditor::getSelectedCurves(std::vector<CurveGui*>* selection)
{
    
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        recursiveSelect(selectedItems[i],selection);
    }
    
}

void
CurveEditor::onItemSelectionChanged()
{
    _imp->tree->blockSignals(true);
    std::vector<CurveGui*> curves;
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        selectedItems[i]->setSelected(false);
        recursiveSelect(selectedItems[i],&curves);
    }
    
    _imp->curveWidget->showCurvesAndHideOthers(curves);
    _imp->tree->blockSignals(false);
    //_imp->curveWidget->centerOn(curves); //this will reframe the view on the curves
}


void
CurveEditor::onItemDoubleClicked(QTreeWidgetItem* item,int)
{
    boost::shared_ptr<NodeGui> node;
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        
        if (item == (*it)->getItem()) {
            node = (*it)->getNode();
            break;
        }
        
        const NodeCurveEditorContext::Elements & elems = (*it)->getElements();
        for (NodeCurveEditorContext::Elements::const_iterator it2 = elems.begin() ; it2 != elems.end(); ++it2) {
            if ((*it2)->getTreeItem() == item) {
                node = (*it)->getNode();
                break;
            }
        }
        if (node) {
            break;
        }
    }
    
    
    for (std::list<RotoCurveEditorContext*>::const_iterator it = _imp->rotos.begin();
         it != _imp->rotos.end(); ++it) {
        
        if ((*it)->getItem() == item) {
            node = (*it)->getNode();
            break;
        }
        const std::list<BezierEditorContext*> & beziers = (*it)->getElements();
        for (std::list<BezierEditorContext*>::const_iterator it2 = beziers.begin(); it2 != beziers.end(); ++it2) {
            
            if ((*it2)->getItem() == item) {
                node = (*it)->getNode();
                break;
            }
            const std::list<NodeCurveEditorElement*> elements = (*it2)->getElements();
            for (std::list<NodeCurveEditorElement*> ::const_iterator it3 = elements.begin(); it3 != elements.end(); ++it3) {
                if ((*it3)->getTreeItem() == item) {
                    node = (*it)->getNode();
                    break;
                }
            }
            if (node) {
                break;
            }
        }
        if (node) {
            break;
        }
    }
    
    
    DockablePanel* panel = 0;
    if (node) {
        node->ensurePanelCreated();
    }
    if (node && node->getParentMultiInstance()) {
        panel = node->getParentMultiInstance()->getSettingPanel();
    } else {
        panel = node->getSettingPanel();
    }
    if (node && panel && node->isVisible()) {
        if ( !node->isSettingsPanelVisible() ) {
            node->setVisibleSettingsPanel(true);
        }
        if ( !node->wasBeginEditCalled() ) {
            node->beginEditKnobs();
        }
        _imp->gui->putSettingsPanelFirst( node->getSettingPanel() );
        _imp->gui->getApp()->redrawAllViewers();
    }
}

NodeCurveEditorElement*
NodeCurveEditorContext::findElement(CurveGui* curve) const
{
    for (Elements::const_iterator it = _nodeElements.begin(); it != _nodeElements.end() ; ++it) {
        if ((*it)->getCurve() == curve) {
            return *it;
        }
    }

    return NULL;
}

NodeCurveEditorElement*
NodeCurveEditorContext::findElement(KnobGui* knob,
                                    int dimension) const
{
    for (Elements::const_iterator it = _nodeElements.begin(); it != _nodeElements.end() ; ++it) {
        if ( ((*it)->getKnobGui() == knob) && ((*it)->getDimension() == dimension) ) {
            return *it;
        }
    }

    return NULL;
}

NodeCurveEditorElement*
NodeCurveEditorContext::findElement(QTreeWidgetItem* item) const
{
    for (Elements::const_iterator it = _nodeElements.begin(); it != _nodeElements.end() ; ++it) {
        if ((*it)->getTreeItem() == item) {
            return *it;
        }
    }

    return NULL;
}

std::list<CurveGui*>
CurveEditor::findCurve(KnobGui* knob,
                       int dimension) const
{
    KnobHolder* holder = knob->getKnob()->getHolder();
    assert(holder);
    Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
    assert(effect);
    
    std::list<CurveGui*> ret;
    
    boost::shared_ptr<RotoContext> roto = effect->getNode()->getRotoContext();
    if (roto) {
        
        for (std::list<RotoCurveEditorContext*>::const_iterator it =_imp->rotos.begin(); it != _imp->rotos.end(); ++it) {
            if (roto == (*it)->getNode()->getNode()->getRotoContext()) {
                std::list<NodeCurveEditorElement*> elems = (*it)->findElement(knob, dimension);
                if (!elems.empty()) {
                    for (std::list<NodeCurveEditorElement*>::iterator it2 = elems.begin(); it2 != elems.end(); ++it2) {
                        ret.push_back((*it2)->getCurve());
                    }
                    return ret;
                }
            }
        }
    } else {
        for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
             it != _imp->nodes.end(); ++it) {
            NodeCurveEditorElement* elem = (*it)->findElement(knob,dimension);
            if (elem) {
                ret.push_back(elem->getCurve());
                return ret;
            }
        }
    }

    return ret;
}

void
CurveEditor::hideCurve(KnobGui* knob,
                       int dimension)
{
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        NodeCurveEditorElement* elem = (*it)->findElement(knob,dimension);
        if (elem) {
            elem->getCurve()->setVisible(false);
            elem->getTreeItem()->setHidden(true);
            checkIfHiddenRecursivly( _imp->tree, elem->getTreeItem() );
            break;
        }
    }
    _imp->curveWidget->update();
}

void
CurveEditor::hideCurves(KnobGui* knob)
{
    for (int i = 0; i < knob->getKnob()->getDimension(); ++i) {
        for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
             it != _imp->nodes.end(); ++it) {
            NodeCurveEditorElement* elem = (*it)->findElement(knob,i);
            if (elem) {
                elem->getCurve()->setVisible(false);
                elem->getTreeItem()->setHidden(true);
                checkIfHiddenRecursivly( _imp->tree, elem->getTreeItem() );
                break;
            }
        }
    }
    _imp->curveWidget->update();
}

void
CurveEditor::showCurve(KnobGui* knob,
                       int dimension)
{
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
         it != _imp->nodes.end(); ++it) {
        NodeCurveEditorElement* elem = (*it)->findElement(knob,dimension);
        if (elem) {
            if ( elem->getCurve()->getInternalCurve()->isAnimated() ) {
                elem->getCurve()->setVisible(true);
                elem->getTreeItem()->setHidden(false);
                if ( elem->getTreeItem()->parent() ) {
                    elem->getTreeItem()->parent()->setHidden(false);
                    elem->getTreeItem()->parent()->setExpanded(true);
                    if ( elem->getTreeItem()->parent()->parent() ) {
                        elem->getTreeItem()->parent()->parent()->setHidden(false);
                        elem->getTreeItem()->parent()->parent()->setExpanded(true);
                    }
                }
            }
            break;
        }
    }
    _imp->curveWidget->update();
}

void
CurveEditor::showCurves(KnobGui* knob)
{
    for (int i = 0; i < knob->getKnob()->getDimension(); ++i) {
        for (std::list<NodeCurveEditorContext*>::const_iterator it = _imp->nodes.begin();
             it != _imp->nodes.end(); ++it) {
            NodeCurveEditorElement* elem = (*it)->findElement(knob,i);
            if (elem) {
                if ( elem->getCurve()->getInternalCurve()->isAnimated() ) {
                    elem->getCurve()->setVisible(true);
                    elem->getTreeItem()->setHidden(false);
                    if ( elem->getTreeItem()->parent() ) {
                        elem->getTreeItem()->parent()->setHidden(false);
                        elem->getTreeItem()->parent()->setExpanded(true);
                        if ( elem->getTreeItem()->parent()->parent() ) {
                            elem->getTreeItem()->parent()->parent()->setHidden(false);
                            elem->getTreeItem()->parent()->parent()->setExpanded(true);
                        }
                    }
                }
                break;
            }
        }
    }
    _imp->curveWidget->update();
}

CurveWidget*
CurveEditor::getCurveWidget() const
{
    return _imp->curveWidget;
}


struct BezierEditorContextPrivate {
    
    CurveEditor* widget;
    RotoCurveEditorContext* context;
    boost::shared_ptr<Bezier> curve;
    QTreeWidgetItem* nameItem;
    QTreeWidgetItem* curveItem;
    BezierCPCurveGui* animCurve;
    std::list<NodeCurveEditorElement*> knobs;
    bool doDeleteItem;
    
    BezierEditorContextPrivate(CurveEditor* widget,
                               const boost::shared_ptr<Bezier>& curve,
                               RotoCurveEditorContext* context)
    : widget(widget)
    , context(context)
    , curve(curve)
    , nameItem(0)
    , curveItem(0)
    , animCurve(0)
    , knobs()
    , doDeleteItem(true)
    {
        
    }
};

BezierEditorContext::BezierEditorContext(QTreeWidget* tree,
                                         CurveEditor* widget,
                                         const boost::shared_ptr<Bezier>& curve,
                    RotoCurveEditorContext* context)
: _imp(new BezierEditorContextPrivate(widget,curve,context))
{
    _imp->nameItem = new QTreeWidgetItem(_imp->context->getItem());
    QString name(_imp->curve->getLabel().c_str());
    _imp->nameItem->setExpanded(true);
    _imp->nameItem->setText(0, name);
    QObject::connect(curve.get(), SIGNAL(keyframeSet(int)), this, SLOT(onKeyframeAdded()));
    QObject::connect(curve.get(), SIGNAL(keyframeRemoved(int)), this, SLOT(onKeyframeRemoved()));
    
    _imp->curveItem = new QTreeWidgetItem(_imp->nameItem);
    _imp->curveItem->setExpanded(true);
    _imp->curveItem->setText(0, "Animation");
    
    CurveWidget* cw = widget->getCurveWidget();
    
    boost::shared_ptr<RotoContext> roto = context->getNode()->getNode()->getRotoContext();
    _imp->animCurve = new BezierCPCurveGui(cw, curve, roto, name, QColor(255,255,255), 1.);
    _imp->animCurve->setVisible(false);
    cw->addCurveAndSetColor(_imp->animCurve);
    
    const std::list<boost::shared_ptr<KnobI> >& knobs = curve->getKnobs();
    
    
    bool hasAtLeast1KnobWithACurveShown = false;
    
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        createElementsForKnob(_imp->nameItem, 0, *it, widget, tree,roto, _imp->knobs, &hasAtLeast1KnobWithACurveShown);
    }

}

BezierEditorContext::~BezierEditorContext()
{
    if (_imp->doDeleteItem) {
        delete _imp->nameItem;
    }
    _imp->widget->getCurveWidget()->removeCurve(_imp->animCurve);
    for (std::list<NodeCurveEditorElement*>::iterator it = _imp->knobs.begin() ; it != _imp->knobs.end();++it) {
        delete *it;
    }
}

void
BezierEditorContext::preventItemDeletion()
{
    _imp->doDeleteItem = false;
}

boost::shared_ptr<Bezier>
BezierEditorContext::getBezier() const
{
    return _imp->curve;
}

QTreeWidgetItem*
BezierEditorContext::getItem() const
{
    return _imp->nameItem;
}

boost::shared_ptr<RotoContext>
BezierEditorContext::getContext() const
{
    return _imp->context->getNode()->getNode()->getRotoContext();
}

void
BezierEditorContext::onNameChanged(const QString & name)
{
    _imp->nameItem->setText(0, name);
}

void
BezierEditorContext::onKeyframeAdded()
{
    _imp->widget->update();
}

void
BezierEditorContext::onKeyframeRemoved()
{
    _imp->widget->update();
}

static void recursiveSelectElement(const std::list<NodeCurveEditorElement*>& elements,
                                   QTreeWidgetItem* cur,
                                   bool mustSelect,
                                   std::vector<CurveGui*> *curves)
{
    if (mustSelect) {
        for (std::list<NodeCurveEditorElement*>::const_iterator it = elements.begin(); it != elements.end(); ++it) {
            CurveGui* curve = (*it)->getCurve();
            cur->setSelected(true);
            if (curve  && curve->getInternalCurve()->isAnimated() ) {
                curves->push_back(curve);
            }
        }
    } else {
        for (std::list<NodeCurveEditorElement*>::const_iterator it = elements.begin(); it != elements.end(); ++it) {
            
            if ((*it)->getTreeItem() == cur) {
                CurveGui* curve = (*it)->getCurve();
                cur->setSelected(true);
                if (curve  && curve->getInternalCurve()->isAnimated() ) {
                    curves->push_back(curve);
                } else {
                    for (int i = 0; i < cur->childCount(); ++i) {
                        recursiveSelectElement(elements, cur->child(i), true, curves);
                    }
                }
                break;
            }
        }
    }
}

void
BezierEditorContext::recursiveSelectBezier(QTreeWidgetItem* cur,bool mustSelect,
                           std::vector<CurveGui*> *curves)
{
    if (mustSelect) {
        cur->setSelected(true);
    }
    if (_imp->nameItem == cur) {
        cur->setSelected(true);
        _imp->curveItem->setSelected(true);
        curves->push_back(_imp->animCurve);
        recursiveSelectElement(_imp->knobs, cur, true, curves);
    } else if (cur == _imp->curveItem) {
        cur->setSelected(true);
        curves->push_back(_imp->animCurve);
    } else {
        recursiveSelectElement(_imp->knobs, cur, false,  curves);
    }
}

const std::list<NodeCurveEditorElement*>&
BezierEditorContext::getElements() const
{
    return _imp->knobs;
}

NodeCurveEditorElement*
BezierEditorContext::findElement(KnobGui* knob,int dimension) const
{
    const std::string& name = knob->getKnob()->getName();
    for (std::list<NodeCurveEditorElement*>::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if ((*it)->getInternalKnob()->getName() == name && (*it)->getDimension() == dimension) {
            return *it;
        }
    }
    return 0;
}

struct RotoCurveEditorContextPrivate
{
    CurveEditor* widget;
    QTreeWidget* tree;
    boost::shared_ptr<NodeGui> node;
    QTreeWidgetItem* nameItem;
    std::list< BezierEditorContext* > curves;
    
    RotoCurveEditorContextPrivate(CurveEditor* widget,QTreeWidget *tree,const boost::shared_ptr<NodeGui>& node)
    : widget(widget)
    , tree(tree)
    , node(node)
    , nameItem(0)
    , curves()
    {
        
    }
};

RotoCurveEditorContext::RotoCurveEditorContext(CurveEditor* widget,
                                               QTreeWidget *tree,
                       const boost::shared_ptr<NodeGui>& node)
: _imp(new RotoCurveEditorContextPrivate(widget,tree,node))
{
    boost::shared_ptr<RotoContext> rotoCtx = node->getNode()->getRotoContext();
    assert(rotoCtx);
    
    _imp->nameItem = new QTreeWidgetItem(tree);
    _imp->nameItem->setExpanded(true);
    _imp->nameItem->setText( 0,_imp->node->getNode()->getLabel().c_str() );
    QObject::connect( node->getNode().get(),SIGNAL( labelChanged(QString) ),this,SLOT( onNameChanged(QString) ) );

    QObject::connect( rotoCtx.get(),SIGNAL( itemRemoved(boost::shared_ptr<RotoItem>,int) ),this,
                     SLOT( onItemRemoved(boost::shared_ptr<RotoItem>,int) ) );
    QObject::connect( rotoCtx.get(),SIGNAL( itemInserted(int) ),this,SLOT( itemInserted(int) ) );
    QObject::connect( rotoCtx.get(),SIGNAL( itemLabelChanged(boost::shared_ptr<RotoItem>) ),this,SLOT( onItemNameChanged(boost::shared_ptr<RotoItem>) ) );
    
    std::list<boost::shared_ptr<RotoDrawableItem> > curves = rotoCtx->getCurvesByRenderOrder();
    
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::iterator it = curves.begin(); it!=curves.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            BezierEditorContext* c = new BezierEditorContext(tree, widget, isBezier, this);
            _imp->curves.push_back(c);
        }
    }
}

RotoCurveEditorContext::~RotoCurveEditorContext()
{
    delete _imp->nameItem;
    for (std::list< BezierEditorContext*>::iterator it = _imp->curves.begin(); it!=_imp->curves.end(); ++it) {
        (*it)->preventItemDeletion();
        delete *it;
    }
}

void
RotoCurveEditorContext::setVisible(bool visible)
{
    for (std::list< BezierEditorContext*>::iterator it = _imp->curves.begin(); it!=_imp->curves.end(); ++it) {
        (*it)->getItem()->setHidden(!visible);
    }
}

boost::shared_ptr<NodeGui>
RotoCurveEditorContext::getNode() const
{
    return _imp->node;
}

QTreeWidgetItem*
RotoCurveEditorContext::getItem() const
{
    return _imp->nameItem;
}

void
RotoCurveEditorContext::onNameChanged(const QString & name)
{
     _imp->nameItem->setText(0,name);
}

void
RotoCurveEditorContext::onItemNameChanged(const boost::shared_ptr<RotoItem>& item)
{
    for (std::list<BezierEditorContext*>::iterator it = _imp->curves.begin(); it != _imp->curves.end(); ++it) {
        if ((*it)->getBezier() == item) {
            (*it)->onNameChanged(item->getLabel().c_str());
        }
    }
}

void
RotoCurveEditorContext::onItemRemoved(const boost::shared_ptr<RotoItem>& item, int)
{
    for (std::list<BezierEditorContext*>::iterator it = _imp->curves.begin(); it != _imp->curves.end(); ++it) {
        if ((*it)->getBezier() == item) {
            delete *it;
            _imp->curves.erase(it);
            return;
        }
    }
}

void
RotoCurveEditorContext::itemInserted(int)
{
    boost::shared_ptr<RotoContext> roto = _imp->node->getNode()->getRotoContext();
    assert(roto);
    boost::shared_ptr<RotoItem> item = roto->getLastInsertedItem();
    boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
    if (isBezier) {
        BezierEditorContext* b = new BezierEditorContext(_imp->tree, _imp->widget, isBezier, this);
        _imp->curves.push_back(b);
    }
}

void
RotoCurveEditorContext::recursiveSelectRoto(QTreeWidgetItem* cur,
                         std::vector<CurveGui*> *curves)
{
    if (cur == _imp->nameItem) {
        cur->setSelected(true);
        for (std::list<BezierEditorContext*>::iterator it = _imp->curves.begin(); it != _imp->curves.end(); ++it) {
            (*it)->recursiveSelectBezier((*it)->getItem(), true,curves);
        }
    } else {
        for (std::list<BezierEditorContext*>::iterator it = _imp->curves.begin(); it != _imp->curves.end(); ++it) {
            (*it)->recursiveSelectBezier(cur,false, curves);
        }
    }
}

const std::list<BezierEditorContext*>&
RotoCurveEditorContext::getElements() const
{
    return _imp->curves;
}

std::list<NodeCurveEditorElement*>
RotoCurveEditorContext::findElement(KnobGui* knob,int dimension) const
{
    
    std::list<NodeCurveEditorElement*> ret;
    
    KnobHolder* holder = knob->getKnob()->getHolder();
    if (!holder) {
        return ret;
    }
    
    Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
    if (!effect) {
        return ret;
    }
    
    boost::shared_ptr<RotoContext> roto = effect->getNode()->getRotoContext();
    if (!roto) {
        return ret;
    }
    
    std::list<boost::shared_ptr<RotoDrawableItem> > selectedBeziers = roto->getSelectedCurves();
    
    for (std::list<BezierEditorContext*>::const_iterator it = _imp->curves.begin(); it != _imp->curves.end(); ++it) {
        
        for (std::list<boost::shared_ptr<RotoDrawableItem> >::iterator it2 = selectedBeziers.begin(); it2 != selectedBeziers.end(); ++it2) {
            if (*it2 == (*it)->getBezier()) {
                NodeCurveEditorElement* found = (*it)->findElement(knob, dimension);
                if (found) {
                    ret.push_back(found);
                }
                break;
            }
        }
        
    }
    return ret;
}

void
CurveEditor::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_F && modCASIsControl(e)) {
        _imp->filterEdit->setFocus();
    }
}

CurveGui*
CurveEditor::getSelectedCurve() const
{
    return _imp->selectedKnobCurve;
}

void
CurveEditor::setSelectedCurve(CurveGui* curve)
{
    KnobCurveGui* knobCurve = dynamic_cast<KnobCurveGui*>(curve);
    if (curve && !knobCurve) {
        return;
    }
    
    _imp->selectedKnobCurve = knobCurve;
    
    if (knobCurve) {
        std::stringstream ss;
        boost::shared_ptr<KnobI> knob = knobCurve->getInternalKnob();
        assert(knob);
        KnobHolder* holder = knob->getHolder();
        assert(holder);
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(holder);
        assert(effect);
        ss << effect->getNode()->getFullyQualifiedName();
        ss << '.';
        ss << knob->getName();
        if (knob->getDimension() > 1) {
            ss << '.';
            ss << knob->getDimensionName(knobCurve->getDimension());
        }
        _imp->knobLabel->setText(ss.str().c_str());
        _imp->knobLabel->setAltered(false);
        std::string expr = knob->getExpression(knobCurve->getDimension());
        if (!expr.empty()) {
            _imp->knobLineEdit->setText(expr.c_str());
            double v = knob->getValueAtWithExpression(_imp->gui->getApp()->getTimeLine()->currentFrame(), knobCurve->getDimension());
            _imp->resultLabel->setText("= " + QString::number(v));
        } else {
            _imp->knobLineEdit->clear();
            _imp->resultLabel->setText("= ");
        }
        _imp->knobLineEdit->setReadOnly(false);
                _imp->resultLabel->setAltered(false);
    } else {
        _imp->knobLabel->setText(tr("No curve selected"));
        _imp->knobLabel->setAltered(true);
        _imp->knobLineEdit->clear();
        _imp->knobLineEdit->setReadOnly(true);
        _imp->resultLabel->setText("= ");
        _imp->resultLabel->setAltered(true);
    }
}

void
CurveEditor::refreshCurrentExpression()
{
    if (!_imp->selectedKnobCurve) {
        return;
    }
    boost::shared_ptr<KnobI> knob = _imp->selectedKnobCurve->getInternalKnob();
    std::string expr = knob->getExpression(_imp->selectedKnobCurve->getDimension());
    double v = knob->getValueAtWithExpression(_imp->gui->getApp()->getTimeLine()->currentFrame(), _imp->selectedKnobCurve->getDimension());
    _imp->knobLineEdit->setText(expr.c_str());
    _imp->resultLabel->setText("= " + QString::number(v));
}

void
CurveEditor::setSelectedCurveExpression(const QString& expression)
{
    
    if (!_imp->selectedKnobCurve) {
        return;
    }
    std::string expr = expression.toStdString();
    boost::shared_ptr<KnobI> knob = _imp->selectedKnobCurve->getKnobGui()->getKnob();
    int dim = _imp->selectedKnobCurve->getDimension();
    std::string exprResult;
    if (!expr.empty()) {
        try {
            knob->validateExpression(expr,dim, false, &exprResult);
        } catch (...) {
            _imp->resultLabel->setText(tr("Error"));
            return;
        }
    }
    _imp->curveWidget->pushUndoCommand(new SetExpressionCommand(knob,false,dim,expr));

}

void
CurveEditor::onExprLineEditFinished()
{
    setSelectedCurveExpression(_imp->knobLineEdit->text());
}