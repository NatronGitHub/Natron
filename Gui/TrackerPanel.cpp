/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <limits>

#include <QApplication>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QHeaderView>
#include <QCheckBox>
#include <QItemSelection>
#include <QtCore/QDebug>


#include "Engine/Settings.h"
#include "Engine/TrackerContext.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerUndoCommand.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/AnimatedCheckBox.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGuiValue.h"
#include "Gui/KnobGuiChoice.h"
#include "Gui/TableModelView.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/ClickableLabel.h"
#include "Gui/SpinBox.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"

#define NUM_COLS 8

#define COL_ENABLED 0
#define COL_LABEL 1
#define COL_MOTION_MODEL 2
#define COL_CENTER_X 3
#define COL_CENTER_Y 4
#define COL_OFFSET_X 5
#define COL_OFFSET_Y 6
#define COL_ERROR 7


NATRON_NAMESPACE_ENTER

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
    assert( index.isValid() );
    if ( !index.isValid() ) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    TableModel* model = dynamic_cast<TableModel*>( _view->model() );
    assert(model);
    if (!model) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }
    TableItem* item = model->item(index);
    assert(item);
    if (!item) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    int col = index.column();
    if ( (col != COL_CENTER_X) &&
         ( col != COL_CENTER_Y) &&
         ( col != COL_OFFSET_X) &&
         ( col != COL_OFFSET_Y) &&
         ( col != COL_ERROR) ) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    // If the item is being edited, do not draw it
    if (_view->editedItem() == item) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    int dim = -1;
    AnimationLevelEnum level = eAnimationLevelNone;
    KnobIPtr knob = _panel->getKnobAt(index.row(), index.column(), &dim);
    assert(knob);
    if ( knob && (dim != -1) ) {
        level = knob->getAnimationLevel(dim);
    }

    bool fillRect = true;
    QBrush brush;
    if (option.state & QStyle::State_Selected) {
        brush = option.palette.highlight();
    } else if (level == eAnimationLevelInterpolatedValue) {
        double r, g, b;
        appPTR->getCurrentSettings()->getInterpolatedColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else if (level == eAnimationLevelOnKeyframe) {
        double r, g, b;
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
    if ( !item->flags().testFlag(Qt::ItemIsEditable) ) {
        pen.setColor(Qt::black);
    } else {
        double r, g, b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        pen.setColor(col);
    }
    painter->setPen(pen);

    QRect textRect( geom.x() + 5, geom.y(), geom.width() - 5, geom.height() );
    QRect r;
    QVariant var = item->data(Qt::DisplayRole);
    double d = var.toDouble();
    QString dataStr = QString::number(d, 'f', 6);
    painter->drawText(textRect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignLeft, dataStr, &r);
} // TrackerTableItemDelegate::paint

struct ItemData
{
    TableItem* item;
    KnobIWPtr knob;
    int dimension;
};

struct TrackDatas
{
    std::vector<ItemData> items;
    TrackMarkerWPtr marker;
};

typedef std::vector<TrackDatas> TrackItems;

struct TrackKeys
{
    std::set<int> userKeys;
    std::set<double> centerKeys;
    bool visible;

    TrackKeys()
        : userKeys(), centerKeys(), visible(false) {}
};

typedef std::map<TrackMarkerWPtr, TrackKeys, std::owner_less<TrackMarkerWPtr>> TrackKeysMap;

struct TrackerPanelPrivate
{
    Q_DECLARE_TR_FUNCTIONS(TrackerPanel)

public:
    TrackerPanel* _publicInterface;
    NodeGuiWPtr node;
    TrackerContextWPtr context;
    TrackItems items;
    QVBoxLayout* mainLayout;
    TableView* view;
    TableModel* model;
    std::unique_ptr<TableItemEditorFactory> itemEditorFactory;
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
    int itemDataChangedRecursion;

    TrackerPanelPrivate(TrackerPanel* publicI,
                        const NodeGuiPtr& n)
        : _publicInterface(publicI)
        , node(n)
        , context()
        , items()
        , mainLayout(0)
        , view(0)
        , model(0)
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
        , itemDataChangedRecursion(0)
    {
        context = n->getNode()->getTrackerContext();
        assert( context.lock() );
    }

    void makeTrackRowItems(const TrackMarker& marker, int row, TrackDatas& data);

    void markersToSelection(const std::list<TrackMarkerPtr>& markers, QItemSelection* selection);

    void selectionFromIndexList(const QModelIndexList& indexes, std::list<TrackMarkerPtr>* markers);

    void selectionToMarkers(const QItemSelection& selection, std::list<TrackMarkerPtr>* markers);

    void createCornerPinFromSelection(const std::list<TrackMarkerPtr> & selection, bool linked, bool useTransformRefFrame, bool invert);

    void setVisibleItemKeyframes(const std::list<int>& keys, bool visible, bool emitSignal);

    void setVisibleItemUserKeyframes(const std::list<int>& keys, bool visible, bool emitSignal);

    void updateTrackKeysInfoBar(int time);
};

TrackerPanel::TrackerPanel(const NodeGuiPtr& n,
                           NodeSettingsPanel* parent)
    : QWidget(parent)
    , _imp( new TrackerPanelPrivate(this, n) )
{
    QObject::connect( parent, SIGNAL(closeChanged(bool)), this, SLOT(onSettingsPanelClosed(bool)) );
    TrackerContextPtr context = getContext();
    QObject::connect( n->getNode()->getApp()->getTimeLine().get(), SIGNAL(frameChanged(SequenceTime,int)), this,
                      SLOT(onTimeChanged(SequenceTime,int)) );
    QObject::connect( context.get(), SIGNAL(selectionChanged(int)), this, SLOT(onContextSelectionChanged(int)) );
    QObject::connect( context.get(), SIGNAL(selectionAboutToChange(int)), this, SLOT(onContextSelectionAboutToChange(int)) );
    QObject::connect( context.get(), SIGNAL(trackInserted(TrackMarkerPtr,int)), this, SLOT(onTrackInserted(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(trackRemoved(TrackMarkerPtr)), this, SLOT(onTrackRemoved(TrackMarkerPtr)) );
    QObject::connect( context.get(), SIGNAL(trackCloned(TrackMarkerPtr)),
                      this, SLOT(onTrackCloned(TrackMarkerPtr)) );
    QObject::connect( context.get(), SIGNAL(trackAboutToClone(TrackMarkerPtr)),
                      this, SLOT(onTrackAboutToClone(TrackMarkerPtr)) );
    QObject::connect( context.get(), SIGNAL(keyframeSetOnTrack(TrackMarkerPtr,int)),
                      this, SLOT(onTrackKeyframeSet(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(keyframeRemovedOnTrack(TrackMarkerPtr,int)),
                      this, SLOT(onTrackKeyframeRemoved(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(allKeyframesRemovedOnTrack(TrackMarkerPtr)),
                      this, SLOT(onTrackAllKeyframesRemoved(TrackMarkerPtr)) );
    QObject::connect( context.get(), SIGNAL(keyframeSetOnTrackCenter(TrackMarkerPtr,int)),
                      this, SLOT(onKeyframeSetOnTrackCenter(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(keyframeRemovedOnTrackCenter(TrackMarkerPtr,int)),
                      this, SLOT(onKeyframeRemovedOnTrackCenter(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(allKeyframesRemovedOnTrackCenter(TrackMarkerPtr)),
                      this, SLOT(onAllKeyframesRemovedOnTrackCenter(TrackMarkerPtr)) );
    QObject::connect( context.get(), SIGNAL(multipleKeyframesSetOnTrackCenter(TrackMarkerPtr,std::list<double>)),
                      this, SLOT(onMultipleKeyframesSetOnTrackCenter(TrackMarkerPtr,std::list<double>)) );
    QObject::connect( context.get(), SIGNAL(multipleKeyframesRemovedOnTrackCenter(TrackMarkerPtr,std::list<double>)),
                      this, SLOT(onMultipleKeysRemovedOnTrackCenter(TrackMarkerPtr,std::list<double>)) );
    QObject::connect( context.get(), SIGNAL(enabledChanged(TrackMarkerPtr,int)), this,
                      SLOT(onEnabledChanged(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(centerKnobValueChanged(TrackMarkerPtr,int,int)), this,
                      SLOT(onCenterKnobValueChanged(TrackMarkerPtr,int,int)) );
    QObject::connect( context.get(), SIGNAL(offsetKnobValueChanged(TrackMarkerPtr,int,int)), this,
                      SLOT(onOffsetKnobValueChanged(TrackMarkerPtr,int,int)) );
    QObject::connect( context.get(), SIGNAL(errorKnobValueChanged(TrackMarkerPtr,int,int)), this,
                      SLOT(onErrorKnobValueChanged(TrackMarkerPtr,int,int)) );
    QObject::connect( context.get(), SIGNAL(motionModelKnobValueChanged(TrackMarkerPtr,int,int)), this,
                      SLOT(onMotionModelKnobValueChanged(TrackMarkerPtr,int,int)) );
    const QSize medButtonSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    const QSize medButtonIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );


    _imp->mainLayout = new QVBoxLayout(this);

    QWidget* trackContainer = new QWidget(this);
    _imp->mainLayout->addWidget(trackContainer);

    QHBoxLayout* trackLayout = new QHBoxLayout(trackContainer);
    trackLayout->setSpacing(2);
    _imp->trackLabel = new ClickableLabel(tr("Track keyframe:"), trackContainer);
    _imp->trackLabel->setSunken(false);
    trackLayout->addWidget(_imp->trackLabel);

    _imp->currentKeyframe = new SpinBox(trackContainer, SpinBox::eSpinBoxTypeDouble);
    _imp->currentKeyframe->setEnabled(false);
    _imp->currentKeyframe->setReadOnly_NoFocusRect(true);
    _imp->currentKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The current keyframe of the pattern for the selected track(s)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    trackLayout->addWidget(_imp->currentKeyframe);

    _imp->ofLabel = new ClickableLabel(tr("of"), trackContainer);
    trackLayout->addWidget(_imp->ofLabel);

    _imp->totalKeyframes = new SpinBox(trackContainer, SpinBox::eSpinBoxTypeInt);
    _imp->totalKeyframes->setEnabled(false);
    _imp->totalKeyframes->setReadOnly_NoFocusRect(true);
    _imp->totalKeyframes->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The pattern keyframe count for all the selected tracks."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    trackLayout->addWidget(_imp->totalKeyframes);

    int medIconSize = TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap prevPix, nextPix, addPix, removePix, clearAnimPix;
    appPTR->getIcon(NATRON_PIXMAP_PREV_USER_KEY, medIconSize, &prevPix);
    appPTR->getIcon(NATRON_PIXMAP_NEXT_USER_KEY, medIconSize, &nextPix);
    appPTR->getIcon(NATRON_PIXMAP_ADD_USER_KEY, medIconSize, &addPix);
    appPTR->getIcon(NATRON_PIXMAP_REMOVE_USER_KEY, medIconSize, &removePix);
    appPTR->getIcon(NATRON_PIXMAP_CLEAR_ALL_ANIMATION, medIconSize, &clearAnimPix);

    _imp->prevKeyframe = new Button(QIcon(prevPix), QString(), trackContainer);
    _imp->prevKeyframe->setFixedSize(medButtonSize);
    _imp->prevKeyframe->setIconSize(medButtonIconSize);
    _imp->prevKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Go to the previous pattern keyframe."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->prevKeyframe->setEnabled(false);
    QObject::connect( _imp->prevKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToPrevKeyframeButtonClicked()) );
    trackLayout->addWidget(_imp->prevKeyframe);

    _imp->nextKeyframe = new Button(QIcon(nextPix), QString(), trackContainer);
    _imp->nextKeyframe->setFixedSize(medButtonSize);
    _imp->nextKeyframe->setIconSize(medButtonIconSize);
    _imp->nextKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Go to the next pattern keyframe."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->nextKeyframe->setEnabled(false);
    QObject::connect( _imp->nextKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToNextKeyframeButtonClicked()) );
    trackLayout->addWidget(_imp->nextKeyframe);

    _imp->addKeyframe = new Button(QIcon(addPix), QString(), trackContainer);
    _imp->addKeyframe->setFixedSize(medButtonSize);
    _imp->addKeyframe->setIconSize(medButtonIconSize);
    _imp->addKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a keyframe to the pattern at the current timeline's time."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->addKeyframe->setEnabled(false);
    QObject::connect( _imp->addKeyframe, SIGNAL(clicked(bool)), this, SLOT(onAddKeyframeButtonClicked()) );
    trackLayout->addWidget(_imp->addKeyframe);

    _imp->removeKeyframe = new Button(QIcon(removePix), QString(), trackContainer);
    _imp->removeKeyframe->setFixedSize(medButtonSize);
    _imp->removeKeyframe->setIconSize(medButtonIconSize);
    _imp->removeKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove keyframe on the pattern at the current timeline's time."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->removeKeyframe->setEnabled(false);
    QObject::connect( _imp->removeKeyframe, SIGNAL(clicked(bool)), this, SLOT(onRemoveKeyframeButtonClicked()) );
    trackLayout->addWidget(_imp->removeKeyframe);

    _imp->clearAnimation = new Button(QIcon(clearAnimPix), QString(), trackContainer);
    _imp->clearAnimation->setFixedSize(medButtonSize);
    _imp->clearAnimation->setIconSize(medButtonIconSize);
    _imp->clearAnimation->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove all keyframes on the pattern for the selected track(s)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->clearAnimation->setEnabled(false);
    QObject::connect( _imp->clearAnimation, SIGNAL(clicked(bool)), this, SLOT(onRemoveAnimationButtonClicked()) );
    trackLayout->addWidget(_imp->clearAnimation);


    trackLayout->addStretch();


    ///-------- TableView

    _imp->view = new TableView(this);
    QObject::connect( _imp->view, SIGNAL(deleteKeyPressed()), this, SLOT(onRemoveButtonClicked()) );
    QObject::connect( _imp->view, SIGNAL(itemRightClicked(TableItem*)), this, SLOT(onItemRightClicked(TableItem*)) );
    TrackerTableItemDelegate* delegate = new TrackerTableItemDelegate(_imp->view, this);
    _imp->itemEditorFactory.reset(new TableItemEditorFactory);
    delegate->setItemEditorFactory( _imp->itemEditorFactory.get() );
    _imp->view->setItemDelegate(delegate);

    _imp->model = new TableModel(0, 0, _imp->view);
    QObject::connect( _imp->model, SIGNAL(s_itemChanged(TableItem*)), this, SLOT(onItemDataChanged(TableItem*)) );
    _imp->view->setTableModel(_imp->model);
    _imp->view->setUniformRowHeights(true);
    QItemSelectionModel *selectionModel = _imp->view->selectionModel();
    QObject::connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
                      SLOT(onModelSelectionChanged(QItemSelection,QItemSelection)) );
    QStringList dimensionNames;
    dimensionNames << tr("Enabled") << tr("Label") << tr("Motion-Model") <<
        tr("Center X") << tr("Center Y") << tr("Offset X") << tr("Offset Y") <<
        tr("Error");
    _imp->view->setColumnCount( dimensionNames.size() );
    _imp->view->setHorizontalHeaderLabels(dimensionNames);

    _imp->view->setAttribute(Qt::WA_MacShowFocusRect, 0);

    _imp->view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _imp->view->header()->setStretchLastSection(true);

    _imp->mainLayout->addWidget(_imp->view);


    const QSize smallButtonSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
    const QSize smallButtonIconSize( TO_DPIX(NATRON_SMALL_BUTTON_ICON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_ICON_SIZE) );

    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->setContentsMargins(0, 0, 0, 0);
    _imp->addButton = new Button(QIcon(), QString::fromUtf8("+"), _imp->buttonsContainer);
    _imp->addButton->setFixedSize(medButtonSize);
    _imp->addButton->setIconSize(smallButtonIconSize);
    _imp->addButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add new track"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonsLayout->addWidget(_imp->addButton);
    QObject::connect( _imp->addButton, SIGNAL(clicked(bool)), this, SLOT(onAddButtonClicked()) );

    _imp->removeButton = new Button(QIcon(), QString::fromUtf8("-"), _imp->buttonsContainer);
    _imp->removeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove selected tracks"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->removeButton->setFixedSize(medButtonSize);
    _imp->removeButton->setIconSize(smallButtonIconSize);
    _imp->buttonsLayout->addWidget(_imp->removeButton);
    QObject::connect( _imp->removeButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveButtonClicked()) );
    QPixmap selectAll;
    appPTR->getIcon(NATRON_PIXMAP_SELECT_ALL, &selectAll);

    _imp->selectAllButton = new Button(QIcon(selectAll), QString(), _imp->buttonsContainer);
    _imp->selectAllButton->setFixedSize(medButtonSize);
    _imp->selectAllButton->setIconSize(smallButtonIconSize);
    _imp->selectAllButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Select all tracks"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonsLayout->addWidget(_imp->selectAllButton);
    QObject::connect( _imp->selectAllButton, SIGNAL(clicked(bool)), this, SLOT(onSelectAllButtonClicked()) );

    _imp->resetTracksButton = new Button(tr("Reset"), _imp->buttonsContainer);
    QObject::connect( _imp->resetTracksButton, SIGNAL(clicked(bool)), this, SLOT(onResetButtonClicked()) );
    _imp->buttonsLayout->addWidget(_imp->resetTracksButton);
    _imp->resetTracksButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Reset selected tracks"), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->averageTracksButton = new Button(tr("Average"), _imp->buttonsContainer);
    QObject::connect( _imp->averageTracksButton, SIGNAL(clicked(bool)), this, SLOT(onAverageButtonClicked()) );
    _imp->buttonsLayout->addWidget(_imp->averageTracksButton);
    _imp->averageTracksButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Creates a new track as the average of the selected tracks"), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->buttonsLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->buttonsContainer);

    ///Restore the table if needed
    std::vector<TrackMarkerPtr> existingMarkers;
    context->getAllMarkers(&existingMarkers);
    blockSelection();
    for (std::size_t i = 0; i < existingMarkers.size(); ++i) {
        addTableRow(existingMarkers[i]);
    }
    unblockSelection();
}

static QString
tooltipFromKnob(const KnobIPtr& knob)
{
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
    QString tt = QString::fromUtf8("<font size=\"4\"><b>%1</b></font>").arg( QString::fromUtf8( knob->getName().c_str() ) );
    QString realTt;

    if (!isChoice) {
        realTt.append( QString::fromUtf8( knob->getHintToolTip().c_str() ) );
    } else {
        realTt.append( QString::fromUtf8( isChoice->getHintToolTipFull().c_str() ) );
    }

    std::vector<std::string> expressions;
    bool exprAllSame = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        expressions.push_back( knob->getExpression(i) );
        if ( (i > 0) && (expressions[i] != expressions[0]) ) {
            exprAllSame = false;
        }
    }

    QString exprTt;
    if (exprAllSame) {
        if ( !expressions[0].empty() ) {
            exprTt = QString::fromUtf8("ret = <b>%1</b><br />").arg( QString::fromUtf8( expressions[0].c_str() ) );
        }
    } else {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::string dimName = knob->getDimensionName(i);
            QString toAppend = QString::fromUtf8("%1 = <b>%2</b><br />").arg( QString::fromUtf8( dimName.c_str() ) ).arg( QString::fromUtf8( expressions[i].c_str() ) );
            exprTt.append(toAppend);
        }
    }

    if ( !exprTt.isEmpty() ) {
        tt += QLatin1String("<br>");
        tt.append(exprTt);
        tt += QLatin1String("</br>");
    }

    if ( !realTt.isEmpty() ) {
        realTt = NATRON_NAMESPACE::convertFromPlainText(realTt.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal);
        tt.append(realTt);
    }

    return tt;
}

static QString
labelToolTipFromScriptName(const TrackMarker& marker)
{
    return ( QString::fromUtf8("<p><b>")
             + QString::fromUtf8( marker.getScriptName_mt_safe().c_str() )
             + QString::fromUtf8("</b></p>")
             +  NATRON_NAMESPACE::convertFromPlainText(QCoreApplication::translate("TrackerPanel", "The label of the track as seen in the viewer."), NATRON_NAMESPACE::WhiteSpaceNormal) );
}

void
TrackerPanelPrivate::makeTrackRowItems(const TrackMarker& marker,
                                       int row,
                                       TrackDatas& data)
{
    ///Enabled
    {
        ItemData d;
        QWidget *checkboxContainer = new QWidget(0);
        QHBoxLayout* checkboxLayout = new QHBoxLayout(checkboxContainer);
        AnimatedCheckBox* checkbox = new AnimatedCheckBox(checkboxContainer);
        checkboxLayout->addWidget(checkbox, Qt::AlignLeft | Qt::AlignVCenter);
        checkboxLayout->setContentsMargins(0, 0, 0, 0);
        checkboxLayout->setSpacing(0);
        checkbox->setFixedSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
        checkbox->setChecked( marker.isEnabled( marker.getCurrentTime() ) );
        checkbox->setAnimation( (int)marker.getEnabledNessAnimationLevel() );
        QObject::connect( checkbox, SIGNAL(clicked(bool)), _publicInterface, SLOT(onItemEnabledCheckBoxChecked(bool)) );
        TableItem* newItem = new TableItem;
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
        newItem->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("When unchecked, this track will no longer be tracked even if selected. Also the transform parameters in the Transform tab will not take this track into account"), NATRON_NAMESPACE::WhiteSpaceNormal) );
        d.item = newItem;
        d.dimension = -1;
        view->setCellWidget(row, COL_ENABLED, checkboxContainer);
        view->setItem(row, COL_ENABLED, newItem);
        view->resizeColumnToContents(COL_ENABLED);
        data.items.push_back(d);
    }

    ///Label
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        newItem->setToolTip( labelToolTipFromScriptName(marker) );
        newItem->setText( QString::fromUtf8( marker.getLabel().c_str() ) );
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_LABEL);
        d.item = newItem;
        d.dimension = -1;
        view->setItem(row, COL_LABEL, newItem);
        data.items.push_back(d);
    }

    ///Motion-model
    {
        ItemData d;
        KnobChoicePtr motionModel = marker.getMotionModelKnob();
        ComboBox* cb = new ComboBox;
        cb->setFocusPolicy(Qt::NoFocus);
        std::vector<ChoiceOption> choices;
        TrackerContext::getMotionModelsAndHelps(false, &choices);
        QObject::connect( cb, SIGNAL(currentIndexChanged(int)), _publicInterface, SLOT(onItemMotionModelChanged(int)) );

        for (std::size_t i = 0; i < choices.size(); ++i) {
            cb->addItem( QString::fromUtf8( choices[i].id.c_str() ), QIcon(), QKeySequence(), QString::fromUtf8( choices[i].tooltip.c_str() ) );
        }
        cb->setCurrentIndex( motionModel->getValue() );
        TableItem* newItem = new TableItem;
        newItem->setToolTip( tooltipFromKnob(motionModel) );
        newItem->setText( QString::fromUtf8( marker.getScriptName_mt_safe().c_str() ) );
        view->resizeColumnToContents(COL_MOTION_MODEL);
        d.item = newItem;
        d.dimension = 0;
        d.knob = motionModel;
        view->setItem(row, COL_MOTION_MODEL, newItem);
        view->setCellWidget(row, COL_MOTION_MODEL, cb);
        data.items.push_back(d);
    }

    //Center X
    KnobDoublePtr center = marker.getCenterKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        newItem->setToolTip( tooltipFromKnob(center) );
        newItem->setData( Qt::DisplayRole, center->getValue() );
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CENTER_X);
        d.item = newItem;
        d.knob = center;
        d.dimension = 0;
        view->setItem(row, COL_CENTER_X, newItem);
        data.items.push_back(d);
    }
    //Center Y
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        newItem->setToolTip( tooltipFromKnob(center) );
        newItem->setData( Qt::DisplayRole, center->getValue(1) );
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_CENTER_Y);
        d.item = newItem;
        d.knob = center;
        d.dimension = 1;
        view->setItem(row, COL_CENTER_Y, newItem);
        data.items.push_back(d);
    }
    ///Offset X
    KnobDoublePtr offset = marker.getOffsetKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        newItem->setToolTip( tooltipFromKnob(offset) );
        newItem->setData( Qt::DisplayRole, offset->getValue() );
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_OFFSET_X);
        d.item = newItem;
        d.knob = offset;
        d.dimension = 0;
        view->setItem(row, COL_OFFSET_X, newItem);
        data.items.push_back(d);
    }
    ///Offset Y
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        newItem->setToolTip( tooltipFromKnob(offset) );
        newItem->setData( Qt::DisplayRole, offset->getValue(1) );
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        view->resizeColumnToContents(COL_OFFSET_Y);
        d.item = newItem;
        d.knob = offset;
        d.dimension = 1;
        view->setItem(row, COL_OFFSET_Y, newItem);
        data.items.push_back(d);
    }

    ///Correlation
    KnobDoublePtr error = marker.getErrorKnob();
    {
        ItemData d;
        TableItem* newItem = new TableItem;
        newItem->setToolTip( tooltipFromKnob(error) );
        newItem->setData( Qt::DisplayRole, error->getValue() );
        newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        view->resizeColumnToContents(COL_ERROR);
        d.item = newItem;
        d.knob = error;
        d.dimension = 0;
        view->setItem(row, COL_ERROR, newItem);
        data.items.push_back(d);
    }
} // TrackerPanelPrivate::makeTrackRowItems

