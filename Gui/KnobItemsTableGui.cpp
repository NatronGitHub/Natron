/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "KnobItemsTableGui.h"

#include <map>
#include <sstream> // stringstream

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QHeaderView>
#include <QCheckBox>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QMimeData>
#include <QUndoCommand>
#include <QClipboard>
#include <QPixmapCache>
#include <QWidget>
#include <QTextLayout>

#include "Engine/Utils.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/DockablePanel.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiButton.h"
#include "Gui/KnobGuiChoice.h"
#include "Gui/KnobGuiValue.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiMacros.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/SpinBox.h"
#include "Gui/TableModelView.h"

#include "Global/StrUtils.h"


#include "Serialization/KnobTableItemSerialization.h"
#include "Serialization/SerializationIO.h"

#define kNatronKnobItemsTableGuiMimeType "Natron/NatronKnobItemsTableGuiMimeType"
NATRON_NAMESPACE_ENTER;

struct ModelItem {
    // The internal item in Engine
    KnobTableItemWPtr internalItem;

    struct ColumnData
    {
        KnobIWPtr knob;
        KnobGuiPtr guiKnob;
        DimSpec knobDimension;
        
        ColumnData()
        : knob()
        , guiKnob()
        , knobDimension(DimSpec::all())
        {
            
        }
    };
    
    TableItemPtr item;
    
    std::vector<ColumnData> columnItems;
    
    // If the item wants to display its label in a column, here it is
    int labelColIndex;
    
    // If the item wants to display the scriptname in a column here it is
    int scriptNameColIndex;
    
    ModelItem()
    : internalItem()
    , item()
    , columnItems()
    , labelColIndex(-1)
    , scriptNameColIndex(-1)
    {

    }
};
typedef std::vector<ModelItem> ModelItemsVec;


struct KnobItemsTableGuiPrivate
{
    KnobItemsTableGui* _publicInterface;
    KnobItemsTableWPtr internalModel;
    DockablePanel* panel;

    KnobTableItemWPtr rightClickedItem;

    TableModelPtr tableModel;
    TableView* tableView;
    boost::scoped_ptr<TableItemEditorFactory> itemEditorFactory;

    ModelItemsVec items;

    // Prevent recursion from selectionChanged signal of QItemSelectionModel
    int selectingModelRecursion;

    Label* userKeyframeLabel;
    SpinBox* currentKeyFrameSpinBox;
    Label* ofTotalKeyFramesLabel;
    SpinBox* numKeyFramesSpinBox;
    Button* goToPrevKeyframeButton;
    Button* goToNextKeyframeButton;
    Button* addKeyFrameButton;
    Button* removeKeyFrameButton;
    Button* clearKeyFramesButton;
    QWidget* keyframesContainerWidget;
    QHBoxLayout* keyframesContainerLayout;

    KnobItemsTableGuiPrivate(KnobItemsTableGui* publicInterface, DockablePanel* panel, const KnobItemsTablePtr& table)
    : _publicInterface(publicInterface)
    , internalModel(table)
    , panel(panel)
    , tableModel()
    , tableView(0)
    , itemEditorFactory()
    , items()
    , selectingModelRecursion(0)
    , userKeyframeLabel(0)
    , currentKeyFrameSpinBox(0)
    , ofTotalKeyFramesLabel(0)
    , numKeyFramesSpinBox(0)
    , goToPrevKeyframeButton(0)
    , goToNextKeyframeButton(0)
    , addKeyFrameButton(0)
    , removeKeyFrameButton(0)
    , clearKeyFramesButton(0)
    , keyframesContainerWidget(0)
    , keyframesContainerLayout(0)
    {
    }

    ModelItemsVec::iterator findItem(const KnobTableItemPtr& internalItem) {
        for (ModelItemsVec::iterator it = items.begin(); it!=items.end(); ++it) {
            if (it->internalItem.lock() == internalItem) {
                return it;
            }
        }
        return items.end();
    }

    ModelItemsVec::iterator findItem(const TableItemConstPtr& item) {
        for (ModelItemsVec::iterator it = items.begin(); it!=items.end(); ++it) {
            if (it->item == item) {
                return it;
            }
        }
        return items.end();
    }

    bool createItemCustomWidgetAtCol(const KnobTableItemPtr& item, int row, int col);

    void createCustomWidgetRecursively(const KnobTableItemPtr& item);
    
    void selectionFromIndexList(const QModelIndexList& indexes, std::list<KnobTableItemPtr>* items);
    
    void selectionToItems(const QItemSelection& selection, std::list<KnobTableItemPtr>* items);
    
    void itemsToSelection(const std::list<KnobTableItemPtr>& items, QItemSelection* selection);

    void createTableItems(const KnobTableItemPtr& item);

    void removeTableItem(const KnobTableItemPtr& item);

    void recreateItemsFromModel();

    void createItemsVecRecursive(const std::vector<KnobTableItemPtr>& items);

    void setItemIcon(const TableItemPtr& tableItem, int col, const KnobTableItemPtr& item);

    void refreshKnobsSelectionState(const std::list<KnobTableItemPtr>& selected, const std::list<KnobTableItemPtr>& deselected);

    void refreshKnobItemSelection(const KnobTableItemPtr& item, bool selected);

    void createKeyFrameWidgets(QWidget* parent);

    void refreshUserKeyFramesWidgets();
};

bool
KnobItemsTableGuiPrivate::createItemCustomWidgetAtCol(const KnobTableItemPtr& item, int row, int col)
{
    DimSpec dim;
    KnobIPtr knob = item->getColumnKnob(col, &dim);
    if (!knob) {
        return false;
    }

    ModelItemsVec::iterator foundItem = findItem(item);
    assert(foundItem != items.end());
    if (foundItem == items.end()) {
        return false;
    }
    assert(col >= 0 && col < (int)foundItem->columnItems.size());

    // destroy existing KnobGui
    foundItem->columnItems[col].guiKnob.reset();

    // Create the Knob Gui
    foundItem->columnItems[col].guiKnob = KnobGui::create(knob, KnobGui::eKnobLayoutTypeTableItemWidget, _publicInterface);
    if (!dim.isAll()) {
        foundItem->columnItems[col].guiKnob->setSingleDimensionalEnabled(true, DimIdx(dim));
    }

    QWidget* container = new QWidget(tableView);
    QHBoxLayout* containerLayout = new QHBoxLayout(container);

    {
        double bgColor[3];
        appPTR->getCurrentSettings()->getRaisedColor(&bgColor[0], &bgColor[1], &bgColor[2]);
        QColor c;
        c.setRgbF(Image::clamp(bgColor[0], 0., 1.), Image::clamp(bgColor[1], 0., 1.), Image::clamp(bgColor[2], 0., 1.));
        QString rgbaStr = QString::fromUtf8("rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue());
        container->setStyleSheet(QString::fromUtf8("QWidget {background-color: %1}").arg(rgbaStr));
    }
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobColorPtr isColor = toKnobColor(knob);

    Qt::Alignment align;
    if (isChoice || isColor) {
        // Align combobox on the left and
        align = (Qt::AlignLeft | Qt::AlignVCenter);
    } else {
        align = Qt::AlignCenter;
    }
    
    containerLayout->setAlignment(align);
    foundItem->columnItems[col].guiKnob->createGUI(container);
    containerLayout->addWidget(foundItem->columnItems[col].guiKnob->getFieldContainer());


    KnobGuiWidgetsPtr firstViewWidgets = foundItem->columnItems[col].guiKnob->getWidgetsForView(ViewIdx(0));
    assert(firstViewWidgets);

    if (isChoice) {
        // remove the shape of the combobox so it looks integrated
        KnobGuiChoice* widget = dynamic_cast<KnobGuiChoice*>(firstViewWidgets.get());
        assert(widget);
        widget->getCombobox()->setDrawShapeEnabled(false);
    }
    KnobGuiButton* isButton = dynamic_cast<KnobGuiButton*>(firstViewWidgets.get());
    KnobGuiValue* isValue = dynamic_cast<KnobGuiValue*>(firstViewWidgets.get());
    if (isButton) {
        // For buttons, just show the icon
        isButton->disableButtonBorder();
    } else if (isValue) {
        // For spinboxes, disable border
        isValue->disableSpinBoxesBorder();
    }

    QModelIndex itemIdx = tableModel->getItemIndex(foundItem->item);

    // Set the widget
    {
        ModelItemsVec::iterator foundItem = findItem(item);
        assert((int)foundItem->columnItems.size() == tableModel->columnCount());
        tableView->setCellWidget(row, col, itemIdx.parent(), container);
    }
    
    return true;
} // createItemCustomWidgetAtCol

QWidget*
KnobItemsTableGui::createKnobHorizontalFieldContainer(QWidget* parent) const
{
    return new QWidget(parent);
}

void
KnobItemsTableGuiPrivate::createCustomWidgetRecursively(const KnobTableItemPtr& item)
{
    int row = item->getIndexInParent();
    int nCols = tableModel->columnCount();
    for (int i = 0; i < nCols; ++i) {
        createItemCustomWidgetAtCol(item, row, i);
    }

    const std::vector<KnobTableItemPtr>& children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        createCustomWidgetRecursively(children[i]);
    }
}

