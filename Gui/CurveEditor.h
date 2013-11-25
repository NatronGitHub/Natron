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

class RectD;
class NodeGui;
class QTreeWidget;
class QTreeWidgetItem;
class CurveWidget;
class CurveGui;
class QHBoxLayout;
class QSplitter;

class NodeCurveEditorElement : public QObject
{
    
    Q_OBJECT

public:
    
    NodeCurveEditorElement(CurveWidget* curveWidget,QTreeWidgetItem* item,CurveGui* curve);
    
    NodeCurveEditorElement():_treeItem(NULL),_curve(),_curveDisplayed(false),_curveWidget(NULL){}
    
    virtual ~NodeCurveEditorElement();
    
    QTreeWidgetItem* getTreeItem() const {return _treeItem;}
    
    CurveGui* getCurve() const {return _curve;}

    bool isCurveVisible() const { return _curveDisplayed; }
    
    
public slots:
    
    void onKeyFrameAdded();
    
    void onKeyFrameRemoved();
    
private:
    
    
    QTreeWidgetItem* _treeItem;
    CurveGui* _curve;
    bool _curveDisplayed;
    CurveWidget* _curveWidget;
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

public slots:

    void onNameChanged(const QString& name);
    
    
private:

    NodeGui* _node;
    Elements _nodeElements;
    QTreeWidgetItem* _nameItem;

};

class CurveEditor  : public QWidget
{



public:

    CurveEditor(QWidget* parent = 0);

    virtual ~CurveEditor(){

        for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
            it!=_nodes.end();++it){
            delete (*it);
        }
        _nodes.clear();
    }

    void addNode(NodeGui *node);

    void removeNode(NodeGui* node);
    
    void centerOn(const RectD& rect);
    
    void centerOn(double bottom,double left,double top,double right);

private:

    std::list<NodeCurveEditorContext*> _nodes;

    QHBoxLayout* _mainLayout;
    QSplitter* _splitter;
    CurveWidget* _curveWidget;
    QTreeWidget* _tree;
};

#endif // CURVEEDITOR_H