TrackerPanel::~TrackerPanel()
{
}

void
TrackerPanel::addTableRow(const TrackMarkerPtr & node)
{
    int newRowIndex = _imp->view->rowCount();

    _imp->model->insertRow(newRowIndex);

    TrackDatas data;
    data.marker = node;
    _imp->makeTrackRowItems(*node, newRowIndex, data);

    assert( (int)_imp->items.size() == newRowIndex );
    _imp->items.push_back(data);

    if (!_imp->selectionBlocked) {
        ///select the new item
        std::list<TrackMarkerPtr> markers;
        markers.push_back(node);
        selectInternal(markers, TrackerContext::eTrackSelectionInternal);
    }
}

void
TrackerPanel::insertTableRow(const TrackMarkerPtr & node,
                             int index)
{
    assert(index >= 0);

    _imp->model->insertRow(index);

    TrackDatas data;
    data.marker = node;
    _imp->makeTrackRowItems(*node, index, data);

    if ( index >= (int)_imp->items.size() ) {
        _imp->items.push_back(data);
    } else {
        TrackItems::iterator it = _imp->items.begin();
        std::advance(it, index);
        _imp->items.insert(it, data);
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
TrackerPanel::getMarkerRow(const TrackMarkerPtr & marker) const
{
    int i = 0;

    for (TrackItems::const_iterator it = _imp->items.begin(); it != _imp->items.end(); ++it, ++i) {
        if (it->marker.lock() == marker) {
            return i;
        }
    }

    return -1;
}

TrackMarkerPtr
TrackerPanel::getRowMarker(int row) const
{
    if ( (row < 0) || ( row >= (int)_imp->items.size() ) ) {
        return TrackMarkerPtr();
    }
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        if (row == (int)i) {
            return _imp->items[i].marker.lock();
        }
    }
    assert(false);

    return TrackMarkerPtr();
}

void
TrackerPanel::removeRow(int row)
{
    if ( (row < 0) || ( row >= (int)_imp->items.size() ) ) {
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
TrackerPanel::removeMarker(const TrackMarkerPtr & marker)
{
    int row = getMarkerRow(marker);

    if (row != -1) {
        removeRow(row);
    }
}

TrackerContextPtr
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
TrackerPanel::getItemAt(int row,
                        int column) const
{
    if ( (row < 0) || ( row >= (int)_imp->items.size() ) || (column < 0) || (column >= NUM_COLS) ) {
        return 0;
    }

    return _imp->items[row].items[column].item;
}

TableItem*
TrackerPanel::getItemAt(const TrackMarkerPtr & marker,
                        int column) const
{
    if ( (column < 0) || (column >= NUM_COLS) ) {
        return 0;
    }
    for (TrackItems::const_iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->marker.lock() == marker) {
            return it->items[column].item;
        }
    }

    return 0;
}

KnobIPtr
TrackerPanel::getKnobAt(int row,
                        int column,
                        int* dimension) const
{
    if ( (row < 0) || ( row >= (int)_imp->items.size() ) || (column < 0) || (column >= NUM_COLS) ) {
        return KnobIPtr();
    }
    *dimension = _imp->items[row].items[column].dimension;

    return _imp->items[row].items[column].knob.lock();
}

void
TrackerPanel::getSelectedRows(std::set<int>* rows) const
{
    const QItemSelection selection = _imp->view->selectionModel()->selection();
    std::list<NodePtr> instances;
    QModelIndexList indexes = selection.indexes();

    for (int i = 0; i < indexes.size(); ++i) {
        rows->insert( indexes[i].row() );
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
    std::list<TrackMarkerPtr> markers;

    getContext()->getSelectedMarkers(&markers);
    if ( !markers.empty() ) {
        getNode()->getNode()->getEffectInstance()->pushUndoCommand( new RemoveTracksCommand( markers, getContext() ) );
    }
}

void
TrackerPanel::onSelectAllButtonClicked()
{
    TrackerContextPtr context = getContext();

    assert(context);
    context->selectAll(TrackerContext::eTrackSelectionInternal);
}

void
TrackerPanel::onResetButtonClicked()
{
    TrackerContextPtr context = getContext();

    assert(context);
    std::list<TrackMarkerPtr> markers;
    context->getSelectedMarkers(&markers);
    context->clearSelection(TrackerContext::eTrackSelectionInternal);
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);

    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->resetTrack();
    }
    context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
    context->addTracksToSelection(markers, TrackerContext::eTrackSelectionInternal);
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
}

TrackMarkerPtr
TrackerPanel::makeTrackInternal()
{
    TrackerContextPtr context = getContext();

    assert(context);
    TrackMarkerPtr ret = context->createMarker();
    assert(ret);
    getNode()->getNode()->getEffectInstance()->pushUndoCommand( new AddTrackCommand(ret, context) );

    return ret;
}

void
TrackerPanel::onAverageButtonClicked()
{
    TrackerContextPtr context = getContext();

    assert(context);
    std::list<TrackMarkerPtr> markers;
    context->getSelectedMarkers(&markers);
    if ( markers.empty() ) {
        Dialogs::warningDialog( tr("Average").toStdString(), tr("No tracks selected").toStdString() );

        return;
    }

    TrackMarkerPtr marker = makeTrackInternal();
    KnobDoublePtr centerKnob = marker->getCenterKnob();

#ifdef AVERAGE_ALSO_PATTERN_QUAD
    KnobDoublePtr topLeftKnob = marker->getPatternTopLeftKnob();
    KnobDoublePtr topRightKnob = marker->getPatternTopRightKnob();
    KnobDoublePtr btmRightKnob = marker->getPatternBtmRightKnob();
    KnobDoublePtr btmLeftKnob = marker->getPatternBtmLeftKnob();
#endif

    RangeD keyframesRange;
    keyframesRange.min = std::numeric_limits<int>::max();
    keyframesRange.max = std::numeric_limits<int>::min();
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        KnobDoublePtr markCenter = (*it)->getCenterKnob();
        double mini, maxi;
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

    bool hasKeyFrame = keyframesRange.min != std::numeric_limits<int>::min() && keyframesRange.max != std::numeric_limits<int>::max();
    for (double t = keyframesRange.min; t <= keyframesRange.max; t += 1) {
        Point avgCenter;
        avgCenter.x = avgCenter.y = 0.;

#ifdef AVERAGE_ALSO_PATTERN_QUAD
        Point avgTopLeft, avgTopRight, avgBtmRight, avgBtmLeft;
        avgTopLeft.x = avgTopLeft.y = avgTopRight.x = avgTopRight.y = avgBtmRight.x = avgBtmRight.y = avgBtmLeft.x = avgBtmLeft.y = 0;
#endif


        for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
            KnobDoublePtr markCenter = (*it)->getCenterKnob();

#ifdef AVERAGE_ALSO_PATTERN_QUAD
            KnobDoublePtr markTopLeft = (*it)->getPatternTopLeftKnob();
            KnobDoublePtr markTopRight = (*it)->getPatternTopRightKnob();
            KnobDoublePtr markBtmRight = (*it)->getPatternBtmRightKnob();
            KnobDoublePtr markBtmLeft = (*it)->getPatternBtmLeftKnob();
#endif

            avgCenter.x += markCenter->getValueAtTime(t, 0);
            avgCenter.y += markCenter->getValueAtTime(t, 1);

#        ifdef AVERAGE_ALSO_PATTERN_QUAD
            avgTopLeft.x += markTopLeft->getValueAtTime(t, 0);
            avgTopLeft.y += markTopLeft->getValueAtTime(t, 1);

            avgTopRight.x += markTopRight->getValueAtTime(t, 0);
            avgTopRight.y += markTopRight->getValueAtTime(t, 1);

            avgBtmRight.x += markBtmRight->getValueAtTime(t, 0);
            avgBtmRight.y += markBtmRight->getValueAtTime(t, 1);

            avgBtmLeft.x += markBtmLeft->getValueAtTime(t, 0);
            avgBtmLeft.y += markBtmLeft->getValueAtTime(t, 1);
#         endif
        }

        int n = markers.size();
        if (n) {
            avgCenter.x /= n;
            avgCenter.y /= n;

#         ifdef AVERAGE_ALSO_PATTERN_QUAD
            avgTopLeft.x /= n;
            avgTopLeft.y /= n;

            avgTopRight.x /= n;
            avgTopRight.y /= n;

            avgBtmRight.x /= n;
            avgBtmRight.y /= n;

            avgBtmLeft.x /= n;
            avgBtmLeft.y /= n;
#         endif
        }

        if (!hasKeyFrame) {
            centerKnob->setValue(avgCenter.x, ViewSpec(0), 0);
            centerKnob->setValue(avgCenter.y, ViewSpec(0), 1);
#         ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValue(avgTopLeft.x, ViewSpec(0), 0);
            topLeftKnob->setValue(avgTopLeft.y, ViewSpec(0), 1);
            topRightKnob->setValue(avgTopRight.x, ViewSpec(0), 0);
            topRightKnob->setValue(avgTopRight.y, ViewSpec(0), 1);
            btmRightKnob->setValue(avgBtmRight.x, ViewSpec(0), 0);
            btmRightKnob->setValue(avgBtmRight.y, ViewSpec(0), 1);
            btmLeftKnob->setValue(avgBtmLeft.x, ViewSpec(0), 0);
            btmLeftKnob->setValue(avgBtmLeft.y, ViewSpec(0), 1);
#         endif
            break;
        } else {
            centerKnob->setValueAtTime(t, avgCenter.x, ViewSpec(0), 0);
            centerKnob->setValueAtTime(t, avgCenter.y, ViewSpec(0), 1);
#         ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValueAtTime(t, avgTopLeft.x, ViewSpec(0), 0);
            topLeftKnob->setValueAtTime(t, avgTopLeft.y, ViewSpec(0), 1);
            topRightKnob->setValueAtTime(t, avgTopRight.x, ViewSpec(0), 0);
            topRightKnob->setValueAtTime(t, avgTopRight.y, ViewSpec(0), 1);
            btmRightKnob->setValueAtTime(t, avgBtmRight.x, ViewSpec(0), 0);
            btmRightKnob->setValueAtTime(t, avgBtmRight.y, ViewSpec(0), 1);
            btmLeftKnob->setValueAtTime(t, avgBtmLeft.x, ViewSpec(0), 0);
            btmLeftKnob->setValueAtTime(t, avgBtmLeft.y, ViewSpec(0), 1);
#         endif
        }
    }
} // TrackerPanel::onAverageButtonClicked

