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

#include "CurveEditor.h"

#include <utility>

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

#include "Gui/CurveWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"


using std::make_pair;
using std::cout;
using std::endl;

void
CurveEditor::recursiveSelect(QTreeWidgetItem* cur,
                             std::vector<CurveGui*> *curves)
{
    if (!cur) {
        return;
    }
    cur->setSelected(true);
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
        NodeCurveEditorElement* elem = (*it)->findElement(cur);
        if (elem) {
            CurveGui* curve = elem->getCurve();
            if ( curve && curve->getInternalCurve()->isAnimated() ) {
                curves->push_back(curve);
            }
            break;
        }
    }
    for (int j = 0; j < cur->childCount(); ++j) {
        recursiveSelect(cur->child(j),curves);
    }
}

CurveEditor::CurveEditor(Gui* gui,
                         boost::shared_ptr<TimeLine> timeline,
                         QWidget *parent)
    : QWidget(parent)
      , _nodes()
      , _mainLayout(NULL)
      , _splitter(NULL)
      , _curveWidget(NULL)
      , _tree(NULL)
      , _undoStack( new QUndoStack() )
{
    setObjectName("CurveEditor");
    _undoAction = _undoStack->createUndoAction( this,tr("&Undo") );
    _undoAction->setShortcuts(QKeySequence::Undo);
    _redoAction = _undoStack->createRedoAction( this,tr("&Redo") );
    _redoAction->setShortcuts(QKeySequence::Redo);

    _mainLayout = new QHBoxLayout(this);
    setLayout(_mainLayout);
    _mainLayout->setContentsMargins(0,0,0,0);
    _mainLayout->setSpacing(0);

    _splitter = new QSplitter(Qt::Horizontal,this);
    _splitter->setObjectName("CurveEditorSplitter");

    _curveWidget = new CurveWidget(gui,timeline,_splitter);
    _curveWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    _tree = new QTreeWidget(_splitter);
    _tree->setObjectName("tree");
    _tree->setSelectionMode(QAbstractItemView::NoSelection);
    _tree->setColumnCount(1);
    _tree->header()->close();
    _tree->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);

    _splitter->addWidget(_tree);
    _splitter->addWidget(_curveWidget);


    _mainLayout->addWidget(_splitter);

    QObject::connect( _tree, SIGNAL( currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*) ),
                      this, SLOT( onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*) ) );
}

CurveEditor::~CurveEditor()
{
    for (std::list<NodeCurveEditorContext*>::iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
        delete (*it);
    }
    _nodes.clear();
    
    for (std::list<RotoCurveEditorContext*>::iterator it = _rotos.begin(); it != _rotos.end(); ++it) {
        delete *it;
    }
    _rotos.clear();
}

std::pair<QAction*,QAction*> CurveEditor::getUndoRedoActions() const
{
    return std::make_pair(_undoAction,_redoAction);
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
        NodeCurveEditorContext* nodeContext = new NodeCurveEditorContext(_tree,_curveWidget,node);
        _nodes.push_back(nodeContext);
    } else {
        RotoCurveEditorContext* rotoEditorCtx = new RotoCurveEditorContext(_curveWidget,_tree,node);
        _rotos.push_back(rotoEditorCtx);
    }
}

void
CurveEditor::removeNode(NodeGui* node)
{
    boost::shared_ptr<RotoContext> roto = node->getNode()->getRotoContext();
    if (!roto) {
        for (std::list<NodeCurveEditorContext*>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            if ( (*it)->getNode().get() == node ) {
                delete (*it);
                _nodes.erase(it);
                break;
            }
        }
    } else {
        for (std::list<RotoCurveEditorContext*>::iterator it = _rotos.begin(); it != _rotos.end(); ++it) {
            if ((*it)->getNode().get() == node) {
                delete *it;
                _rotos.erase(it);
                break;
            }
        }
    }
    _curveWidget->centerOn(-10,500,-10,10);
}

