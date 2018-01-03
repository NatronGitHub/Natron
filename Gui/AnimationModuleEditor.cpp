/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "AnimationModuleEditor.h"

#include <stdexcept>

#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QApplication>
#include <QKeyEvent>

#include "Serialization/WorkspaceSerialization.h"


#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/Button.h"
#include "Gui/CurveGui.h"
#include "Gui/ComboBox.h"
#include "Gui/Menu.h"
#include "Gui/NodeAnim.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Label.h"
#include "Gui/KnobAnim.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeGui.h"
#include "Gui/Splitter.h"
#include "Gui/GuiMacros.h"
#include "Gui/SpinBox.h"

#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobUndoCommand.h"
#include "Engine/Utils.h"
NATRON_NAMESPACE_ENTER


enum KeyFrameInterpolationChoiceMenuEnum
{
    eKeyFrameInterpolationChoiceMenuConstant,
    eKeyFrameInterpolationChoiceMenuLinear,
    eKeyFrameInterpolationChoiceMenuSmooth,
    eKeyFrameInterpolationChoiceMenuHoriz,
    eKeyFrameInterpolationChoiceMenuCubic,
    eKeyFrameInterpolationChoiceMenuCatRom,
    eKeyFrameInterpolationChoiceMenuBroken,
    eKeyFrameInterpolationChoiceMenuFree

};

inline KeyframeTypeEnum toKeyFrameType(KeyFrameInterpolationChoiceMenuEnum v)
{
    switch (v) {
        case eKeyFrameInterpolationChoiceMenuBroken:
            return eKeyframeTypeBroken;
        case eKeyFrameInterpolationChoiceMenuCatRom:
            return eKeyframeTypeCatmullRom;
        case eKeyFrameInterpolationChoiceMenuConstant:
            return eKeyframeTypeConstant;
        case eKeyFrameInterpolationChoiceMenuCubic:
            return eKeyframeTypeCubic;
        case eKeyFrameInterpolationChoiceMenuFree:
            return eKeyframeTypeFree;
        case eKeyFrameInterpolationChoiceMenuHoriz:
            return eKeyframeTypeHorizontal;
        case eKeyFrameInterpolationChoiceMenuLinear:
            return eKeyframeTypeLinear;
        case eKeyFrameInterpolationChoiceMenuSmooth:
        default:
            return eKeyframeTypeSmooth;
    }
}

inline KeyFrameInterpolationChoiceMenuEnum fromKeyFrameType(KeyframeTypeEnum v)
{
    switch (v) {
        case eKeyframeTypeBroken:
            return eKeyFrameInterpolationChoiceMenuBroken;
        case eKeyframeTypeCatmullRom:
            return eKeyFrameInterpolationChoiceMenuCatRom;
        case eKeyframeTypeConstant:
            return eKeyFrameInterpolationChoiceMenuConstant;
        case eKeyframeTypeCubic:
            return eKeyFrameInterpolationChoiceMenuCubic;
        case eKeyframeTypeFree:
            return eKeyFrameInterpolationChoiceMenuFree;
        case eKeyframeTypeHorizontal:
            return eKeyFrameInterpolationChoiceMenuHoriz;
        case eKeyframeTypeLinear:
            return eKeyFrameInterpolationChoiceMenuLinear;
        case eKeyframeTypeSmooth:
            return eKeyFrameInterpolationChoiceMenuSmooth;
        default:
            return eKeyFrameInterpolationChoiceMenuLinear;
    }
}

class AnimationModuleEditorPrivate
{
public:
    AnimationModuleEditorPrivate(AnimationModuleEditor *qq);

    /* attributes */
    AnimationModuleEditor *publicInterface;
    QVBoxLayout *mainLayout;


    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    Button* displayViewChoice;
    Button* showOnlyAnimatedButton;

    ComboBox* expressionLanguageChoice;
    Label* knobLabel;
    LineEdit* knobExpressionLineEdit;
    Label* expressionResultLabel;

    Label* keyframeLabel;
    Label* keyframeLeftSlopeLabel;
    SpinBox* keyframeLeftSlopeSpinBox;
    Label* keyframeTimeLabel;
    SpinBox* keyframeTimeSpinBox;
    Label* keyframeValueLabel;
    SpinBox* keyframeValueSpinBox;
    LineEdit* keyframeValueLineEdit;
    Label* keyframeRightSlopeLabel;
    SpinBox* keyframeRightSlopeSpinBox;
    ComboBox* keyframeInterpolationChoice;

    AnimItemDimViewKeyFrame selectedKeyFrame;

    Splitter *treeAndViewSplitter;


    AnimationModuleTreeView *treeView;

    AnimationModuleView* view;

    AnimationModulePtr model;

    int nRefreshExpressionResultRequests;

    QTreeWidgetItem* rightClickedItem;

    // If there's a single selected keyframe, get it,otherwise return false;
    bool getKeyframe(AnimItemDimViewIndexID* curve, KeyFrame* key) const;

    void refreshExpressionResult();

};

AnimationModuleEditorPrivate::AnimationModuleEditorPrivate(AnimationModuleEditor *publicInterface)
: publicInterface(publicInterface)
, mainLayout(0)
, buttonsContainer(0)
, buttonsLayout(0)
, displayViewChoice(0)
, showOnlyAnimatedButton(0)
, expressionLanguageChoice(0)
, knobLabel(0)
, knobExpressionLineEdit(0)
, expressionResultLabel(0)
, keyframeLabel(0)
, keyframeLeftSlopeLabel(0)
, keyframeLeftSlopeSpinBox(0)
, keyframeTimeLabel(0)
, keyframeTimeSpinBox(0)
, keyframeValueLabel(0)
, keyframeValueSpinBox(0)
, keyframeValueLineEdit(0)
, keyframeRightSlopeLabel(0)
, keyframeRightSlopeSpinBox(0)
, keyframeInterpolationChoice(0)
, selectedKeyFrame()
, treeAndViewSplitter(0)
, treeView(0)
, view(0)
, model()
, nRefreshExpressionResultRequests(0)
, rightClickedItem(0)
{
}

/**
 * @brief AnimationModuleEditor::AnimationModuleEditor
 *
 * Creates a AnimationModuleEditor.
 */
