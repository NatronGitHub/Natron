/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "TrackerPanel.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QHeaderView>
#include <QCheckBox>
#include <QItemSelection>
#include <QDebug>


#include "Engine/Settings.h"
#include "Engine/TrackerContext.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackMarker.h"

#include "Gui/NodeSettingsPanel.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGuiDouble.h"
#include "Gui/KnobGuiChoice.h"
#include "Gui/TableModelView.h"
#include "Gui/Utils.h"
#include "Gui/NodeGui.h"
#include "Gui/TrackerUndoCommand.h"
#include "Gui/NodeGraph.h"
#include "Gui/ClickableLabel.h"
#include "Gui/SpinBox.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"

#define NUM_COLS 10

#define COL_ENABLED 0
#define COL_LABEL 1
#define COL_SCRIPT_NAME 2
#define COL_MOTION_MODEL 3
#define COL_CENTER_X 4
#define COL_CENTER_Y 5
#define COL_OFFSET_X 6
#define COL_OFFSET_Y 7
#define COL_CORRELATION 8
#define COL_WEIGHT 9

#define kCornerPinInvertParamName "invert"


NATRON_NAMESPACE_ENTER;

class TrackerTableItemDelegate
: public QStyledItemDelegate
{
    TableView* _view;
    TrackerPanel* _panel;
    
public:
    
    explicit TrackerTableItemDelegate(TableView* view,
                               TrackerPanel* panel);
    
private:
    
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

TrackerTableItemDelegate::TrackerTableItemDelegate(TableView* view,
                                     TrackerPanel* panel)
: QStyledItemDelegate(view)
, _view(view)
, _panel(panel)
{
}

void
TrackerTableItemDelegate::paint(QPainter * painter,
                         const QStyleOptionViewItem & option,
                         const QModelIndex & index) const
{
    assert(index.isValid());
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    
    TableModel* model = dynamic_cast<TableModel*>( _view->model() );
    assert(model);
    if (!model) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    TableItem* item = model->item(index);
    assert(item);
    if (!item) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    
    int col = index.column();
    if (col != COL_CENTER_X &&
        col != COL_CENTER_Y &&
        col != COL_OFFSET_X &&
        col != COL_OFFSET_Y &&
        col != COL_WEIGHT &&
        col != COL_CORRELATION) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    
    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    
    int dim;
    Natron::AnimationLevelEnum level = eAnimationLevelNone;
    boost::shared_ptr<KnobI> knob = _panel->getKnobAt(index.row(), index.column(), &dim);
    assert(knob);
    if (knob) {
        level = knob->getAnimationLevel(dim);
    }
    
    bool fillRect = true;
    QBrush brush;
    if (option.state & QStyle::State_Selected) {
        brush = option.palette.highlight();
    } else if (level == eAnimationLevelInterpolatedValue) {
        double r,g,b;
        appPTR->getCurrentSettings()->getInterpolatedColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else if (level == eAnimationLevelOnKeyframe) {
        double r,g,b;
        appPTR->getCurrentSettings()->getKeyframeColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else {
        fillRect = false;
    }
    if (fillRect) {
        painter->fillRect( geom, brush);
    }
    
    QPen pen = painter->pen();
    if (!item->flags().testFlag(Qt::ItemIsEditable)) {
        pen.setColor(Qt::black);
    } else {
        double r,g,b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        pen.setColor(col);
    }
    painter->setPen(pen);
    
    QRect textRect( geom.x() + 5,geom.y(),geom.width() - 5,geom.height() );
    QRect r;
    QString data;
    QVariant var = item->data(Qt::DisplayRole);
    if (var.canConvert(QVariant::String)) {
        data = var.toString();
    } else if (var.canConvert(QVariant::Double)) {
        double d = var.toDouble();
        data = QString::number(d);
    } else if (var.canConvert(QVariant::Int)) {
        int i = var.toInt();
        data = QString::number(i);
    }
    
    painter->drawText(textRect,Qt::TextSingleLine,data,&r);
    
}

struct ItemData
{
    TableItem* item;
    boost::weak_ptr<KnobI> knob;
    int dimension;
};

struct TrackDatas
{
    std::vector<ItemData> items;
    boost::weak_ptr<TrackMarker> marker;
};

typedef std::vector< TrackDatas> TrackItems;

struct TrackKeys
{
    std::set<int> userKeys;
    std::set<double> centerKeys;
    bool visible;
    
    TrackKeys(): userKeys(), centerKeys(), visible(false) {}
    
};

typedef std::map<boost::weak_ptr<TrackMarker>,TrackKeys  > TrackKeysMap;

struct TrackerPanelPrivate
{
    TrackerPanel* _publicInterface;
    boost::weak_ptr<NodeGui> node;
    boost::weak_ptr<TrackerContext> context;
  
    TrackItems items;
    
    QVBoxLayout* mainLayout;
    
    TableView* view;
    TableModel* model;
    
    Natron::Label* exportLabel;
    QWidget* exportContainer;
    QHBoxLayout* exportLayout;
    ComboBox* exportChoice;
    Button* exportButton;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    Button* addButton;
    Button* removeButton;
    Button* selectAllButton;
    Button* resetTracksButton;
    Button* averageTracksButton;

    int selectionBlocked;
    int selectionRecursion;
    
    TrackKeysMap keys;
    
    ClickableLabel* trackLabel;
    SpinBox* currentKeyframe;
    ClickableLabel* ofLabel;
    SpinBox* totalKeyframes;
    Button* prevKeyframe;
    Button* nextKeyframe;
    Button* addKeyframe;
    Button* removeKeyframe;
    Button* clearAnimation;
    
    TrackerPanelPrivate(TrackerPanel* publicI, const NodeGuiPtr& n)
    : _publicInterface(publicI)
    , node(n)
    , context()
    , items()
    , mainLayout(0)
    , view(0)
    , model(0)
    , exportLabel(0)
    , exportContainer(0)
    , exportLayout(0)
    , exportChoice(0)
    , exportButton(0)
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , removeButton(0)
    , selectAllButton(0)
    , resetTracksButton(0)
    , averageTracksButton(0)
    , selectionBlocked(0)
    , selectionRecursion(0)
    , keys()
    , trackLabel(0)
    , currentKeyframe(0)
    , ofLabel(0)
    , totalKeyframes(0)
    , prevKeyframe(0)
    , nextKeyframe(0)
    , addKeyframe(0)
    , removeKeyframe(0)
    , clearAnimation(0)
    {
        context = n->getNode()->getTrackerContext();
        assert(context.lock());
    }
    
    void makeTrackRowItems(const TrackMarker& marker, int row, TrackDatas& data);
        
    void markersToSelection(const std::list<boost::shared_ptr<TrackMarker> >& markers, QItemSelection* selection);
    
    void selectionToMarkers(const QItemSelection& selection, std::list<boost::shared_ptr<TrackMarker> >* markers);
    
    void createCornerPinFromSelection(const std::list<boost::shared_ptr<TrackMarker> > & selection,bool linked,bool useTransformRefFrame,bool invert);
    
    void setVisibleItemKeyframes(const std::list<int>& keys,bool visible, bool emitSignal);
    
    void setVisibleItemUserKeyframes(const std::list<int>& keys,bool visible, bool emitSignal);
    
    void updateTrackKeysInfoBar(int time);
};

TrackerPanel::TrackerPanel(const NodeGuiPtr& n,
                           QWidget* parent)
: QWidget(parent)
, _imp(new TrackerPanelPrivate(this, n))
{
    boost::shared_ptr<TrackerContext> context = getContext();
    
    QObject::connect( n->getNode()->getApp()->getTimeLine().get(), SIGNAL( frameChanged(SequenceTime,int) ), this,
                     SLOT( onTimeChanged(SequenceTime, int) ) );
    
    QObject::connect(context.get(), SIGNAL(selectionChanged(int)), this, SLOT(onContextSelectionChanged(int)));
    QObject::connect(context.get(), SIGNAL(selectionAboutToChange(int)), this, SLOT(onContextSelectionAboutToChange(int)));
    
    QObject::connect(context.get(), SIGNAL(trackInserted(boost::shared_ptr<TrackMarker>,int)), this, SLOT(onTrackInserted(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(trackRemoved(boost::shared_ptr<TrackMarker>)), this, SLOT(onTrackRemoved(boost::shared_ptr<TrackMarker>)));
    
    QObject::connect(context.get(), SIGNAL(trackCloned(boost::shared_ptr<TrackMarker>)),
                     this, SLOT(onTrackCloned(boost::shared_ptr<TrackMarker>)));
    QObject::connect(context.get(), SIGNAL(trackAboutToClone(boost::shared_ptr<TrackMarker>)),
                     this, SLOT(onTrackAboutToClone(boost::shared_ptr<TrackMarker>)));

    
    QObject::connect(context.get(), SIGNAL(keyframeSetOnTrack(boost::shared_ptr<TrackMarker>,int)),
                     this, SLOT(onTrackKeyframeSet(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(keyframeRemovedOnTrack(boost::shared_ptr<TrackMarker>,int)),
                     this, SLOT(onTrackKeyframeRemoved(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(allKeyframesRemovedOnTrack(boost::shared_ptr<TrackMarker>)),
                     this, SLOT(onTrackAllKeyframesRemoved(boost::shared_ptr<TrackMarker>)));
    
    QObject::connect(context.get(), SIGNAL(keyframeSetOnTrackCenter(boost::shared_ptr<TrackMarker>,int)),
                    this, SLOT(onKeyframeSetOnTrackCenter(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(keyframeRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>,int)),
                     this, SLOT(onKeyframeRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>,int)));
    QObject::connect(context.get(), SIGNAL(allKeyframesRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>)),
                     this, SLOT(onAllKeyframesRemovedOnTrackCenter(boost::shared_ptr<TrackMarker>)));
    QObject::connect(context.get(), SIGNAL(multipleKeyframesSetOnTrackCenter(boost::shared_ptr<TrackMarker>,std::list<double>)),
                     this, SLOT(onMultipleKeyframesSetOnTrackCenter(boost::shared_ptr<TrackMarker>,std::list<double>)));
    
    
    QObject::connect(context.get(), SIGNAL(enabledChanged(boost::shared_ptr<TrackMarker>,int)),this,
                     SLOT(onEnabledChanged(boost::shared_ptr<TrackMarker>,int)));
                     
    QObject::connect(context.get(), SIGNAL(centerKnobValueChanged(boost::shared_ptr<TrackMarker>,int,int)), this,
                     SLOT(onCenterKnobValueChanged(boost::shared_ptr<TrackMarker>, int, int)));
    QObject::connect(context.get(), SIGNAL(offsetKnobValueChanged(boost::shared_ptr<TrackMarker>,int,int)), this,
                     SLOT(onOffsetKnobValueChanged(boost::shared_ptr<TrackMarker>, int, int)));
    QObject::connect(context.get(), SIGNAL(correlationKnobValueChanged(boost::shared_ptr<TrackMarker>,int,int)), this,
                     SLOT(onCorrelationKnobValueChanged(boost::shared_ptr<TrackMarker>, int, int)));
    QObject::connect(context.get(), SIGNAL(weightKnobValueChanged(boost::shared_ptr<TrackMarker>,int,int)), this,
                     SLOT(onWeightKnobValueChanged(boost::shared_ptr<TrackMarker>, int, int)));
    QObject::connect(context.get(), SIGNAL(motionModelKnobValueChanged(boost::shared_ptr<TrackMarker>,int,int)), this,
                     SLOT(onMotionModelKnobValueChanged(boost::shared_ptr<TrackMarker>, int, int)));
        
    _imp->mainLayout = new QVBoxLayout(this);
    
    QWidget* trackContainer = new QWidget(this);
    _imp->mainLayout->addWidget(trackContainer);
    
    QHBoxLayout* trackLayout = new QHBoxLayout(trackContainer);
    trackLayout->setSpacing(2);
    _imp->trackLabel = new ClickableLabel(tr("Track keyframe:"),trackContainer);
    _imp->trackLabel->setSunken(false);
    trackLayout->addWidget(_imp->trackLabel);
    
    _imp->currentKeyframe = new SpinBox(trackContainer,SpinBox::eSpinBoxTypeDouble);
    _imp->currentKeyframe->setEnabled(false);
    _imp->currentKeyframe->setReadOnly(true);
    _imp->currentKeyframe->setToolTip(GuiUtils::convertFromPlainText(tr("The current keyframe for the selected track(s)."), Qt::WhiteSpaceNormal));
    trackLayout->addWidget(_imp->currentKeyframe);
    
    _imp->ofLabel = new ClickableLabel(tr("of"),trackContainer);
    trackLayout->addWidget(_imp->ofLabel);
    
    _imp->totalKeyframes = new SpinBox(trackContainer,SpinBox::eSpinBoxTypeInt);
    _imp->totalKeyframes->setEnabled(false);
    _imp->totalKeyframes->setReadOnly(true);
    _imp->totalKeyframes->setToolTip(GuiUtils::convertFromPlainText(tr("The keyframe count for all the selected tracks."), Qt::WhiteSpaceNormal));
    trackLayout->addWidget(_imp->totalKeyframes);
    
    QPixmap prevPix,nextPix,addPix,removePix,clearAnimPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS_KEY,NATRON_MEDIUM_BUTTON_ICON_SIZE, &prevPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT_KEY, NATRON_MEDIUM_BUTTON_ICON_SIZE,&nextPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_USER_KEY, NATRON_MEDIUM_BUTTON_ICON_SIZE,&addPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_USER_KEY,NATRON_MEDIUM_BUTTON_ICON_SIZE, &removePix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_ALL_ANIMATION,NATRON_MEDIUM_BUTTON_ICON_SIZE, &clearAnimPix);
    
    _imp->prevKeyframe = new Button(QIcon(prevPix),QString(),trackContainer);
    _imp->prevKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->prevKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE));
    _imp->prevKeyframe->setToolTip(GuiUtils::convertFromPlainText(tr("Go to the previous keyframe."), Qt::WhiteSpaceNormal));
    _imp->prevKeyframe->setEnabled(false);
    QObject::connect( _imp->prevKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onGoToPrevKeyframeButtonClicked() ) );
    trackLayout->addWidget(_imp->prevKeyframe);
    
    _imp->nextKeyframe = new Button(QIcon(nextPix),QString(),trackContainer);
    _imp->nextKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->nextKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE));
    _imp->nextKeyframe->setToolTip(GuiUtils::convertFromPlainText(tr("Go to the next keyframe."), Qt::WhiteSpaceNormal));
    _imp->nextKeyframe->setEnabled(false);
    QObject::connect( _imp->nextKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onGoToNextKeyframeButtonClicked() ) );
    trackLayout->addWidget(_imp->nextKeyframe);
    
    _imp->addKeyframe = new Button(QIcon(addPix),QString(),trackContainer);
    _imp->addKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->addKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE));
    _imp->addKeyframe->setToolTip(GuiUtils::convertFromPlainText(tr("Add keyframe at the current timeline's time."), Qt::WhiteSpaceNormal));
    _imp->addKeyframe->setEnabled(false);
    QObject::connect( _imp->addKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onAddKeyframeButtonClicked() ) );
    trackLayout->addWidget(_imp->addKeyframe);
    
    _imp->removeKeyframe = new Button(QIcon(removePix),QString(),trackContainer);
    _imp->removeKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->removeKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE));
    _imp->removeKeyframe->setToolTip(GuiUtils::convertFromPlainText(tr("Remove keyframe at the current timeline's time."), Qt::WhiteSpaceNormal));
    _imp->removeKeyframe->setEnabled(false);
    QObject::connect( _imp->removeKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onRemoveKeyframeButtonClicked() ) );
    trackLayout->addWidget(_imp->removeKeyframe);
    
    _imp->clearAnimation = new Button(QIcon(clearAnimPix),QString(),trackContainer);
    _imp->clearAnimation->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearAnimation->setIconSize(QSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE));
    _imp->clearAnimation->setToolTip(GuiUtils::convertFromPlainText(tr("Remove all animation for the selected track(s)."), Qt::WhiteSpaceNormal));
    _imp->clearAnimation->setEnabled(false);
    QObject::connect( _imp->clearAnimation, SIGNAL( clicked(bool) ), this, SLOT( onRemoveAnimationButtonClicked() ) );
    trackLayout->addWidget(_imp->clearAnimation);
    
    
    trackLayout->addStretch();

    
    ///-------- TableView
    
    _imp->view = new TableView(this);
    QObject::connect( _imp->view,SIGNAL( deleteKeyPressed() ),this,SLOT( onRemoveButtonClicked() ) );
    QObject::connect( _imp->view,SIGNAL( itemRightClicked(TableItem*) ),this,SLOT( onItemRightClicked(TableItem*) ) );
    TrackerTableItemDelegate* delegate = new TrackerTableItemDelegate(_imp->view,this);
    _imp->view->setItemDelegate(delegate);
    
    _imp->model = new TableModel(0,0,_imp->view);
    QObject::connect( _imp->model,SIGNAL( s_itemChanged(TableItem*) ),this,SLOT( onItemDataChanged(TableItem*) ) );
    _imp->view->setTableModel(_imp->model);
    _imp->view->setUniformRowHeights(true);
    QItemSelectionModel *selectionModel = _imp->view->selectionModel();
    QObject::connect( selectionModel, SIGNAL( selectionChanged(QItemSelection,QItemSelection) ),this,
                     SLOT( onModelSelectionChanged(QItemSelection,QItemSelection) ) );
    
    

    QStringList dimensionNames;
    dimensionNames << tr("Enabled") << tr("Label") << tr("Script-name") << tr("Motion-Model") <<
    tr("Center X") << tr("Center Y") << tr("Offset X") << tr("Offset Y") <<
    tr("Correlation") << tr("Weight");
    _imp->view->setColumnCount( dimensionNames.size() );
    _imp->view->setHorizontalHeaderLabels(dimensionNames);
    
    