void
TrackerPanelPrivate::markersToSelection(const std::list<TrackMarkerPtr>& markers,
                                        QItemSelection* selection)
{
    for (std::list<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it) {
        int row = _publicInterface->getMarkerRow(*it);
        if (row == -1) {
            continue;
        }
        QModelIndex left = model->index(row, 0);
        QModelIndex right = model->index(row, NUM_COLS - 1);
        assert( left.isValid() && right.isValid() );
        QItemSelectionRange range(left, right);
        selection->append(range);
    }
}

void
TrackerPanelPrivate::selectionFromIndexList(const QModelIndexList& indexes,
                                            std::list<TrackMarkerPtr>* markers)
{
    for (int i = 0; i < indexes.size(); ++i) {
        //Check that the item is valid
        assert(indexes[i].isValid() && indexes[i].row() >= 0 && indexes[i].row() < (int)items.size() && indexes[i].column() >= 0 && indexes[i].column() < NUM_COLS);

        //Check that the items vector is in sync with the model
        assert( items[indexes[i].row()].items[indexes[i].column()].item == model->item(indexes[i]) );

        TrackMarkerPtr marker = items[indexes[i].row()].marker.lock();
        if (marker) {
            if ( std::find(markers->begin(), markers->end(), marker) == markers->end() ) {
                markers->push_back(marker);
            }
        }
    }
}