AnimationModuleEditor::AnimationModuleEditor(const std::string& scriptName,
                                 Gui *gui,
                                 const TimeLinePtr& timeline,
                                 QWidget *parent)
    : QWidget(parent),
    PanelWidget(scriptName, this, gui),
    _imp( new AnimationModuleEditorPrivate(this) )
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsLayout->setContentsMargins(0, 0, 0, 0);
    _imp->buttonsLayout->setSpacing(0);

    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);

    QPixmap pixCurveEditor, pixStacked, pixAutoHideOn, pixAutohideOff;
    appPTR->getIcon(NATRON_PIXMAP_CURVE_EDITOR, iconSize, &pixCurveEditor);
    appPTR->getIcon(NATRON_PIXMAP_ANIMATION_MODULE, iconSize, &pixStacked);
    appPTR->getIcon(NATRON_PIXMAP_UNHIDE_UNMODIFIED, iconSize, &pixAutoHideOn);
    appPTR->getIcon(NATRON_PIXMAP_HIDE_UNMODIFIED, iconSize, &pixAutohideOff);

    QSize medSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    QSize medIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

    {
        _imp->displayViewChoice = new Button(_imp->buttonsContainer);
        QIcon ic;
        ic.addPixmap(pixStacked, QIcon::Normal, QIcon::On);
        ic.addPixmap(pixCurveEditor, QIcon::Normal, QIcon::Off);
        _imp->displayViewChoice->setIcon(ic);
        _imp->displayViewChoice->setCheckable(true);
        _imp->displayViewChoice->setChecked(true);
        _imp->displayViewChoice->setDown(true);
        _imp->displayViewChoice->setFixedSize(medSize);
        _imp->displayViewChoice->setIconSize(medIconSize);
        _imp->displayViewChoice->setFocusPolicy(Qt::NoFocus);
        setToolTipWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleStackView, "<p>" + tr("Switch between Dope Sheet + CurveEditor and Curve Editor only").toStdString() + "</p>" + "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->displayViewChoice);
        connect(_imp->displayViewChoice, SIGNAL(clicked(bool)), this, SLOT(onDisplayViewClicked(bool)));
        _imp->buttonsLayout->addWidget(_imp->displayViewChoice);
    }
    {
        _imp->showOnlyAnimatedButton = new Button(_imp->buttonsContainer);
        QIcon ic;
        ic.addPixmap(pixAutoHideOn, QIcon::Normal, QIcon::On);
        ic.addPixmap(pixAutohideOff, QIcon::Normal, QIcon::Off);
        _imp->showOnlyAnimatedButton->setIcon(ic);
        _imp->showOnlyAnimatedButton->setCheckable(true);
        _imp->showOnlyAnimatedButton->setChecked(true);
        _imp->showOnlyAnimatedButton->setDown(false);
        _imp->showOnlyAnimatedButton->setFixedSize(medSize);
        _imp->showOnlyAnimatedButton->setIconSize(medIconSize);
        _imp->showOnlyAnimatedButton->setFocusPolicy(Qt::NoFocus);
        setToolTipWithShortcut(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleShowOnlyAnimated, "<p>" + tr("When checked, only animated items will appear in the animation module").toStdString() + "</p>" + "<p><b>" + tr("Keyboard shortcut: %1").toStdString() + "</b></p>", _imp->showOnlyAnimatedButton);
        connect(_imp->showOnlyAnimatedButton, SIGNAL(clicked(bool)), this, SLOT(onShowOnlyAnimatedButtonClicked(bool)));
        _imp->buttonsLayout->addWidget(_imp->showOnlyAnimatedButton);
    }

    _imp->buttonsLayout->addSpacing(TO_DPIX(10));

    _imp->keyframeLabel = new Label(_imp->buttonsContainer);
    _imp->keyframeLabel->setIsModified(false);
    _imp->keyframeLabel->setText(tr("Keyframe:"));

    _imp->keyframeLeftSlopeLabel = new Label(_imp->buttonsContainer);
    _imp->keyframeLeftSlopeLabel->setIsModified(false);
    QString leftSlopeTT = convertFromPlainText(tr("Left tangent of the currently selected keyframe"), WhiteSpaceNormal);
    _imp->keyframeLeftSlopeLabel->setToolTip(leftSlopeTT);
    _imp->keyframeLeftSlopeLabel->setText(tr("L"));
    QColor slopeColor;
    slopeColor.setRgbF(1., 0.35, 0.35);
    _imp->keyframeLeftSlopeLabel->setCustomTextColor(slopeColor);

    _imp->keyframeLeftSlopeSpinBox = new SpinBox(_imp->buttonsContainer, SpinBox::eSpinBoxTypeDouble);
    _imp->keyframeLeftSlopeSpinBox->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredUnderlinedText, true, slopeColor);
    _imp->keyframeLeftSlopeSpinBox->setToolTip(leftSlopeTT);
    QObject::connect(_imp->keyframeLeftSlopeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onLeftSlopeSpinBoxValueChanged(double)));

    _imp->keyframeTimeLabel = new Label(_imp->buttonsContainer);
    _imp->keyframeTimeLabel->setIsModified(false);
    QString timeTT = convertFromPlainText(tr("Frame number of the currently selected keyframe"), WhiteSpaceNormal);
    _imp->keyframeTimeLabel->setText(tr("Frame"));
    _imp->keyframeTimeLabel->setToolTip(timeTT);

    _imp->keyframeTimeSpinBox = new SpinBox(_imp->buttonsContainer, SpinBox::eSpinBoxTypeInt);
    _imp->keyframeTimeSpinBox->setToolTip(timeTT);
    QObject::connect(_imp->keyframeTimeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onKeyframeTimeSpinBoxValueChanged(double)));

    QString valueTT = convertFromPlainText(tr("Value of the currently selected keyframe"), WhiteSpaceNormal);
    _imp->keyframeValueLabel = new Label(_imp->buttonsContainer);
    _imp->keyframeValueLabel->setToolTip(valueTT);
    _imp->keyframeValueLabel->setIsModified(false);
    _imp->keyframeValueLabel->setText(tr("Value"));

    _imp->keyframeValueSpinBox = new SpinBox(_imp->buttonsContainer);
    _imp->keyframeValueSpinBox->setToolTip(valueTT);
    QObject::connect(_imp->keyframeValueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onKeyframeValueSpinBoxValueChanged(double)));

    _imp->keyframeValueLineEdit = new LineEdit(_imp->buttonsContainer);
    _imp->keyframeValueLineEdit->setText(tr("Enter Text..."));
    _imp->keyframeValueLineEdit->setToolTip(valueTT);
    QObject::connect(_imp->keyframeValueLineEdit, SIGNAL(editingFinished()), this, SLOT(onKeyframeValueLineEditEditFinished()));

    _imp->keyframeRightSlopeLabel = new Label(_imp->buttonsContainer);
    _imp->keyframeRightSlopeLabel->setIsModified(false);
    _imp->keyframeRightSlopeLabel->setText(tr("R"));
    QString rightSlopeTT = convertFromPlainText(tr("Right slope of the currently selected keyframe"), WhiteSpaceNormal);
    _imp->keyframeRightSlopeLabel->setToolTip(rightSlopeTT);
    _imp->keyframeRightSlopeLabel->setCustomTextColor(slopeColor);

    _imp->keyframeRightSlopeSpinBox = new SpinBox(_imp->buttonsContainer, SpinBox::eSpinBoxTypeDouble);
    _imp->keyframeRightSlopeSpinBox->setToolTip(rightSlopeTT);
    _imp->keyframeRightSlopeSpinBox->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredUnderlinedText, true, slopeColor);
    QObject::connect(_imp->keyframeRightSlopeSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onRightSlopeSpinBoxValueChanged(double)));

    QString interpTT = convertFromPlainText(tr("Interpolation of the curve between the currently selected keyframe and the next keyframe"), WhiteSpaceNormal);
    _imp->keyframeInterpolationChoice = new ComboBox(_imp->buttonsContainer);
    _imp->keyframeInterpolationChoice->setToolTip(interpTT);
    _imp->keyframeInterpolationChoice->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _imp->keyframeInterpolationChoice->setFixedWidth(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE) * 3);
    QObject::connect(_imp->keyframeInterpolationChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(onKeyfameInterpolationChoiceMenuChanged(int)));

    {
        int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
        QPixmap pixConstant, pixLinear, pixSmooth, pixHorizontal, pixCubic, pixCatmullRom, pixBroken, pixFree;
        appPTR->getIcon(NATRON_PIXMAP_INTERP_CONSTANT, iconSize, &pixConstant);
        appPTR->getIcon(NATRON_PIXMAP_INTERP_LINEAR, iconSize, &pixLinear);
        appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_Z, iconSize, &pixSmooth);
        appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_H, iconSize, &pixHorizontal);
        appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_C, iconSize, &pixCubic);
        appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE_R, iconSize, &pixCatmullRom);
        appPTR->getIcon(NATRON_PIXMAP_INTERP_BREAK, iconSize, &pixBroken);
        appPTR->getIcon(NATRON_PIXMAP_INTERP_CURVE, iconSize, &pixFree);

        {
            ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupAnimationModule,
                                                                kShortcutActionAnimationModuleConstant,
                                                                kShortcutActionAnimationModuleConstantLabel,
                                                                _imp->keyframeInterpolationChoice);
            action->setIcon(QIcon(pixConstant));
            _imp->keyframeInterpolationChoice->addAction(action);
        }
        {
            ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupAnimationModule,
                                                                kShortcutActionAnimationModuleLinear,
                                                                kShortcutActionAnimationModuleLinearLabel,
                                                                _imp->keyframeInterpolationChoice);
            action->setIcon(QIcon(pixLinear));
            _imp->keyframeInterpolationChoice->addAction(action);
        }
        {
            ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupAnimationModule,
                                                                kShortcutActionAnimationModuleSmooth,
                                                                kShortcutActionAnimationModuleSmoothLabel,
                                                                _imp->keyframeInterpolationChoice);
            action->setIcon(QIcon(pixSmooth));
            _imp->keyframeInterpolationChoice->addAction(action);
        }
        {
            ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupAnimationModule,
                                                                kShortcutActionAnimationModuleHorizontal,
                                                                kShortcutActionAnimationModuleHorizontalLabel,
                                                                _imp->keyframeInterpolationChoice);
            action->setIcon(QIcon(pixHorizontal));
            _imp->keyframeInterpolationChoice->addAction(action);
        }
        {
            ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupAnimationModule,
                                                                kShortcutActionAnimationModuleCubic,
                                                                kShortcutActionAnimationModuleCubicLabel,
                                                                _imp->keyframeInterpolationChoice);
            action->setIcon(QIcon(pixCubic));
            _imp->keyframeInterpolationChoice->addAction(action);
        }
        {
            ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupAnimationModule,
                                                                kShortcutActionAnimationModuleCatmullrom,
                                                                kShortcutActionAnimationModuleCatmullromLabel,
                                                                _imp->keyframeInterpolationChoice);
            action->setIcon(QIcon(pixCatmullRom));
            _imp->keyframeInterpolationChoice->addAction(action);
        }
        {
            ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupAnimationModule,
                                                                kShortcutActionAnimationModuleBreak,
                                                                kShortcutActionAnimationModuleBreakLabel,
                                                                _imp->keyframeInterpolationChoice);
            action->setIcon(QIcon(pixBroken));
            _imp->keyframeInterpolationChoice->addAction(action);
        }
        {
            QAction* action = new QAction(_imp->keyframeInterpolationChoice);
            action->setText(tr("Custom Tangents"));
            action->setIcon(QIcon(pixFree));
            _imp->keyframeInterpolationChoice->addAction(action);
        }



    }

    _imp->expressionLanguageChoice = new ComboBox(_imp->buttonsContainer);
    _imp->expressionLanguageChoice->addItem(tr("ExprTk"));
    _imp->expressionLanguageChoice->addItem(tr("Python"));
    QString langTooltip = NATRON_NAMESPACE::convertFromPlainText(tr("Select the language used by this expression. ExprTk-based expressions are very simple and extremely fast expressions but a bit more constrained than "
                                                                    "Python-based expressions which allow all the flexibility of the Python A.P.I to the expense of being a lot more expensive to evaluate."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->expressionLanguageChoice->setToolTip(langTooltip);
    QObject::connect( _imp->expressionLanguageChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(onExprLanguageCurrentIndexChanged(int)) );

    _imp->knobLabel = new Label(_imp->buttonsContainer);
    _imp->knobLabel->setIsModified(false);
    _imp->knobLabel->setText( tr("No curve selected") );
    _imp->knobExpressionLineEdit = new LineEdit(_imp->buttonsContainer);
    _imp->knobExpressionLineEdit->setPlaceholderText(tr("curve (No Expression)"));
    QObject::connect( _imp->knobExpressionLineEdit, SIGNAL(editingFinished()), this, SLOT(onExprLineEditFinished()) );
    _imp->expressionResultLabel = new Label(_imp->buttonsContainer);
    _imp->expressionResultLabel->setText( QString::fromUtf8("= ") );

    _imp->buttonsLayout->addWidget(_imp->keyframeLabel);
    _imp->buttonsLayout->addSpacing(TO_DPIX(5));
    _imp->buttonsLayout->addWidget(_imp->keyframeLeftSlopeLabel);
    _imp->buttonsLayout->addSpacing(TO_DPIX(1));
    _imp->buttonsLayout->addWidget(_imp->keyframeLeftSlopeSpinBox);
    _imp->buttonsLayout->addSpacing(TO_DPIX(2));
    _imp->buttonsLayout->addWidget(_imp->keyframeTimeLabel);
    _imp->buttonsLayout->addSpacing(TO_DPIX(1));
    _imp->buttonsLayout->addWidget(_imp->keyframeTimeSpinBox);
    _imp->buttonsLayout->addSpacing(TO_DPIX(2));
    _imp->buttonsLayout->addWidget(_imp->keyframeValueLabel);
    _imp->buttonsLayout->addSpacing(TO_DPIX(1));
    _imp->buttonsLayout->addWidget(_imp->keyframeValueSpinBox);
    _imp->buttonsLayout->addWidget(_imp->keyframeValueLineEdit);
    _imp->buttonsLayout->addSpacing(TO_DPIX(2));
    _imp->buttonsLayout->addWidget(_imp->keyframeRightSlopeLabel);
    _imp->buttonsLayout->addSpacing(TO_DPIX(1));
    _imp->buttonsLayout->addWidget(_imp->keyframeRightSlopeSpinBox);
    _imp->buttonsLayout->addSpacing(TO_DPIX(2));
    _imp->buttonsLayout->addWidget(_imp->keyframeInterpolationChoice);

    _imp->buttonsLayout->addSpacing(TO_DPIX(5));

    _imp->buttonsLayout->addWidget(_imp->expressionLanguageChoice);
    _imp->buttonsLayout->addWidget(_imp->knobLabel);
    _imp->buttonsLayout->addSpacing(TO_DPIX(5));
    _imp->buttonsLayout->addWidget(_imp->knobExpressionLineEdit);
    _imp->buttonsLayout->addSpacing(TO_DPIX(5));
    _imp->buttonsLayout->addWidget(_imp->expressionResultLabel);
    _imp->buttonsLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->buttonsContainer);

    _imp->treeAndViewSplitter = new Splitter(Qt::Horizontal, gui, this);

    _imp->model = AnimationModule::create(gui, this, timeline);
    _imp->treeView = new AnimationModuleTreeView(_imp->model, gui, _imp->treeAndViewSplitter);
    //_imp->treeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    _imp->treeView->setFocusPolicy(Qt::NoFocus);
    _imp->treeAndViewSplitter->addWidget(_imp->treeView);

    _imp->view = new AnimationModuleView(_imp->treeAndViewSplitter);
    _imp->view->initialize(gui, _imp->model);

    _imp->treeAndViewSplitter->addWidget(_imp->view);


    {
        QList<int> sizes;
        sizes.push_back(300);
        sizes.push_back(700);
        _imp->treeAndViewSplitter->setSizes_mt_safe(sizes);
        //_imp->treeAndViewSplitter->setStretchFactor(0, 2);
    }
    _imp->mainLayout->addWidget(_imp->treeAndViewSplitter);

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)), this, SLOT(onSelectionModelSelectionChanged(bool)) );

    connect( _imp->model->getSelectionModel().get(), SIGNAL(selectionChanged(bool)), _imp->treeView, SLOT(onSelectionModelKeyframeSelectionChanged(bool)) );

    connect( _imp->model.get(), SIGNAL(nodeAdded(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAdded(NodeAnimPtr)) );

    connect( _imp->model.get(), SIGNAL(nodeAboutToBeRemoved(NodeAnimPtr)),
             _imp->treeView, SLOT(onNodeAboutToBeRemoved(NodeAnimPtr)) );

    connect( _imp->treeView->verticalScrollBar(), SIGNAL(valueChanged(int)),
             _imp->view, SLOT(onAnimationTreeViewScrollbarMoved(int)) );

    connect( _imp->treeView, SIGNAL(itemExpanded(QTreeWidgetItem*)),
             _imp->view, SLOT(onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem*)) );

    connect( _imp->treeView, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
             _imp->view, SLOT(onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem*)) );

    connect( _imp->treeView, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onTreeItemClicked(QTreeWidgetItem*,int)));

    connect( _imp->treeView, SIGNAL(itemRightClicked(QPoint,QTreeWidgetItem*)), this, SLOT(onTreeItemRightClicked(QPoint,QTreeWidgetItem*)));
    
    onSelectionModelSelectionChanged(false);
    refreshKeyFrameWidgetsEnabledNess();

    QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimelineTimeChanged(SequenceTime,int)) );


    QObject::connect( this, SIGNAL(mustRefreshExpressionResultsLater()), this, SLOT(onMustRefreshExpressionResultsLaterReceived()), Qt::QueuedConnection );
}