#if QT_VERSION < 0x050000
    _imp->view->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->view->header()->setStretchLastSection(true);
    
    _imp->mainLayout->addWidget(_imp->view);

    
    _imp->exportLabel = new Natron::Label( tr("Export data"),this);
    _imp->mainLayout->addWidget(_imp->exportLabel);
    _imp->mainLayout->addSpacing(10);
    _imp->exportContainer = new QWidget(this);
    _imp->exportLayout = new QHBoxLayout(_imp->exportContainer);
    _imp->exportLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->exportChoice = new ComboBox(_imp->exportContainer);
    _imp->exportChoice->setToolTip( QString::fromUtf8("<p><b>") + tr("CornerPin (Use current frame):") + QString::fromUtf8("</p></b>") +
                                   QString::fromUtf8("<p>") + tr("Warp the image according to the relative transform using the current frame as reference.") + QString::fromUtf8("</p>") +
                                   QString::fromUtf8("<p><b>") + tr("CornerPin (Use transform ref frame):") + QString::fromUtf8("</p></b>") +
                                   QString::fromUtf8("<p>") + tr("Warp the image according to the relative transform using the "
                                              "reference frame specified in the transform tab.") + QString::fromUtf8("</p>") +
                                   QString::fromUtf8("<p><b>") + tr("CornerPin (Stabilize):") + QString::fromUtf8("</p></b>") +
                                   QString::fromUtf8("<p>") + tr("Transform the image so that the tracked points do not move.") + QString::fromUtf8("</p>")
                                   //                                      "<p><b>" + tr("Transform (Stabilize):</p></b>"
                                   //                                      "<p>" + tr("Transform the image so that the tracked points do not move.") + "</p>"
                                   //                                      "<p><b>" + tr("Transform (Match-move):</p></b>"
                                   //                                      "<p>" + tr("Transform another image so that it moves to match the tracked points.") + "</p>"
                                   //                                      "<p>" + tr("The linked versions keep a link between the new node and the track, the others just copy"
                                   //                                      " the values.") + "</p>"
                                   );
    std::vector<std::string> choices;
    std::vector<std::string> helps;
    
    choices.push_back(tr("CornerPin (Use current frame. Linked)").toStdString());
    helps.push_back(tr("Warp the image according to the relative transform using the current frame as reference.").toStdString());
    //
    //    choices.push_back(tr("CornerPinOFX (Use transform ref frame. Linked)").toStdString());
    //    helps.push_back(tr("Warp the image according to the relative transform using the "
    //                       "reference frame specified in the transform tab.").toStdString());
    
    choices.push_back(tr("CornerPin (Stabilize. Linked)").toStdString());
    helps.push_back(tr("Transform the image so that the tracked points do not move.").toStdString());
    
    choices.push_back( tr("CornerPin (Use current frame. Copy)").toStdString() );
    helps.push_back( tr("Same as the linked version except that it copies values instead of "
                        "referencing them via a link to the track").toStdString() );
    
    choices.push_back(tr("CornerPin (Stabilize. Copy)").toStdString());
    helps.push_back(tr("Same as the linked version except that it copies values instead of "
                       "referencing them via a link to the track").toStdString());
    
    choices.push_back( tr("CornerPin (Use transform ref frame. Copy)").toStdString() );
    helps.push_back( tr("Same as the linked version except that it copies values instead of "
                        "referencing them via a link to the track").toStdString() );
    
    
    //    choices.push_back(tr("Transform (Stabilize. Linked)").toStdString());
    //    helps.push_back(tr("Transform the image so that the tracked points do not move.").toStdString());
    //
    //    choices.push_back(tr("Transform (Match-move. Linked)").toStdString());
    //    helps.push_back(tr("Transform another image so that it moves to match the tracked points.").toStdString());
    //
    //    choices.push_back(tr("Transform (Stabilize. Copy)").toStdString());
    //    helps.push_back(tr("Same as the linked version except that it copies values instead of "
    //                       "referencing them via a link to the track").toStdString());
    //
    //    choices.push_back(tr("Transform (Match-move. Copy)").toStdString());
    //    helps.push_back(tr("Same as the linked version except that it copies values instead of "
    //                       "referencing them via a link to the track").toStdString());
    for (U32 i = 0; i < choices.size(); ++i) {
        _imp->exportChoice->addItem( QString::fromUtf8(choices[i].c_str()),QIcon(),QKeySequence(),QString::fromUtf8(helps[i].c_str()));
    }
    _imp->exportLayout->addWidget(_imp->exportChoice);
    
    _imp->exportButton = new Button(tr("Export"),_imp->exportContainer);
    QObject::connect(_imp->exportButton,SIGNAL(clicked(bool)),this,SLOT(onExportButtonClicked()));
    _imp->exportLayout->addWidget(_imp->exportButton);
    _imp->exportLayout->addStretch();
    _imp->mainLayout->addWidget(_imp->exportContainer);
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->setContentsMargins(0, 0, 0, 0);
    _imp->addButton = new Button(QIcon(),QString::fromUtf8("+"),_imp->buttonsContainer);
    _imp->addButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _imp->addButton->setToolTip(GuiUtils::convertFromPlainText(tr("Add new track"), Qt::WhiteSpaceNormal));
    _imp->buttonsLayout->addWidget(_imp->addButton);
    QObject::connect( _imp->addButton, SIGNAL( clicked(bool) ), this, SLOT( onAddButtonClicked() ) );
    
    _imp->removeButton = new Button(QIcon(),QString::fromUtf8("-"),_imp->buttonsContainer);
    _imp->removeButton->setToolTip(GuiUtils::convertFromPlainText(tr("Remove selected tracks"), Qt::WhiteSpaceNormal));
    _imp->removeButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _imp->buttonsLayout->addWidget(_imp->removeButton);
    QObject::connect( _imp->removeButton, SIGNAL( clicked(bool) ), this, SLOT( onRemoveButtonClicked() ) );
    QPixmap selectAll;
    appPTR->getIcon(NATRON_PIXMAP_SELECT_ALL, &selectAll);
    
    _imp->selectAllButton = new Button(QIcon(selectAll),QString(),_imp->buttonsContainer);
    _imp->selectAllButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _imp->selectAllButton->setIconSize(QSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE));
    _imp->selectAllButton->setToolTip(GuiUtils::convertFromPlainText(tr("Select all tracks"), Qt::WhiteSpaceNormal));
    _imp->buttonsLayout->addWidget(_imp->selectAllButton);
    QObject::connect( _imp->selectAllButton, SIGNAL( clicked(bool) ), this, SLOT( onSelectAllButtonClicked() ) );
    
    _imp->resetTracksButton = new Button(tr("Reset"),_imp->buttonsContainer);
    QObject::connect( _imp->resetTracksButton, SIGNAL( clicked(bool) ), this, SLOT( onResetButtonClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->resetTracksButton);
    _imp->resetTracksButton->setToolTip(GuiUtils::convertFromPlainText(tr("Reset selected items."), Qt::WhiteSpaceNormal));
    
    _imp->buttonsLayout->addStretch();
    
    _imp->mainLayout->addWidget(_imp->buttonsContainer);
 
    ///Restore the table if needed
    std::vector<boost::shared_ptr<TrackMarker> > existingMarkers;
    context->getAllMarkers(&existingMarkers);
    blockSelection();
    for (std::size_t i = 0; i < existingMarkers.size(); ++i) {
        addTableRow(existingMarkers[i]);
    }
    unblockSelection();
}