NodeCurveEditorContext::NodeCurveEditorContext(QTreeWidget* tree,
                                               CurveWidget* curveWidget,
                                               const boost::shared_ptr<NodeGui> &node)
    : _node(node)
      , _nodeElements()
      , _nameItem()
{
    QTreeWidgetItem* nameItem = new QTreeWidgetItem(tree);

    nameItem->setText( 0,_node->getNode()->getName().c_str() );

    QObject::connect( node.get(),SIGNAL( nameChanged(QString) ),this,SLOT( onNameChanged(QString) ) );
    const std::map<boost::shared_ptr<KnobI>,KnobGui*> & knobs = node->getKnobs();
    bool hasAtLeast1KnobWithACurve = false;
    bool hasAtLeast1KnobWithACurveShown = false;

    for (std::map<boost::shared_ptr<KnobI>,KnobGui*>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        const boost::shared_ptr<KnobI> & k = it->first;
        KnobGui* kgui = it->second;
        if ( !k->canAnimate() || ( k->typeName() == File_Knob::typeNameStatic() ) ) {
            continue;
        }

        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>( k.get() )->getSignalSlotHandler();
        QObject::connect( kgui,SIGNAL( keyFrameSet() ),curveWidget,SLOT( onCurveChanged() ) );
        QObject::connect( kgui,SIGNAL( keyFrameRemoved() ),curveWidget,SLOT( onCurveChanged() ) );
        QObject::connect( kgui, SIGNAL( keyInterpolationChanged() ),curveWidget, SLOT( refreshDisplayedTangents() ) );
        QObject::connect( kgui, SIGNAL( keyInterpolationChanged() ),curveWidget, SLOT( refreshDisplayedTangents() ) );
        QObject::connect( kgui, SIGNAL( refreshCurveEditor() ),curveWidget, SLOT( onCurveChanged() ) );

        hasAtLeast1KnobWithACurve = true;

        QTreeWidgetItem* knobItem = new QTreeWidgetItem(nameItem);

        knobItem->setText( 0,k->getDescription().c_str() );
        CurveGui* knobCurve = NULL;
        bool hideKnob = true;
        if (k->getDimension() == 1) {
            
            KnobCurveGui* knobCurve = new KnobCurveGui(curveWidget,kgui->getCurve(0),kgui,0,k->getDescription().c_str(),QColor(255,255,255),1.);
            
            if ( !k->getCurve(0)->isAnimated() ) {
                knobItem->setHidden(true);
            } else {
                hasAtLeast1KnobWithACurveShown = true;
                hideKnob = false;
            }
            knobCurve->setVisible(false);
        } else {
            for (int j = 0; j < k->getDimension(); ++j) {
                QTreeWidgetItem* dimItem = new QTreeWidgetItem(knobItem);
                dimItem->setText( 0,k->getDimensionName(j).c_str() );
                QString curveName = QString( k->getDescription().c_str() ) + "." + QString( k->getDimensionName(j).c_str() );
                
                KnobCurveGui* dimCurve = new KnobCurveGui(curveWidget,kgui->getCurve(j),kgui,j,curveName,QColor(255,255,255),1.);
                
                NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,kgui,j,dimItem,dimCurve);
                _nodeElements.push_back(elem);
                if ( !dimCurve->getInternalCurve()->isAnimated() ) {
                    dimItem->setHidden(true);
                } else {
                    hasAtLeast1KnobWithACurveShown = true;
                    hideKnob = false;
                }
                dimCurve->setVisible(false);
            }
        }


        if (hideKnob) {
            knobItem->setHidden(true);
        }
        NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,kgui,0,knobItem,knobCurve);
        _nodeElements.push_back(elem);
    }
    if (hasAtLeast1KnobWithACurve) {
        NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,(KnobGui*)NULL,-1,
                                                                  nameItem,(CurveGui*)NULL);
        _nodeElements.push_back(elem);
        if (!hasAtLeast1KnobWithACurveShown) {
            nameItem->setHidden(true);
        }
        _nameItem = nameItem;
    }
}