AnimationModuleEditor::~AnimationModuleEditor()
{
}

AnimationModuleEditor::AnimationModuleDisplayViewMode
AnimationModuleEditor::getDisplayMode() const
{
    return _imp->displayViewChoice->isChecked() ? eAnimationModuleDisplayViewModeStacked : eAnimationModuleDisplayViewModeCurveEditor;
}

bool
AnimationModuleEditor::isOnlyAnimatedItemsVisibleButtonChecked() const
{
    return _imp->showOnlyAnimatedButton->isChecked();
}

void
AnimationModuleEditor::addNode(const NodeGuiPtr& nodeGui)
{
    _imp->model->addNode(nodeGui);
}

void
AnimationModuleEditor::removeNode(const NodeGuiPtr& node)
{
    _imp->model->removeNode(node);
}

void
AnimationModuleEditor::centerOn(double xMin,
                                double xMax)
{
    _imp->view->centerOn(xMin, xMax);
}

void
AnimationModuleEditor::refreshSelectionBboxAndRedrawView()
{
    _imp->view->refreshSelectionBboxAndRedraw();
}

TimeValue
AnimationModuleEditor::getTimelineCurrentTime() const
{
    return TimeValue(getGui()->getApp()->getTimeLine()->currentFrame());
}

AnimationModulePtr
AnimationModuleEditor::getModel() const
{
    return _imp->model;
}