void
TrackerPanelPrivate::makeTrackRowItems(const TrackMarker& marker, int row, TrackDatas& data)
{
    ///Enabled
    {
        ItemData d;
        QCheckBox* checkbox = new QCheckBox();
        checkbox->setChecked(marker.isEnabled());
        QObject::connect( checkbox,SIGNAL( toggled(bool) ),_publicInterface,SLOT( onItemEnabledCheckBoxChecked(bool) ) );
        view->setCellWidget(row, COL_ENABLED, checkbox);
        TableItem* newItem = new TableItem;
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
        newItem->setToolTip(QObject::tr("When checked, this track will no longer be tracked even if selected"));
        view->setItem(row, COL_ENABLED, newItem);
        view->resizeColumnToContents(COL_ENABLED);
        d.item = newItem;
        d.dimension = -1;
        data.items.push_back(d);
    }
    
    ///Label
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_LABEL, newItem);
        newItem->setToolTip(QObject::tr("The label of the item as seen in the viewer"));
        newItem->setText(QString::fromUtf8(marker.getLabel().c_str()));
        view->resizeColumnToContents(COL_LABEL);
        d.item = newItem;
        d.dimension = -1;
        data.items.push_back(d);
    }
    
    ///Script name
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_SCRIPT_NAME, newItem);
        newItem->setToolTip(QObject::tr("The script-name of the item as exposed to Python scripts"));
        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
        newItem->setText(QString::fromUtf8(marker.getScriptName().c_str()));
        view->resizeColumnToContents(COL_SCRIPT_NAME);
        d.item = newItem;
        d.dimension = -1;
        data.items.push_back(d);
    }
    
    ///Motion-model
    {
        ItemData d;
        boost::shared_ptr<KnobChoice> motionModel = marker.getMotionModelKnob();
        ComboBox* cb = new ComboBox;
        std::vector<std::string> choices,helps;
        TrackerContext::getMotionModelsAndHelps(&choices,&helps);
        cb->setCurrentIndex(motionModel->getValue());
        QObject::connect( cb,SIGNAL( currentIndexChanged(int) ),_publicInterface,SLOT( onItemMotionModelChanged(int) ) );
        assert(choices.size() == helps.size());
        for (std::size_t i = 0; i < choices.size(); ++i) {
            cb->addItem(QString::fromUtf8(choices[i].c_str()), QIcon(),QKeySequence(), QString::fromUtf8(helps[i].c_str()));
        }
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_MOTION_MODEL, newItem);
        newItem->setToolTip(QObject::tr("The motion model to use for tracking"));
        newItem->setText(QString::fromUtf8(marker.getScriptName().c_str()));
        view->setCellWidget(row, COL_MOTION_MODEL, cb);
        view->resizeColumnToContents(COL_MOTION_MODEL);
        d.item = newItem;
        d.dimension = 0;
        d.knob = motionModel;
        data.items.push_back(d);
    }
    
    //Center X
    boost::shared_ptr<KnobDouble> center = marker.getCenterKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_CENTER_X, newItem);
        newItem->setToolTip(QObject::tr("The x coordinate of the center of the track"));
        newItem->setData(Qt::DisplayRole, center->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CENTER_X);
        d.item = newItem;
        d.knob = center;
        d.dimension = 0;
        data.items.push_back(d);
    }
    //Center Y
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_CENTER_Y, newItem);
        newItem->setToolTip(QObject::tr("The y coordinate of the center of the track"));
        newItem->setData(Qt::DisplayRole, center->getValue(1));
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CENTER_Y);
        d.item = newItem;
        d.knob = center;
        d.dimension = 1;
        data.items.push_back(d);
    }
    ///Offset X
    boost::shared_ptr<KnobDouble> offset = marker.getOffsetKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_OFFSET_X, newItem);
        newItem->setToolTip(QObject::tr("The x offset applied to the search window for the track"));
        newItem->setData(Qt::DisplayRole, offset->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_OFFSET_X);
        d.item = newItem;
        d.knob = offset;
        d.dimension = 0;
        data.items.push_back(d);
    }
    ///Offset Y
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_OFFSET_Y, newItem);
        newItem->setToolTip(QObject::tr("The y offset applied to the search window for the track"));
        newItem->setData(Qt::DisplayRole, offset->getValue(1));
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_OFFSET_Y);
        d.item = newItem;
        d.knob = offset;
        d.dimension = 1;
        data.items.push_back(d);
    }
    
    ///Correlation
    boost::shared_ptr<KnobDouble> correlation = marker.getCorrelationKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_CORRELATION, newItem);
        newItem->setToolTip(QObject::tr(correlation->getHintToolTip().c_str()));
        newItem->setData(Qt::DisplayRole, correlation->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CORRELATION);
        d.item = newItem;
        d.knob = correlation;
        d.dimension = 0;
        data.items.push_back(d);
    }

    
    ///Weight
    boost::shared_ptr<KnobDouble> weight = marker.getWeightKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        view->setItem(row, COL_WEIGHT, newItem);
        newItem->setToolTip(QObject::tr("The weight determines the amount this marker contributes to the final solution"));
        newItem->setData(Qt::DisplayRole, weight->getValue());
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_WEIGHT);
        d.item = newItem;
        d.knob = weight;
        d.dimension = 0;
        data.items.push_back(d);
    }

}