NodeCurveEditorContext::~NodeCurveEditorContext()
{
    for (U32 i = 0; i < _nodeElements.size(); ++i) {
        delete _nodeElements[i];
    }
    _nodeElements.clear();
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
        item->setExpanded(false);
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

void
NodeCurveEditorElement::checkVisibleState()
{
    if (!_curve) {
        return;
    }
    // even when there is only one keyframe, there may be tangents!
    if ( (_curve->getInternalCurve()->getKeyFramesCount() > 0) && !_knob->getKnob()->isSlave(_dimension) ) {
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
            std::vector<CurveGui*> curves;
            _curveWidget->getVisibleCurves(&curves);
            curves.push_back(_curve);
            _curveWidget->showCurvesAndHideOthers(curves);
            if (wasEmpty) {
                _curveWidget->centerOn(curves);
            }
        }
    } else {
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
    // also update the gui of the knob to indicate the animation is gone
    // the reason doesn't matter here
    //_knob->onInternalValueChanged(_dimension,Natron::eValueChangedReasonPluginEdited);
} // checkVisibleState

NodeCurveEditorElement::NodeCurveEditorElement(QTreeWidget *tree,
                                               CurveWidget* curveWidget,
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
      ,_dimension(dimension)
{
    if (knob) {
        QObject::connect( knob,SIGNAL( keyFrameSet() ),this,SLOT( checkVisibleState() ) );
        QObject::connect( knob,SIGNAL( keyFrameRemoved() ),this,SLOT( checkVisibleState() ) );
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
    _curveWidget->removeCurve(_curve);
    delete _treeItem;
}

void
CurveEditor::centerOn(const std::vector<boost::shared_ptr<Curve> > & curves)
{
    // find the curve's gui
    std::vector<CurveGui*> curvesGuis;

    for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
        const NodeCurveEditorContext::Elements & elems = (*it)->getElements();
        for (U32 i = 0; i < elems.size(); ++i) {
            CurveGui* curve = elems[i]->getCurve();
            if (curve) {
                std::vector<boost::shared_ptr<Curve> >::const_iterator found =
                    std::find( curves.begin(), curves.end(), curve->getInternalCurve() );
                if ( found != curves.end() ) {
                    curvesGuis.push_back(curve);
                    elems[i]->getTreeItem()->setSelected(true);
                } else {
                    elems[i]->getTreeItem()->setSelected(false);
                }
            } else {
                elems[i]->getTreeItem()->setSelected(false);
            }
        }
    }
    _curveWidget->centerOn(curvesGuis);
    _curveWidget->showCurvesAndHideOthers(curvesGuis);
}

void
CurveEditor::onCurrentItemChanged(QTreeWidgetItem* current,
                                  QTreeWidgetItem* /*previous*/)
{
    std::vector<CurveGui*> curves;

    QList<QTreeWidgetItem*> selectedItems = _tree->selectedItems();
    for (int i = 0; i < selectedItems.size(); ++i) {
        selectedItems.at(i)->setSelected(false);
    }
    recursiveSelect(current,&curves);

    _curveWidget->showCurvesAndHideOthers(curves);
    _curveWidget->centerOn(curves); //remove this if you don't want the editor to switch to a curve on a selection change
}

NodeCurveEditorElement*
NodeCurveEditorContext::findElement(CurveGui* curve) const
{
    for (U32 i = 0; i < _nodeElements.size(); ++i) {
        if (_nodeElements[i]->getCurve() == curve) {
            return _nodeElements[i];
        }
    }

    return NULL;
}

NodeCurveEditorElement*
NodeCurveEditorContext::findElement(KnobGui* knob,
                                    int dimension) const
{
    for (U32 i = 0; i < _nodeElements.size(); ++i) {
        if ( (_nodeElements[i]->getKnob() == knob) && (_nodeElements[i]->getDimension() == dimension) ) {
            return _nodeElements[i];
        }
    }

    return NULL;
}

NodeCurveEditorElement*
NodeCurveEditorContext::findElement(QTreeWidgetItem* item) const
{
    for (U32 i = 0; i < _nodeElements.size(); ++i) {
        if (_nodeElements[i]->getTreeItem() == item) {
            return _nodeElements[i];
        }
    }

    return NULL;
}

CurveGui*
CurveEditor::findCurve(KnobGui* knob,
                       int dimension) const
{
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
        NodeCurveEditorElement* elem = (*it)->findElement(knob,dimension);
        if (elem) {
            return elem->getCurve();
        }
    }

    return (CurveGui*)NULL;
}

void
CurveEditor::hideCurve(KnobGui* knob,
                       int dimension)
{
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
        NodeCurveEditorElement* elem = (*it)->findElement(knob,dimension);
        if (elem) {
            elem->getCurve()->setVisible(false);
            elem->getTreeItem()->setHidden(true);
            checkIfHiddenRecursivly( _tree, elem->getTreeItem() );
            break;
        }
    }
    _curveWidget->update();
}