AnimationModuleView*
AnimationModuleEditor::getView() const
{
    return _imp->view;
}

AnimationModuleTreeView*
AnimationModuleEditor::getTreeView() const
{
    return _imp->treeView;
}


void
AnimationModuleEditor::keyPressEvent(QKeyEvent* e)
{
    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    bool accept = true;

    if ( isKeybind(kShortcutGroupNodegraph, kShortcutActionGraphRenameNode, modifiers, key) ) {
        _imp->model->renameSelectedNode();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleStackView, modifiers, key) ) {
        bool checked = _imp->displayViewChoice->isChecked();
        _imp->displayViewChoice->setChecked(!checked);
        onDisplayViewClicked(!checked);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleRemoveKeys, modifiers, key) ) {
        onRemoveSelectedKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleConstant, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeConstant);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleLinear, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeLinear);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSmooth, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeSmooth);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCatmullrom, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeCatmullRom);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCubic, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeCubic);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleHorizontal, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeHorizontal);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleBreak, modifiers, key) ) {
        _imp->model->setSelectedKeysInterpolation(eKeyframeTypeBroken);
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenterAll, modifiers, key) ) {
        onCenterAllCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCenter, modifiers, key) ) {
        onCenterOnSelectedCurvesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleSelectAll, modifiers, key) ) {
        onSelectAllKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModuleCopy, modifiers, key) ) {
        onCopySelectedKeyFramesToClipBoardActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModulePasteKeyframes, modifiers, key) ) {
        onPasteClipBoardKeyFramesActionTriggered();
    } else if ( isKeybind(kShortcutGroupAnimationModule, kShortcutActionAnimationModulePasteKeyframesAbsolute, modifiers, key) ) {
        onPasteClipBoardKeyFramesAbsoluteActionTriggered();
    } else if ( key == Qt::Key_Plus ) { // zoom in/out doesn't care about modifiers
        // one wheel click = +-120 delta
        zoomDisplayedView(120);
    } else if ( key == Qt::Key_Minus ) { // zoom in/out doesn't care about modifiers
        // one wheel click = +-120 delta
        zoomDisplayedView(-120);
    } else {
        accept = false;
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
        QWidget::keyPressEvent(e);
    }
}