TrackerPanel::~TrackerPanel()
{
    
}

void
TrackerPanel::addTableRow(const boost::shared_ptr<TrackMarker> & node)
{
    int newRowIndex = _imp->view->rowCount();
    _imp->model->insertRow(newRowIndex);

    TrackDatas data;
    data.marker = node;
    _imp->makeTrackRowItems(*node, newRowIndex, data);
    
    assert((int)_imp->items.size() == newRowIndex);
    _imp->items.push_back(data);
    
    if (!_imp->selectionBlocked) {
        ///select the new item
        std::list<boost::shared_ptr<TrackMarker> > markers;
        markers.push_back(node);
        selectInternal(markers, TrackerContext::eTrackSelectionSettingsPanel);
    }
}

void
TrackerPanel::insertTableRow(const boost::shared_ptr<TrackMarker> & node, int index)
{
    assert(index >= 0);
    
    _imp->model->insertRow(index);
    
    TrackDatas data;
    data.marker = node;
    _imp->makeTrackRowItems(*node, index, data);
    
    if (index >= (int)_imp->items.size()) {
        _imp->items.push_back(data);
    } else {
        TrackItems::iterator it = _imp->items.begin();
        std::advance(it, index);
        _imp->items.insert(it, data);
    }
    
    if (!_imp->selectionBlocked) {
        ///select the new item
        std::list<boost::shared_ptr<TrackMarker> > markers;
        markers.push_back(node);
        selectInternal(markers, TrackerContext::eTrackSelectionSettingsPanel);
    }
}

void
TrackerPanel::blockSelection()
{
    ++_imp->selectionBlocked;
}

void
TrackerPanel::unblockSelection()
{
    if (_imp->selectionBlocked > 0) {
        --_imp->selectionBlocked;
    }
}

int
TrackerPanel::getMarkerRow(const boost::shared_ptr<TrackMarker> & marker) const
{
    int i = 0;
    for (TrackItems::const_iterator it = _imp->items.begin(); it != _imp->items.end(); ++it, ++i) {
        if (it->marker.lock() == marker) {
            return i;
        }
    }
    return -1;
}

boost::shared_ptr<TrackMarker>
TrackerPanel::getRowMarker(int row) const
{
    if (row < 0 || row >= (int)_imp->items.size()) {
        return boost::shared_ptr<TrackMarker>();
    }
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        if (row == (int)i) {
            return _imp->items[i].marker.lock();
        }
    }
    assert(false);
}

void
TrackerPanel::removeRow(int row)
{
    if (row < 0 || row >= (int)_imp->items.size()) {
        return;
    }
    blockSelection();
    _imp->model->removeRows(row);
    unblockSelection();
    TrackItems::iterator it = _imp->items.begin();
    std::advance(it, row);
    _imp->items.erase(it);

}

void
TrackerPanel::removeMarker(const boost::shared_ptr<TrackMarker> & marker)
{
    int row = getMarkerRow(marker);
    if (row != -1) {
        removeRow(row);
    }
}

boost::shared_ptr<TrackerContext>
TrackerPanel::getContext() const
{
    return _imp->context.lock();
}

NodeGuiPtr
TrackerPanel::getNode() const
{
    return _imp->node.lock();
}

TableItem*
TrackerPanel::getItemAt(int row, int column) const
{
    if (row < 0 || row >= (int)_imp->items.size() || column < 0 || column >= NUM_COLS) {
        return 0;
    }
    return _imp->items[row].items[column].item;
}

TableItem*
TrackerPanel::getItemAt(const boost::shared_ptr<TrackMarker> & marker, int column) const
{
    if (column < 0 || column >= NUM_COLS) {
        return 0;
    }
    for (TrackItems::const_iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->marker.lock() == marker) {
            return it->items[column].item;
        }
    }
    return 0;
}

boost::shared_ptr<KnobI>
TrackerPanel::getKnobAt(int row, int column, int* dimension) const
{
    if (row < 0 || row >= (int)_imp->items.size() || column < 0 || column >= NUM_COLS) {
        return boost::shared_ptr<KnobI>();
    }
    *dimension = _imp->items[row].items[column].dimension;
    return _imp->items[row].items[column].knob.lock();
}

void
TrackerPanel::getSelectedRows(std::set<int>* rows) const
{
    const QItemSelection selection = _imp->view->selectionModel()->selection();
    std::list<boost::shared_ptr<Node> > instances;
    QModelIndexList indexes = selection.indexes();
    for (int i = 0; i < indexes.size(); ++i) {
        rows->insert(indexes[i].row());
    }
    
}

void
TrackerPanel::onAddButtonClicked()
{
    makeTrackInternal();
}

void
TrackerPanel::pushUndoCommand(QUndoCommand* command)
{
    NodeGuiPtr node = getNode();
    if (node) {
        NodeSettingsPanel* panel = node->getSettingPanel();
        assert(panel);
        panel->pushUndoCommand(command);
    }
}

void
TrackerPanel::onRemoveButtonClicked()
{

    std::list<boost::shared_ptr<TrackMarker> > markers;
    getContext()->getSelectedMarkers(&markers);
    if (!markers.empty()) {
        pushUndoCommand(new RemoveTracksCommand(markers, getContext()));
    }
}

void
TrackerPanel::onSelectAllButtonClicked()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    context->selectAll(TrackerContext::eTrackSelectionInternal);
}

void
TrackerPanel::onResetButtonClicked()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    std::list<boost::shared_ptr<TrackMarker> > markers;
    context->getSelectedMarkers(&markers);
    
    context->clearSelection(TrackerContext::eTrackSelectionInternal);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        (*it)->resetTrack();
    }
    context->addTracksToSelection(markers, TrackerContext::eTrackSelectionInternal);
}

boost::shared_ptr<TrackMarker>
TrackerPanel::makeTrackInternal()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    boost::shared_ptr<TrackMarker> ret = context->createMarker();
    assert(ret);
    pushUndoCommand(new AddTrackCommand(ret,context));
    return ret;
}

void
TrackerPanel::onAverageButtonClicked()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    std::list<boost::shared_ptr<TrackMarker> > markers;
    context->getSelectedMarkers(&markers);
    if ( markers.empty() ) {
        Dialogs::warningDialog( tr("Average").toStdString(), tr("No tracks selected").toStdString() );
        return;
    }
    
    boost::shared_ptr<TrackMarker> marker = makeTrackInternal();
    
    boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
    
#ifdef AVERAGE_ALSO_PATTERN_QUAD
    boost::shared_ptr<KnobDouble> topLeftKnob = marker->getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> topRightKnob = marker->getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> btmRightKnob = marker->getPatternBtmRightKnob();
    boost::shared_ptr<KnobDouble> btmLeftKnob = marker->getPatternBtmLeftKnob();