void
CurveEditor::hideCurves(KnobGui* knob)
{
    for (int i = 0; i < knob->getKnob()->getDimension(); ++i) {
        for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
             it != _nodes.end(); ++it) {
            NodeCurveEditorElement* elem = (*it)->findElement(knob,i);
            if (elem) {
                elem->getCurve()->setVisible(false);
                elem->getTreeItem()->setHidden(true);
                checkIfHiddenRecursivly( _tree, elem->getTreeItem() );
                break;
            }
        }
    }
    _curveWidget->update();
}

void
CurveEditor::showCurve(KnobGui* knob,
                       int dimension)
{
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
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
    _curveWidget->update();
}

void
CurveEditor::showCurves(KnobGui* knob)
{
    for (int i = 0; i < knob->getKnob()->getDimension(); ++i) {
        for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
             it != _nodes.end(); ++it) {
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
    _curveWidget->update();
}

CurveWidget*
CurveEditor::getCurveWidget() const
{
    return _curveWidget;
}

struct CPEditorContextPrivate
{
    CurveWidget* widget;
    BezierEditorContext* bezier;
    boost::shared_ptr<BezierCP> cp;
    QTreeWidgetItem* itemX;
    QTreeWidgetItem* itemY;
    QTreeWidgetItem* itemLeftX;
    QTreeWidgetItem* itemLeftY;
    QTreeWidgetItem* itemRightX;
    QTreeWidgetItem* itemRightY;
    
    BezierCPCurveGui* curveX;
    BezierCPCurveGui* curveY;
    BezierCPCurveGui* curveLeftX;
    BezierCPCurveGui* curveLeftY;
    BezierCPCurveGui* curveRightX;
    BezierCPCurveGui* curveRightY;
    
    CPEditorContextPrivate(CurveWidget* widget,const boost::shared_ptr<BezierCP>& cp,BezierEditorContext* bezier)
    : widget(widget)
    , bezier(bezier)
    , cp(cp)
    , itemX(0)
    , itemY(0)
    , itemLeftX(0)
    , itemLeftY(0)
    , itemRightX(0)
    , itemRightY(0)
    , curveX(0)
    , curveY(0)
    , curveLeftX(0)
    , curveLeftY(0)
    , curveRightX(0)
    , curveRightY(0)
    {
     
    }
};

CPEditorContext::CPEditorContext(CurveWidget* widget,const boost::shared_ptr<BezierCP>& cp,BezierEditorContext* bezier)
: _imp(new CPEditorContextPrivate(widget,cp,bezier))
{
    QTreeWidgetItem* parent = bezier->getItem();
    _imp->itemX = new QTreeWidgetItem(parent);
    _imp->itemX->setText(0, "x");
    
    boost::shared_ptr<RotoContext> roto = bezier->getContext();
    assert(roto);
    
    _imp->curveX = new BezierCPCurveGui(widget,cp->getXCurve(),cp,roto,"x",QColor(255,255,255),1.);
    widget->addCurveAndSetColor(_imp->curveX);
    
    _imp->itemY = new QTreeWidgetItem(parent);
    _imp->itemY->setText(0, "y");
    _imp->curveY = new BezierCPCurveGui(widget,cp->getYCurve(),cp,roto,"y",QColor(255,255,255),1.);
    widget->addCurveAndSetColor(_imp->curveY);
    
    _imp->itemLeftX = new QTreeWidgetItem(parent);
    _imp->itemLeftX->setText(0, "leftX");
    _imp->curveLeftX = new BezierCPCurveGui(widget,cp->getLeftXCurve(),cp,roto,"leftX",QColor(255,255,255),1.);
    widget->addCurveAndSetColor(_imp->curveLeftX);
    
    _imp->itemLeftY = new QTreeWidgetItem(parent);
    _imp->itemLeftY->setText(0, "leftY");
    _imp->curveLeftY = new BezierCPCurveGui(widget,cp->getLeftYCurve(),cp,roto,"leftY",QColor(255,255,255),1.);
    widget->addCurveAndSetColor(_imp->curveLeftY);
    
    _imp->itemRightX = new QTreeWidgetItem(parent);
    _imp->itemRightX->setText(0, "rightX");
    _imp->curveRightX = new BezierCPCurveGui(widget,cp->getRightXCurve(),cp,roto,"rightX",QColor(255,255,255),1.);
    widget->addCurveAndSetColor(_imp->curveRightX);
    
    _imp->itemRightY = new QTreeWidgetItem(parent);
    _imp->itemRightY->setText(0, "rightY");
    _imp->curveRightY = new BezierCPCurveGui(widget,cp->getRightYCurve(),cp,roto,"rightY",QColor(255,255,255),1.);
    widget->addCurveAndSetColor(_imp->curveRightY);
    
}

CPEditorContext::~CPEditorContext()
{
    delete _imp->itemX;
    delete _imp->itemY;
    delete _imp->itemLeftX;
    delete _imp->itemLeftY;
    delete _imp->itemRightX;
    delete _imp->itemRightY;
    
    _imp->widget->removeCurve(_imp->curveX);
    _imp->widget->removeCurve(_imp->curveY);
    _imp->widget->removeCurve(_imp->curveLeftX);
    _imp->widget->removeCurve(_imp->curveLeftY);
    _imp->widget->removeCurve(_imp->curveRightX);
    _imp->widget->removeCurve(_imp->curveRightY);
}

BezierCP*
CPEditorContext::getCP() const
{
    return _imp->cp.get();
}

Bezier*
CPEditorContext::getBezier() const
{
    return _imp->bezier->getBezier();
}

struct BezierEditorContextPrivate {
    
    CurveWidget* widget;
    RotoCurveEditorContext* context;
    Bezier* curve;
    QTreeWidgetItem* item;
    std::list<CPEditorContext*> cps,fps;
    
    
    BezierEditorContextPrivate(CurveWidget* widget,Bezier* curve,RotoCurveEditorContext* context)
    : widget(widget)
    , context(context)
    , curve(curve)
    {
        
    }
};

BezierEditorContext::BezierEditorContext(CurveWidget* widget,
                                         Bezier* curve,
                    RotoCurveEditorContext* context)
: _imp(new BezierEditorContextPrivate(widget,curve,context))
{
    _imp->item = new QTreeWidgetItem(_imp->context->getItem());
    _imp->item->setText(0, _imp->curve->getName_mt_safe().c_str());
    _imp->item->setExpanded(false);
    QObject::connect(curve, SIGNAL(controlPointAdded()), this, SLOT(onControlPointAdded()));
    QObject::connect(curve, SIGNAL(controlPointRemoved()), this, SLOT(onControlPointRemoved()));
}

BezierEditorContext::~BezierEditorContext()
{
    for (std::list<CPEditorContext*>::iterator it = _imp->cps.begin(); it!=_imp->cps.end(); ++it) {
        delete *it;
    }
    for (std::list<CPEditorContext*>::iterator it = _imp->fps.begin(); it!=_imp->fps.end(); ++it) {
        delete *it;
    }

}

Bezier*
BezierEditorContext::getBezier() const
{
    return _imp->curve;
}

QTreeWidgetItem*
BezierEditorContext::getItem() const
{
    return _imp->item;
}

boost::shared_ptr<RotoContext>
BezierEditorContext::getContext() const
{
    return _imp->context->getNode()->getNode()->getRotoContext();
}

void
BezierEditorContext::onNameChanged(const QString & name)
{
    _imp->item->setText(0, name);
}

void
BezierEditorContext::onControlPointAdded()
{
    buildMissingCPs();
}

void
BezierEditorContext::onControlPointRemoved()
{
    
}

void
BezierEditorContext::removeUnexistingCPs()
{
    ///Make sure all points in the bezier exist
    
    {
        const std::list<boost::shared_ptr<BezierCP> >& cps = _imp->curve->getControlPoints();
    
        std::list<CPEditorContext*> toRemove;
        std::list<CPEditorContext*> toKeep;
        for (std::list<CPEditorContext*>::iterator it2 = _imp->cps.begin(); it2!=_imp->cps.end(); ++it2) {
            std::list<boost::shared_ptr<BezierCP> >::const_iterator found =
            std::find(cps.begin(),cps.end(),(*it2)->getCP());
            
            if (found == cps.end()) {
                toRemove.push_back(*it2);
            } else {
                toKeep.push_back(*it2);
            }
            
        }
        
        for (std::list<CPEditorContext*>::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
            delete *it;
        }
        _imp->cps = toKeep;
    }
    
    {
        const std::list<boost::shared_ptr<BezierCP> >& fps = _imp->curve->getControlPoints();
        
        std::list<CPEditorContext*> toRemove;
        std::list<CPEditorContext*> toKeep;
        for (std::list<CPEditorContext*>::iterator it2 = _imp->fps.begin(); it2!=_imp->fps.end(); ++it2) {
            std::list<boost::shared_ptr<BezierCP> >::const_iterator found =
            std::find(fps.begin(),fps.end(),(*it2)->getCP());
            
            if (found == fps.end()) {
                toRemove.push_back(*it2);
            } else {
                toKeep.push_back(*it2);
            }
            
        }
        
        for (std::list<CPEditorContext*>::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
            delete *it;
        }
        _imp->fps = toKeep;
    }

}

void
BezierEditorContext::buildMissingCPs()
{
    ///Make sure all points in the bezier are created
    const std::list<boost::shared_ptr<BezierCP> >& cps = _imp->curve->getControlPoints();
    for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin(); it != cps.end(); ++it) {
        
        bool found = false;
        for (std::list<CPEditorContext*>::iterator it2 = _imp->cps.begin(); it2!=_imp->cps.end(); ++it2) {
            if ((*it2)->getCP() == it->get()) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            CPEditorContext* c = new CPEditorContext(_imp->widget,*it,this);
            _imp->cps.push_back(c);
        }
    }
    
    const std::list<boost::shared_ptr<BezierCP> >& fps = _imp->curve->getControlPoints();
    for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it = fps.begin(); it != fps.end(); ++it) {
        
        bool found = false;
        for (std::list<CPEditorContext*>::iterator it2 = _imp->fps.begin(); it2!=_imp->fps.end(); ++it2) {
            if ((*it2)->getCP() == it->get()) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            CPEditorContext* c = new CPEditorContext(_imp->widget,*it,this);
            _imp->fps.push_back(c);
        }
    }
    
}