void
AnimationModuleEditor::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyUpEvent(e);
    QWidget::keyReleaseEvent(e);
}

void
AnimationModuleEditor::enterEvent(QEvent *e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
AnimationModuleEditor::leaveEvent(QEvent *e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
AnimationModuleEditor::onInputEventCalled()
{
    takeClickFocus();
}

boost::shared_ptr<QUndoStack>
AnimationModuleEditor::getUndoStack() const
{
    return _imp->model ? _imp->model->getUndoStack() : boost::shared_ptr<QUndoStack>();
}

bool
AnimationModuleEditor::saveProjection(SERIALIZATION_NAMESPACE::ViewportData* /*data*/)
{

    /*if (!_imp->view->hasDrawnOnce()) {
        return false;
    }
    _imp->view->getProjection(&data->left, &data->bottom, &data->zoomFactor, &data->par);
*/
    return false;
}

bool
AnimationModuleEditor::loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& /*data*/)
{
    //_imp->view->setProjection(data.left, data.bottom, data.zoomFactor, data.par);
    return false;
}


void
AnimationModuleEditor::setSelectedCurveExpression(const QString& expression, ExpressionLanguageEnum lang)
{



    std::vector<CurveGuiPtr> curves = _imp->treeView->getSelectedCurves();

    if (curves.empty() || curves.size() > 1) {
        throw std::invalid_argument(tr("Cannot set an expression on multiple items").toStdString());
    }
    KnobAnimPtr isKnobAnim = toKnobAnim(curves.front()->getItem());
    if (!isKnobAnim) {
        throw std::invalid_argument(tr("Cannot set an expression on something else than a knob").toStdString());
    }
    KnobIPtr knob = isKnobAnim->getInternalKnob();
    const CurveGuiPtr& curve = curves.front();
    DimIdx dimension = curve->getDimension();
    ViewIdx view = curve->getView();
    std::string exprResult;
    std::string curExpr = knob->getExpression(dimension, view);
    if (curExpr == expression.toStdString()) {
        return;
    }
    if ( !expression.isEmpty() ) {
        try {
            knob->validateExpression(expression.toStdString(), lang, dimension, view, false /*hasRetVariable*/, &exprResult);
        } catch (...) {
            _imp->expressionResultLabel->setText( tr("Error") );
            return;
        }
    }
    pushUndoCommand( new SetExpressionCommand(knob, lang, false /*hasRetVariable*/, dimension, view, expression.toStdString()) );
}

void
AnimationModuleEditor::onExprLanguageCurrentIndexChanged(int /*index*/)
{
    onExprLineEditFinished();
}

void
AnimationModuleEditor::onExprLineEditFinished()
{

    try {
        ExpressionLanguageEnum language = _imp->expressionLanguageChoice->activeIndex() == 0 ? eExpressionLanguageExprTk : eExpressionLanguagePython;
        setSelectedCurveExpression( _imp->knobExpressionLineEdit->text(), language );
    } catch(const std::exception& /*e*/) {
        //Dialogs::errorDialog(tr("Animation Module").toStdString(), e.what());
    }
}

void
AnimationModuleEditor::onShowOnlyAnimatedButtonClicked(bool clicked)
{
    _imp->showOnlyAnimatedButton->setDown(clicked);
    std::vector<NodeAnimPtr> nodes;
    _imp->model->getTopLevelNodes(false /*onlyVisible*/, &nodes);
    for (std::vector<NodeAnimPtr>::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        (*it)->refreshVisibility();
    }
    _imp->view->update();
}

void
AnimationModuleEditor::onDisplayViewClicked(bool clicked)
{
    _imp->displayViewChoice->setDown(clicked);
    _imp->view->update();
}

void
AnimationModuleEditor::onSelectionModelSelectionChanged(bool /*recurse*/)
{
    std::vector<CurveGuiPtr> curves = _imp->treeView->getSelectedCurves();
    bool expressionFieldsEnabled = true;
    if (curves.size() > 1 || curves.size() == 0) {
        expressionFieldsEnabled = false;
    }
    QString knobLabel, currentExpression;
    ExpressionLanguageEnum language = eExpressionLanguagePython;
    if (expressionFieldsEnabled) {
        CurveGuiPtr curve = curves.front();
        KnobAnimPtr knob = toKnobAnim(curve->getItem());
        if (!knob) {
            expressionFieldsEnabled = false;
        } else {
            knobLabel = knob->getViewDimensionLabel(curve->getDimension(), curve->getView());
            NamedKnobHolderPtr isHolderNamed = boost::dynamic_pointer_cast<NamedKnobHolder>(knob->getInternalKnob()->getHolder());
            if (isHolderNamed) {
                QString holderName = QString::fromUtf8(isHolderNamed->getScriptName_mt_safe().c_str());
                holderName += QLatin1Char('.');
                knobLabel.prepend(holderName);
            }
            KnobIPtr internalKnob = knob->getInternalKnob();

            currentExpression = QString::fromUtf8(internalKnob->getExpression(curve->getDimension(), curve->getView()).c_str());
            language = internalKnob->getExpressionLanguage(curve->getView(), curve->getDimension());
        }

    }
    _imp->knobLabel->setVisible(expressionFieldsEnabled);
    if (expressionFieldsEnabled) {
        switch (language) {
            case eExpressionLanguagePython:
                _imp->expressionLanguageChoice->setCurrentIndex(1, false);
                break;
            case eExpressionLanguageExprTk:
                _imp->expressionLanguageChoice->setCurrentIndex(0, false);
                break;

        }
        _imp->knobLabel->setText(tr("%1:").arg(knobLabel));
        _imp->knobExpressionLineEdit->setText(currentExpression);
    }

    _imp->expressionLanguageChoice->setVisible(expressionFieldsEnabled);
    _imp->knobExpressionLineEdit->setVisible(expressionFieldsEnabled);
    _imp->expressionResultLabel->setVisible(expressionFieldsEnabled);

    _imp->refreshExpressionResult();

    // Also refresh keyframe widgets
    refreshKeyframeWidgetsFromSelection();
} // onSelectionModelSelectionChanged

void
AnimationModuleEditorPrivate::refreshExpressionResult()
{
    if (publicInterface->getGui()->isGUIFrozen()) {
        return;
    }
    if (!expressionResultLabel->isVisible()) {
        return;
    }
    std::vector<CurveGuiPtr> curves = treeView->getSelectedCurves();
    if (curves.size() > 1 || curves.size() == 0) {
        return;
    }
    AnimItemDimViewIndexID id = curves.front()->getCurveID();
    KnobAnimPtr knob = toKnobAnim(id.item);
    if (!knob) {
        return;
    }
    KnobIPtr internalKnob = knob->getInternalKnob();
    if (!internalKnob) {
        return;
    }

    bool expressionIsError = false;

    std::string exprError;
    internalKnob->isLinkValid(id.dim, id.view, &exprError);
    if (!exprError.empty()) {
        expressionIsError = true;
    }

    if (expressionIsError) {
        expressionResultLabel->setText(AnimationModuleEditor::tr(" = Invalid Expression"));
    } else {
        QString internalKnobValueStr;
        KnobStringBasePtr isStringKnob = toKnobStringBase(internalKnob);
        KnobBoolBasePtr isBoolKnob = toKnobBool(internalKnob);
        KnobIntBasePtr isIntKnob = toKnobInt(internalKnob);
        KnobDoubleBasePtr isDoubleKnob = toKnobDouble(internalKnob);
        if (isStringKnob) {
            internalKnobValueStr = QString::fromUtf8(isStringKnob->getValue(id.dim, id.view).c_str());
        } else if (isBoolKnob) {
            bool val = isBoolKnob->getValue(id.dim, id.view);
            internalKnobValueStr = val ? AnimationModuleEditor::tr("True") : AnimationModuleEditor::tr("False");
        } else if (isIntKnob) {
            internalKnobValueStr = QString::number(isIntKnob->getValue(id.dim, id.view));
        } else if (isDoubleKnob) {
            internalKnobValueStr = QString::number(isDoubleKnob->getValue(id.dim, id.view), 'f', 2);
        }
        expressionResultLabel->setText(QString::fromUtf8("= ") + internalKnobValueStr);

    }
} // refreshExpressionResult

void
AnimationModuleEditor::onMustRefreshExpressionResultsLaterReceived()
{
    if (!_imp->nRefreshExpressionResultRequests) {
        return;
    }
    _imp->nRefreshExpressionResultRequests = 0;
    _imp->refreshExpressionResult();
}

void
AnimationModuleEditor::onTimelineTimeChanged(SequenceTime /*time*/, int /*reason*/)
{
    ++_imp->nRefreshExpressionResultRequests;
    Q_EMIT mustRefreshExpressionResultsLater();
}


void
AnimationModuleEditor::onRemoveSelectedKeyFramesActionTriggered()
{
    _imp->model->deleteSelectedKeyframes();
}

void
AnimationModuleEditor::onCopySelectedKeyFramesToClipBoardActionTriggered()
{
    _imp->model->copySelectedKeys();
}

void
AnimationModuleEditor::onPasteClipBoardKeyFramesActionTriggered()
{
    _imp->model->pasteKeys(_imp->model->getKeyFramesClipBoard(), true /*relative*/);
}

void
AnimationModuleEditor::onPasteClipBoardKeyFramesAbsoluteActionTriggered()
{
    _imp->model->pasteKeys(_imp->model->getKeyFramesClipBoard(), false /*relative*/);
}

void
AnimationModuleEditor::onSelectAllKeyFramesActionTriggered()
{

    // select keyframes of visible curves only
    _imp->model->getSelectionModel()->selectAllVisibleCurvesKeyFrames();


}

void
AnimationModuleEditor::onSetInterpolationActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    KeyframeTypeEnum type = (KeyframeTypeEnum)action->data().toInt();
    _imp->model->setSelectedKeysInterpolation(type);
}