#endif
    
    RangeD keyframesRange;
    keyframesRange.min = INT_MAX;
    keyframesRange.max = INT_MIN;
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        boost::shared_ptr<KnobDouble> markCenter = (*it)->getCenterKnob();

        double mini,maxi;
        bool hasKey = markCenter->getFirstKeyFrameTime(ViewSpec(0), 0, &mini);
        if (!hasKey) {
            continue;
        }
        if (mini < keyframesRange.min) {
            keyframesRange.min = mini;
        }
        hasKey = markCenter->getLastKeyFrameTime(ViewSpec(0), 0, &maxi);
        
        ///both dimensions must have keyframes
        assert(hasKey);
        if (maxi > keyframesRange.max) {
            keyframesRange.max = maxi;
        }
    }
    
    bool hasKeyFrame = keyframesRange.min != INT_MIN && keyframesRange.max != INT_MAX;
    for (double t = keyframesRange.min; t <= keyframesRange.max; t += 1) {
        Natron::Point avgCenter;
        avgCenter.x = avgCenter.y = 0.;

#ifdef AVERAGE_ALSO_PATTERN_QUAD
        Natron::Point avgTopLeft, avgTopRight,avgBtmRight,avgBtmLeft;
        avgTopLeft.x = avgTopLeft.y = avgTopRight.x = avgTopRight.y = avgBtmRight.x = avgBtmRight.y = avgBtmLeft.x = avgBtmLeft.y = 0;
#endif
        
        
        for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
            boost::shared_ptr<KnobDouble> markCenter = (*it)->getCenterKnob();
            
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            boost::shared_ptr<KnobDouble> markTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<KnobDouble> markTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<KnobDouble> markBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<KnobDouble> markBtmLeft = (*it)->getPatternBtmLeftKnob();
#endif
            
            avgCenter.x += markCenter->getValueAtTime(t, 0);
            avgCenter.y += markCenter->getValueAtTime(t, 1);
            
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            avgTopLeft.x += markTopLeft->getValueAtTime(t, 0);
            avgTopLeft.y += markTopLeft->getValueAtTime(t, 1);
            
            avgTopRight.x += markTopRight->getValueAtTime(t, 0);
            avgTopRight.y += markTopRight->getValueAtTime(t, 1);
            
            avgBtmRight.x += markBtmRight->getValueAtTime(t, 0);
            avgBtmRight.y += markBtmRight->getValueAtTime(t, 1);
            
            avgBtmLeft.x += markBtmLeft->getValueAtTime(t, 0);
            avgBtmLeft.y += markBtmLeft->getValueAtTime(t, 1);
#endif
        }
        
        avgCenter.x /= markers.size();
        avgCenter.y /= markers.size();
        
#ifdef AVERAGE_ALSO_PATTERN_QUAD
        avgTopLeft.x /= markers.size();
        avgTopLeft.y /= markers.size();
        
        avgTopRight.x /= markers.size();
        avgTopRight.y /= markers.size();
        
        avgBtmRight.x /= markers.size();
        avgBtmRight.y /= markers.size();
        
        avgBtmLeft.x /= markers.size();
        avgBtmLeft.y /= markers.size();
#endif
        
        if (!hasKeyFrame) {
            centerKnob->setValue(avgCenter.x, ViewSpec(0), 0);
            centerKnob->setValue(avgCenter.y, ViewSpec(0), 1);
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValue(avgTopLeft.x, ViewSpec(0), 0);
            topLeftKnob->setValue(avgTopLeft.y, ViewSpec(0), 1);
            topRightKnob->setValue(avgTopRight.x, ViewSpec(0), 0);
            topRightKnob->setValue(avgTopRight.y, ViewSpec(0), 1);
            btmRightKnob->setValue(avgBtmRight.x, ViewSpec(0), 0);
            btmRightKnob->setValue(avgBtmRight.y, ViewSpec(0), 1);
            btmLeftKnob->setValue(avgBtmLeft.x, ViewSpec(0), 0);
            btmLeftKnob->setValue(avgBtmLeft.y, ViewSpec(0), 1);
#endif
            break;
        } else {
            centerKnob->setValueAtTime(t, avgCenter.x, ViewSpec(0), 0);
            centerKnob->setValueAtTime(t, avgCenter.y, ViewSpec(0), 1);
#ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValueAtTime(t, avgTopLeft.x, ViewSpec(0), 0);
            topLeftKnob->setValueAtTime(t, avgTopLeft.y, ViewSpec(0), 1);
            topRightKnob->setValueAtTime(t, avgTopRight.x, ViewSpec(0), 0);
            topRightKnob->setValueAtTime(t, avgTopRight.y, ViewSpec(0), 1);
            btmRightKnob->setValueAtTime(t, avgBtmRight.x, ViewSpec(0), 0);
            btmRightKnob->setValueAtTime(t, avgBtmRight.y, ViewSpec(0), 1);
            btmLeftKnob->setValueAtTime(t, avgBtmLeft.x, ViewSpec(0), 0);
            btmLeftKnob->setValueAtTime(t, avgBtmLeft.y, ViewSpec(0), 1);
#endif
        }
    }
}

static
boost::shared_ptr<KnobDouble>
getCornerPinPoint(Natron::Node* node,
                  bool isFrom,
                  int index)
{
    assert(0 <= index && index < 4);
    QString name = isFrom ? QString::fromUtf8("from%1").arg(index + 1) : QString::fromUtf8("to%1").arg(index + 1);
    boost::shared_ptr<KnobI> knob = node->getKnobByName( name.toStdString() );
    assert(knob);
    boost::shared_ptr<KnobDouble>  ret = boost::dynamic_pointer_cast<KnobDouble>(knob);
    assert(ret);
    return ret;
}

void
TrackerPanelPrivate::createCornerPinFromSelection(const std::list<boost::shared_ptr<TrackMarker> > & selection,bool linked,bool useTransformRefFrame,bool invert)
{
    if ( (selection.size() > 4) || selection.empty() ) {
        Dialogs::errorDialog( QObject::tr("Export").toStdString(),
                            QObject::tr("Export to corner pin needs between 1 and 4 selected tracks.").toStdString() );
        
        return;
    }
    
    boost::shared_ptr<TrackerContext> ctx = context.lock();
    
    boost::shared_ptr<KnobDouble> centers[4];
    int i = 0;
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = selection.begin(); it != selection.end(); ++it, ++i) {
        centers[i] = (*it)->getCenterKnob();
        assert(centers[i]);
    }
    
    NodeGuiPtr node = _publicInterface->getNode();
    
    GuiAppInstance* app = node->getDagGui()->getGui()->getApp();
    CreateNodeArgs args(QString::fromUtf8(PLUGINID_OFX_CORNERPIN), eCreateNodeReasonInternal, node->getNode()->getGroup());
    NodePtr cornerPin = app->createNode(args);
    if (!cornerPin) {
        return;
    }
    
    ///Move the node on the right of the tracker node
    boost::shared_ptr<NodeGuiI> cornerPinGui_i = cornerPin->getNodeGui();
    NodeGui* cornerPinGui = dynamic_cast<NodeGui*>(cornerPinGui_i.get());
    assert(cornerPinGui);

    QPointF mainInstancePos = node->scenePos();
    mainInstancePos = cornerPinGui->mapToParent( cornerPinGui->mapFromScene(mainInstancePos) );
    cornerPinGui->refreshPosition( mainInstancePos.x() + node->getSize().width() * 2, mainInstancePos.y() );
    
    boost::shared_ptr<KnobDouble> toPoints[4];
    boost::shared_ptr<KnobDouble> fromPoints[4];
    
    int timeForFromPoints = useTransformRefFrame ? ctx->getTransformReferenceFrame() : app->getTimeLine()->currentFrame();
    
    for (unsigned int i = 0; i < selection.size(); ++i) {
        fromPoints[i] = getCornerPinPoint(cornerPin.get(), true, i);
        assert(fromPoints[i] && centers[i]);
        for (int j = 0; j < fromPoints[i]->getDimension(); ++j) {
            fromPoints[i]->setValue(centers[i]->getValueAtTime(timeForFromPoints,j), ViewSpec(0), j);
        }
        
        toPoints[i] = getCornerPinPoint(cornerPin.get(), false, i);
        assert(toPoints[i]);
        if (!linked) {
            toPoints[i]->cloneAndUpdateGui(centers[i].get());
        } else {
            Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(centers[i]->getHolder());
            assert(effect);
            
            std::stringstream ss;
            ss << "thisGroup." << effect->getNode()->getFullyQualifiedName() << "." << centers[i]->getName() << ".get()[dimension]";
            std::string expr = ss.str();
            dynamic_cast<KnobI*>(toPoints[i].get())->setExpression(0, expr, false);
            dynamic_cast<KnobI*>(toPoints[i].get())->setExpression(1, expr, false);
        }
    }
    
    ///Disable all non used points
    for (unsigned int i = selection.size(); i < 4; ++i) {
        QString enableName = QString::fromUtf8("enable%1").arg(i + 1);
        boost::shared_ptr<KnobI> knob = cornerPin->getKnobByName( enableName.toStdString() );
        assert(knob);
        KnobBool* enableKnob = dynamic_cast<KnobBool*>( knob.get() );
        assert(enableKnob);
        enableKnob->setValue(false, ViewSpec(0), 0);
    }
    
    if (invert) {
        boost::shared_ptr<KnobI> invertKnob = cornerPin->getKnobByName(kCornerPinInvertParamName);
        assert(invertKnob);
        KnobBool* isBool = dynamic_cast<KnobBool*>(invertKnob.get());
        assert(isBool);
        isBool->setValue(true, ViewSpec(0), 0);
    }

}

