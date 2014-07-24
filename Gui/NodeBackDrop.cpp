//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "NodeBackDrop.h"

#include <QVBoxLayout>
#include <QTextDocument>
#include <QGraphicsTextItem>
#include <QGraphicsPolygonItem>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QThread>
#include <QMenu>

#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Settings.h"

#include "Gui/Gui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiTypes.h"

#define RESIZE_HANDLE_SIZE 20


struct NodeBackDropPrivate
{
    
    NodeBackDrop* _publicInterface;
    NodeGraph* graph;
    
    QGraphicsRectItem* header;
    QGraphicsTextItem* name;
    
    mutable QMutex nameStringMutex;
    QString nameString;
    
    QGraphicsTextItem* label;
    QGraphicsPolygonItem* resizeHandle;
    
    QGraphicsLineItem* slaveMasterLink;
    NodeBackDrop* master;
    
    DockablePanel* settingsPanel;
    
    boost::shared_ptr<String_Knob> knobLabel;
    
    mutable QMutex positionMutex;
    mutable QMutex bboxMutex;
    
    bool isSelected;
    
    NodeBackDropPrivate(NodeBackDrop* publicInterface,NodeGraph* dag)
    : _publicInterface(publicInterface)
    , graph(dag)
    , header(0)
    , name(0)
    , nameStringMutex()
    , nameString()
    , label(0)
    , resizeHandle(0)
    , slaveMasterLink(0)
    , master(0)
    , settingsPanel(0)
    , knobLabel()
    , positionMutex()
    , isSelected(false)
    {
        
    }
    
    void setNameInternal(const QString& name);
    
    void setColorInternal(const QColor& color);
    
    void refreshLabelText(const QString& text);
    
};

NodeBackDrop::NodeBackDrop(NodeGraph* dag,QGraphicsItem* parent)
: QGraphicsRectItem(parent)
, NamedKnobHolder(dag->getGui()->getApp())
, _imp(new NodeBackDropPrivate(this,dag))
{

   
 
    

}

void NodeBackDrop::initialize(const QString& name,bool requestedByLoad,QVBoxLayout *dockContainer)
{
        
    QString tooltip(tr("The node backdrop is useful to group nodes and identify them in the node graph. You can also "
                    "move all the nodes inside the backdrop."));
    _imp->settingsPanel = new DockablePanel(_imp->graph->getGui(), //< pointer to the gui
                                            this, //< pointer to the knob holder (this)
                                            dockContainer, //< pointer to the layout that will contain this settings panel
                                            DockablePanel::FULLY_FEATURED, //< use a fully featured header with editable text
                                            false, //< don't use scroll areas for tabs
                                            name, //< initial name
                                            Qt::convertFromPlainText(tooltip,Qt::WhiteSpaceNormal), //< help tooltip
                                            false, //< no default page
                                            tr("BackDrop"), //< default page name
                                            dockContainer->parentWidget());
    
    
    ///initialize knobs here
    initializeKnobsPublic();
    
    QObject::connect(_imp->settingsPanel,SIGNAL(nameChanged(QString)),this,SLOT(onNameChanged(QString)));
    QObject::connect(_imp->settingsPanel,SIGNAL(colorChanged(QColor)),this,SLOT(onColorChanged(QColor)));
    dockContainer->addWidget(_imp->settingsPanel);
    
    if (!requestedByLoad) {
        _imp->graph->getGui()->putSettingsPanelFirst(_imp->settingsPanel);
        _imp->graph->getGui()->addVisibleDockablePanel(_imp->settingsPanel);
    } else {
        _imp->settingsPanel->setClosed(true);
    }
    
    
    
    setZValue(-10);
    
    _imp->header = new QGraphicsRectItem(this);
    _imp->header->setZValue(-9);
    
    _imp->name = new QGraphicsTextItem(name,this);
    _imp->name->setDefaultTextColor(QColor(0,0,0,255));
    _imp->name->setZValue(-8);
    
    _imp->label = new QGraphicsTextItem("",this);
    _imp->label->setDefaultTextColor(QColor(0,0,0,255));
    _imp->label->setZValue(-7);
    
    
    _imp->resizeHandle = new QGraphicsPolygonItem(this);
    _imp->resizeHandle->setZValue(-7);
    
    
    ///initialize knobs gui now
    _imp->settingsPanel->initializeKnobs();
    
    float r,g,b;
    appPTR->getCurrentSettings()->getDefaultBackDropColor(&r, &g, &b);
    QColor color;
    color.setRgbF(r, g, b);
    _imp->setColorInternal(color);
    
    _imp->setNameInternal(name);


    
}

NodeBackDrop::~NodeBackDrop()
{
    
}