void
KnobItemsTableGuiPrivate::createKeyFrameWidgets(QWidget* parent)
{
    keyframesContainerWidget = new QWidget(parent);

    keyframesContainerLayout = new QHBoxLayout(keyframesContainerWidget);
    keyframesContainerLayout->setSpacing(2);
    userKeyframeLabel = new ClickableLabel(_publicInterface->tr("Manual Keyframe(s):"), keyframesContainerWidget);
    userKeyframeLabel->setEnabled(false);
    keyframesContainerLayout->addWidget(userKeyframeLabel);

    currentKeyFrameSpinBox = new SpinBox(keyframesContainerWidget, SpinBox::eSpinBoxTypeDouble);
    currentKeyFrameSpinBox->setEnabled(false);
    currentKeyFrameSpinBox->setReadOnly_NoFocusRect(true);
    currentKeyFrameSpinBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(_publicInterface->tr("The current manual keyframe for the selected item(s)"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    keyframesContainerLayout->addWidget(currentKeyFrameSpinBox);

    ofTotalKeyFramesLabel = new ClickableLabel(QString::fromUtf8("of"), keyframesContainerWidget);
    ofTotalKeyFramesLabel->setEnabled(false);
    keyframesContainerLayout->addWidget(ofTotalKeyFramesLabel);

    numKeyFramesSpinBox = new SpinBox(keyframesContainerWidget, SpinBox::eSpinBoxTypeInt);
    numKeyFramesSpinBox->setEnabled(false);
    numKeyFramesSpinBox->setReadOnly_NoFocusRect(true);
    numKeyFramesSpinBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(_publicInterface->tr("The manual keyframes count for all the selected items"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    keyframesContainerLayout->addWidget(numKeyFramesSpinBox);

    const QSize medButtonSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    const QSize medButtonIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

    int medIconSize = TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap prevPix, nextPix, addPix, removePix, clearAnimPix;
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, medIconSize, &prevPix);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_KEY, medIconSize, &nextPix);
    appPTR->getIcon(NATRON_PIXMAP_ADD_KEYFRAME, medIconSize, &addPix);
    appPTR->getIcon(NATRON_PIXMAP_REMOVE_KEYFRAME, medIconSize, &removePix);
    appPTR->getIcon(NATRON_PIXMAP_CLEAR_ALL_ANIMATION, medIconSize, &clearAnimPix);

    goToPrevKeyframeButton = new Button(QIcon(prevPix), QString(), keyframesContainerWidget);
    goToPrevKeyframeButton->setFixedSize(medButtonSize);
    goToPrevKeyframeButton->setIconSize(medButtonIconSize);
    goToPrevKeyframeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(_publicInterface->tr("Go to the previous keyframe."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    goToPrevKeyframeButton->setEnabled(false);
    QObject::connect( goToPrevKeyframeButton, SIGNAL(clicked(bool)), _publicInterface, SLOT(onGoToPrevKeyframeButtonClicked()) );
    keyframesContainerLayout->addWidget(goToPrevKeyframeButton);

    goToNextKeyframeButton = new Button(QIcon(nextPix), QString(), keyframesContainerWidget);
    goToNextKeyframeButton->setFixedSize(medButtonSize);
    goToNextKeyframeButton->setIconSize(medButtonIconSize);
    goToNextKeyframeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(_publicInterface->tr("Go to the next keyframe."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    goToNextKeyframeButton->setEnabled(false);
    QObject::connect( goToNextKeyframeButton, SIGNAL(clicked(bool)), _publicInterface, SLOT(onGoToNextKeyframeButtonClicked()) );
    keyframesContainerLayout->addWidget(goToNextKeyframeButton);

    addKeyFrameButton = new Button(QIcon(addPix), QString(), keyframesContainerWidget);
    addKeyFrameButton->setFixedSize(medButtonSize);
    addKeyFrameButton->setIconSize(medButtonIconSize);
    addKeyFrameButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(_publicInterface->tr("Add a manual keyframe at the current timeline's time"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    addKeyFrameButton->setEnabled(false);
    QObject::connect( addKeyFrameButton, SIGNAL(clicked(bool)), _publicInterface, SLOT(onAddKeyframeButtonClicked()) );
    keyframesContainerLayout->addWidget(addKeyFrameButton);

    removeKeyFrameButton = new Button(QIcon(removePix), QString(), keyframesContainerWidget);
    removeKeyFrameButton->setFixedSize(medButtonSize);
    removeKeyFrameButton->setIconSize(medButtonIconSize);
    removeKeyFrameButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(_publicInterface->tr("Remove keyframe at the current timeline's time"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    removeKeyFrameButton->setEnabled(false);
    QObject::connect( removeKeyFrameButton, SIGNAL(clicked(bool)), _publicInterface, SLOT(onRemoveKeyframeButtonClicked()) );
    keyframesContainerLayout->addWidget(removeKeyFrameButton);

    clearKeyFramesButton = new Button(QIcon(clearAnimPix), QString(), keyframesContainerWidget);
    clearKeyFramesButton->setFixedSize(medButtonSize);
    clearKeyFramesButton->setIconSize(medButtonIconSize);
    clearKeyFramesButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(_publicInterface->tr("Remove all animation for the selected item(s)"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    clearKeyFramesButton->setEnabled(false);
    QObject::connect( clearKeyFramesButton, SIGNAL(clicked(bool)), _publicInterface, SLOT(onRemoveAnimationButtonClicked()) );
    keyframesContainerLayout->addWidget(clearKeyFramesButton);
    
    
    keyframesContainerLayout->addStretch();
} // createKeyFrameWidgets

/**
 * @brief Helper class to properly relfect animation level on a knob in the background of the cell
 **/
class AnimatedKnobItemDelegate
: public QStyledItemDelegate
{
    KnobItemsTableGuiPrivate* _imp;
public:

    explicit AnimatedKnobItemDelegate(KnobItemsTableGuiPrivate* imp)
    : QStyledItemDelegate()
    , _imp(imp)
    {

    }

private:

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;

};



static QSizeF viewItemTextLayout(QTextLayout &textLayout, int lineWidth)
{
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}

static void drawItemText(QPainter *p, const QStyleOptionViewItemV4 *option, const QRect &rect, QStyle* style)
{
    const QWidget *widget = option->widget;
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1;

    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = option->features & QStyleOptionViewItemV2::WrapText;
    QTextOption textOption;
    textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    textOption.setTextDirection(option->direction);
    textOption.setAlignment(QStyle::visualAlignment(option->direction, option->displayAlignment));
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option->font);
    textLayout.setText(option->text);

    viewItemTextLayout(textLayout, textRect.width());

    QString elidedText;
    qreal height = 0;
    qreal width = 0;
    int elidedIndex = -1;
    const int lineCount = textLayout.lineCount();
    for (int j = 0; j < lineCount; ++j) {
        const QTextLine line = textLayout.lineAt(j);
        if (j + 1 <= lineCount - 1) {
            const QTextLine nextLine = textLayout.lineAt(j + 1);
            if ((nextLine.y() + nextLine.height()) > textRect.height()) {
                int start = line.textStart();
                int length = line.textLength() + nextLine.textLength();
                //const QStackTextEngine engine(textLayout.text().mid(start, length), option->font);
                elidedText = textLayout.text().mid(start, length); //engine.elidedText(option->textElideMode, textRect.width());
                height += line.height();
                width = textRect.width();
                elidedIndex = j;
                break;
            }
        }
        if (line.naturalTextWidth() > textRect.width()) {
            int start = line.textStart();
            int length = line.textLength();
            //const QStackTextEngine engine(textLayout.text().mid(start, length), option->font);
            elidedText = textLayout.text().mid(start, length);//engine.elidedText(option->textElideMode, textRect.width());
            height += line.height();
            width = textRect.width();
            elidedIndex = j;
            break;
        }
        width = qMax<qreal>(width, line.width());
        height += line.height();
    }

    const QRect layoutRect = QStyle::alignedRect(option->direction, option->displayAlignment,
                                                 QSize(int(width), int(height)), textRect);
    const QPointF position = layoutRect.topLeft();
    for (int i = 0; i < lineCount; ++i) {
        const QTextLine line = textLayout.lineAt(i);
        if (i == elidedIndex) {
            qreal x = position.x() + line.x();
            qreal y = position.y() + line.y() + line.ascent();
            p->save();
            p->setFont(option->font);
            p->drawText(QPointF(x, y), elidedText);
            p->restore();
            break;
        }
        line.draw(p, position);
    }
} // drawItemText

void
AnimatedKnobItemDelegate::paint(QPainter * painter,
                                const QStyleOptionViewItem & option,
                                const QModelIndex & index) const
{

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    // Paint the label column
    // Use the selection color for the text color

    painter->save();


    const QWidget* widget = opt.widget;
    QStyle* style = _imp->tableView->style();


    // draw the icon
    if (!opt.icon.isNull()) {
        QRect iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, widget);
        QIcon::Mode mode = QIcon::Normal;
        if (!(opt.state & QStyle::State_Enabled)) {
            mode = QIcon::Disabled;
        } else if (opt.state & QStyle::State_Selected) {
            mode = QIcon::Selected;
        }
        QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
        opt.icon.paint(painter, iconRect, opt.decorationAlignment, mode, state);
    }

    // Set pen color
    {
        double textColor[3];
        if (opt.state & QStyle::State_Selected) {
            appPTR->getCurrentSettings()->getSelectionColor(&textColor[0], &textColor[1], &textColor[2]);
        } else {
            appPTR->getCurrentSettings()->getTextColor(&textColor[0], &textColor[1], &textColor[2]);
        }


        QColor c;
        c.setRgbF(textColor[0], textColor[1], textColor[2]);
        QPen pen = painter->pen();
        pen.setColor(c);
        painter->setPen(pen);
    }

    // draw the text
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    drawItemText(painter, &opt, textRect, style);

    painter->restore();


} // paint

class KnobItemsTableView : public TableView
{
    KnobItemsTableGuiPrivate* _imp;
public:

    KnobItemsTableView(KnobItemsTableGuiPrivate* imp, Gui* gui, QWidget* parent)
    : TableView(gui, parent)
    , _imp(imp)
    {
    }

    virtual ~KnobItemsTableView()
    {

    }

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent *e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent *e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void setupAndExecDragObject(QDrag* drag,
                                        const QModelIndexList& rows,
                                        Qt::DropActions supportedActions,
                                        Qt::DropAction defaultAction) OVERRIDE FINAL;

    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void drawRow(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

KnobItemsTableGui::KnobItemsTableGui(const KnobItemsTablePtr& table, DockablePanel* panel, QWidget* parent)
: _imp(new KnobItemsTableGuiPrivate(this, panel, table))
{

    setContainerWidget(panel);


    if (_imp->internalModel.lock()->isUserKeyframesWidgetsEnabled()) {
        _imp->createKeyFrameWidgets(parent);
    }

    _imp->tableView = new KnobItemsTableView(_imp.get(), panel->getGui(), parent);

    connect(_imp->tableView, SIGNAL(itemRightClicked(QPoint,TableItemPtr)), this, SLOT(onViewItemRightClicked(QPoint,TableItemPtr)));

#if QT_VERSION < 0x050000
    _imp->tableView->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->tableView->header()->setStretchLastSection(true);

    AnimatedKnobItemDelegate* delegate = new AnimatedKnobItemDelegate(_imp.get());
    _imp->itemEditorFactory.reset(new TableItemEditorFactory);
    delegate->setItemEditorFactory( _imp->itemEditorFactory.get() );
    _imp->tableView->setItemDelegate(delegate);

    int nCols = table->getColumnsCount();

    KnobItemsTable::KnobItemsTableTypeEnum knobTableType = table->getType();
    TableModel::TableModelTypeEnum type = TableModel::eTableModelTypeTable;
    switch (knobTableType) {
        case KnobItemsTable::eKnobItemsTableTypeTable:
            type = TableModel::eTableModelTypeTable;
            break;
        case KnobItemsTable::eKnobItemsTableTypeTree:
            type = TableModel::eTableModelTypeTree;
            break;
    }
    _imp->tableModel = TableModel::create(nCols, type);
    QObject::connect( _imp->tableModel.get(), SIGNAL(itemDataChanged(TableItemPtr,int, int)), this, SLOT(onTableItemDataChanged(TableItemPtr,int, int)) );
    _imp->tableView->setTableModel(_imp->tableModel);

    QItemSelectionModel *selectionModel = _imp->tableView->selectionModel();
    QObject::connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
                     SLOT(onModelSelectionChanged(QItemSelection,QItemSelection)) );



    std::vector<QString> headerLabels(nCols), headerIcons(nCols), headerTooltips(nCols);
    for (int i = 0; i < nCols; ++i) {
        headerIcons[i] = QString::fromUtf8(table->getColumnIcon(i).c_str());
        headerLabels[i] = QString::fromUtf8(table->getColumnText(i).c_str());
        headerTooltips[i] = QString::fromUtf8(table->getColumnTooltip(i).c_str());
    }

    QString iconsPath = QString::fromUtf8(table->getIconsPath().c_str());
    StrUtils::ensureLastPathSeparator(iconsPath);

    std::vector<std::pair<QString, QIcon> > headerDatas(nCols);
    for (int i = 0; i < nCols; ++i) {
        if (!headerLabels[i].isEmpty()) {
            headerDatas[i].first = headerLabels[i];
        }
        if (!headerIcons[i].isEmpty()) {
            QString filePath = iconsPath + headerIcons[i];
            QPixmap p;
            if (p.load(filePath) && !p.isNull()) {
                QIcon ic(p);
                headerDatas[i].second = ic;
            }
        }
    }
    _imp->tableModel->setHorizontalHeaderData(headerDatas);
    for (int i = 0; i < nCols; ++i) {
        _imp->tableModel->setHeaderData(i, Qt::Horizontal, QVariant(headerTooltips[i]), Qt::ToolTipRole);
    }

    _imp->tableView->setUniformRowHeights(table->getRowsHaveUniformHeight());
    _imp->tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);


    if (knobTableType == KnobItemsTable::eKnobItemsTableTypeTree) {
        _imp->tableView->setItemsExpandable(true);
        _imp->tableView->setRootIsDecorated(true);
        _imp->tableView->setExpandsOnDoubleClick(true);
    }

    bool dndSupported = table->isDragAndDropSupported();
    _imp->tableView->setDragEnabled(dndSupported);
    _imp->tableView->setAcceptDrops(dndSupported);

    if (dndSupported) {
        if (table->isDropFromExternalSourceSupported()) {
            _imp->tableView->setDragDropMode(QAbstractItemView::DragDrop);
        } else {
            _imp->tableView->setDragDropMode(QAbstractItemView::InternalMove);
        }
    }

    
    _imp->recreateItemsFromModel();

    connect(table.get(), SIGNAL(selectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)), this, SLOT(onModelSelectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(itemRemoved(KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onModelItemRemoved( KnobTableItemPtr,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(itemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onModelItemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)));



} // KnobItemsTableGui::KnobItemsTableGui

KnobItemsTableGui::~KnobItemsTableGui()
{
    
}

void
KnobItemsTableGui::addWidgetsToLayout(QLayout* layout)
{

    QGridLayout* isGridLayout = dynamic_cast<QGridLayout*>(layout);
    QBoxLayout* isBoxLayout = dynamic_cast<QBoxLayout*>(layout);
    if (isGridLayout) {
        int rc = isGridLayout->rowCount();
        if (_imp->keyframesContainerWidget) {
            isGridLayout->addWidget(_imp->userKeyframeLabel, rc, 0, 1, 1, Qt::AlignRight);
            isGridLayout->addWidget(_imp->keyframesContainerWidget, rc, 1, 1, 1);
        }
        isGridLayout->addWidget(_imp->tableView, rc + 1, 0, 1, 2);
    } else if (isBoxLayout) {
        QWidget* rowContainer = new QWidget(layout->widget());
        QHBoxLayout* rowContainerLayout = new QHBoxLayout(rowContainer);
        rowContainerLayout->addWidget(_imp->userKeyframeLabel);
        rowContainerLayout->addWidget(_imp->keyframesContainerWidget);
        isBoxLayout->addWidget(rowContainer);
        isBoxLayout->addWidget(_imp->tableView);
    } else {
        assert(false);
    }

}


std::vector<KnobGuiPtr>
KnobItemsTableGui::getKnobsForItem(const KnobTableItemPtr& item) const
{
    std::vector<KnobGuiPtr> ret;
    ModelItemsVec::const_iterator foundItem = _imp->findItem(item);
    if (foundItem == _imp->items.end()) {
        return ret;
    }
    for (std::size_t i = 0; i < foundItem->columnItems.size(); ++i) {
        if (foundItem->columnItems[i].guiKnob) {
            ret.push_back(foundItem->columnItems[i].guiKnob);
        }
    }
    return ret;
}

void
KnobItemsTableView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    QStyleOptionViewItemV2 opt = viewOptions();
    QStyle::State extraFlags = QStyle::State_None;
    if (isEnabled()) {
        extraFlags |= QStyle::State_Enabled;
    }
    if (window()->isActiveWindow()) {
        extraFlags |= QStyle::State_Active;
    }
    QPoint oldBO = painter->brushOrigin();
    if (verticalScrollMode() == QAbstractItemView::ScrollPerPixel) {
        painter->setBrushOrigin(QPoint(0, verticalOffset()));
    }

    const int indent = indentation();
    QRect primitive(rect.right() + 1, rect.top(), indent, rect.height());



    TableItemPtr item = _imp->tableModel->getItem(index);
    assert(item);
    bool hasChildren = item->getChildren().size() > 0;
    bool expanded = isExpanded(index);
    int nIteration = 0;
    do {

        QStyleOptionViewItemV2 itemOpt = opt;
        itemOpt.state &= ~QStyle::State_MouseOver;
        itemOpt.state &= ~QStyle::State_Selected;
        itemOpt.state |= extraFlags;

        primitive.moveLeft(primitive.left() - indent);
        itemOpt.rect = primitive;

        TableItemPtr parentItem = item->getParentItem();
        bool moreSiblings = false;
        if (parentItem) {
            moreSiblings = parentItem->getChildren().size() > 1 && item->getRowInParent() < (int)parentItem->getChildren().size() - 1;
        }

        if (nIteration == 0) {
            itemOpt.state |= (expanded ? QStyle::State_Open : QStyle::State_None);
            itemOpt.state |= (hasChildren ? QStyle::State_Children : QStyle::State_None);
        }

        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &itemOpt, painter, this);
        item = item->getParentItem();
        ++nIteration;

    } while (item);
    painter->setBrushOrigin(oldBO);


} // drawBranches

void
KnobItemsTableView::drawRow(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // The full row rectangle
    QRect rowRect = option.rect;

    // The rectangle at which the item starts if it is a child of another item
    QRect itemRect = visualRect(index);

    QRect branchRect( 0, rowRect.y(), itemRect.x(), rowRect.height() );


    TableItemPtr item = _imp->tableModel->getItem(index);
    assert(item);
    ModelItemsVec::iterator found = _imp->findItem(item);
    assert(found != _imp->items.end());

    // Draw the label section which is the only one using a regular item and not a KnobGui
    KnobTableItemPtr internalItem = found->internalItem.lock();
    bool isSelected = internalItem->getModel()->isItemSelected(internalItem);
    int labelCol = internalItem->getLabelColumnIndex();
    int xOffset = itemRect.x();
    {
        int colX = columnViewportPosition(labelCol);
        int colW = header()->sectionSize(labelCol);

        int x;
        if (labelCol == 0) {
            x = xOffset + colX;
        } else {
            x = colX;
        }
        QStyleOptionViewItemV4 opt = option;

        opt.rect.setRect(x, option.rect.y(), colW, option.rect.height());
        style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);

        if (isSelected) {
            opt.state |= QStyle::State_Selected;
        }
        QModelIndex modelIndex = _imp->tableModel->index(index.row(), labelCol, index.parent());
        itemDelegate(modelIndex)->paint(painter, opt, modelIndex);
    }
    

    drawBranches(painter, branchRect, index);

}

void
KnobItemsTableView::setupAndExecDragObject(QDrag* drag,
                                           const QModelIndexList& rows,
                                           Qt::DropActions supportedActions,
                                           Qt::DropAction defaultAction)

{

    std::list<KnobTableItemPtr> items;
    for (QModelIndexList::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        // Get the first col item
        TableItemPtr item = _imp->tableModel->getItem(*it);
        assert(item);
        if (!item) {
            continue;
        }
        ModelItemsVec::iterator found = _imp->findItem(item);
        if (found == _imp->items.end()) {
            continue;
        }
        KnobTableItemPtr internalItem = found->internalItem.lock();
        if (!internalItem) {
            continue;
        }
        items.push_back(internalItem);

    }

    if (items.empty()) {
        return;
    }

    // Make up drag data
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;

    NodeGuiPtr nodeUI = _imp->panel->getNodeGui();
    assert(nodeUI);
    obj.nodeScriptName = nodeUI->getNode()->getFullyQualifiedName();

    for (std::list<KnobTableItemPtr>::iterator it = items.begin(); it!= items.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
        (*it)->toSerialization(s.get());
        obj.items.push_back(s);
    }

    std::ostringstream ss;
    SERIALIZATION_NAMESPACE::write(ss, obj, std::string());

    QByteArray dataArray(ss.str().c_str());

    QMimeData *data = new QMimeData;
    data->setData(QLatin1String(kNatronKnobItemsTableGuiMimeType), dataArray);
    drag->setMimeData(data);
    
    if (drag->exec(supportedActions, defaultAction) == Qt::MoveAction) {
        
        // If the target is NULL, we have no choice but to remove data from the original table.
        // This means the drop finished on another instance of Natron
        if (!drag->target()) {
            KnobItemsTablePtr model = _imp->internalModel.lock();
            // If the target table is not this one, we have no choice but to remove from this table the items out of undo/redo operation
            for (std::list<KnobTableItemPtr>::iterator it = items.begin(); it!= items.end(); ++it) {
                model->removeItem(*it, eTableChangeReasonInternal);
            }
        }
    }
    
} // setupAndExecDragObject

void
KnobItemsTableView::dragMoveEvent(QDragMoveEvent *e)
{
    const QMimeData* mimedata = e->mimeData();
    if ( !mimedata->hasFormat( QLatin1String(kNatronKnobItemsTableGuiMimeType) ) || !_imp->internalModel.lock()->isDragAndDropSupported() ) {
        e->ignore();
    } else {
        e->accept();
    }
    TableView::dragMoveEvent(e);
}

void
KnobItemsTableView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData* mimedata = e->mimeData();
    if ( !mimedata->hasFormat( QLatin1String(kNatronKnobItemsTableGuiMimeType) ) || !_imp->internalModel.lock()->isDragAndDropSupported() ) {
        e->ignore();
    } else {
        e->accept();
    }
    TableView::dragEnterEvent(e);

}

void
KnobItemsTableGui::onViewItemRightClicked(const QPoint& globalPos, const TableItemPtr& item)
{
    ModelItemsVec::iterator foundItem = _imp->findItem(item);
    if (foundItem == _imp->items.end()) {
        return;
    }
    KnobTableItemPtr internalItem = foundItem->internalItem.lock();
    if (!internalItem) {
        return;
    }

    _imp->rightClickedItem = internalItem;

    std::string actionShortcutGroup;
    NodeGuiPtr node = _imp->panel->getNodeGui();
    if (node) {
        actionShortcutGroup = node->getNode()->getPlugin()->getPluginShortcutGroup();
    }

    Menu m(_imp->panel);
    // Add basic menu actions
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphRemoveNodes,
                                                            tr("Delete").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onDeleteItemsActionTriggered()) );
        m.addAction(action);

    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphCopy,
                                                            tr("Copy").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onCopyItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphCut,
                                                            tr("Cut").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onCutItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphPaste,
                                                            tr("Paste").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onPasteItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphDuplicate,
                                                            tr("Duplicate").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onDuplicateItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphSelectAll,
                                                            tr("Select All").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onSelectAllItemsActionTriggered()) );
        m.addAction(action);
    }

    m.addSeparator();

    KnobChoicePtr rightClickMenuKnob = toKnobChoice(internalItem->getKnobByName(kNatronOfxParamRightClickMenu));
    if (rightClickMenuKnob) {
        internalItem->refreshRightClickMenu();
        NodeGui::populateMenuRecursive(rightClickMenuKnob, internalItem, actionShortcutGroup, this, &m);
    }

    m.exec(globalPos);
    _imp->rightClickedItem.reset();
}

void
KnobItemsTableGui::onRightClickActionTriggered()
{
    ActionWithShortcut* action = dynamic_cast<ActionWithShortcut*>( sender() );

    if (!action) {
        return;
    }


    KnobTableItemPtr internalItem = _imp->rightClickedItem.lock();
    assert(internalItem);
    if (!internalItem) {
        return;
    }


    const std::vector<std::pair<QString, QKeySequence> >& shortcuts = action->getShortcuts();
    assert( !shortcuts.empty() );
    std::string knobName = shortcuts.front().first.toStdString();
    KnobIPtr knob = internalItem->getKnobByName(knobName);
    if (!knob) {
        // Plug-in specified invalid knob name in the menu
        return;
    }
    KnobButtonPtr button = toKnobButton(knob);
    if (!button) {
        // Plug-in must only use buttons inside menu
        return;
    }
    if ( button->getIsCheckable() ) {
        button->setValue( !button->getValue() );
    } else {
        button->trigger();
    }
}

void
KnobItemsTableView::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Delete) || (e->key() == Qt::Key_Backspace) ) {
        _imp->_publicInterface->onDeleteItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_C) && modCASIsControl(e) ) {
        _imp->_publicInterface->onCopyItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_V) && modCASIsControl(e) ) {
        _imp->_publicInterface->onPasteItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_X) && modCASIsControl(e) ) {
        _imp->_publicInterface->onCutItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_C) && modCASIsAlt(e) ) {
        _imp->_publicInterface->onDuplicateItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_A) && modCASIsControl(e) ) {
        _imp->_publicInterface->onSelectAllItemsActionTriggered();
    } else {
        TableView::keyPressEvent(e);
    }
}