void
TrackerPanel::onExportButtonClicked()
{
    int index = _imp->exportChoice->activeIndex();
    std::list<boost::shared_ptr<TrackMarker> > selection;
    getContext()->getSelectedMarkers(&selection);
    ///This is the full list, decomment when everything will be possible to do
    //    switch (index) {
    //        case 0:
    //            _imp->createCornerPinFromSelection(selection, true, false);
    //            break;
    //        case 1:
    //            _imp->createCornerPinFromSelection(selection, true, true);
    //            break;
    //        case 2:
    //            _imp->createCornerPinFromSelection(selection, false, false);
    //            break;
    //        case 3:
    //            _imp->createCornerPinFromSelection(selection, false, true);
    //            break;
    //        case 4:
    //            _imp->createTransformFromSelection(selection, true, eExportTransformTypeStabilize);
    //            break;
    //        case 5:
    //            _imp->createTransformFromSelection(selection, true, eExportTransformTypeMatchMove);
    //            break;
    //        case 6:
    //            _imp->createTransformFromSelection(selection, false, eExportTransformTypeStabilize);
    //            break;
    //        case 7:
    //            _imp->createTransformFromSelection(selection, false, eExportTransformTypeMatchMove);
    //            break;
    //        default:
    //            break;
    //    }
    switch (index) {
        case 0:
            _imp->createCornerPinFromSelection(selection, true, false, false);
            break;
        case 1:
            _imp->createCornerPinFromSelection(selection, true, false, true);
            break;
        case 2:
            _imp->createCornerPinFromSelection(selection, false, false, false);
            break;
        case 3:
            _imp->createCornerPinFromSelection(selection, false, false, true);
            break;
        case 4:
            _imp->createCornerPinFromSelection(selection, false, true, false);
            break;
        default:
            break;
    }
}

void
TrackerPanelPrivate::markersToSelection(const std::list<boost::shared_ptr<TrackMarker> >& markers, QItemSelection* selection)
{
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = markers.begin(); it!=markers.end(); ++it) {
        int row = _publicInterface->getMarkerRow(*it);
        if (row == -1) {
            qDebug() << "Attempt to select invalid marker" << (*it)->getScriptName().c_str();
            continue;
        }
        QModelIndex left = model->index(row, 0);
        QModelIndex right = model->index(row, NUM_COLS - 1);
        assert(left.isValid() && right.isValid());
        QItemSelectionRange range(left,right);
        selection->append(range);
    }

}

void
TrackerPanelPrivate::selectionToMarkers(const QItemSelection& selection, std::list<boost::shared_ptr<TrackMarker> >* markers)
{
    QModelIndexList indexes = selection.indexes();
    for (int i = 0; i < indexes.size(); ++i) {
        
        //Check that the item is valid
        assert(indexes[i].isValid() && indexes[i].row() >= 0 && indexes[i].row() < (int)items.size() && indexes[i].column() >= 0 && indexes[i].column() < NUM_COLS);
        
        //Check that the items vector is in sync with the model
        assert(items[indexes[i].row()].items[indexes[i].column()].item == model->item(indexes[i]));
        
        boost::shared_ptr<TrackMarker> marker = items[indexes[i].row()].marker.lock();
        if (marker) {
            if (std::find(markers->begin(), markers->end(), marker) == markers->end()) {
                markers->push_back(marker);
            }
        }
    }
}

void
TrackerPanel::onTrackAboutToClone(const boost::shared_ptr<TrackMarker>& marker)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found != _imp->keys.end()) {
        
            std::list<int> keys, userKeys;
            for (std::set<int>::iterator it = found->second.userKeys.begin(); it!=found->second.userKeys.end(); ++it) {
                userKeys.push_back(*it);
            }
            for (std::set<double>::iterator it = found->second.centerKeys.begin(); it!=found->second.centerKeys.end(); ++it) {
                keys.push_back(*it);
            }

        if (!found->second.visible) {

            _imp->setVisibleItemKeyframes(keys, false, false);
            _imp->setVisibleItemUserKeyframes(userKeys, false, false);
            ///Hack, making the visible flag to true, for the onTrackCloned() function
            found->second.visible = true;
        }
        found->second.userKeys.clear();
        found->second.centerKeys.clear();
    }

}

void
TrackerPanel::onTrackCloned(const boost::shared_ptr<TrackMarker>& marker)
{
    TrackKeys& k = _imp->keys[marker];
    
    
        marker->getUserKeyframes(&k.userKeys);
        marker->getCenterKeyframes(&k.centerKeys);
        
        std::list<int> keys, userKeys;
        for (std::set<int>::iterator it = k.userKeys.begin(); it!=k.userKeys.end(); ++it) {
            userKeys.push_back(*it);
        }
        for (std::set<double>::iterator it = k.centerKeys.begin(); it!=k.centerKeys.end(); ++it) {
            keys.push_back(*it);
        }
    
    if (k.visible) {
        _imp->setVisibleItemKeyframes(keys, true, true);
        _imp->setVisibleItemUserKeyframes(userKeys, true, true);
        ///Hack, making the visible flag to true, for the onTrackCloned() function
        k.visible = true;
    }
    
}

void
TrackerPanel::onSelectionAboutToChangeInternal(const std::list<boost::shared_ptr<TrackMarker> >& selection)
{
    
    ///Remove visible keyframes on timeline
    std::list<int> toRemove,toRemoveUser;
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = selection.begin(); it!=selection.end(); ++it) {
        
        TrackKeysMap::iterator found = _imp->keys.find(*it);
        if (found != _imp->keys.end()) {
            found->second.visible = false;
            found->second.userKeys.clear();
            found->second.centerKeys.clear();
        }
        
        std::set<int> userKeys;
        std::set<double> centerKeys;
        (*it)->getUserKeyframes(&userKeys);
        (*it)->getCenterKeyframes(&centerKeys);
        for (std::set<int>::iterator it2 = userKeys.begin(); it2 != userKeys.end(); ++it2) {
            toRemoveUser.push_back(*it2);
        }
        for (std::set<double>::iterator it2 = centerKeys.begin(); it2 != centerKeys.end(); ++it2) {
            toRemove.push_back(*it2);
        }
    }
    if (!toRemove.empty()) {
        _imp->node.lock()->getNode()->getApp()->removeMultipleKeyframeIndicator(toRemove, false);
    }
    if (!toRemoveUser.empty()) {
        _imp->node.lock()->getNode()->getApp()->removeUserMultipleKeyframeIndicator(toRemoveUser, false);
    }

}

void
TrackerPanel::onContextSelectionAboutToChange(int reason)
{
    TrackerContext::TrackSelectionReason selectionReason = (TrackerContext::TrackSelectionReason)reason;
    if (selectionReason == TrackerContext::eTrackSelectionSettingsPanel) {
        //avoid recursions
        return;
    }
    std::list<boost::shared_ptr<TrackMarker> > selection;
    getContext()->getSelectedMarkers(&selection);
    onSelectionAboutToChangeInternal(selection);
}

void
TrackerPanel::onContextSelectionChanged(int reason)
{
    std::list<boost::shared_ptr<TrackMarker> > selection;
    getContext()->getSelectedMarkers(&selection);
    selectInternal(selection, reason);
}

void
TrackerPanel::onModelSelectionChanged(const QItemSelection & oldSelection,const QItemSelection & newSelection)
{
    if (_imp->selectionRecursion > 0) {
        return;
    }
    std::list<boost::shared_ptr<TrackMarker> > oldMarkers,markers;
    _imp->selectionToMarkers(newSelection, &markers);
    _imp->selectionToMarkers(oldSelection, &oldMarkers);
    onSelectionAboutToChangeInternal(oldMarkers);
    clearAndSelectTracks(markers, TrackerContext::eTrackSelectionSettingsPanel);
}

void
TrackerPanel::clearAndSelectTracks(const std::list<boost::shared_ptr<TrackMarker> >& markers, int reason)
{
    selectInternal(markers, reason);
}

void
TrackerPanel::selectInternal(const std::list<boost::shared_ptr<TrackMarker> >& markers, int reason)
{
    TrackerContext::TrackSelectionReason selectionReason = (TrackerContext::TrackSelectionReason)reason;
    if (_imp->selectionRecursion > 0) {
        return;
    }
    ++_imp->selectionRecursion;
    if (!_imp->selectionBlocked) {
        ///select the new item
        QItemSelection selection;
        _imp->markersToSelection(markers, &selection);
        
        _imp->view->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        
        boost::shared_ptr<TimeLine> timeline = _imp->node.lock()->getNode()->getApp()->getTimeLine();
        _imp->updateTrackKeysInfoBar(timeline->currentFrame());
        
        std::list<int> keysToAdd, userKeysToAdd;
        for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = markers.begin(); it!=markers.end(); ++it) {
            TrackKeys k;
            k.visible = true;
            (*it)->getCenterKeyframes(&k.centerKeys);
            (*it)->getUserKeyframes(&k.userKeys);
            for (std::set<double>::iterator it2 = k.centerKeys.begin(); it2 != k.centerKeys.end(); ++it2) {
                keysToAdd.push_back(*it2);
            }
            for (std::set<int>::iterator it2 = k.userKeys.begin(); it2 != k.userKeys.end(); ++it2) {
                userKeysToAdd.push_back(*it2);
            }
            _imp->keys[*it] = k;
        }
        if (!keysToAdd.empty()) {
            _imp->node.lock()->getNode()->getApp()->addMultipleKeyframeIndicatorsAdded(keysToAdd, true);
        }
        if (!userKeysToAdd.empty()) {
            _imp->node.lock()->getNode()->getApp()->addUserMultipleKeyframeIndicatorsAdded(userKeysToAdd, true);
        }
        
        boost::shared_ptr<TrackerContext> context = getContext();
        assert(context);
        context->beginEditSelection();
        context->clearSelection(selectionReason);
        context->addTracksToSelection(markers, selectionReason);
        context->endEditSelection(selectionReason);
    }
    
    

    
    --_imp->selectionRecursion;
}