void NodeBackDrop::initializeKnobs()
{
    _imp->knobLabel = Natron::createKnob<String_Knob>(this, tr("Label").toStdString());
    _imp->knobLabel->setAnimationEnabled(false);
    _imp->knobLabel->setAsMultiLine();
    _imp->knobLabel->setUsesRichText(true);
    _imp->knobLabel->setHintToolTip(tr("Text to display on the backdrop.").toStdString());
}

void NodeBackDropPrivate::setNameInternal(const QString& n)
{
    {
        QMutexLocker l(&nameStringMutex);
        nameString = n;
    }
    QString textLabel = n;
    textLabel.prepend("<div align=\"center\"><font size = 6>");
    textLabel.append("</font></div>");
    name->setHtml(textLabel);
    name->adjustSize();
    QRectF bbox = _publicInterface->boundingRect();
    _publicInterface->resize(bbox.width(), bbox.height());
}

void NodeBackDrop::onNameChanged(const QString& name)
{
    _imp->setNameInternal(name);
   
}

void NodeBackDrop::setPos_mt_safe(const QPointF& pos)
{
    {
        QMutexLocker l(&_imp->positionMutex);
        setPos(pos);
    }
    emit positionChanged();
}

QPointF NodeBackDrop::getPos_mt_safe() const
{
    QMutexLocker l(&_imp->positionMutex);
    return pos();
}

boost::shared_ptr<String_Knob> NodeBackDrop::getLabelKnob() const
{
    return _imp->knobLabel;
}

QColor NodeBackDrop::getCurrentColor() const
{
    return _imp->settingsPanel->getCurrentColor();
}

void NodeBackDrop::setCurrentColor(const QColor& color)
{
    _imp->settingsPanel->setCurrentColor(color);
    _imp->setColorInternal(color);
}

QString NodeBackDrop::getName() const
{
    QMutexLocker l(&_imp->nameStringMutex);
    return _imp->nameString;
}

std::string NodeBackDrop::getName_mt_safe() const
{
    QMutexLocker l(&_imp->nameStringMutex);
    return _imp->nameString.toStdString();
}

void NodeBackDrop::setName(const QString& str)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->setNameInternal(str);
    _imp->settingsPanel->onNameChanged(str);
}

void NodeBackDrop::onColorChanged(const QColor &color) {
    _imp->setColorInternal(color);
}

void NodeBackDropPrivate::setColorInternal(const QColor& color)
{
    _publicInterface->setBrush(color);
    
    if (isSelected) {
        float r,g,b;
        appPTR->getCurrentSettings()->getDefaultSelectedNodeColor(&r, &g, &b);
        QColor selCol;
        selCol.setRgbF(r, g, b);
        header->setBrush(selCol);
        resizeHandle->setBrush(selCol);
    } else {
        QColor brightenColor;
        brightenColor.setRgbF(Natron::clamp(color.redF() * 1.2),
                              Natron::clamp(color.greenF() * 1.2),
                              Natron::clamp(color.blueF() * 1.2));
        header->setBrush(brightenColor);
        resizeHandle->setBrush(brightenColor);
    }
}

DockablePanel* NodeBackDrop::getSettingsPanel() const
{
    return _imp->settingsPanel;
}

bool NodeBackDrop::isSettingsPanelClosed() const { return _imp->settingsPanel->isClosed(); }

void NodeBackDrop::setSettingsPanelClosed(bool closed)
{
    _imp->settingsPanel->setClosed(closed);
}

void NodeBackDrop::resize(int w,int h)
{
    QMutexLocker l(&_imp->bboxMutex);
    QPointF p = pos();
    QPointF thisItemPos = mapFromParent(p);
    
    QRectF textBbox = _imp->name->boundingRect();
    if (w < textBbox.width()) {
        w = textBbox.width();
    }
    
    int minH = (textBbox.height() * 1.5) + 20;
    
    if (h < minH) {
        h = minH;
    }
    
    
    setRect(QRectF(thisItemPos.x(),thisItemPos.y(),w,h));
    
    _imp->header->setRect(QRect(thisItemPos.x(),thisItemPos.y(),w,textBbox.height() * 1.5));
    
    _imp->name->setPos(thisItemPos.x() + w / 2 - textBbox.width() / 2,thisItemPos.y() + 0.25 * textBbox.height());
    _imp->label->setPos(thisItemPos.x(), thisItemPos.y() + textBbox.height() * 1.5 + 10);
    _imp->label->setTextWidth(w);
    
    QPolygonF resizeHandle;
    QPointF bottomRight(thisItemPos.x() + w,thisItemPos.y() + h);
    resizeHandle.push_back(QPointF(bottomRight.x() - 20,bottomRight.y()));
    resizeHandle.push_back(bottomRight);
    resizeHandle.push_back(QPointF(bottomRight.x(), bottomRight.y() - 20));
    _imp->resizeHandle->setPolygon(resizeHandle);
}