void
AnimationModuleEditor::onCenterAllCurvesActionTriggered()
{
    _imp->view->centerOnAllItems();
}

void
AnimationModuleEditor::onCenterOnSelectedCurvesActionTriggered()
{
    _imp->view->centerOnSelection();
}

void
AnimationModuleEditor::zoomDisplayedView(int delta)
{
    QWheelEvent e(mapFromGlobal( QCursor::pos() ), delta, Qt::NoButton, Qt::NoModifier); // one wheel click = +-120 delta
    _imp->view->wheelEvent(&e);

}

QIcon
AnimationModuleEditor::getIcon() const
{
    int iconSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap p;
    appPTR->getIcon(NATRON_PIXMAP_ANIMATION_MODULE, iconSize, &p);
    return QIcon(p);
}

bool
AnimationModuleEditorPrivate::getKeyframe(AnimItemDimViewIndexID* curve, KeyFrame* key) const
{
    const AnimItemDimViewKeyFramesMap& selectedCurves = model->getSelectionModel()->getCurrentKeyFramesSelection();
    if (selectedCurves.size() > 1 || selectedCurves.empty()) {
        return false;
    }

    AnimItemDimViewKeyFramesMap::const_iterator it = selectedCurves.begin();
    const KeyFrameSet& keys = it->second;
    if (keys.size() == 0 || keys.size() > 1) {
        return false;
    }
    *curve = it->first;
    *key = *keys.begin();
    return true;
}

AnimItemDimViewKeyFrame
AnimationModuleEditor::getCurrentlySelectedKeyFrame() const
{
    return _imp->selectedKeyFrame;
}