void
TrackerPanel::onItemRightClicked(TableItem* /*item*/)
{
    
}

void
TrackerPanel::onItemDataChanged(TableItem* item)
{
    int time = _imp->node.lock()->getDagGui()->getGui()->getApp()->getTimeLine()->currentFrame();
    
    for (std::size_t it = 0; it < _imp->items.size(); ++it) {
        for (std::size_t i = 0; i < _imp->items[it].items.size(); ++i) {
            if (_imp->items[it].items[i].item == item) {
                
                boost::shared_ptr<TrackMarker> marker = _imp->items[it].marker.lock();
                if (!marker) {
                    return;
                }
                switch (i) {
                    case COL_ENABLED:
                    case COL_MOTION_MODEL:
                        //Columns with a custom cell widget are handled in their respective slots
                       break;
                    case COL_LABEL: {
                        std::string label = item->data(Qt::DisplayRole).toString().toStdString();
                        marker->setLabel(label);
                        NodePtr node = getContext()->getNode();
                        if (node) {
                            node->getApp()->redrawAllViewers();
                        }
                    }   break;
                    case COL_CENTER_X:
                    case COL_CENTER_Y:
                    case COL_OFFSET_X:
                    case COL_OFFSET_Y:
                    case COL_WEIGHT:
                    case COL_CORRELATION: {
                        boost::shared_ptr<KnobDouble> knob = boost::dynamic_pointer_cast<KnobDouble>(_imp->items[it].items[i].knob.lock());
                        assert(knob);
                        int dim = _imp->items[it].items[i].dimension;
                        double value = item->data(Qt::DisplayRole).toDouble();
                        if (knob->isAnimationEnabled() && knob->isAnimated(dim)) {
                            KeyFrame kf;
                            knob->setValueAtTime(time, value, ViewSpec(0), dim, Natron::eValueChangedReasonNatronGuiEdited, &kf);
                        } else {
                            knob->setValue(value, ViewSpec(0), dim, Natron::eValueChangedReasonNatronGuiEdited, 0);
                        }
                        
                    }   break;
                    case COL_SCRIPT_NAME:
                        //This is not editable
                        break;
                }
            }
        }
    }
}

void
TrackerPanel::onItemEnabledCheckBoxChecked(bool checked)
{
    QCheckBox* widget = qobject_cast<QCheckBox*>(sender());
    assert(widget);
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        QWidget* cellW = _imp->view->cellWidget(i, COL_ENABLED);
        if (widget == cellW) {
            boost::shared_ptr<TrackMarker> marker = _imp->items[i].marker.lock();
            marker->setEnabled(checked, Natron::eValueChangedReasonNatronGuiEdited);
            break;
        }
    }
}

void
TrackerPanel::onItemMotionModelChanged(int index)
{
    ComboBox* widget = qobject_cast<ComboBox*>(sender());
    assert(widget);
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        QWidget* cellW = _imp->view->cellWidget(i, COL_MOTION_MODEL);
        if (widget == cellW) {
            boost::shared_ptr<TrackMarker> marker = _imp->items[i].marker.lock();
            marker->getMotionModelKnob()->setValue(index, ViewSpec(0), 0, Natron::eValueChangedReasonNatronGuiEdited, 0);
            break;
        }
    }
}

void
TrackerPanel::onTrackKeyframeSet(const boost::shared_ptr<TrackMarker>& marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        TrackKeys k;
        k.userKeys.insert(key);
        _imp->keys[marker] = k;
    } else {
        _imp->updateTrackKeysInfoBar(key);
        std::pair<std::set<int>::iterator,bool> ret = found->second.userKeys.insert(key);
        if (ret.second && found->second.visible) {
            _imp->node.lock()->getNode()->getApp()->addUserKeyframeIndicator(key);
        }

    }
}

void
TrackerPanel::onTrackKeyframeRemoved(const boost::shared_ptr<TrackMarker>& marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
    std::set<int>::iterator it2 = found->second.userKeys.find(key);
    if ( it2 != found->second.userKeys.end() ) {
        found->second.userKeys.erase(it2);
        if (found->second.visible) {
            AppInstance* app = _imp->node.lock()->getNode()->getApp();
            _imp->updateTrackKeysInfoBar(app->getTimeLine()->currentFrame());
            app->removeUserKeyFrameIndicator(key);
        }
    }
}

void
TrackerPanel::onTrackAllKeyframesRemoved(const boost::shared_ptr<TrackMarker>& marker)
{
    TrackKeysMap::iterator it = _imp->keys.find(marker);
    if (it == _imp->keys.end()) {
        return;
    }
    std::list<SequenceTime> toRemove;
    
    for (std::set<int>::iterator it2 = it->second.userKeys.begin(); it2 != it->second.userKeys.end(); ++it2) {
        toRemove.push_back(*it2);
    }
    it->second.userKeys.clear();

    
    if (it->second.visible) {
        AppInstance* app = _imp->node.lock()->getNode()->getApp();
        _imp->updateTrackKeysInfoBar(app->getTimeLine()->currentFrame());
        app->removeUserMultipleKeyframeIndicator(toRemove, true);
    }

}

void
TrackerPanel::onKeyframeSetOnTrackCenter(const boost::shared_ptr<TrackMarker> &marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        TrackKeys k;
        k.centerKeys.insert(key);
        _imp->keys[marker] = k;
    } else {
        
        std::pair<std::set<double>::iterator,bool> ret = found->second.centerKeys.insert(key);
        if (ret.second && found->second.visible) {
            AppInstance* app = _imp->node.lock()->getNode()->getApp();
            _imp->updateTrackKeysInfoBar(app->getTimeLine()->currentFrame());
            app->addKeyframeIndicator(key);
        }
        
    }
}

void
TrackerPanel::onKeyframeRemovedOnTrackCenter(const boost::shared_ptr<TrackMarker>& marker, int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        return;
    }
    std::set<double>::iterator it2 = found->second.centerKeys.find(key);
    if ( it2 != found->second.centerKeys.end() ) {
        found->second.centerKeys.erase(it2);
        if (found->second.visible) {
            _imp->node.lock()->getNode()->getApp()->removeKeyFrameIndicator(key);
        }
    }
}

void
TrackerPanel::onAllKeyframesRemovedOnTrackCenter(const boost::shared_ptr<TrackMarker> &marker)
{
    TrackKeysMap::iterator it = _imp->keys.find(marker);
    if (it == _imp->keys.end()) {
        return;
    }
    std::list<SequenceTime> toRemove;
    
    for (std::set<double>::iterator it2 = it->second.centerKeys.begin(); it2 != it->second.centerKeys.end(); ++it2) {
        toRemove.push_back(*it2);
    }
    it->second.centerKeys.clear();
    
    
    if (it->second.visible) {
        _imp->node.lock()->getNode()->getApp()->removeMultipleKeyframeIndicator(toRemove, true);
    }
}

void
TrackerPanel::onMultipleKeyframesSetOnTrackCenter(const boost::shared_ptr<TrackMarker>& marker, const std::list<double>& keys)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);
    if (found == _imp->keys.end()) {
        TrackKeys k;
        for (std::list<double>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            k.centerKeys.insert(*it);
        }
        _imp->keys[marker] = k;
    } else {
        
        std::list<int> reallyInserted;
        for (std::list<double>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
            std::pair<std::set<double>::iterator,bool> ret = found->second.centerKeys.insert(*it);
            if (ret.second) {
                reallyInserted.push_back(*it);
            }
            
        }
        if (!reallyInserted.empty() && found->second.visible) {
            _imp->node.lock()->getNode()->getApp()->addMultipleKeyframeIndicatorsAdded(reallyInserted, true);
        }
        
    }
}

void
TrackerPanelPrivate::setVisibleItemKeyframes(const std::list<int>& keyframes,bool visible, bool emitSignal)
{
  
    if (!visible) {
        node.lock()->getNode()->getApp()->removeMultipleKeyframeIndicator(keyframes, emitSignal);
    } else {
        node.lock()->getNode()->getApp()->addMultipleKeyframeIndicatorsAdded(keyframes, emitSignal);
    }

}

void
TrackerPanelPrivate::setVisibleItemUserKeyframes(const std::list<int>& keyframes,bool visible, bool emitSignal)
{
    
    if (!visible) {
        node.lock()->getNode()->getApp()->removeUserMultipleKeyframeIndicator(keyframes, emitSignal);
    } else {
        node.lock()->getNode()->getApp()->addUserMultipleKeyframeIndicatorsAdded(keyframes, emitSignal);
    }
    
}