NodeGuiPtr
KnobItemsTableGui::getNodeGui() const
{
    return _imp->panel->getNodeGui();
}

Gui*
KnobItemsTableGui::getGui() const
{
    return _imp->panel->getGui();
}

const QUndoCommand*
KnobItemsTableGui::getLastUndoCommand() const
{
    return _imp->panel->getLastUndoCommand();
}

void
KnobItemsTableGui::pushUndoCommand(QUndoCommand* cmd)
{
    _imp->panel->pushUndoCommand(cmd);
}

KnobGuiPtr
KnobItemsTableGui::getKnobGui(const KnobIPtr& knob) const {
    for (ModelItemsVec::const_iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        for (std::size_t i = 0; i < it->columnItems.size(); ++i) {
            if (it->columnItems[i].knob.lock() == knob) {
                return it->columnItems[i].guiKnob;
            }
        }
    }
    return KnobGuiPtr();
}

int
KnobItemsTableGui::getItemsSpacingOnSameLine() const
{
    return 0;
}

KnobItemsTablePtr
KnobItemsTableGui::getInternalTable() const
{
    return _imp->internalModel.lock();
}



class RemoveItemsUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveItemsUndoCommand)

public:

    struct Item
    {
        KnobTableItemPtr item;
        KnobTableItemPtr parent;
        int indexInParent;
    };
    
    RemoveItemsUndoCommand(KnobItemsTableGuiPrivate* table,
                           const std::list<KnobTableItemPtr>& items)
    : QUndoCommand()
    , _table(table)
    , _items()
    {
        for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it!=items.end(); ++it) {
            Item i;
            i.item = *it;
            i.indexInParent = (*it)->getIndexInParent();
            i.parent = (*it)->getParent();
            _items.push_back(i);
        }
    }

    virtual ~RemoveItemsUndoCommand()
    {
        
    }

    virtual void undo() OVERRIDE FINAL
    {
        KnobItemsTablePtr table = _table->internalModel.lock();
        for (std::list<Item>::const_iterator it = _items.begin(); it!=_items.end(); ++it) {
            table->insertItem(it->indexInParent, it->item, it->parent, eTableChangeReasonInternal);
        }
    }
    
    virtual void redo() OVERRIDE FINAL
    {
        KnobItemsTablePtr table = _table->internalModel.lock();
        for (std::list<Item>::const_iterator it = _items.begin(); it!=_items.end(); ++it) {
            table->removeItem(it->item, eTableChangeReasonInternal);
        }

    }