void
AnimationModuleEditor::refreshKeyFrameWidgetsEnabledNess()
{
    CurveTypeEnum dataType = eCurveTypeString;
    bool showKeyframeWidgets = bool(_imp->selectedKeyFrame.id.item);
    if (_imp->selectedKeyFrame.id.item) {
        dataType = _imp->selectedKeyFrame.id.item->getInternalAnimItem()->getKeyFrameDataType();
    }
    bool canHaveTangents = dataType == eCurveTypeInt || dataType == eCurveTypeDouble;
    if (dataType == eCurveTypeBool) {
        _imp->keyframeValueSpinBox->setMinimum(0);
        _imp->keyframeValueSpinBox->setMaximum(1);
    } else {
        _imp->keyframeValueSpinBox->setMinimum(-std::numeric_limits<double>::infinity());
        _imp->keyframeValueSpinBox->setMaximum(std::numeric_limits<double>::infinity());
    }

    _imp->keyframeLabel->setEnabled(showKeyframeWidgets);
    _imp->keyframeLeftSlopeLabel->setEnabled(showKeyframeWidgets && canHaveTangents);
    _imp->keyframeLeftSlopeSpinBox->setEnabled(showKeyframeWidgets && canHaveTangents);
    _imp->keyframeTimeLabel->setEnabled(showKeyframeWidgets);
    _imp->keyframeTimeSpinBox->setEnabled(showKeyframeWidgets);
    _imp->keyframeValueLabel->setEnabled(showKeyframeWidgets);
    _imp->keyframeValueLineEdit->setEnabled(showKeyframeWidgets);
    _imp->keyframeValueLineEdit->setVisible(dataType == eCurveTypeString || dataType == eCurveTypeChoice);
    _imp->keyframeValueSpinBox->setEnabled(showKeyframeWidgets);
    _imp->keyframeValueSpinBox->setVisible(dataType == eCurveTypeInt || dataType == eCurveTypeDouble || dataType == eCurveTypeBool);
    _imp->keyframeRightSlopeLabel->setEnabled(showKeyframeWidgets && canHaveTangents);
    _imp->keyframeRightSlopeSpinBox->setEnabled(showKeyframeWidgets && canHaveTangents);
    _imp->keyframeInterpolationChoice->setEnabled(showKeyframeWidgets && canHaveTangents);
}

void
AnimationModuleEditor::refreshKeyframeWidgetsFromSelection()
{
    bool showKeyframeWidgets = true;


    bool selectedKeyChanged = false;
    {
        AnimItemDimViewIndexID selectedKeyID;
        KeyFrame selectedKey;
        showKeyframeWidgets = _imp->getKeyframe(&selectedKeyID, &selectedKey);
        if (selectedKeyID.item != _imp->selectedKeyFrame.id.item ||
            selectedKeyID.view != _imp->selectedKeyFrame.id.view ||
            selectedKeyID.dim != _imp->selectedKeyFrame.id.dim) {
            _imp->selectedKeyFrame.id = selectedKeyID;
            selectedKeyChanged = true;
        }
        _imp->selectedKeyFrame.key = selectedKey;
    }

    CurveTypeEnum dataType = eCurveTypeString;
    if (showKeyframeWidgets) {
        dataType = _imp->selectedKeyFrame.id.item->getInternalAnimItem()->getKeyFrameDataType();
    }


    if (selectedKeyChanged) {
        refreshKeyFrameWidgetsEnabledNess();
    }
    if (showKeyframeWidgets) {
        _imp->keyframeLeftSlopeSpinBox->setValue(_imp->selectedKeyFrame.key.getLeftDerivative());
        _imp->keyframeRightSlopeSpinBox->setValue(_imp->selectedKeyFrame.key.getRightDerivative());
        _imp->keyframeValueSpinBox->setType(dataType == eCurveTypeDouble ? SpinBox::eSpinBoxTypeDouble : SpinBox::eSpinBoxTypeInt);
        KeyframeTypeEnum interp = _imp->selectedKeyFrame.key.getInterpolation();
        KeyFrameInterpolationChoiceMenuEnum menuVal = fromKeyFrameType(interp);
        _imp->keyframeInterpolationChoice->setCurrentIndex((int)menuVal, false);

        _imp->keyframeTimeSpinBox->setValue(_imp->selectedKeyFrame.key.getTime());
        if (dataType == eCurveTypeString || dataType == eCurveTypeChoice) {
            std::string stringValue;
            _imp->selectedKeyFrame.key.getPropertySafe(kKeyFramePropString, 0, &stringValue);
            _imp->keyframeValueLineEdit->setText(QString::fromUtf8(stringValue.c_str()));
        } else {
            _imp->keyframeValueSpinBox->setValue(_imp->selectedKeyFrame.key.getValue());
        }

    }
} // refreshKeyframeWidgetsFromSelection

void
AnimationModuleEditor::onKeyfameInterpolationChoiceMenuChanged(int index)
{
    KeyframeTypeEnum type = toKeyFrameType((KeyFrameInterpolationChoiceMenuEnum)index);

    AnimItemDimViewIndexID id;
    KeyFrame keyData;
    if (!_imp->getKeyframe(&id, &keyData)) {
        return;
    }

    CurveTypeEnum curveType = id.item->getInternalAnimItem()->getKeyFrameDataType();
    if (curveType != eCurveTypeDouble && curveType != eCurveTypeInt)  {
        Dialogs::errorDialog(tr("Interpolation").toStdString(), tr("You cannot change the interpolation type for this type of animation curve").toStdString());
        return;
    }


    AnimItemDimViewKeyFramesMap selectedKeys;
    selectedKeys[id].insert(keyData);
    _imp->model->pushUndoCommand(new SetKeysInterpolationCommand(selectedKeys,
                                                                 _imp->model,
                                                                 type));


}

void
AnimationModuleEditor::onLeftSlopeSpinBoxValueChanged(double value)
{

    AnimItemDimViewKeyFrame keyframe;
    if (!_imp->getKeyframe(&keyframe.id, &keyframe.key)) {
        return;
    }

    _imp->model->pushUndoCommand(new MoveTangentCommand(_imp->model,
                                                        MoveTangentCommand::eSelectedTangentLeft,
                                                        keyframe,
                                                        value));
}

void
AnimationModuleEditor::onRightSlopeSpinBoxValueChanged(double value)
{
    AnimItemDimViewKeyFrame keyframe;
    if (!_imp->getKeyframe(&keyframe.id, &keyframe.key)) {
        return;
    }
    _imp->model->pushUndoCommand(new MoveTangentCommand(_imp->model,
                                                        MoveTangentCommand::eSelectedTangentRight,
                                                        keyframe,
                                                        value));

}

void
AnimationModuleEditor::onKeyframeTimeSpinBoxValueChanged(double value)
{
    AnimItemDimViewIndexID id;
    KeyFrame keyData;
    if (!_imp->getKeyframe(&id, &keyData)) {
        return;
    }
    AnimItemDimViewKeyFramesMap selectedKeys;
    selectedKeys[id].insert(keyData);
    double dt = value - keyData.getTime();
    _imp->model->pushUndoCommand(new WarpKeysCommand(selectedKeys, _imp->model, std::vector<NodeAnimPtr >(), std::vector<TableItemAnimPtr >(), dt, 0));
}