void
TrackerPanelPrivate::selectionToMarkers(const QItemSelection& selection,
                                        std::list<TrackMarkerPtr>* markers)
{
    QModelIndexList indexes = selection.indexes();

    selectionFromIndexList(indexes, markers);
}

void
TrackerPanel::onTrackAboutToClone(const TrackMarkerPtr& marker)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);

    if ( found != _imp->keys.end() ) {
        std::list<int> keys, userKeys;
        for (std::set<int>::iterator it = found->second.userKeys.begin(); it != found->second.userKeys.end(); ++it) {
            userKeys.push_back(*it);
        }
        for (std::set<double>::iterator it = found->second.centerKeys.begin(); it != found->second.centerKeys.end(); ++it) {
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
TrackerPanel::onTrackCloned(const TrackMarkerPtr& marker)
{
    TrackKeys& k = _imp->keys[marker];


    marker->getUserKeyframes(&k.userKeys);
    marker->getCenterKeyframes(&k.centerKeys);

    std::list<int> keys, userKeys;
    for (std::set<int>::iterator it = k.userKeys.begin(); it != k.userKeys.end(); ++it) {
        userKeys.push_back(*it);
    }
    for (std::set<double>::iterator it = k.centerKeys.begin(); it != k.centerKeys.end(); ++it) {
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
TrackerPanel::onSelectionAboutToChangeInternal(const std::list<TrackMarkerPtr>& selection)
{
    ///Remove visible keyframes on timeline
    std::list<int> toRemove, toRemoveUser;

    for (std::list<TrackMarkerPtr>::const_iterator it = selection.begin(); it != selection.end(); ++it) {
        TrackKeysMap::iterator found = _imp->keys.find(*it);
        if ( found != _imp->keys.end() ) {
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
    if ( !toRemove.empty() ) {
        _imp->node.lock()->getNode()->getApp()->removeMultipleKeyframeIndicator(toRemove, true);
    }
    if ( !toRemoveUser.empty() ) {
        _imp->node.lock()->getNode()->getApp()->removeUserMultipleKeyframeIndicator(toRemoveUser, true);
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
    std::list<TrackMarkerPtr> selection;
    getContext()->getSelectedMarkers(&selection);
    onSelectionAboutToChangeInternal(selection);
}

void
TrackerPanel::onContextSelectionChanged(int reason)
{
    std::list<TrackMarkerPtr> selection;

    getContext()->getSelectedMarkers(&selection);
    selectInternal(selection, reason);
}

void
TrackerPanel::onModelSelectionChanged(const QItemSelection & /*addedToSelection*/,
                                      const QItemSelection & /*removedFromSelection*/)
{
    if (_imp->selectionRecursion > 0) {
        return;
    }
    std::list<TrackMarkerPtr> oldMarkers, markers;
    _imp->context.lock()->getSelectedMarkers(&oldMarkers);
    QModelIndexList newSelection = _imp->view->selectionModel()->selectedRows();
    _imp->selectionFromIndexList(newSelection, &markers);
    onSelectionAboutToChangeInternal(oldMarkers);
    clearAndSelectTracks(markers, TrackerContext::eTrackSelectionSettingsPanel);
}

void
TrackerPanel::clearAndSelectTracks(const std::list<TrackMarkerPtr>& markers,
                                   int reason)
{
    selectInternal(markers, reason);
}

void
TrackerPanel::selectInternal(const std::list<TrackMarkerPtr>& markers,
                             int reason)
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
        if (selectionReason != TrackerContext::eTrackSelectionSettingsPanel) {
            _imp->view->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }


        std::list<int> keysToAdd, userKeysToAdd;
        for (std::list<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it) {
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
        if ( !keysToAdd.empty() ) {
            _imp->node.lock()->getNode()->getApp()->addMultipleKeyframeIndicatorsAdded(keysToAdd, true);
        }
        if ( !userKeysToAdd.empty() ) {
            _imp->node.lock()->getNode()->getApp()->addUserMultipleKeyframeIndicatorsAdded(userKeysToAdd, true);
        }

        TrackerContextPtr context = getContext();
        assert(context);
        context->beginEditSelection(TrackerContext::eTrackSelectionSettingsPanel);
        context->clearSelection(TrackerContext::eTrackSelectionSettingsPanel);
        context->addTracksToSelection(markers, TrackerContext::eTrackSelectionSettingsPanel);
        context->endEditSelection(TrackerContext::eTrackSelectionSettingsPanel);

        TimeLinePtr timeline = _imp->node.lock()->getNode()->getApp()->getTimeLine();
        _imp->updateTrackKeysInfoBar( timeline->currentFrame() );
    }


    --_imp->selectionRecursion;
} // TrackerPanel::selectInternal

void
TrackerPanel::onItemRightClicked(TableItem* /*item*/)
{
}

void
TrackerPanel::onItemDataChanged(TableItem* item)
{
    if (_imp->itemDataChangedRecursion) {
        return;
    }

    int time = _imp->node.lock()->getDagGui()->getGui()->getApp()->getTimeLine()->currentFrame();

    for (std::size_t it = 0; it < _imp->items.size(); ++it) {
        for (std::size_t i = 0; i < _imp->items[it].items.size(); ++i) {
            if (_imp->items[it].items[i].item == item) {
                TrackMarkerPtr marker = _imp->items[it].marker.lock();
                if (!marker) {
                    continue;
                }
                switch (i) {
                case COL_ENABLED:
                case COL_MOTION_MODEL:
                    //Columns with a custom cell widget are handled in their respective slots
                    break;
                case COL_LABEL: {
                    std::string label = item->data(Qt::DisplayRole).toString().toStdString();
                    marker->setLabel(label);
                    marker->setScriptName(label);
                    _imp->items[it].items[COL_LABEL].item->setToolTip( labelToolTipFromScriptName(*marker) );
                    NodePtr node = getContext()->getNode();
                    if (node) {
                        node->getApp()->redrawAllViewers();
                    }
                    break;
                }
                case COL_CENTER_X:
                case COL_CENTER_Y:
                case COL_OFFSET_X:
                case COL_OFFSET_Y:
                case COL_ERROR: {
                    KnobDoublePtr knob = std::dynamic_pointer_cast<KnobDouble>( _imp->items[it].items[i].knob.lock() );
                    assert(knob);
                    int dim = _imp->items[it].items[i].dimension;
                    double value = item->data(Qt::DisplayRole).toDouble();
                    if ( knob->isAnimationEnabled() && knob->isAnimated(dim) ) {
                        KeyFrame kf;
                        knob->setValueAtTime(time, value, ViewSpec(0), dim, eValueChangedReasonNatronGuiEdited, &kf);
                    } else {
                        knob->setValue(value, ViewSpec(0), dim, eValueChangedReasonNatronGuiEdited, 0);
                    }
                    if (i != COL_ERROR) {
                        NodePtr node = getContext()->getNode();
                        if (node) {
                            node->getApp()->redrawAllViewers();
                        }
                    }
                    break;
                }
                }
            }
        }
    }
} // TrackerPanel::onItemDataChanged

void
TrackerPanel::onItemEnabledCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* widget = qobject_cast<AnimatedCheckBox*>( sender() );
    QWidget* widgetContainer = widget->parentWidget();

    assert(widget);
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        QWidget* cellW = _imp->view->cellWidget(i, COL_ENABLED);
        if (widgetContainer == cellW) {
            TrackMarkerPtr marker = _imp->items[i].marker.lock();
            TrackerContextPtr context = getContext();
            // Set the selection to only this marker otherwise all markers will get the enabled value
            context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
            context->clearSelection(TrackerContext::eTrackSelectionInternal);
            context->addTrackToSelection(marker, TrackerContext::eTrackSelectionInternal);
            context->endEditSelection(TrackerContext::eTrackSelectionInternal);
            marker->setEnabledFromGui(marker->getCurrentTime(), checked);
            widget->setAnimation( marker->getEnabledNessAnimationLevel() );
            break;
        }
    }
    getNode()->getNode()->getApp()->redrawAllViewers();
}

void
TrackerPanel::onItemMotionModelChanged(int index)
{
    ComboBox* widget = qobject_cast<ComboBox*>( sender() );

    assert(widget);
    for (std::size_t i = 0; i < _imp->items.size(); ++i) {
        QWidget* cellW = _imp->view->cellWidget(i, COL_MOTION_MODEL);
        if (widget == cellW) {
            TrackMarkerPtr marker = _imp->items[i].marker.lock();
            TrackerContextPtr context = getContext();
            // Set the selection to only this marker otherwise all markers will get the enabled value
            context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
            context->clearSelection(TrackerContext::eTrackSelectionInternal);
            context->addTrackToSelection(marker, TrackerContext::eTrackSelectionInternal);
            context->endEditSelection(TrackerContext::eTrackSelectionInternal);

            marker->setMotionModelFromGui(index);
            break;
        }
    }
}

void
TrackerPanel::onTrackKeyframeSet(const TrackMarkerPtr& marker,
                                 int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);

    if ( found == _imp->keys.end() ) {
        TrackKeys k;
        k.userKeys.insert(key);
        _imp->keys[marker] = k;
    } else {
        _imp->updateTrackKeysInfoBar(key);
        std::pair<std::set<int>::iterator, bool> ret = found->second.userKeys.insert(key);
        if (ret.second && found->second.visible) {
            _imp->node.lock()->getNode()->getApp()->addUserKeyframeIndicator(key);
        }
    }
}

void
TrackerPanel::onTrackKeyframeRemoved(const TrackMarkerPtr& marker,
                                     int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);

    if ( found == _imp->keys.end() ) {
        return;
    }
    std::set<int>::iterator it2 = found->second.userKeys.find(key);
    if ( it2 != found->second.userKeys.end() ) {
        found->second.userKeys.erase(it2);
        if (found->second.visible) {
            AppInstancePtr app = _imp->node.lock()->getNode()->getApp();
            _imp->updateTrackKeysInfoBar( app->getTimeLine()->currentFrame() );
            app->removeUserKeyFrameIndicator(key);
        }
    }
}

void
TrackerPanel::onTrackAllKeyframesRemoved(const TrackMarkerPtr& marker)
{
    TrackKeysMap::iterator it = _imp->keys.find(marker);

    if ( it == _imp->keys.end() ) {
        return;
    }
    std::list<SequenceTime> toRemove;

    for (std::set<int>::iterator it2 = it->second.userKeys.begin(); it2 != it->second.userKeys.end(); ++it2) {
        toRemove.push_back(*it2);
    }
    it->second.userKeys.clear();


    if (it->second.visible) {
        AppInstancePtr app = _imp->node.lock()->getNode()->getApp();
        _imp->updateTrackKeysInfoBar( app->getTimeLine()->currentFrame() );
        app->removeUserMultipleKeyframeIndicator(toRemove, true);
    }
}

void
TrackerPanel::onKeyframeSetOnTrackCenter(const TrackMarkerPtr &marker,
                                         int key)
{
    if (!marker) {
        return;
    }
    TrackKeysMap::iterator found = _imp->keys.find(marker);

    if ( found == _imp->keys.end() ) {
        TrackKeys k;
        k.centerKeys.insert(key);
        _imp->keys[marker] = k;
    } else {
        std::pair<std::set<double>::iterator, bool> ret = found->second.centerKeys.insert(key);
        if (ret.second && found->second.visible) {
            NodeGuiPtr node = _imp->node.lock();
            if (!node) {
                return;
            }
            NodePtr internalNode = node->getNode();
            if (!internalNode) {
                return;
            }
            AppInstancePtr app = internalNode->getApp();
            if (!app) {
                return;
            }
            _imp->updateTrackKeysInfoBar( app->getTimeLine()->currentFrame() );
            app->addKeyframeIndicator(key);
        }
    }
}

void
TrackerPanel::onKeyframeRemovedOnTrackCenter(const TrackMarkerPtr& marker,
                                             int key)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);

    if ( found == _imp->keys.end() ) {
        return;
    }
    std::set<double>::iterator it2 = found->second.centerKeys.find(key);
    if ( it2 != found->second.centerKeys.end() ) {
        found->second.centerKeys.erase(it2);
        if (found->second.visible) {
            NodeGuiPtr node = _imp->node.lock();
            if (!node) {
                return;
            }
            NodePtr internalNode = node->getNode();
            if (!internalNode) {
                return;
            }
            AppInstancePtr app = internalNode->getApp();
            if (!app) {
                return;
            }
            app->removeKeyFrameIndicator(key);
        }
    }
}