private:

    KnobItemsTableGuiPrivate* _table;
    std::list<Item> _items;
};



class DragItemsUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DragItemsUndoCommand)

public:

    struct Item
    {
        KnobTableItemPtr item;
        KnobTableItemWPtr oldParent;
        KnobTableItemWPtr newParent;
        int indexInOldParent, indexInNewParent;
    };


    DragItemsUndoCommand(KnobItemsTableGuiPrivate* table,
                         const KnobItemsTablePtr& originalTable,
                         const std::list<DragItemsUndoCommand::Item>& items)
    : QUndoCommand()
    , _table(table)
    , _originalTable(originalTable)
    , _items(items)
    {
        setText(tr("Re-organize items"));
    }

    virtual ~DragItemsUndoCommand()
    {

    }

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    void moveItem(int indexInparent, const KnobTableItemPtr& parent, const KnobTableItemPtr& item, const KnobItemsTablePtr& fromTable, const KnobItemsTablePtr& toTable);

    KnobItemsTableGuiPrivate* _table;
    KnobItemsTablePtr _originalTable;
    std::list<Item> _items;
};

void
DragItemsUndoCommand::moveItem(int indexInParent, const KnobTableItemPtr& parent, const KnobTableItemPtr& item, const KnobItemsTablePtr& fromTable, const KnobItemsTablePtr& toTable)
{
    fromTable->removeItem(item, eTableChangeReasonInternal);
    toTable->insertItem(indexInParent, item, parent, eTableChangeReasonInternal);
    if (parent) {
        ModelItemsVec::iterator foundParent = _table->findItem(parent);
        if (foundParent != _table->items.end()) {
            _table->tableView->setExpanded(_table->tableModel->getItemIndex(foundParent->item), true);
        }
    }


    _table->createCustomWidgetRecursively(item);


}

