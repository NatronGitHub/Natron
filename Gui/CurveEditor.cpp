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
    for (std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
         it != _nodes.end(); ++it) {
        delete (*it);
    }
    _nodes.clear();
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

    NodeCurveEditorContext* nodeContext = new NodeCurveEditorContext(_tree,_curveWidget,node);
    _nodes.push_back(nodeContext);
}

void
CurveEditor::removeNode(NodeGui* node)
{
    for (std::list<NodeCurveEditorContext*>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
        if ( (*it)->getNode().get() == node ) {
            delete (*it);
            _nodes.erase(it);
            break;
        }
    }
    _curveWidget->centerOn(-10,500,-10,10);
}

NodeCurveEditorContext::NodeCurveEditorContext(QTreeWidget* tree,
                                               CurveWidget* curveWidget,
                                               boost::shared_ptr<NodeGui> node)
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

        hasAtLeast1KnobWithACurve = true;

        QTreeWidgetItem* knobItem = new QTreeWidgetItem(nameItem);

        knobItem->setText( 0,k->getDescription().c_str() );
        CurveGui* knobCurve = NULL;
        bool hideKnob = true;
        if (k->getDimension() == 1) {
            knobCurve = curveWidget->createCurve( k->getCurve(0),kgui,0,k->getDescription().c_str() );
            if ( !k->getCurve(0)->isAnimated() ) {
                knobItem->setHidden(true);
            } else {
                hasAtLeast1KnobWithACurveShown = true;
                hideKnob = false;
            }
        } else {
            for (int j = 0; j < k->getDimension(); ++j) {
                QTreeWidgetItem* dimItem = new QTreeWidgetItem(knobItem);
                dimItem->setText( 0,k->getDimensionName(j).c_str() );
                QString curveName = QString( k->getDescription().c_str() ) + "." + QString( k->getDimensionName(j).c_str() );
                CurveGui* dimCurve = curveWidget->createCurve(k->getCurve(j),kgui,j,curveName);
                NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,kgui,j,dimItem,dimCurve);
                _nodeElements.push_back(elem);
                if ( !dimCurve->getInternalCurve()->isAnimated() ) {
                    dimItem->setHidden(true);
                } else {
                    hasAtLeast1KnobWithACurveShown = true;
                    hideKnob = false;
                }
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
    _knob->onInternalValueChanged(_dimension,Natron::PLUGIN_EDITED);
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