void
TrackerPanel::onAllKeyframesRemovedOnTrackCenter(const TrackMarkerPtr &marker)
{
    TrackKeysMap::iterator it = _imp->keys.find(marker);

    if ( it == _imp->keys.end() ) {
        return;
    }
    std::list<SequenceTime> toRemove;

    for (std::set<double>::iterator it2 = it->second.centerKeys.begin(); it2 != it->second.centerKeys.end(); ++it2) {
        toRemove.push_back(*it2);
    }
    it->second.centerKeys.clear();


    if (it->second.visible) {
        NodeGuiPtr node = _imp->node.lock();
        if (!node) {
            return;
        }
        NodePtr internalNode = node->getNode();
        if (!internalNode) {
            return;
        }
        AppInstancePtr app = internalNode->getApp();
        if (!app) {
            return;
        }
        app->removeMultipleKeyframeIndicator(toRemove, true);
    }
}

void
TrackerPanel::onMultipleKeysRemovedOnTrackCenter(const TrackMarkerPtr &marker,
                                                 const std::list<double> &keys)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);

    if ( found == _imp->keys.end() ) {
        return;
    }
    std::list<SequenceTime> toRemove;

    for (std::list<double>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        std::set<double>::iterator it2 = found->second.centerKeys.find(*it);
        if ( it2 != found->second.centerKeys.end() ) {
            found->second.centerKeys.erase(it2);
        }
        toRemove.push_back(*it);
    }


    if (found->second.visible) {
        NodeGuiPtr node = _imp->node.lock();
        if (!node) {
            return;
        }
        NodePtr internalNode = node->getNode();
        if (!internalNode) {
            return;
        }
        AppInstancePtr app = internalNode->getApp();
        if (!app) {
            return;
        }
        app->removeMultipleKeyframeIndicator(toRemove, true);
    }
}