void
DragItemsUndoCommand::undo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        moveItem(it->indexInOldParent, it->oldParent.lock(), it->item, _table->internalModel.lock(), _originalTable);
    }
}

void
DragItemsUndoCommand::redo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        moveItem(it->indexInNewParent, it->newParent.lock(), it->item, _originalTable, _table->internalModel.lock());
    }

}


void
KnobItemsTableView::dropEvent(QDropEvent* e)
{

    KnobItemsTablePtr table = _imp->internalModel.lock();
    if (!table->isDragAndDropSupported()) {
        return;
    }
    const QMimeData* mimedata = e->mimeData();
    static const QString mimeDataType(QLatin1String(kNatronKnobItemsTableGuiMimeType));
    if ( !mimedata->hasFormat(mimeDataType) ) {
        e->ignore();
        TableView::dropEvent(e);
        return;
    } else {
        e->accept();
    }

    QVariant data = mimedata->data(mimeDataType);
    QString serializationStr = data.toString();
    std::stringstream ss(serializationStr.toStdString());
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    try {
        SERIALIZATION_NAMESPACE::read(std::string(), ss, &obj);
    } catch (...) {
        e->ignore();
        return;
    }


    // Find the original table from which the knob was from
    // Operation was move, hence remove items from this view
    KnobItemsTablePtr originalTable;
    {
        NodePtr originalNode = _imp->_publicInterface->getGui()->getApp()->getProject()->getNodeByFullySpecifiedName(obj.nodeScriptName);
        if (!originalNode) {
            e->ignore();
            return;
        }

        NodeGuiPtr originalNodeUI = boost::dynamic_pointer_cast<NodeGui>(originalNode->getNodeGui());
        assert(originalNodeUI);
        NodeSettingsPanel* originalPanel = originalNodeUI->getSettingPanel();
        assert(originalPanel);
        if (originalPanel) {
            KnobItemsTableGuiPtr originalTableUI = originalPanel->getKnobItemsTable();
            if (originalTableUI) {
                originalTable = originalTableUI->getInternalTable();
            }
        }
    }

    KnobTableItemPtr targetInternalItem;
    {
        TableItemPtr targetItem = itemAt(e->pos());
        if (targetItem) {
            ModelItemsVec::iterator found = _imp->findItem(targetItem);
            if (found != _imp->items.end()) {
                targetInternalItem = found->internalItem.lock();
            }

        }
    }

    std::list<KnobTableItemPtr> droppedItems;
    for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = obj.items.begin(); it != obj.items.end(); ++it) {
        KnobTableItemPtr newItem = table->createItemFromSerialization(*it);
        if (newItem) {
            droppedItems.push_back(newItem);
        }
    }
    if (droppedItems.empty()) {
        return;
    }


    //OnItem, AboveItem, BelowItem, OnViewport
    DropIndicatorPosition position = dropIndicatorPosition();

    e->accept();

    std::list<DragItemsUndoCommand::Item> dndItems;
    for (std::list<KnobTableItemPtr>::const_iterator it = droppedItems.begin(); it!=droppedItems.end(); ++it) {

        DragItemsUndoCommand::Item d;
        d.oldParent = (*it)->getParent();
        d.indexInOldParent = (*it)->getIndexInParent();
        d.item = (*it);
        switch (position) {
            case QAbstractItemView::AboveItem: {

                assert(targetInternalItem);
                int targetItemIndex = targetInternalItem->getIndexInParent();

                assert(d.indexInOldParent != -1 && targetItemIndex != -1);

                // If the dropped item is already into the children and after the found index don't decrement
                if (d.indexInOldParent > targetItemIndex) {
                    d.indexInNewParent = targetItemIndex;
                } else {
                    //The item "above" in the tree is index - 1 in the internal list which is ordered from bottom to top
                    d.indexInNewParent = targetItemIndex == 0 ? 0 : targetItemIndex - 1;
                }
                d.newParent = targetInternalItem->getParent();

                break;
            }
            case QAbstractItemView::BelowItem: {
                assert(targetInternalItem);
                int targetItemIndex = targetInternalItem->getIndexInParent();

                assert(d.indexInOldParent != -1 && targetItemIndex != -1);

                // If the dropped item is already into the children and before the found index don't decrement
                if (d.indexInOldParent < targetItemIndex) {
                    d.indexInNewParent = targetItemIndex;
                } else {
                    // The item "below" in the tree is index + 1 in the internal list which is ordered from bottom to top
                    d.indexInNewParent = targetItemIndex + 1;
                }
                d.newParent = targetInternalItem->getParent();

                dndItems.push_back(d);
                break;
            }
            case QAbstractItemView::OnItem: {
                // Only allow dropping when the layout is a tree
                if (_imp->internalModel.lock()->getType() == KnobItemsTable::eKnobItemsTableTypeTree) {
                    // always insert on-top of others
                    d.indexInNewParent = 0;
                    d.newParent = targetInternalItem;
                } else {
                    continue;
                }
                break;
            }
            case QAbstractItemView::OnViewport: {
                // Only allow dragging on viewport for tables
                if (_imp->internalModel.lock()->getType() == KnobItemsTable::eKnobItemsTableTypeTable) {
                    // Append the top-level item
                    d.indexInNewParent = -1;
                } else {
                    continue;
                }
                break;
            }
        }
        dndItems.push_back(d);

    }
    if (!dndItems.empty()) {
        _imp->panel->pushUndoCommand(new DragItemsUndoCommand(_imp, originalTable, dndItems));
    }

    
} // dropEvent



class PasteItemUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(PasteItemUndoCommand)

public:


    struct PastedItem
    {
        QTreeWidgetItem* treeItem;
        RotoItemPtr rotoItem;
        RotoItemPtr itemCopy;
    };


    PasteItemUndoCommand(KnobItemsTableGuiPrivate* table,
                         const KnobTableItemPtr& target,
                         const SERIALIZATION_NAMESPACE::KnobItemsTableSerialization& source)
    : QUndoCommand()
    , _targetItem(target)
    , _originalTargetItemSerialization()
    , _sourceItemsCopies()
    , _sourceItemSerialization()
    {
        // Make sure tables match content type
        KnobItemsTablePtr model = table->internalModel.lock();
        assert(source.tableIdentifier == model->getTableIdentifier());
        
        // Remember the state of the target item
        if (target) {
            target->toSerialization(&_originalTargetItemSerialization);
        }
        
        // If this is a tree and the item can receive children, add as sub children
        if ((!target || target->isItemContainer()) && model->getType() == KnobItemsTable::eKnobItemsTableTypeTree) {
            for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = source.items.begin(); it!=source.items.end(); ++it) {
                KnobTableItemPtr copy = model->createItemFromSerialization(*it);
                if (copy) {
                    _sourceItemsCopies.push_back(copy);
                }
            }
        } else {
            // Should have been caught earlier with a nice error dialog for the user:
            // You cannot paste multiple items onto one
            assert(source.items.size() == 1);
            _sourceItemSerialization = source.items.front();
        }
        setText(tr("Paste Item(s)"));
    }

    virtual ~PasteItemUndoCommand()
    {
    }

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    KnobTableItemPtr _targetItem;
    SERIALIZATION_NAMESPACE::KnobTableItemSerialization _originalTargetItemSerialization;
    
    // Only used when pasting multiple items as children of a container
    std::list<KnobTableItemPtr> _sourceItemsCopies;
    
    // Only used when pasting an item onto another one
    SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr _sourceItemSerialization;
};

void
PasteItemUndoCommand::redo()
{
    if (_sourceItemSerialization) {
        // We paste 1 item onto another
        assert(_targetItem);
        _targetItem->fromSerialization(*_sourceItemSerialization);
    } else {
        // We paste multiple items as children of a container
        for (std::list<KnobTableItemPtr>::const_iterator it = _sourceItemsCopies.begin(); it!=_sourceItemsCopies.end(); ++it) {
            _targetItem->getModel()->insertItem(-1, *it, _targetItem, eTableChangeReasonInternal);
        }
    }
}

void
PasteItemUndoCommand::undo()
{
    if (_sourceItemSerialization) {
        // We paste 1 item onto another
        assert(_targetItem);
        _targetItem->fromSerialization(_originalTargetItemSerialization);
    } else {
        // We paste multiple items as children of a container
        for (std::list<KnobTableItemPtr>::const_iterator it = _sourceItemsCopies.begin(); it!=_sourceItemsCopies.end(); ++it) {
            _targetItem->getModel()->removeItem(*it, eTableChangeReasonInternal);
        }
    }
}

class DuplicateItemUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DuplicateItemUndoCommand)

public:


    DuplicateItemUndoCommand(KnobItemsTableGuiPrivate* table,
                             const std::list<KnobTableItemPtr>& items)
    : QUndoCommand()
    , _table(table)
    , _items()
    {
        for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
            SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
            (*it)->toSerialization(s.get());
            KnobTableItemPtr dup = table->internalModel.lock()->createItemFromSerialization(s);
            assert(dup);
            _duplicates.push_back(dup);
        }
        
        
        
    }

    virtual ~DuplicateItemUndoCommand()
    {

    }

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    
private:
    

    KnobItemsTableGuiPrivate* _table;
    std::list<KnobTableItemPtr> _items, _duplicates;
};

void
DuplicateItemUndoCommand::redo()
{
    assert(_duplicates.size() == _items.size());
    std::list<KnobTableItemPtr>::const_iterator ito = _items.begin();
    for (std::list<KnobTableItemPtr>::const_iterator it = _duplicates.begin(); it != _duplicates.end(); ++it, ++ito) {
        int itemIndex = (*ito)->getIndexInParent();
        itemIndex += 1;
        KnobTableItemPtr parent = (*ito)->getParent();
        _table->internalModel.lock()->insertItem(itemIndex, *it, parent, eTableChangeReasonInternal);
    }
    
}

void
DuplicateItemUndoCommand::undo()
{
    assert(_duplicates.size() == _items.size());
    for (std::list<KnobTableItemPtr>::const_iterator it = _duplicates.begin(); it != _duplicates.end(); ++it) {
        _table->internalModel.lock()->removeItem(*it, eTableChangeReasonInternal);
    }
}

void
KnobItemsTableGui::onDeleteItemsActionTriggered()
{
    std::list<KnobTableItemPtr> selection = _imp->internalModel.lock()->getSelectedItems();
    if (selection.empty()) {
        return;
    }
    pushUndoCommand(new RemoveItemsUndoCommand(_imp.get(), selection));
}

void
KnobItemsTableGui::onCopyItemsActionTriggered()
{
    KnobItemsTablePtr model = _imp->internalModel.lock();
    std::list<KnobTableItemPtr> selection = model->getSelectedItems();
    if (selection.empty()) {
        return;
    }
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    obj.tableIdentifier = model->getTableIdentifier();
    obj.nodeScriptName = model->getNode()->getFullyQualifiedName();
    for (std::list<KnobTableItemPtr>::const_iterator it = selection.begin(); it!=selection.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
        (*it)->toSerialization(s.get());
        obj.items.push_back(s);
    }
    
    std::ostringstream ss;
    try {
        SERIALIZATION_NAMESPACE::write(ss, obj, std::string());
    } catch (...) {
        return;
    }
    
    QMimeData* mimedata = new QMimeData;
    QByteArray data( ss.str().c_str() );
    mimedata->setData(QLatin1String("text/plain"), data);
    QClipboard* clipboard = QApplication::clipboard();
    
    // Ownership is transferred to the clipboard
    clipboard->setMimeData(mimedata);
}

void
KnobItemsTableGui::onPasteItemsActionTriggered()
{
    KnobItemsTablePtr model = _imp->internalModel.lock();
    std::list<KnobTableItemPtr> selection = model->getSelectedItems();

    if (selection.size() > 1) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("You can only copy an item onto another one or on the view itself").toStdString());
        return;
    }
    
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    if (!data) {
        return;
    }
    std::string str;
    {
        QByteArray array = data->data(QLatin1String("text/plain"));
        if (array.data()) {
            str = std::string(array.data());
        }
    }
    
    std::istringstream ss(str);
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    try {
        SERIALIZATION_NAMESPACE::read(std::string(), ss, &obj);
    } catch (...) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("You cannot copy this kind of data here").toStdString());
        return;
    }
    
    // Check that table is of the same type
    if (obj.tableIdentifier != model->getTableIdentifier()) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("You cannot copy this kind of data here").toStdString());
        return;
    }
    
    KnobTableItemPtr target;
    if (!selection.empty()) {
        target = selection.front();
    }
    
    if (obj.items.size() > 1 && target && !target->isItemContainer()) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("%1 is not a container, you can only copy a single item onto it").arg(QString::fromUtf8(target->getScriptName_mt_safe().c_str())).toStdString());
        return;
    }
    
    pushUndoCommand(new PasteItemUndoCommand(_imp.get(), target, obj));
    
    
}

void
KnobItemsTableGui::onCutItemsActionTriggered()
{
    onCopyItemsActionTriggered();
    onDeleteItemsActionTriggered();
    
}

void
KnobItemsTableGui::onDuplicateItemsActionTriggered()
{
    std::list<KnobTableItemPtr> selection = _imp->internalModel.lock()->getSelectedItems();
    if (selection.empty()) {
        return;
    }
    pushUndoCommand(new DuplicateItemUndoCommand(_imp.get(), selection));
}

void
KnobItemsTableGui::onTableItemDataChanged(const TableItemPtr& item, int col, int role)
{
    if (!item) {
        return;
    }
    if (role != Qt::DisplayRole) {
        return;
    }
    ModelItemsVec::iterator found = _imp->findItem(item);
    if (found == _imp->items.end()) {
        return;
    }
    KnobTableItemPtr internalItem = found->internalItem.lock();
    if (!internalItem) {
        return;
    }

    // If the column is handled by a knob GUI, then we do not bother handling interfacing with the knob here since everything is handled in the KnobGui side
    DimSpec knobDim;
    KnobIPtr knob = internalItem->getColumnKnob(col, &knobDim);
    if (knob) {
        return;
    }

    std::string colName = internalItem->getColumnName(col);
    if (colName == kKnobTableItemColumnLabel) {
        QString label = item->getText(col);

        internalItem->setLabel(label.toStdString(), eTableChangeReasonPanel);
    }
    
}