void
AnimationModuleEditor::onKeyframeValueLineEditEditFinished()
{
    AnimItemDimViewIndexID id;
    KeyFrame keyData;
    if (!_imp->getKeyframe(&id, &keyData)) {
        return;
    }
    KnobStringBasePtr isKnobString = boost::dynamic_pointer_cast<KnobStringBase>(id.item->getInternalAnimItem());
    if (!isKnobString) {
        return;
    }

    std::string str = _imp->keyframeValueLineEdit->text().toStdString();
    isKnobString->getHolder()->beginMultipleEdits(tr("Set KeyFrame on %1").arg(QString::fromUtf8(isKnobString->getLabel().c_str())).toStdString());
    isKnobString->setValueAtTime(keyData.getTime(), str, id.view);
    isKnobString->getHolder()->endMultipleEdits();
}

void
AnimationModuleEditor::onKeyframeValueSpinBoxValueChanged(double value)
{
    AnimItemDimViewIndexID id;
    KeyFrame keyData;
    if (!_imp->getKeyframe(&id, &keyData)) {
        return;
    }
    AnimItemDimViewKeyFramesMap selectedKeys;
    selectedKeys[id].insert(keyData);
    double dv = value - keyData.getValue();
    _imp->model->pushUndoCommand(new WarpKeysCommand(selectedKeys, _imp->model, std::vector<NodeAnimPtr >(), std::vector<TableItemAnimPtr >(), 0, dv));
}

void
AnimationModuleEditor::setItemVisibility(QTreeWidgetItem* item, bool visible, bool recurseOnParent)
{
    item->setData(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE, visible);
    item->setIcon(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, _imp->model->getItemVisibilityIcon(visible));
    if (visible) {
        item->setExpanded(true);
    }
    if (recurseOnParent) {
        QTreeWidgetItem* parent = item->parent();
        while (parent) {
            setItemVisibility(parent, visible, recurseOnParent);
            parent = parent->parent();
        }
    }
    
}

void
AnimationModuleEditor::setItemsVisibility(const std::list<QTreeWidgetItem*>& items, bool visible, bool recurseOnParent)
{
    for (std::list<QTreeWidgetItem*>::const_iterator it = items.begin(); it!=items.end(); ++it) {
        setItemVisibility(*it, visible, recurseOnParent);
    }
}

void
AnimationModuleEditor::invertItemVisibility(QTreeWidgetItem* item)
{
    bool isItemVisible = item->data(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE).toBool();
    setItemVisibility(item, !isItemVisible, false);
}

bool
AnimationModuleEditor::setItemVisibilityRecursive(QTreeWidgetItem* item, bool visible, const std::list<QTreeWidgetItem*>& itemsToIgnore)
{
    if (std::find(itemsToIgnore.begin(), itemsToIgnore.end(), item) != itemsToIgnore.end()) {
        return true;
    }

    bool foundItemToIgnoreInChildren = false;
    int nChildren = item->childCount();
    for (int i = 0; i < nChildren; ++i) {
        QTreeWidgetItem* child = item->child(i);
        if (child) {
            foundItemToIgnoreInChildren |= setItemVisibilityRecursive(child, visible, itemsToIgnore);
        }
    }


    if (!foundItemToIgnoreInChildren || visible) {
        AnimationModuleEditor::setItemVisibility(item, visible, false);
    }
    return foundItemToIgnoreInChildren;
}

void
AnimationModuleEditor::setOtherItemsVisibility(const std::list<QTreeWidgetItem*>& items, bool visibile)
{

    int nItems = _imp->treeView->topLevelItemCount();
    for (int i = 0; i < nItems; ++i) {
        QTreeWidgetItem* child = _imp->treeView->topLevelItem(i);
        if (child) {
            setItemVisibilityRecursive(child, visibile, items);
        }
    }

}

static QTreeWidgetItem* findFirstDifferentItemRecursive(QTreeWidgetItem* item, const std::list<QTreeWidgetItem*>& originalItems)
{
    if (std::find(originalItems.begin(), originalItems.end(), item) == originalItems.end()) {
        return item;
    }

    int nChildren = item->childCount();
    for (int i = 0; i < nChildren; ++i) {
        QTreeWidgetItem* child = item->child(i);
        if (!child) {
            continue;
        }
        QTreeWidgetItem* ret = findFirstDifferentItemRecursive(child, originalItems);
        if (ret) {
            return ret;
        }
    }
    return 0;
}

void
AnimationModuleEditor::setOtherItemsVisibilityAuto(const std::list<QTreeWidgetItem*>& items)
{
    int nItems = _imp->treeView->topLevelItemCount();
    QTreeWidgetItem* foundOtherItem = 0;
    for (int i = 0; i < nItems; ++i) {
        QTreeWidgetItem* child = _imp->treeView->topLevelItem(i);
        if (!child) {
            continue;
        }
        foundOtherItem = findFirstDifferentItemRecursive(child, items);
        if (foundOtherItem) {
            break;
        }
    }

    if (foundOtherItem) {
        bool otherCurrentVisibility = foundOtherItem->data(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE).toBool();
        setOtherItemsVisibility(items, !otherCurrentVisibility);
    }

}

void
AnimationModuleEditor::onTreeItemClicked(QTreeWidgetItem* item ,int col)
{
    if (col == ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {

        bool toggleOthers = qApp->keyboardModifiers().testFlag(Qt::ControlModifier);
        if (toggleOthers) {
            std::list<QTreeWidgetItem*> items;
            items.push_back(item);
            setOtherItemsVisibilityAuto(items);
        } else {
            invertItemVisibility(item);
        }
        _imp->view->redraw();
        _imp->treeView->update();
    }
}

void
AnimationModuleEditor::onTreeItemRightClicked(const QPoint& globalPos, QTreeWidgetItem* item)
{
    _imp->rightClickedItem = item;
    Menu menu(this);

    {
        QAction* action = new QAction(&menu);
        action->setText(tr("Show/Hide others"));
        action->setIcon(_imp->model->getItemVisibilityIcon(true));
        menu.addAction(action);
        QObject::connect(action, SIGNAL(triggered()), this, SLOT(onShowHideOtherItemsRightClickMenuActionTriggered()));
    }
    menu.exec(globalPos);
    _imp->rightClickedItem = 0;
}

void
AnimationModuleEditor::onShowHideOtherItemsRightClickMenuActionTriggered()
{
    assert(_imp->rightClickedItem);
    std::list<QTreeWidgetItem*> items;
    items.push_back(_imp->rightClickedItem);
    setOtherItemsVisibilityAuto(items);
}

void
AnimationModuleEditor::notifyGuiClosing()
{
    _imp->model->setAboutToBeDestroyed(true);
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_AnimationModuleEditor.cpp"
