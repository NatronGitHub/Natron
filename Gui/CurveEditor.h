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

#include <QWidget>
#include <QtGui/QUndoCommand>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "Global/GlobalDefines.h"

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
class NodeCurveEditorElement : public QObject
{
    
    Q_OBJECT

public:
    
    NodeCurveEditorElement(QTreeWidget* tree,CurveWidget* curveWidget,KnobGui* knob,int dimension,
                           boost::shared_ptr<QTreeWidgetItem> item,CurveGui* curve);
    
    NodeCurveEditorElement():_treeItem(),_curve(),_curveDisplayed(false),_curveWidget(NULL){}
    
    virtual ~NodeCurveEditorElement();
    
    boost::shared_ptr<QTreeWidgetItem> getTreeItem() const {return _treeItem;}
    
    CurveGui* getCurve() const {return _curve;}

    bool isCurveVisible() const { return _curveDisplayed; }
    
    int getDimension() const { return _dimension; }

    KnobGui* getKnob() const { return _knob; }
    
public slots:
    
    void checkVisibleState();
    
    
private:
    
    
    boost::shared_ptr<QTreeWidgetItem> _treeItem;
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

    virtual ~NodeCurveEditorContext() ;

    NodeGui* getNode() const { return _node; }

    const Elements& getElements() const  {return _nodeElements; }

    NodeCurveEditorElement* findElement(CurveGui* curve);

    NodeCurveEditorElement* findElement(KnobGui* knob,int dimension);

    NodeCurveEditorElement* findElement(QTreeWidgetItem* item);

public slots:

    void onNameChanged(const QString& name);
    
    
private:

    NodeGui* _node;
    Elements _nodeElements;
    boost::shared_ptr<QTreeWidgetItem> _nameItem;

};

class CurveEditor  : public QWidget
{

    Q_OBJECT

public:

    CurveEditor(QWidget* parent = 0);

    virtual ~CurveEditor();

    void addNode(NodeGui *node);

    void removeNode(NodeGui* node);
    
    void centerOn(const std::vector<boost::shared_ptr<Curve> >& curves);
    
    std::pair<QAction*,QAction*> getUndoRedoActions() const;

    void addKeyFrame(KnobGui* knob,SequenceTime time,int dimension);

    void removeKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key);

    void setKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key,double x,const Variant& y);

public slots:
    
    void onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*);

private:
    
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