void
KnobItemsTableGui::onSelectAllItemsActionTriggered()
{
    _imp->internalModel.lock()->selectAll(eTableChangeReasonInternal);
}

void
KnobItemsTableGuiPrivate::selectionFromIndexList(const QModelIndexList& indexes, std::list<KnobTableItemPtr>* outItems)
{
    std::set<KnobTableItemPtr> uniqueItems;
    for (QModelIndexList::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
        //Check that the item is valid
        assert(it->isValid() && it->column() >= 0 && it->column() < tableModel->columnCount());
        
        // Get the table item corresponding to the index
        TableItemPtr tableItem = tableModel->getItem(it->row(), it->parent());
        if (!tableItem) {
            continue;
        }

        // Get the internal knobtableitem corresponding to the table item
        ModelItemsVec::iterator found = findItem(tableItem);
        assert(found != items.end());
        if (found == items.end()) {
            continue;
        }
        
        KnobTableItemPtr internalItem = found->internalItem.lock();
        assert(internalItem);
        if (!internalItem) {
            continue;
        }
        uniqueItems.insert(internalItem);
    }
    
    for (std::set<KnobTableItemPtr>::iterator it = uniqueItems.begin(); it != uniqueItems.end(); ++it) {
        outItems->push_back(*it);
    }
}

void
KnobItemsTableGuiPrivate::itemsToSelection(const std::list<KnobTableItemPtr>& inItems, QItemSelection* selection)
{
    
    for (std::list<KnobTableItemPtr>::const_iterator it = inItems.begin(); it!=inItems.end(); ++it) {
        ModelItemsVec::iterator found = findItem(*it);
        if (found == items.end()) {
            continue;
        }
        assert(!found->columnItems.empty());
        QModelIndex leftMost = tableModel->getItemIndex(found->item);
        QModelIndex rightMost = tableModel->index(leftMost.row(), tableModel->columnCount() - 1, leftMost.parent());
        QItemSelectionRange t(leftMost, rightMost);
        for (std::size_t i = 0; i < found->columnItems.size(); ++i) {
            selection->append(t);
        }
    }
}

void
KnobItemsTableGuiPrivate::selectionToItems(const QItemSelection& selection, std::list<KnobTableItemPtr>* items)
{
    QModelIndexList indexes = selection.indexes();
    selectionFromIndexList(indexes, items);
}

void
KnobItemsTableGui::onModelSelectionChanged(const QItemSelection& selected,const QItemSelection& deselected)
{
    // Watch for recursion
    if (_imp->selectingModelRecursion) {
        return;
    }
    
    // Convert indexes to items
    std::list<KnobTableItemPtr> deselectedItems, selectedItems;
    _imp->selectionToItems(selected, &selectedItems);
    _imp->selectionToItems(deselected, &deselectedItems);

    _imp->refreshKnobsSelectionState(selectedItems, deselectedItems);

    _imp->refreshUserKeyFramesWidgets();

    // Select the items in the model internally
    KnobItemsTablePtr model = _imp->internalModel.lock();
    model->beginEditSelection();
    model->removeFromSelection(deselectedItems, eTableChangeReasonPanel);
    model->addToSelection(selectedItems, eTableChangeReasonPanel);
    model->endEditSelection(eTableChangeReasonPanel);

    getNodeGui()->getDagGui()->getGui()->refreshTimelineGuiKeyframesLater();
}

void
KnobItemsTableGui::onModelSelectionChanged(const std::list<KnobTableItemPtr>& addedToSelection, const std::list<KnobTableItemPtr>& removedFromSelection, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        // Do not recurse
        return;
    }

    // Refresh the view
    QItemSelection selectionToAdd, selectionToRemove;
    _imp->itemsToSelection(addedToSelection, &selectionToAdd);
    _imp->itemsToSelection(removedFromSelection, &selectionToRemove);
    QItemSelectionModel* selectionModel = _imp->tableView->selectionModel();

    _imp->refreshKnobsSelectionState(addedToSelection, removedFromSelection);
    
    // Ensure we don't recurse indefinitely in the selection
    ++_imp->selectingModelRecursion;
    if (!removedFromSelection.empty()) {
        selectionModel->select(selectionToRemove, QItemSelectionModel::Deselect);
    }
    if (!addedToSelection.empty()) {
        selectionModel->select(selectionToAdd, QItemSelectionModel::Select);
    }
    --_imp->selectingModelRecursion;

    _imp->refreshUserKeyFramesWidgets();
    getNodeGui()->getDagGui()->getGui()->refreshTimelineGuiKeyframesLater();
    
}


void
KnobItemsTableGui::onItemAnimationCurveChanged(std::list<double> /*added*/, std::list<double> /*removed*/, ViewIdx /*view*/)
{
    _imp->refreshUserKeyFramesWidgets();
    getNodeGui()->getDagGui()->getGui()->refreshTimelineGuiKeyframesLater();
}


void
KnobItemsTableGui::onItemLabelChanged(const QString& label, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }

    KnobTableItem* item = dynamic_cast<KnobTableItem*>(sender());
    if (!item) {
        return;
    }


    int labelColIndex = item->getLabelColumnIndex();
    if (labelColIndex == -1) {
        return;
    }

    TableItemPtr tableItem;
    for (ModelItemsVec::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->internalItem.lock().get() == item) {
            tableItem = it->item;
            break;
        }
    }
    if (!tableItem) {
        return;
    }


    tableItem->setText(labelColIndex, label);
}

void
KnobItemsTableGuiPrivate::setItemIcon(const TableItemPtr& tableItem, int col, const KnobTableItemPtr& item)
{
    QString iconFilePath = QString::fromUtf8(item->getIconLabelFilePath().c_str());
    if (iconFilePath.isEmpty()) {
        return;
    }
    QPixmap pix;
    if (!QPixmapCache::find(iconFilePath, pix) ) {
        if (!pix.load(iconFilePath)) {
            return;
        }
        QPixmapCache::insert(iconFilePath, pix);
    }


    if (std::max( pix.width(), pix.height() ) != NATRON_MEDIUM_BUTTON_ICON_SIZE) {
        pix = pix.scaled(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    tableItem->setIcon(col, QIcon(pix));

}

void
KnobItemsTableGui::onItemIconChanged(TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }

    KnobTableItem* item = dynamic_cast<KnobTableItem*>(sender());
    if (!item) {
        return;
    }
    KnobTableItemPtr itemShared = toKnobTableItem(item->shared_from_this());

    int labelColIndex = item->getLabelColumnIndex();
    if (labelColIndex == -1) {
        return;
    }

    TableItemPtr tableItem;
    for (ModelItemsVec::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->internalItem.lock().get() == item) {
            tableItem = it->item;
            break;
        }
    }
    if (!tableItem) {
        return;
    }

    _imp->setItemIcon(tableItem, labelColIndex, itemShared);
}

void
KnobItemsTableGui::onModelItemRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }
    disconnect(item.get(), SIGNAL(curveAnimationChanged(std::list<double>, std::list<double>, ViewIdx)), this, SLOT(onItemAnimationCurveChanged(std::list<double>,std::list<double>, ViewIdx)));
    _imp->removeTableItem(item);
}

void
KnobItemsTableGui::onModelItemInserted(int /*index*/, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }
    connect(item.get(), SIGNAL(curveAnimationChanged(std::list<double>,std::list<double>, ViewIdx)), this, SLOT(onItemAnimationCurveChanged(std::list<double>, std::list<double>, ViewIdx)), Qt::UniqueConnection);
    _imp->createTableItems(item);
}

void
KnobItemsTableGuiPrivate::createItemsVecRecursive(const std::vector<KnobTableItemPtr>& items)
{
    for (std::size_t i = 0; i < items.size(); ++i) {
        createTableItems(items[i]);
        const std::vector<KnobTableItemPtr>& children = items[i]->getChildren();
        if (!children.empty()) {
            createItemsVecRecursive(children);
        }
    }
}

void
KnobItemsTableGuiPrivate::recreateItemsFromModel()
{
    assert(items.empty());
    const std::vector<KnobTableItemPtr>& items = internalModel.lock()->getTopLevelItems();
    createItemsVecRecursive(items);
}


static QString
labelToolTipFromScriptName(const KnobTableItemPtr& item)
{
    return ( QString::fromUtf8("<p><b>")
            + QString::fromUtf8( item->getScriptName_mt_safe().c_str() )
            + QString::fromUtf8("</b></p>")
            +  NATRON_NAMESPACE::convertFromPlainText(QCoreApplication::translate("KnobItemsTableGui", "The label of the item"), NATRON_NAMESPACE::WhiteSpaceNormal) );
}

void
KnobItemsTableGuiPrivate::removeTableItem(const KnobTableItemPtr& item)
{
    ModelItemsVec::iterator foundItem = findItem(item);
    if (foundItem == items.end()) {
        return;
    }
    TableItemPtr tableItem = foundItem->item;
    if (!tableItem) {
        return;
    }
    tableModel->removeItem(tableItem);
    items.erase(foundItem);
}