void
BezierEditorContext::onTreeItemExpanded(QTreeWidgetItem* item)
{
    if (item == _imp->item) {
        buildMissingCPs();
    }
}




struct RotoCurveEditorContextPrivate
{
    CurveWidget* widget;
    boost::shared_ptr<NodeGui> node;
    QTreeWidgetItem* nameItem;
    std::list< BezierEditorContext* > curves;
    
    RotoCurveEditorContextPrivate(CurveWidget* widget,const boost::shared_ptr<NodeGui>& node)
    : widget(widget)
    , node(node)
    , nameItem(0)
    , curves()
    {
        
    }
};

RotoCurveEditorContext::RotoCurveEditorContext(CurveWidget* widget,
                                               QTreeWidget *tree,
                       const boost::shared_ptr<NodeGui>& node)
: _imp(new RotoCurveEditorContextPrivate(widget,node))
{
    boost::shared_ptr<RotoContext> rotoCtx = node->getNode()->getRotoContext();
    assert(rotoCtx);
    
    _imp->nameItem = new QTreeWidgetItem(tree);
    _imp->nameItem->setText( 0,_imp->node->getNode()->getName().c_str() );
    QObject::connect( node.get(),SIGNAL( nameChanged(QString) ),this,SLOT( onNameChanged(QString) ) );
    QObject::connect( node.get(),SIGNAL( itemRemoved(RotoItem*,int) ),this,SLOT( onItemRemoved(RotoItem*,int) ) );
    QObject::connect( node.get(),SIGNAL( itemInserted(int) ),this,SLOT( itemInserted(int) ) );
    
    std::list<boost::shared_ptr<Bezier> > curves = rotoCtx->getCurvesByRenderOrder();
    
    for (std::list<boost::shared_ptr<Bezier> >::iterator it = curves.begin(); it!=curves.end(); ++it) {
        BezierEditorContext* c = new BezierEditorContext(widget,it->get(),this);
        _imp->curves.push_back(c);
    }
}

RotoCurveEditorContext::~RotoCurveEditorContext()
{
    delete _imp->nameItem;
    for (std::list< BezierEditorContext*>::iterator it = _imp->curves.begin(); it!=_imp->curves.end(); ++it) {
        delete *it;
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
RotoCurveEditorContext::onItemNameChanged(RotoItem* item)
{
    for (std::list<BezierEditorContext*>::iterator it = _imp->curves.begin(); it != _imp->curves.end(); ++it) {
        if ((*it)->getBezier() == item) {
            (*it)->onNameChanged(item->getName_mt_safe().c_str());
        }
    }
}

void
RotoCurveEditorContext::onItemRemoved(RotoItem* item,int)
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
    Bezier* isBezier = dynamic_cast<Bezier*>(item.get());
    if (isBezier) {
        BezierEditorContext* b = new BezierEditorContext(_imp->widget,isBezier,this);
        _imp->curves.push_back(b);
    }
}

