#include "DopeSheetEditor.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>

#include "Gui/DopeSheet.h"
#include "Gui/DopeSheetHierarchyView.h"
#include "Gui/DopeSheetView.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"


////////////////////////// DopeSheetEditor //////////////////////////

class DopeSheetEditorPrivate
{
public:
    DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui);

    /* attributes */
    DopeSheetEditor *q_ptr;
    Gui *gui;

    QVBoxLayout *mainLayout;
    QHBoxLayout *helpersLayout;

    DopeSheet *model;

    QSplitter *splitter;
    HierarchyView *hierarchyView;
    DopeSheetView *dopeSheetView;

    QPushButton *toggleTripleSyncBtn;
};

DopeSheetEditorPrivate::DopeSheetEditorPrivate(DopeSheetEditor *qq, Gui *gui)  :
    q_ptr(qq),
    gui(gui),
    mainLayout(0),
    helpersLayout(0),
    model(0),
    splitter(0),
    hierarchyView(0),
    dopeSheetView(0),
    toggleTripleSyncBtn(0)
{}

/**
 * @brief DopeSheetEditor::DopeSheetEditor
 *
 * Creates a DopeSheetEditor.
 */
DopeSheetEditor::DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent) :
    QWidget(parent),
    ScriptObject(),
    _imp(new DopeSheetEditorPrivate(this, gui))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->helpersLayout = new QHBoxLayout();
    _imp->helpersLayout->setContentsMargins(0, 0, 0, 0);

    _imp->toggleTripleSyncBtn = new QPushButton(this);

    QPixmap tripleSyncBtnPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_UNLOCKED, &tripleSyncBtnPix);

    _imp->toggleTripleSyncBtn->setIcon(QIcon(tripleSyncBtnPix));
    _imp->toggleTripleSyncBtn->setToolTip(tr("Toggle triple synchronization"));
    _imp->toggleTripleSyncBtn->setCheckable(true);
    _imp->toggleTripleSyncBtn->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    
    connect(_imp->toggleTripleSyncBtn, SIGNAL(toggled(bool)),
            this, SLOT(toggleTripleSync(bool)));

    _imp->helpersLayout->addWidget(_imp->toggleTripleSyncBtn);
    _imp->helpersLayout->addStretch();

    _imp->splitter = new QSplitter(Qt::Horizontal, this);
    _imp->splitter->setHandleWidth(1);

    _imp->model = new DopeSheet(gui, timeline);

    _imp->hierarchyView = new HierarchyView(_imp->model, gui, _imp->splitter);

    _imp->splitter->addWidget(_imp->hierarchyView);
    _imp->splitter->setStretchFactor(0, 1);

    _imp->dopeSheetView = new DopeSheetView(_imp->model, _imp->hierarchyView, gui, timeline, _imp->splitter);

    _imp->splitter->addWidget(_imp->dopeSheetView);
    _imp->splitter->setStretchFactor(1, 5);

    _imp->mainLayout->addWidget(_imp->splitter);
    _imp->mainLayout->addLayout(_imp->helpersLayout);

    // Main model -> HierarchyView connections
    connect(_imp->model, SIGNAL(nodeAdded(DSNode *)),
            _imp->hierarchyView, SLOT(onNodeAdded(DSNode *)));

    connect(_imp->model, SIGNAL(nodeAboutToBeRemoved(DSNode *)),
            _imp->hierarchyView, SLOT(onNodeAboutToBeRemoved(DSNode *)));

    connect(_imp->model, SIGNAL(keyframeSetOrRemoved(DSKnob *)),
            _imp->hierarchyView, SLOT(onKeyframeSetOrRemoved(DSKnob *)));

    connect(_imp->model->getSelectionModel(), SIGNAL(keyframeSelectionChangedFromModel()),
            _imp->hierarchyView, SLOT(onKeyframeSelectionChanged()));

    // Main model -> DopeSheetView connections
    connect(_imp->model, SIGNAL(nodeAdded(DSNode*)),
            _imp->dopeSheetView, SLOT(onNodeAdded(DSNode *)));

    connect(_imp->model, SIGNAL(nodeAboutToBeRemoved(DSNode*)),
            _imp->dopeSheetView, SLOT(onNodeAboutToBeRemoved(DSNode *)));

    connect(_imp->model, SIGNAL(modelChanged()),
            _imp->dopeSheetView, SLOT(redraw()));

    connect(_imp->model->getSelectionModel(), SIGNAL(keyframeSelectionChangedFromModel()),
            _imp->dopeSheetView, SLOT(onKeyframeSelectionChanged()));

    // HierarchyView -> DopeSheetView connections
    connect(_imp->hierarchyView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            _imp->dopeSheetView, SLOT(onHierarchyViewScrollbarMoved(int)));

    connect(_imp->hierarchyView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            _imp->dopeSheetView, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)));

    connect(_imp->hierarchyView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
            _imp->dopeSheetView, SLOT(onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem*)));
}

DopeSheetEditor::~DopeSheetEditor()
{}

void DopeSheetEditor::addNode(boost::shared_ptr<NodeGui> nodeGui)
{
    _imp->model->addNode(nodeGui);
}

void DopeSheetEditor::removeNode(NodeGui *node)
{
    _imp->model->removeNode(node);
}

void DopeSheetEditor::centerOn(double xMin, double xMax)
{
    _imp->dopeSheetView->centerOn(xMin, xMax);
}

void DopeSheetEditor::toggleTripleSync(bool enabled)
{
    Natron::PixmapEnum tripleSyncPixmapValue = (enabled)
            ? Natron::NATRON_PIXMAP_LOCKED
            : Natron::NATRON_PIXMAP_UNLOCKED;

    QPixmap tripleSyncBtnPix;
    appPTR->getIcon(tripleSyncPixmapValue, &tripleSyncBtnPix);

    _imp->toggleTripleSyncBtn->setIcon(QIcon(tripleSyncBtnPix));
    _imp->toggleTripleSyncBtn->setDown(enabled);
    _imp->gui->setTripleSyncEnabled(enabled);
}