void
KnobItemsTableGuiPrivate::createTableItems(const KnobTableItemPtr& item)
{
    // The item should not exist in the table GUI yet.
    assert(findItem(item) == items.end());
    
    int itemRow = item->getIndexInParent();
    
    int nCols = tableModel->columnCount();
    

    items.push_back(ModelItem());
    ModelItem &mitem = items.back();
    mitem.columnItems.resize(nCols);
    mitem.item = TableItem::create(tableModel);
    mitem.internalItem = item;

    TableItemPtr parentItem;
    KnobTableItemPtr knobParentItem = item->getParent();
    if (knobParentItem) {
        ModelItemsVec::iterator foundParent = findItem(knobParentItem);
        assert(foundParent != items.end());
        if (foundParent != items.end()) {
            parentItem = foundParent->item;
        }
    }
    if (parentItem) {
        parentItem->insertChild(itemRow, mitem.item);
    } else {
        tableModel->insertTopLevelItem(itemRow, mitem.item);
    }

    for (int i = 0; i < nCols; ++i) {
        
        ModelItem::ColumnData d;
        // If this column represents a knob, this is the knob
        d.knob = item->getColumnKnob(i, &d.knobDimension);
        

        
        if (d.knob.lock()) {
            // If we have a knob, create the custom widget
            createItemCustomWidgetAtCol(item, itemRow, i);
        } else {
            // Ok the column must be kKnobTableItemColumnLabel
            // otherwise we don't know what the user want
            std::string columnID = item->getColumnName(i);
            if (columnID == kKnobTableItemColumnLabel) {
                mitem.labelColIndex = i;
                mitem.item->setToolTip(i, labelToolTipFromScriptName(item) );
                mitem.item->setFlags(i, Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
                mitem.item->setText(i, QString::fromUtf8( item->getLabel().c_str() ) );
                setItemIcon(mitem.item, i, item);
            } else {
                mitem.item->setFlags(i, Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            }


        }
        
        tableView->resizeColumnToContents(i);
        mitem.columnItems[i] = d;

    }

    tableView->setExpanded(tableModel->getItemIndex(mitem.item), true);

    // Create custom widgets for knobs
    for (int i = 0; i < nCols; ++i) {
        createItemCustomWidgetAtCol(item, itemRow, i);
    }

    QObject::connect(item.get(), SIGNAL(labelChanged(QString,TableChangeReasonEnum)), _publicInterface, SLOT(onItemLabelChanged(QString,TableChangeReasonEnum)), Qt::UniqueConnection);
    QObject::connect(item.get(), SIGNAL(labelIconChanged(TableChangeReasonEnum)), _publicInterface, SLOT(onItemIconChanged(TableChangeReasonEnum)), Qt::UniqueConnection);

    
} // createTableItems

void
KnobItemsTableGuiPrivate::refreshKnobItemSelection(const KnobTableItemPtr& item, bool selected)
{
    ModelItemsVec::iterator found = findItem(item);
    if (found == items.end()) {
        return;
    }
    for (std::size_t i = 0; i < found->columnItems.size(); ++i) {
        if (found->columnItems[i].guiKnob) {
            found->columnItems[i].guiKnob->reflectKnobSelectionState(selected);
        }
    }
}

void
KnobItemsTableGuiPrivate::refreshKnobsSelectionState(const std::list<KnobTableItemPtr>& selected, const std::list<KnobTableItemPtr>& deselected)
{
    for (std::list<KnobTableItemPtr>::const_iterator it = selected.begin(); it != selected.end(); ++it) {
        refreshKnobItemSelection(*it, true);
    }
    for (std::list<KnobTableItemPtr>::const_iterator it = deselected.begin(); it != deselected.end(); ++it) {
        refreshKnobItemSelection(*it, false);
    }
}

void
KnobItemsTableGuiPrivate::refreshUserKeyFramesWidgets()
{
    if (!keyframesContainerWidget) {
        return;
    }
    std::list<KnobTableItemPtr> selectedItems = internalModel.lock()->getSelectedItems();

    std::set<double> keys;
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        (*it)->getMasterKeyFrameTimes(ViewIdx(0), &keys);
    }

    numKeyFramesSpinBox->setValue( (double)keys.size() );

    double currentTime = _publicInterface->getGui()->getApp()->getProject()->currentFrame();

    if ( keys.empty() ) {
        currentKeyFrameSpinBox->setValue(0.);
        currentKeyFrameSpinBox->setAnimation(0);
    } else {
        ///get the first time that is equal or greater to the current time
        std::set<double>::iterator lowerBound = keys.lower_bound(currentTime);
        int dist = 0;
        if ( lowerBound != keys.end() ) {
            dist = std::distance(keys.begin(), lowerBound);
        }

        if ( lowerBound == keys.end() ) {
            ///we're after the last keyframe
            currentKeyFrameSpinBox->setValue( (double)keys.size() );
            currentKeyFrameSpinBox->setAnimation(1);
        } else if (*lowerBound == currentTime) {
            currentKeyFrameSpinBox->setValue(dist + 1);
            currentKeyFrameSpinBox->setAnimation(2);
        } else {
            ///we're in-between 2 keyframes, interpolate
            if ( lowerBound == keys.begin() ) {
                currentKeyFrameSpinBox->setValue(1.);
            } else {
                std::set<double>::iterator prev = lowerBound;
                if ( prev != keys.begin() ) {
                    --prev;
                }
                currentKeyFrameSpinBox->setValue( (double)(currentTime - *prev) / (double)(*lowerBound - *prev) + dist );
            }

            currentKeyFrameSpinBox->setAnimation(1);
        }
    }

    const bool enabled = selectedItems.size() > 0;

    userKeyframeLabel->setEnabled(enabled);
    currentKeyFrameSpinBox->setEnabled(enabled);
    ofTotalKeyFramesLabel->setEnabled(enabled);
    numKeyFramesSpinBox->setEnabled(enabled);
    goToPrevKeyframeButton->setEnabled(enabled);
    goToNextKeyframeButton->setEnabled(enabled);
    addKeyFrameButton->setEnabled(enabled);
    removeKeyFrameButton->setEnabled(enabled);
    clearKeyFramesButton->setEnabled(enabled);

} // refreshUserKeyFramesWidgets

void
KnobItemsTableGui::refreshAfterTimeChanged()
{
    _imp->refreshUserKeyFramesWidgets();
}

void
KnobItemsTableGui::onGoToPrevKeyframeButtonClicked()
{
    std::list<KnobTableItemPtr> selectedItems = _imp->internalModel.lock()->getSelectedItems();
    std::set<double> keys;
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        (*it)->getMasterKeyFrameTimes(ViewIdx(0), &keys);
    }

    if (keys.empty()) {
        return;
    }

    TimeLinePtr timeline = getGui()->getApp()->getProject()->getTimeLine();
    double currentTime = timeline->currentFrame();

    double prevKeyTime = 0;
    std::set<double>::iterator lb = keys.lower_bound(currentTime);
    if (lb == keys.end()) {
        // Check if the last keyframe is prior to the current time
        std::set<double>::reverse_iterator last = keys.rbegin();
        if (*last >= currentTime) {
            return;
        } else {
            prevKeyTime = *last;
        }
    } else if (lb == keys.begin()) {
        // No keyframe before
        return;
    } else {
        --lb;
        prevKeyTime = *lb;
    }

    timeline->seekFrame(prevKeyTime, false, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);


}

void
KnobItemsTableGui::onGoToNextKeyframeButtonClicked()
{
    std::list<KnobTableItemPtr> selectedItems = _imp->internalModel.lock()->getSelectedItems();
    std::set<double> keys;
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        (*it)->getMasterKeyFrameTimes(ViewIdx(0), &keys);
    }

    if (keys.empty()) {
        return;
    }

    TimeLinePtr timeline = getGui()->getApp()->getProject()->getTimeLine();
    double currentTime = timeline->currentFrame();

    double nextKeyTime = 0;
    std::set<double>::iterator ub = keys.upper_bound(currentTime);
    if (ub == keys.end()) {
        return;
    } else {
        nextKeyTime = *ub;
    }

    timeline->seekFrame(nextKeyTime, false, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);
}

void
KnobItemsTableGui::onAddKeyframeButtonClicked()
{
    TimeLinePtr timeline = getGui()->getApp()->getProject()->getTimeLine();
    double currentTime = timeline->currentFrame();

    std::list<KnobTableItemPtr> selectedItems = _imp->internalModel.lock()->getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        (*it)->setKeyFrame(TimeValue(currentTime), ViewIdx(0), 0);
    }

}

void
KnobItemsTableGui::onRemoveKeyframeButtonClicked()
{
    TimeLinePtr timeline = getGui()->getApp()->getProject()->getTimeLine();
    double currentTime = timeline->currentFrame();

    std::list<KnobTableItemPtr> selectedItems = _imp->internalModel.lock()->getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        (*it)->deleteValueAtTime(TimeValue(currentTime), ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);
    }
}

void
KnobItemsTableGui::onRemoveAnimationButtonClicked()
{
    std::list<KnobTableItemPtr> selectedItems = _imp->internalModel.lock()->getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        (*it)->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);
    }

}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING
#include "moc_KnobItemsTableGui.cpp"