void
TrackerPanel::onMultipleKeyframesSetOnTrackCenter(const TrackMarkerPtr& marker,
                                                  const std::list<double>& keys)
{
    TrackKeysMap::iterator found = _imp->keys.find(marker);

    if ( found == _imp->keys.end() ) {
        TrackKeys k;
        for (std::list<double>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            k.centerKeys.insert(*it);
        }
        _imp->keys[marker] = k;
    } else {
        std::list<int> reallyInserted;
        for (std::list<double>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            std::pair<std::set<double>::iterator, bool> ret = found->second.centerKeys.insert(*it);
            if (ret.second) {
                reallyInserted.push_back(*it);
            }
        }
        if (!reallyInserted.empty() && found->second.visible) {
            NodeGuiPtr node = _imp->node.lock();
            if (!node) {
                return;
            }
            NodePtr internalNode = node->getNode();
            if (!internalNode) {
                return;
            }
            AppInstancePtr app = internalNode->getApp();
            if (!app) {
                return;
            }
            app->addMultipleKeyframeIndicatorsAdded(reallyInserted, true);
        }
    }
}

void
TrackerPanelPrivate::setVisibleItemKeyframes(const std::list<int>& keyframes,
                                             bool visible,
                                             bool emitSignal)
{
    NodeGuiPtr n = node.lock();
    if (!n) {
        return;
    }
    NodePtr internalNode = n->getNode();
    if (!internalNode) {
        return;
    }
    AppInstancePtr app = internalNode->getApp();
    if (!app) {
        return;
    }
    if (!visible) {
        app->removeMultipleKeyframeIndicator(keyframes, emitSignal);
    } else {
        app->addMultipleKeyframeIndicatorsAdded(keyframes, emitSignal);
    }
}