void NodeBackDrop::getSize(int& w,int& h) const
{
    QMutexLocker l(&_imp->bboxMutex);
    QRectF bbox = boundingRect();
    w = bbox.width();
    h = bbox.height();
}

void NodeBackDrop::onKnobValueChanged(KnobI* k,Natron::ValueChangedReason /*reason*/)
{
    if (k == _imp->knobLabel.get()) {
        QString text(_imp->knobLabel->getValue().c_str());
        _imp->refreshLabelText(text);
    }
}

void NodeBackDrop::refreshTextLabelFromKnob()
{
    _imp->refreshLabelText(QString(_imp->knobLabel->getValue().c_str()));
    ///if the knob is slaved, restore the visual link too
    
}

void NodeBackDropPrivate::refreshLabelText(const QString &text)
{
    QString textLabel = text;
    textLabel.replace("\n", "<br>");
    textLabel.prepend("<div align=\"left\">");
    textLabel.append("</div>");
    QFont f;
    String_KnobGui::parseFont(textLabel, f);
    label->setFont(f);

    label->setHtml(textLabel);
    
    QRectF labelBbox = label->boundingRect();
    QRectF nameBbox = name->boundingRect();
    QRectF bbox = _publicInterface->boundingRect();
    int w = std::max(std::max(bbox.width(), labelBbox.width()),nameBbox.width());
    int h = std::max(labelBbox.height() + nameBbox.height() * 1.5 + 10, bbox.height());
    _publicInterface->resize(w, h);
    _publicInterface->update();

}

bool NodeBackDrop::isNearbyHeader(const QPointF& scenePos) {
    QPointF p = mapFromScene(scenePos);
    QRectF headerBbox = _imp->header->boundingRect();
    headerBbox.adjust(-5, -5, 5, 5);
    return headerBbox.contains(p);
}

bool NodeBackDrop::isNearbyResizeHandle(const QPointF& scenePos)
{
    QPointF p = mapFromScene(scenePos);
    QPolygonF resizePoly = _imp->resizeHandle->polygon();
    return resizePoly.containsPoint(p,Qt::OddEvenFill);
}

void NodeBackDrop::setSelected(bool selected)
{
    _imp->isSelected = selected;
    _imp->setColorInternal(_imp->settingsPanel->getCurrentColor());
}


void NodeBackDrop::slaveTo(NodeBackDrop* master)
{
    _imp->master = master;
    assert(!_imp->slaveMasterLink);
    dynamic_cast<KnobI*>(_imp->knobLabel.get())->slaveTo(0, master->getLabelKnob(), 0);

    _imp->slaveMasterLink = new QGraphicsLineItem(parentItem());
    _imp->slaveMasterLink->setZValue(-10);
    QPen pen;
    pen.setWidth(3);
    pen.setBrush(QColor(200,100,100));
    _imp->slaveMasterLink->setPen(pen);
    QObject::connect(_imp->master, SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));
    QObject::connect(this, SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));
}

void NodeBackDrop::unslave()
{
    QObject::disconnect(_imp->master, SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));
    QObject::disconnect(this, SIGNAL(positionChanged()), this, SLOT(refreshSlaveMasterLinkPosition()));
    assert(_imp->slaveMasterLink);
    delete _imp->slaveMasterLink;
    dynamic_cast<KnobI*>(_imp->knobLabel.get())->unSlave(0, true);
    _imp->slaveMasterLink = 0;
    _imp->master = 0;
}

bool NodeBackDrop::isSlave() const
{
    return _imp->master != 0;
}

NodeBackDrop* NodeBackDrop::getMaster() const
{
    return _imp->master;
}

void NodeBackDrop::refreshSlaveMasterLinkPosition() {
    if (!_imp->master || !_imp->slaveMasterLink) {
        return;
    }
    
    QRectF bboxThisNode = boundingRect();
    QRectF bboxMasterNode = _imp->master->boundingRect();
    
    QPointF dst = _imp->slaveMasterLink->mapFromItem(_imp->master,QPointF(bboxMasterNode.x(),bboxMasterNode.y())
                                                + QPointF(bboxMasterNode.width() / 2., bboxMasterNode.height() / 2.));
    QPointF src = _imp->slaveMasterLink->mapFromItem(this,QPointF(bboxThisNode.x(),bboxThisNode.y())
                                                + QPointF(bboxThisNode.width() / 2., bboxThisNode.height() / 2.));
    _imp->slaveMasterLink->setLine(QLineF(src,dst));
}

void NodeBackDrop::deactivate()
{
    if (_imp->slaveMasterLink) {
        _imp->slaveMasterLink->hide();
    }
    setActive(false);
    setVisible(false);
    setSettingsPanelClosed(true);

}

void NodeBackDrop::activate()
{
    if (_imp->slaveMasterLink) {
        _imp->slaveMasterLink->show();
    }
    setActive(true);
    setVisible(true);
}