void
TrackerPanel::onSettingsPanelClosed(bool closed)
{
    boost::shared_ptr<TimeLine> timeline = getNode()->getNode()->getApp()->getTimeLine();
    if (closed) {
        ///remove all keyframes from the structure kept
        std::list<int> toRemove, toRemoveUser;
        
        for (TrackKeysMap::iterator it = _imp->keys.begin(); it != _imp->keys.end(); ++it) {

                if (it->second.visible) {
                    it->second.visible = false;
                    toRemoveUser.insert(toRemoveUser.end(),it->second.userKeys.begin(), it->second.userKeys.end());
                    toRemove.insert(toRemove.end(),it->second.centerKeys.begin(), it->second.centerKeys.end());
                }
        }
    
        _imp->setVisibleItemKeyframes(toRemove, false, true);
        _imp->setVisibleItemUserKeyframes(toRemoveUser, false, true);
        _imp->keys.clear();
    } else {
        ///rebuild all the keyframe structure
        
        std::list< boost::shared_ptr<TrackMarker> > selectedMarkers;
        getContext()->getSelectedMarkers(&selectedMarkers);
        
        std::list<int> toAdd, toAddUser;
        for (TrackItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            boost::shared_ptr<TrackMarker> marker = it->marker.lock();
            if (!marker) {
                continue;
            }
            TrackKeys keys;
            marker->getUserKeyframes(&keys.userKeys);
            keys.visible = false;
            marker->getCenterKeyframes(&keys.centerKeys);
            
            
            std::pair<TrackKeysMap::iterator,bool> ret = _imp->keys.insert(std::make_pair(marker, keys));
            assert(ret.second);
            
            std::list< boost::shared_ptr<TrackMarker> >::iterator foundSelected =
            std::find(selectedMarkers.begin(), selectedMarkers.end(), marker);
            
            ///If the item is selected, make its keyframes visible
            if (foundSelected != selectedMarkers.end()) {
                toAddUser.insert(toAddUser.end(),keys.userKeys.begin(), keys.userKeys.end());
                toAdd.insert(toAdd.end(), keys.centerKeys.begin(), keys.centerKeys.end());
                ret.first->second.visible = true;
                break;
            }
            
            
        }
        _imp->setVisibleItemKeyframes(toAdd, true, true);
        _imp->setVisibleItemUserKeyframes(toAddUser, true, true);
    }
}

void
TrackerPanel::onTrackInserted(const boost::shared_ptr<TrackMarker>& marker, int index)
{
    insertTableRow(marker, index);
}

void
TrackerPanel::onTrackRemoved(const boost::shared_ptr<TrackMarker>& marker)
{
    removeMarker(marker);
}


void
TrackerPanel::onCenterKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension, int reason)
{
    if (reason == Natron::eValueChangedReasonNatronGuiEdited) {
        return;
    }
    
    boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
    for (int i = 0; i < centerKnob->getDimension(); ++i) {
        if (dimension == -1 || i == dimension) {
            int col = i == 0 ? COL_CENTER_X : COL_CENTER_Y;
            TableItem* item = getItemAt(marker, col);
            if (!item) {
                continue;
            }
            double v = centerKnob->getValue(i);
            item->setData(Qt::DisplayRole, v);
        }
    }
    
}

void
TrackerPanel::onOffsetKnobValueChanged(const boost::shared_ptr<TrackMarker>& marker,int dimension, int reason)
{
    if (reason == Natron::eValueChangedReasonNatronGuiEdited) {
        return;
    }
    boost::shared_ptr<KnobDouble> offsetKnob = marker->getOffsetKnob();
    for (int i = 0; i < offsetKnob->getDimension(); ++i) {
        if (dimension == -1 || i == dimension) {
            int col = i == 0 ? COL_OFFSET_X : COL_OFFSET_Y;
            TableItem* item = getItemAt(marker, col);
            if (!item) {
                continue;
            }
            item->setData(Qt::DisplayRole, offsetKnob->getValue(i));
        }
    }
}

void
TrackerPanel::onCorrelationKnobValueChanged(const boost::shared_ptr<TrackMarker> &marker,int /*dimension*/, int reason)
{
    if (reason == Natron::eValueChangedReasonNatronGuiEdited) {
        return;
    }
    TableItem* item = getItemAt(marker, COL_CORRELATION);
    if (!item) {
        return;
    }
    item->setData(Qt::DisplayRole, marker->getCorrelationKnob()->getValue(0));
}

void
TrackerPanel::onWeightKnobValueChanged(const boost::shared_ptr<TrackMarker> &marker,int /*dimension*/, int reason)
{
    if (reason == Natron::eValueChangedReasonNatronGuiEdited) {
        return;
    }
    TableItem* item = getItemAt(marker, COL_WEIGHT);
    if (!item) {
        return;
    }
    item->setData(Qt::DisplayRole, marker->getWeightKnob()->getValue(0));
}

void
TrackerPanel::onMotionModelKnobValueChanged(const boost::shared_ptr<TrackMarker> &marker,int /*dimension*/, int reason)
{
    if (reason == Natron::eValueChangedReasonNatronGuiEdited) {
        return;
    }
    int row = getMarkerRow(marker);
    if (row == -1) {
        return;
    }
    ComboBox* w = dynamic_cast<ComboBox*>(_imp->view->cellWidget(row, COL_MOTION_MODEL));
    if (!w) {
        return;
    }
    w->setCurrentIndex_no_emit(marker->getMotionModelKnob()->getValue(0));
}

void
TrackerPanel::onEnabledChanged(const boost::shared_ptr<TrackMarker>& marker,int reason)
{
    if (reason == Natron::eValueChangedReasonNatronGuiEdited) {
        return;
    }
    int row = getMarkerRow(marker);
    if (row == -1) {
        return;
    }
    QCheckBox* w = dynamic_cast<QCheckBox*>(_imp->view->cellWidget(row, COL_ENABLED));
    if (!w) {
        return;
    }
    w->setChecked(marker->isEnabled());
}

void
TrackerPanel::onGoToPrevKeyframeButtonClicked()
{
    getContext()->goToPreviousKeyFrame(getNode()->getNode()->getApp()->getTimeLine()->currentFrame());
}

void
TrackerPanel::onGoToNextKeyframeButtonClicked()
{
    getContext()->goToNextKeyFrame(getNode()->getNode()->getApp()->getTimeLine()->currentFrame());
}

void
TrackerPanel::onAddKeyframeButtonClicked()
{
    int time = getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<boost::shared_ptr<TrackMarker> > markers;
    getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        (*it)->setUserKeyframe(time);
    }
}

void
TrackerPanel::onRemoveKeyframeButtonClicked()
{
    int time = getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<boost::shared_ptr<TrackMarker> > markers;
    getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        (*it)->removeUserKeyframe(time);
    }
}

void
TrackerPanel::onRemoveAnimationButtonClicked()
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        (*it)->removeAllKeyframes();
    }
}

void
TrackerPanelPrivate::updateTrackKeysInfoBar(int time)
{
    std::set<int> keyframes;
    std::list<boost::shared_ptr<TrackMarker> > markers;
    context.lock()->getSelectedMarkers(&markers);
    
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        std::set<int> keys;
        (*it)->getUserKeyframes(&keys);
        keyframes.insert(keys.begin(), keys.end());
        
    }
    
    prevKeyframe->setEnabled(!keyframes.empty());
    nextKeyframe->setEnabled(!keyframes.empty());
    addKeyframe->setEnabled(!markers.empty());
    removeKeyframe->setEnabled(!markers.empty());
    clearAnimation->setEnabled(!markers.empty());
    
    totalKeyframes->setValue( (double)keyframes.size() );
    
    if ( keyframes.empty() ) {
        currentKeyframe->setValue(1.);
        currentKeyframe->setAnimation(0);
    } else {
        ///get the first time that is equal or greater to the current time
        std::set<int>::iterator lowerBound = keyframes.lower_bound(time);
        int dist = 0;
        if ( lowerBound != keyframes.end() ) {
            dist = std::distance(keyframes.begin(), lowerBound);
        }
        
        if ( lowerBound == keyframes.end() ) {
            ///we're after the last keyframe
            currentKeyframe->setValue( (double)keyframes.size() );
            currentKeyframe->setAnimation(1);
        } else if (*lowerBound == time) {
            currentKeyframe->setValue(dist + 1);
            currentKeyframe->setAnimation(2);
        } else {
            ///we're in-between 2 keyframes, interpolate
            if ( lowerBound == keyframes.begin() ) {
                currentKeyframe->setValue(1.);
            } else {
                std::set<int>::iterator prev = lowerBound;
                if (prev != keyframes.begin()) {
                    --prev;
                }
                currentKeyframe->setValue( (double)(time - *prev) / (double)(*lowerBound - *prev) + dist );
            }
            
            currentKeyframe->setAnimation(1);
        }
    }
}

void
TrackerPanel::onTimeChanged(SequenceTime time, int reason)
{
    _imp->updateTrackKeysInfoBar(time);
    
    std::vector<boost::shared_ptr<TrackMarker> > markers;
    _imp->context.lock()->getAllMarkers(&markers);
    
    for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        
        (*it)->refreshAfterTimeChange(reason == eTimelineChangeReasonPlaybackSeek, time);
    }
    
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;

#include "moc_TrackerPanel.cpp"