void
TrackerPanelPrivate::setVisibleItemUserKeyframes(const std::list<int>& keyframes,
                                                 bool visible,
                                                 bool emitSignal)
{
    NodeGuiPtr n = node.lock();
    if (!n) {
        return;
    }
    NodePtr internalNode = n->getNode();
    if (!internalNode) {
        return;
    }
    AppInstancePtr app = internalNode->getApp();
    if (!app) {
        return;
    }
    if (!visible) {
        app->removeUserMultipleKeyframeIndicator(keyframes, emitSignal);
    } else {
        app->addUserMultipleKeyframeIndicatorsAdded(keyframes, emitSignal);
    }
}

void
TrackerPanel::onSettingsPanelClosed(bool closed)
{
    TimeLinePtr timeline = getNode()->getNode()->getApp()->getTimeLine();

    if (closed) {
        ///remove all keyframes from the structure kept
        std::list<int> toRemove, toRemoveUser;

        for (TrackKeysMap::iterator it = _imp->keys.begin(); it != _imp->keys.end(); ++it) {
            if (it->second.visible) {
                it->second.visible = false;
                toRemoveUser.insert( toRemoveUser.end(), it->second.userKeys.begin(), it->second.userKeys.end() );
                toRemove.insert( toRemove.end(), it->second.centerKeys.begin(), it->second.centerKeys.end() );
            }
        }

        _imp->setVisibleItemKeyframes(toRemove, false, true);
        _imp->setVisibleItemUserKeyframes(toRemoveUser, false, true);
        _imp->keys.clear();
    } else {
        ///rebuild all the keyframe structure

        std::list<TrackMarkerPtr> selectedMarkers;
        getContext()->getSelectedMarkers(&selectedMarkers);

        std::list<int> toAdd, toAddUser;
        for (TrackItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            TrackMarkerPtr marker = it->marker.lock();
            if (!marker) {
                continue;
            }
            TrackKeys keys;
            marker->getUserKeyframes(&keys.userKeys);
            keys.visible = false;
            marker->getCenterKeyframes(&keys.centerKeys);


            std::pair<TrackKeysMap::iterator, bool> ret = _imp->keys.insert( std::make_pair(marker, keys) );
            assert(ret.second);

            std::list<TrackMarkerPtr>::iterator foundSelected =
                std::find(selectedMarkers.begin(), selectedMarkers.end(), marker);

            ///If the item is selected, make its keyframes visible
            if ( foundSelected != selectedMarkers.end() ) {
                toAddUser.insert( toAddUser.end(), keys.userKeys.begin(), keys.userKeys.end() );
                toAdd.insert( toAdd.end(), keys.centerKeys.begin(), keys.centerKeys.end() );
                ret.first->second.visible = true;
                break;
            }
        }
        _imp->setVisibleItemKeyframes(toAdd, true, true);
        _imp->setVisibleItemUserKeyframes(toAddUser, true, true);
    }
} // TrackerPanel::onSettingsPanelClosed

