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

#ifndef CURVEEDITOR_H
#define CURVEEDITOR_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <QWidget>

#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

#include "Gui/CurveEditorUndoRedo.h"

class RectD;
class NodeGui;
class QTreeWidget;
class QTreeWidgetItem;
class CurveWidget;
class Curve;
class CurveGui;
class QHBoxLayout;
class QSplitter;
class KnobGui;
class KeyFrame;
class Variant;
class QAction;
class TimeLine;
class NodeCurveEditorElement : public QObject
{
    
    Q_OBJECT

public:
    
    NodeCurveEditorElement(QTreeWidget* tree,CurveWidget* curveWidget,KnobGui* knob,int dimension,
                           QTreeWidgetItem* item,CurveGui* curve);
    
    NodeCurveEditorElement():_treeItem(),_curve(),_curveDisplayed(false),_curveWidget(NULL){}
    
    virtual ~NodeCurveEditorElement() OVERRIDE;
    
    QTreeWidgetItem* getTreeItem() const WARN_UNUSED_RETURN {return _treeItem;}
    
    CurveGui* getCurve() const WARN_UNUSED_RETURN {return _curve;}

    bool isCurveVisible() const WARN_UNUSED_RETURN { return _curveDisplayed; }
    
    int getDimension() const WARN_UNUSED_RETURN { return _dimension; }

    KnobGui* getKnob() const WARN_UNUSED_RETURN { return _knob; }
    
public slots:
    
    void checkVisibleState();
        
private:
    
    
    QTreeWidgetItem* _treeItem;
    CurveGui* _curve;
    bool _curveDisplayed;
    CurveWidget* _curveWidget;
    QTreeWidget* _treeWidget;
    KnobGui* _knob;
    int _dimension;
};

class NodeCurveEditorContext : public QObject
{
    Q_OBJECT

public:

    typedef std::vector< NodeCurveEditorElement* > Elements;

    NodeCurveEditorContext(QTreeWidget *tree,CurveWidget* curveWidget,NodeGui* node);

    virtual ~NodeCurveEditorContext() OVERRIDE;

    NodeGui* getNode() const WARN_UNUSED_RETURN { return _node; }

    const Elements& getElements() const WARN_UNUSED_RETURN {return _nodeElements; }

    NodeCurveEditorElement* findElement(CurveGui* curve) const WARN_UNUSED_RETURN;

    NodeCurveEditorElement* findElement(KnobGui* knob,int dimension) const WARN_UNUSED_RETURN;

    NodeCurveEditorElement* findElement(QTreeWidgetItem* item) const WARN_UNUSED_RETURN;

public slots:

    void onNameChanged(const QString& name);
    
    
private:
    // FIXME: PIMPL
    NodeGui* _node;
    Elements _nodeElements;
    QTreeWidgetItem* _nameItem;

};

class CurveEditor  : public QWidget
{

    Q_OBJECT

public:

    CurveEditor(boost::shared_ptr<TimeLine> timeline,QWidget* parent = 0);

    virtual ~CurveEditor() OVERRIDE;

    void addNode(NodeGui *node);

    void removeNode(NodeGui* node);
    
    void centerOn(const std::vector<boost::shared_ptr<Curve> >& curves);
    
    std::pair<QAction*,QAction*> getUndoRedoActions() const WARN_UNUSED_RETURN;

    CurveGui* findCurve(KnobGui* knob,int dimension) const WARN_UNUSED_RETURN;
    
    void hideCurves(KnobGui* knob);
    
    void showCurves(KnobGui* knob);
    
    CurveWidget* getCurveWidget() const WARN_UNUSED_RETURN;
    
public slots:
    
    void onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*);

private:
    // FIXME: PIMPL
    void recursiveSelect(QTreeWidgetItem* cur,std::vector<CurveGui*> *curves);
    
    std::list<NodeCurveEditorContext*> _nodes;

    QHBoxLayout* _mainLayout;
    QSplitter* _splitter;
    CurveWidget* _curveWidget;
    QTreeWidget* _tree;
    boost::scoped_ptr<QUndoStack> _undoStack;
    QAction* _undoAction,*_redoAction;
};



#endif // CURVEEDITOR_H