void
TrackerPanel::onTrackInserted(const TrackMarkerPtr& marker,
                              int index)
{
    insertTableRow(marker, index);
}

void
TrackerPanel::onTrackRemoved(const TrackMarkerPtr& marker)
{
    removeMarker(marker);
}

void
TrackerPanel::onCenterKnobValueChanged(const TrackMarkerPtr& marker,
                                       int dimension,
                                       int reason)
{
    if (reason == eValueChangedReasonNatronGuiEdited) {
        return;
    }

    ++_imp->itemDataChangedRecursion;
    KnobDoublePtr centerKnob = marker->getCenterKnob();
    for (int i = 0; i < centerKnob->getDimension(); ++i) {
        if ( (dimension == -1) || (i == dimension) ) {
            int col = i == 0 ? COL_CENTER_X : COL_CENTER_Y;
            TableItem* item = getItemAt(marker, col);
            if (!item) {
                continue;
            }
            double v = centerKnob->getValue(i);
            item->setData(Qt::DisplayRole, v);
        }
    }
    --_imp->itemDataChangedRecursion;
}

void
TrackerPanel::onOffsetKnobValueChanged(const TrackMarkerPtr& marker,
                                       int dimension,
                                       int reason)
{
    if (reason == eValueChangedReasonNatronGuiEdited) {
        return;
    }
    ++_imp->itemDataChangedRecursion;
    KnobDoublePtr offsetKnob = marker->getOffsetKnob();
    for (int i = 0; i < offsetKnob->getDimension(); ++i) {
        if ( (dimension == -1) || (i == dimension) ) {
            int col = i == 0 ? COL_OFFSET_X : COL_OFFSET_Y;
            TableItem* item = getItemAt(marker, col);
            if (!item) {
                continue;
            }
            item->setData( Qt::DisplayRole, offsetKnob->getValue(i) );
        }
    }
    --_imp->itemDataChangedRecursion;
}

void
TrackerPanel::onErrorKnobValueChanged(const TrackMarkerPtr &marker,
                                      int /*dimension*/,
                                      int reason)
{
    if (reason == eValueChangedReasonNatronGuiEdited) {
        return;
    }
    TableItem* item = getItemAt(marker, COL_ERROR);
    if (!item) {
        return;
    }
    ++_imp->itemDataChangedRecursion;
    item->setData( Qt::DisplayRole, marker->getErrorKnob()->getValue(0) );
    --_imp->itemDataChangedRecursion;
}

void
TrackerPanel::onMotionModelKnobValueChanged(const TrackMarkerPtr &marker,
                                            int /*dimension*/,
                                            int reason)
{
    if (reason == eValueChangedReasonNatronGuiEdited) {
        return;
    }
    int row = getMarkerRow(marker);
    if (row == -1) {
        return;
    }
    ComboBox* w = dynamic_cast<ComboBox*>( _imp->view->cellWidget(row, COL_MOTION_MODEL) );
    if (!w) {
        return;
    }
    int index = marker->getMotionModelKnob()->getValue();
    w->setCurrentIndex_no_emit(index);
}

void
TrackerPanel::onEnabledChanged(const TrackMarkerPtr& marker,
                               int reason)
{
    if (reason == eValueChangedReasonNatronGuiEdited) {
        return;
    }
    int row = getMarkerRow(marker);
    if (row == -1) {
        return;
    }
    QWidget* w = _imp->view->cellWidget(row, COL_ENABLED);
    if (!w) {
        return;
    }
    const QObjectList& childrenObjects = w->children();
    for (QObjectList::const_iterator it = childrenObjects.begin(); it != childrenObjects.end(); ++it) {
        AnimatedCheckBox* isCheckbox = dynamic_cast<AnimatedCheckBox*>(*it);
        if (isCheckbox) {
            isCheckbox->setChecked( marker->isEnabled( marker->getCurrentTime() ) );
            isCheckbox->setAnimation( (int)marker->getEnabledNessAnimationLevel() );
            if (reason != eValueChangedReasonTimeChanged) {
                getNode()->getNode()->getApp()->redrawAllViewers();
            }
            break;
        }
    }
}

void
TrackerPanel::onGoToPrevKeyframeButtonClicked()
{
    getContext()->goToPreviousKeyFrame( getNode()->getNode()->getApp()->getTimeLine()->currentFrame() );
}

void
TrackerPanel::onGoToNextKeyframeButtonClicked()
{
    getContext()->goToNextKeyFrame( getNode()->getNode()->getApp()->getTimeLine()->currentFrame() );
}

void
TrackerPanel::onAddKeyframeButtonClicked()
{
    int time = getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<TrackMarkerPtr> markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->setUserKeyframe(time);
    }
}

void
TrackerPanel::onRemoveKeyframeButtonClicked()
{
    int time = getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<TrackMarkerPtr> markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->removeUserKeyframe(time);
    }
}

void
TrackerPanel::onRemoveAnimationButtonClicked()
{
    std::list<TrackMarkerPtr> markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->removeAllUserKeyframes();
    }
}

void
TrackerPanelPrivate::updateTrackKeysInfoBar(int time)
{
    std::set<int> keyframes;
    std::list<TrackMarkerPtr> markers;

    context.lock()->getSelectedMarkers(&markers);

    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        std::set<int> keys;
        (*it)->getUserKeyframes(&keys);
        keyframes.insert( keys.begin(), keys.end() );
    }

    prevKeyframe->setEnabled( !keyframes.empty() );
    nextKeyframe->setEnabled( !keyframes.empty() );
    addKeyframe->setEnabled( !markers.empty() );
    removeKeyframe->setEnabled( !markers.empty() );
    clearAnimation->setEnabled( !markers.empty() );

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
                if ( prev != keyframes.begin() ) {
                    --prev;
                }
                currentKeyframe->setValue( (double)(time - *prev) / (double)(*lowerBound - *prev) + dist );
            }

            currentKeyframe->setAnimation(1);
        }
    }
} // TrackerPanelPrivate::updateTrackKeysInfoBar

void
TrackerPanel::onTimeChanged(SequenceTime time,
                            int reason)
{
    _imp->updateTrackKeysInfoBar(time);

    std::vector<TrackMarkerPtr> markers;
    _imp->context.lock()->getAllMarkers(&markers);

    for (std::vector<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->refreshAfterTimeChange(reason == eTimelineChangeReasonPlaybackSeek, time);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING

#include "moc_TrackerPanel.cpp"
