/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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


#include "KnobGui.h"

#include <cassert>
#include <stdexcept>

#include <QUndoCommand>
#include <QMessageBox>

#include <boost/weak_ptr.hpp>

#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/Project.h"
#include "Engine/UndoCommand.h"

#include "Gui/KnobGuiPrivate.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/UndoCommand_qt.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ClickableLabel.h"
#include "Gui/TabGroup.h"


NATRON_NAMESPACE_ENTER


/////////////// KnobGui
KnobGui::KnobGui(const KnobIPtr& knob,
                 KnobLayoutTypeEnum layoutType,
                 KnobGuiContainerI* container)
    : QObject()
    , KnobGuiI()
    , boost::enable_shared_from_this<KnobGui>()
    , _imp( new KnobGuiPrivate(this, knob, layoutType, container) )
{
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct KnobGui::MakeSharedEnabler: public KnobGui
{
    MakeSharedEnabler(const KnobIPtr& knob,
                      KnobLayoutTypeEnum layoutType,
                      KnobGuiContainerI* container) : KnobGui(knob, layoutType, container) {
    }
};


KnobGuiPtr
KnobGui::create(const KnobIPtr& knob,
                KnobLayoutTypeEnum layoutType,
                KnobGuiContainerI* container)
{
    KnobGuiPtr ret = boost::make_shared<KnobGui::MakeSharedEnabler>(knob, layoutType, container);
    ret->initialize();
    return ret;
}


KnobGui::~KnobGui()
{
}


KnobIPtr
KnobGui::getKnob() const
{
    return _imp->knob.lock();
}


void
KnobGui::initialize()
{
    KnobIPtr knob = getKnob();
    KnobGuiPtr thisShared = shared_from_this();

    assert(thisShared);

    NodeGuiPtr nodeUI = getContainer()->getNodeGui();

    KnobHelperPtr helper = toKnobHelper(knob);
    assert(helper);
    if (helper) {
        KnobSignalSlotHandler* handler = helper->getSignalSlotHandler().get();
        QObject::connect( handler, SIGNAL(mustRefreshKnobGui(ViewSetSpec,DimSpec,ValueChangedReasonEnum)), this, SLOT(onMustRefreshGuiActionTriggered(ViewSetSpec,DimSpec,ValueChangedReasonEnum)) );
        QObject::connect( handler, SIGNAL(secretChanged()), this, SLOT(setSecret()) );
        QObject::connect( handler, SIGNAL(enabledChanged()), this, SLOT(setEnabledSlot()) );
        QObject::connect( handler, SIGNAL(selectedMultipleTimes(bool)), this, SLOT(onKnobMultipleSelectionChanged(bool)) );
        QObject::connect( handler, SIGNAL(frozenChanged(bool)), this, SLOT(onFrozenChanged(bool)) );
        QObject::connect( handler, SIGNAL(helpChanged()), this, SLOT(onHelpChanged()) );
        QObject::connect( handler, SIGNAL(expressionChanged(DimIdx,ViewIdx)), this, SLOT(onExprChanged(DimIdx,ViewIdx)) );
        QObject::connect( handler, SIGNAL(hasModificationsChanged()), this, SLOT(onHasModificationsChanged()) );
        QObject::connect( handler, SIGNAL(labelChanged()), this, SLOT(onLabelChanged()) );
        QObject::connect( handler, SIGNAL(dimensionNameChanged(DimIdx)), this, SLOT(onDimensionNameChanged(DimIdx)) );
        QObject::connect( handler, SIGNAL(viewerContextSecretChanged()), this, SLOT(onViewerContextSecretChanged()) );
        QObject::connect( handler, SIGNAL(availableViewsChanged()), this, SLOT(onInternalKnobAvailableViewsChanged()) );
        QObject::connect( handler, SIGNAL(dimensionsVisibilityChanged(ViewSetSpec)), this, SLOT(onInternalKnobDimensionsVisibilityChanged(ViewSetSpec)) );
        QObject::connect( handler, SIGNAL(linkChanged()), this, SLOT(onInternalKnobLinksChanged()) );
    }
    KnobHolderPtr holder = knob->getHolder();
    AppInstancePtr app;
    if (holder) {
        app = holder->getApp();
    }
    if (app) {
        ProjectPtr project = app->getProject();
        QObject::connect(project.get(), SIGNAL(projectViewsChanged()), this, SLOT(onProjectViewsChanged()));
    }


    if (knob->getHolder() && knob->getHolder()->getApp()) {

    }
    QObject::connect( this, SIGNAL(s_doUpdateGuiLater()), this, SLOT(onDoUpdateGuiLaterReceived()), Qt::QueuedConnection );
    QObject::connect( this, SIGNAL(s_updateModificationsStateLater()), this, SLOT(onDoUpdateModificationsStateLaterReceived()), Qt::QueuedConnection );
    QObject::connect( this, SIGNAL( s_refreshDimensionsVisibilityLater()), this, SLOT(onRefreshDimensionsVisibilityLaterReceived()), Qt::QueuedConnection);
}

KnobGuiContainerI*
KnobGui::getContainer()
{
    return _imp->container;
}

void
KnobGui::destroyWidgetsLater()
{
    assert(!_imp->guiRemoved);

    // First remove any other knobs that is on the same layout line from the main layout
    for (std::list<KnobGuiWPtr>::const_iterator it = _imp->otherKnobsInMainLayout.begin(); it != _imp->otherKnobsInMainLayout.end(); ++it) {
        KnobGuiPtr otherKnob = it->lock();
        if (!otherKnob) {
            continue;
        }
        // Remove the 2 container widgets that were added in addWidgetsToPreviousKnobLayout()
        _imp->mainLayout->removeWidget(otherKnob->_imp->mainContainer);
        _imp->mainLayout->removeWidget(otherKnob->_imp->labelContainer);
    }


    // Now delete the main container, this will delete all children recursively
    _imp->mainContainer->deleteLater();
    _imp->mainContainer = 0;

    _imp->views.clear();
    _imp->guiRemoved = true;

    removeTabWidget();
}

void
KnobGuiPrivate::removeViewGui(KnobGuiPrivate::PerViewWidgetsMap::iterator it)
{

    if (it->second.field) {
        it->second.field->deleteLater();
        it->second.field = 0;
    }
}

void
KnobGui::removeTabWidget()
{
    if (_imp->tabGroup) {
        _imp->tabGroup->deleteLater();
        _imp->tabGroup = 0;
    }
}

void
KnobGui::setGuiRemoved()
{
    _imp->guiRemoved = true;
}

Gui*
KnobGui::getGui() const
{
    return _imp->container->getGui();
}

const QUndoCommand*
KnobGui::getLastUndoCommand() const
{
    return _imp->container->getLastUndoCommand();
}

void
KnobGui::pushUndoCommand(QUndoCommand* cmd)
{
    if ( getKnob()->getCanUndo() && getKnob()->getEvaluateOnChange() ) {
        _imp->container->pushUndoCommand(cmd);
    } else {
        cmd->redo();
        delete cmd;
    }
}


void
KnobGui::pushUndoCommand(const UndoCommandPtr& command)
{
    pushUndoCommand(new UndoCommand_qt(command));
}

void
KnobGui::pushUndoCommand(UndoCommand* command)
{
    UndoCommandPtr p(command);
    pushUndoCommand(p);
}



TabGroup*
KnobGui::getTabWidget() const
{
    return _imp->tabGroup;
}

TabGroup*
KnobGui::getOrCreateTabWidget()
{
    if (_imp->tabGroup) {
        return _imp->tabGroup;
    }

    _imp->tabGroup = new TabGroup( _imp->container->getContainerWidget() );
    return _imp->tabGroup;
}

void
KnobGui::setSingleDimensionalEnabled(bool enabled, DimIdx dimension)
{
    _imp->singleDimensionEnabled = enabled;
    _imp->singleDimension = dimension;
}

bool
KnobGui::isSingleDimensionalEnabled(DimIdx* dimension) const
{
    *dimension = _imp->singleDimension;
    return _imp->singleDimensionEnabled;
}

void
KnobGuiPrivate::refreshIsOnNewLineFlag()
{
    KnobIPtr  k = knob.lock();

    KnobIPtr prevKnobOnLine;
    // Find all knobs on the same layout line
    {
        KnobsVec siblings;
        if (layoutType == KnobGui::eKnobLayoutTypeViewerUI && k->getHolder()->getInViewerContextKnobIndex(k) != -1) {
            siblings = k->getHolder()->getViewerUIKnobs();
        } else {
            bool foundTableControl = false;
            std::list<KnobItemsTablePtr> tables = k->getHolder()->getAllItemsTables();
            for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it != tables.end(); ++it) {
                if ((*it)->isTableControlKnob(k)) {
                    siblings = (*it)->getTableControlKnobs();
                    foundTableControl = true;
                    break;
                }
            }
            if (!foundTableControl) {
                KnobIPtr parentKnob = k->getParentKnob();
                KnobGroupPtr isParentGroup = toKnobGroup(parentKnob);
                if (isParentGroup) {
                    // If the parent knob is a group, knobs on the same line have to be found in the children
                    // of the parent
                    siblings = isParentGroup->getChildren();
                } else {
                    // Parent is a page, find the siblings in the children of the page
                    KnobPagePtr isParentPage = toKnobPage(parentKnob);
                    if (isParentPage) {
                        siblings = isParentPage->getChildren();
                    }
                }
            }
           
        }
        if (!siblings.empty()) {
            // Find the previous knob on the line in the layout
            for (std::size_t i = 0; i < siblings.size(); ++i) {
                if (siblings[i] == k) {
                    if (i > 0) {
                        if ((layoutType == KnobGui::eKnobLayoutTypePage && !siblings[i - 1]->isNewLineActivated()) ||
                            (layoutType == KnobGui::eKnobLayoutTypeViewerUI && siblings[i - 1]->getInViewerContextLayoutType() != eViewerContextLayoutTypeAddNewLine)) {
                            prevKnobOnLine = siblings[i - 1];
                        }
                    }
                    break;
                }
            }
        }
    }
 
    isOnNewLine = true;
    prevKnob = prevKnobOnLine;

    if (prevKnobOnLine) {
        if (layoutType == KnobGui::eKnobLayoutTypeViewerUI && prevKnobOnLine->getInViewerContextLayoutType() != eViewerContextLayoutTypeAddNewLine) {
            isOnNewLine = false;
        } else if (layoutType == KnobGui::eKnobLayoutTypePage && !prevKnobOnLine->isNewLineActivated()) {
            isOnNewLine = false;
        }
    }

} // refreshIsOnNewLineFlag

bool
KnobGui::isLabelOnSameColumn() const
{
    // If createGui has not been called yet, return false
    if (_imp->views.empty()) {
        return false;
    }
    KnobGuiWidgetsPtr widgets = _imp->views.begin()->second.widgets;
    if (!widgets) {
        return false;
    }
    return widgets->isLabelOnSameColumn();

}

QWidget*
KnobGui::getFieldContainer() const
{
    return _imp->mainContainer;
}

QWidget*
KnobGui::getLabelContainer() const
{
    return _imp->labelContainer;
}


KnobGuiWidgetsPtr
KnobGui::getWidgetsForView(ViewIdx view)
{
    KnobGuiPrivate::PerViewWidgetsMap::const_iterator foundView = _imp->views.find(view);
    if (foundView == _imp->views.end()) {
        return KnobGuiWidgetsPtr();
    }
    return foundView->second.widgets;
}

static int getLayoutLeftMargin(KnobGui::KnobLayoutTypeEnum type)
{
    switch (type) {
        case KnobGui::eKnobLayoutTypeViewerUI:
            return 0;
        case KnobGui::eKnobLayoutTypePage:
            return 3;
        case KnobGui::eKnobLayoutTypeTableItemWidget:
            return 0;
    }
    return 0;
}
static int getLayoutBottomMargin(KnobGui::KnobLayoutTypeEnum type)
{
    switch (type) {
        case KnobGui::eKnobLayoutTypeViewerUI:
            return 0;
        case KnobGui::eKnobLayoutTypePage:
            return NATRON_SETTINGS_VERTICAL_SPACING_PIXELS;
        case KnobGui::eKnobLayoutTypeTableItemWidget:
            return 0;
    }
    return 0;
}

static int getLayoutSpacing(KnobGui::KnobLayoutTypeEnum type)
{
    switch (type) {
        case KnobGui::eKnobLayoutTypeViewerUI:
            return 0;
        case KnobGui::eKnobLayoutTypePage:
            return 2;
        case KnobGui::eKnobLayoutTypeTableItemWidget:
            return 0;
    }
    return 0;
}

void
KnobGui::createViewContainers(ViewIdx view)
{


    KnobGuiPtr thisShared = shared_from_this();

    KnobGuiPrivate::ViewWidgets& viewWidgets = _imp->views[view];

    // Create a new container and layout
    viewWidgets.field = _imp->container->createKnobHorizontalFieldContainer(_imp->viewsContainer);
    viewWidgets.fieldLayout = new QHBoxLayout(viewWidgets.field);

    int leftMargin = getLayoutLeftMargin(_imp->layoutType);
    int bottomMargin = getLayoutBottomMargin(_imp->layoutType);
    int layoutSpacing = TO_DPIY(getLayoutSpacing(_imp->layoutType));
    viewWidgets.fieldLayout->setContentsMargins( TO_DPIX(leftMargin), 0, 0, TO_DPIY(bottomMargin) );
    viewWidgets.fieldLayout->setSpacing(layoutSpacing);
    viewWidgets.fieldLayout->setAlignment(Qt::AlignLeft);


    KnobIPtr knob = _imp->knob.lock();

    // Create custom interact if any. For parametric knobs, the interact is used to draw below the curves
    // not to replace entirely the widget
    KnobParametricPtr isParametric = toKnobParametric(knob);
    OverlayInteractBasePtr customInteract = knob->getCustomInteract();
    if (customInteract && !isParametric) {
        _imp->customInteract = new CustomParamInteract(shared_from_this(), customInteract, viewWidgets.field);
        viewWidgets.fieldLayout->addWidget(_imp->customInteract);
    } else {
        viewWidgets.widgets.reset(appPTR->createGuiForKnob(thisShared, view));
    }

    _imp->viewsContainerLayout->addWidget(viewWidgets.field);

} // createViewWidgets

void
KnobGuiPrivate::createViewWidgets(KnobGuiPrivate::PerViewWidgetsMap::iterator it)
{

    // createViewContainers must have been called before
    assert(it->second.widgets);
    KnobIPtr k = knob.lock();

    // Create per-view label
    {
        std::vector<std::string> projectViews;
        if (k->getHolder() && k->getHolder()->getApp()) {
            projectViews = k->getHolder()->getApp()->getProject()->getProjectViewNames();
        }

        std::string viewShortName, viewLongName;
        if (it->first >= 0 && it->first < (int)projectViews.size()) {
            if (!projectViews[it->first].empty()) {
                viewLongName = projectViews[it->first];//[0];
                if (!viewLongName.empty()) {
                    viewShortName.push_back(viewLongName[0]);
                } else {
                    viewLongName.push_back('*');
                }
            }
        }
        it->second.viewLongName = QString::fromUtf8(viewLongName.c_str());
        it->second.viewLabel = new KnobClickableLabel(QString::fromUtf8(viewShortName.c_str()), _publicInterface->shared_from_this(), it->first, it->second.field);
        it->second.fieldLayout->addWidget(it->second.viewLabel);
        if (views.size() == 1) {
            it->second.viewLabel->hide();
        }

    }

    it->second.widgets->createWidget(it->second.fieldLayout);
    it->second.widgets->updateToolTip();

}


void
KnobGuiPrivate::createLabel(QWidget* parentWidget)
{
    if (layoutType == KnobGui::eKnobLayoutTypeTableItemWidget) {
        return;
    }
    // createViewContainers must have been called
    assert(!views.empty());
    KnobGuiWidgetsPtr firstViewWidgets = views.begin()->second.widgets;
    bool mustCreateLabel = firstViewWidgets->mustCreateLabelWidget();

    KnobIPtr k = knob.lock();
    KnobGuiPtr thisShared = _publicInterface->shared_from_this();

    std::string labelText,labelIconFilePath;
    if (layoutType == KnobGui::eKnobLayoutTypeViewerUI) {
        labelText = k->getInViewerContextLabel();
        labelIconFilePath = k->getInViewerContextIconFilePath(false);
    } else {
        if (!views.empty()) {
            // Use a label based on the implementation
            labelText = firstViewWidgets->getDescriptionLabel();
        }
        labelIconFilePath = k->getIconLabel();
    }

    labelContainer = new QWidget(parentWidget);
    QHBoxLayout* labelLayout = new QHBoxLayout(labelContainer);

    int leftMargin = getLayoutLeftMargin(layoutType);
    int bottomMargin = getLayoutBottomMargin(layoutType);

    labelLayout->setContentsMargins( TO_DPIX(leftMargin), 0, 0, TO_DPIY(bottomMargin) );
    int layoutSpacing = TO_DPIY(getLayoutSpacing(layoutType));
    labelLayout->setSpacing(layoutSpacing);

    if (mustCreateLabel) {

        descriptionLabel = new KnobClickableLabel(QString(), thisShared, ViewSetSpec::all(), parentWidget);
        KnobGuiContainerHelper::setLabelFromTextAndIcon(descriptionLabel, QString::fromUtf8(labelText.c_str()), QString::fromUtf8(labelIconFilePath.c_str()), firstViewWidgets->isLabelBold());
        QObject::connect( descriptionLabel, SIGNAL(clicked(bool)), _publicInterface, SIGNAL(labelClicked(bool)) );

        if (descriptionLabel->text().isEmpty() && (!descriptionLabel->pixmap() || descriptionLabel->pixmap()->isNull())) {
            descriptionLabel->hide();
        }
        if (layoutType == KnobGui::eKnobLayoutTypePage) {
            // Make a warning indicator
            warningIndicator = new Label(parentWidget);
            warningIndicator->setVisible(false);

            QFontMetrics fm(descriptionLabel->font(), 0);
            int pixSize = fm.height();
            QPixmap stdErrorPix;
            stdErrorPix = KnobGuiContainerHelper::getStandardIcon(QMessageBox::Critical, pixSize, descriptionLabel);
            warningIndicator->setPixmap(stdErrorPix);
            
        }

    }

    // if multi-view, create an arrow to show/hide all dimensions
    viewUnfoldArrow = new GroupBoxLabel(labelContainer);
    viewUnfoldArrow->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    viewUnfoldArrow->setChecked(true);
    if (views.size() == 1) {
        viewUnfoldArrow->setVisible(false);
    }
    QObject::connect( viewUnfoldArrow, SIGNAL(checked(bool)), _publicInterface, SLOT(onMultiViewUnfoldClicked(bool)) );
    labelLayout->addWidget(viewUnfoldArrow);

    if (warningIndicator) {
        labelLayout->addWidget(warningIndicator);
    }
    if (descriptionLabel) {
        labelLayout->addWidget(descriptionLabel);
    }
    if (viewUnfoldArrow) {
        labelLayout->addWidget(viewUnfoldArrow);
    }


} // createLabel


static void
addVerticalLineSpacer(QBoxLayout* layout)
{
    layout->addSpacing( TO_DPIX(5) );
    QFrame* line = new QFrame( layout->parentWidget() );
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Raised);
    QPalette palette;
    palette.setColor(QPalette::Foreground, Qt::black);
    line->setPalette(palette);
    layout->addWidget(line);
    layout->addSpacing( TO_DPIX(5) );
}

void
KnobGuiPrivate::addWidgetsToPreviousKnobLayout()
{
    KnobIPtr prev = prevKnob.lock();
    assert(prev);
    KnobGuiPtr prevKnobGui = container->getKnobGui(prev);
    assert(prevKnobGui);

    KnobIPtr thisKnob = knob.lock();

    // Recurse on previous knobs until the first one on the same line
    QHBoxLayout* firstKnobOnLineMainLayout = 0;
    KnobGuiPtr firstKnobGuiOnLine;
    {
        KnobIPtr tmpPrev = prev;
        KnobGuiPtr tmpPrevGui = prevKnobGui;
        while (tmpPrevGui) {
            firstKnobGuiOnLine = tmpPrevGui;
            firstKnobOnLineMainLayout = tmpPrevGui->_imp->mainLayout;
            tmpPrev = tmpPrevGui->_imp->prevKnob.lock();
            if (!tmpPrev) {
                break;
            }
            tmpPrevGui = container->getKnobGui(tmpPrev);
        }

        assert(firstKnobOnLineMainLayout);
        assert(firstKnobGuiOnLine);
    }

    // Register this knob to the first knob on the line
    firstKnobGuiOnLine->_imp->otherKnobsInMainLayout.push_back(_publicInterface->shared_from_this());

    // Check if we need to add spacing or stretch
    if (layoutType == KnobGui::eKnobLayoutTypeViewerUI) {
        switch (thisKnob->getInViewerContextLayoutType()) {
            case eViewerContextLayoutTypeSeparator:
                addVerticalLineSpacer(firstKnobOnLineMainLayout);
                break;
            case eViewerContextLayoutTypeSpacing: {
                int spacing = TO_DPIX(prev->getInViewerContextItemSpacing());
                if (spacing > 0) {
                    firstKnobOnLineMainLayout->addSpacing(spacing);
                }
            }   break;
            case eViewerContextLayoutTypeStretchAfter:
            case eViewerContextLayoutTypeAddNewLine:
                //firstKnobOnLineMainLayout->addStretch();
                break;
            default:
                break;
        }
    } else if (layoutType == KnobGui::eKnobLayoutTypePage) {
        int spacing = TO_DPIX(prev->getSpacingBetweenitems());
        // Default sapcing is 0 on knobs, but use the default for the widget container so the UI doesn't appear cluttered
        // The minimum allowed spacing should be 1px
        if (spacing == 0) {
            spacing = TO_DPIX(container->getItemsSpacingOnSameLine());
        }
        if (spacing > 0) {
            firstKnobOnLineMainLayout->addSpacing(spacing);
        }
    }

    // Add the label to the main layout
    firstKnobOnLineMainLayout->addWidget(labelContainer);

    // Separate the label and the actual field
    firstKnobOnLineMainLayout->addWidget(mainContainer);

    // Set the pointer to the mainLayout of the knob starting the line
    firstKnobOnLine = firstKnobGuiOnLine;
} // addWidgetsToPreviousKnobLayout


void
KnobGui::createGUI(QWidget* parentWidget)
{
    _imp->guiRemoved = false;
    KnobIPtr knob = getKnob();

    // Set the isOnNewLineFlag 
    _imp->refreshIsOnNewLineFlag();


    // Create the main container
    _imp->mainContainer = new QWidget(parentWidget);
    _imp->mainLayout = new QHBoxLayout(_imp->mainContainer);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    // Create the per-view container
    _imp->viewsContainer = new QWidget(_imp->mainContainer);
    _imp->viewsContainerLayout = new QVBoxLayout(_imp->viewsContainer);
    _imp->viewsContainerLayout->setSpacing(TO_DPIY(NATRON_SETTINGS_VERTICAL_SPACING_PIXELS));
    _imp->viewsContainerLayout->setContentsMargins(0, 0, 0, 0);



    // Parmetric knobs use the customInteract to actually draw something on top of the background

    // Create row widgets container for each view
    std::list<ViewIdx> views = knob->getViewsList();
    for (std::list<ViewIdx>::iterator it = views.begin(); it != views.end(); ++it) {
        createViewContainers(*it);
    }


    assert(!_imp->views.empty());
    KnobGuiWidgetsPtr firstViewWidgets = _imp->views.begin()->second.widgets;
    assert(firstViewWidgets);



    _imp->createLabel(parentWidget);

    // For the viewer layout, if a knob creates a new line, prepend the layout with
    // its label because it is not using a grid layout.
    if (_imp->isOnNewLine && _imp->layoutType == eKnobLayoutTypeViewerUI) {
        _imp->mainLayout->addWidget(_imp->labelContainer);
    }

    _imp->mainLayout->addWidget(_imp->viewsContainer);



    // Now create the widgets for each view, they will be appended to the row layout created in createViewContainers
    for (KnobGuiPrivate::PerViewWidgetsMap::iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
        _imp->createViewWidgets(it);
    }


    // If not on a new line, add to the previous knob layout this knob's label
    if (!_imp->isOnNewLine) {
        _imp->addWidgetsToPreviousKnobLayout();
    }

    if ( firstViewWidgets->shouldAddStretch() ) {
        if ((_imp->layoutType == eKnobLayoutTypePage && knob->isNewLineActivated()) ||
            (_imp->layoutType == eKnobLayoutTypeViewerUI && knob->getInViewerContextLayoutType() == eViewerContextLayoutTypeAddNewLine)) {
            _imp->endOfLineSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
            if (_imp->mustAddSpacerByDefault) {
                addSpacerItemAtEndOfLine();
            }
        } else if (_imp->layoutType == eKnobLayoutTypeViewerUI && knob->getInViewerContextLayoutType() == eViewerContextLayoutTypeStretchAfter) {
            KnobGuiPtr firstKnobGuiOnLine = _imp->firstKnobOnLine.lock();
            if (firstKnobGuiOnLine) {
                firstKnobGuiOnLine->_imp->mainLayout->addStretch();
            } else {
                _imp->mainLayout->addStretch();
            }

        }
    }


    if (_imp->descriptionLabel) {
        toolTip(_imp->descriptionLabel, ViewIdx(0));
    }


    _imp->widgetCreated = true;


    // Refresh modifications state
    refreshModificationsStateNow();

    // Refresh link state
    onInternalKnobLinksChanged();

    
    // Refresh animation and expression state on all views
    for (int i = 0; i < knob->getNDimensions(); ++i) {
        for (KnobGuiPrivate::PerViewWidgetsMap::const_iterator it = _imp->views.begin(); it!=_imp->views.end(); ++it) {
            onExprChanged(DimIdx(i), it->first);
        }
    }

    // Refresh secretness and enabledness
    setSecret();
    setEnabledSlot();

} // KnobGui::createGUI

void
KnobGui::addSpacerItemAtEndOfLine()
{
    if (!_imp->endOfLineSpacer) {
        _imp->mustAddSpacerByDefault = true;
        return;
    }
    if (_imp->spacerAdded) {
        return;
    }
    _imp->spacerAdded = true;
    KnobGuiPtr firstKnobGuiOnLine = _imp->firstKnobOnLine.lock();
    if (firstKnobGuiOnLine) {
        firstKnobGuiOnLine->_imp->mainLayout->addSpacerItem(_imp->endOfLineSpacer);
    } else {
        _imp->mainLayout->addSpacerItem(_imp->endOfLineSpacer);
    }

}

void
KnobGui::removeSpacerItemAtEndOfLine()
{
    if (!_imp->endOfLineSpacer) {
        _imp->mustAddSpacerByDefault = false;
        return;
    }
    if (!_imp->spacerAdded) {
        return;
    }
    _imp->spacerAdded = false;
    KnobGuiPtr firstKnobGuiOnLine = _imp->firstKnobOnLine.lock();
    if (firstKnobGuiOnLine) {
        firstKnobGuiOnLine->_imp->mainLayout->removeItem(_imp->endOfLineSpacer);
    } else {
        _imp->mainLayout->removeItem(_imp->endOfLineSpacer);
    }

}

void
KnobGui::onRightClickClicked(const QPoint & pos)
{
    QWidget *widget = qobject_cast<QWidget *>( sender() );

    if (widget) {
        ViewSetSpec view(widget->property(KNOB_RIGHT_CLICK_VIEW_PROPERTY).toInt());
        DimSpec dim(widget->property(KNOB_RIGHT_CLICK_DIM_PROPERTY).toInt());
        showRightClickMenuForDimension(pos, dim, view);
    }
}

void
KnobGui::showRightClickMenuForDimension(const QPoint &,
                                        DimSpec dimension, ViewSetSpec view)
{
    KnobIPtr knob = getKnob();
    bool isViewerKnob = _imp->layoutType == KnobGui::eKnobLayoutTypeViewerUI;
    if ( (!isViewerKnob && knob->getIsSecret()) || (isViewerKnob && knob->getInViewerContextSecret()) ) {
        return;
    }

    createAnimationMenu(_imp->copyRightClickMenu, dimension, view);
    if (!_imp->views.empty()) {
        KnobGuiWidgetsPtr widgets = _imp->views.begin()->second.widgets;
        if (widgets) {
            widgets->addRightClickMenuEntries(_imp->copyRightClickMenu);
        }
    }
    _imp->copyRightClickMenu->exec( QCursor::pos() );
} // showRightClickMenuForDimension

Menu*
KnobGui::createInterpolationMenu(QMenu* menu,
                                 DimSpec dimension, ViewSetSpec view,
                                 bool isEnabled)
{
    Menu* interpolationMenu = new Menu(menu);
    QString title;

    if (dimension.isAll()) {
        if (view.isAll()) {
            title = tr("Interpolation (all views)");
        } else {
            title = tr("Interpolation (all dimensions)");
        }
    } else {
        title = tr("Interpolation");
    }
    interpolationMenu->setTitle(title);
    if (!isEnabled) {
        interpolationMenu->menuAction()->setEnabled(false);
    }

    QList<QVariant> genericActionData;
    genericActionData.push_back((int)dimension);
    genericActionData.push_back((int)view);

    {
        QAction* constantInterpAction = new QAction(tr("Constant"), interpolationMenu);
        QList<QVariant> actionData = genericActionData;
        actionData.push_back((int)eKeyframeTypeConstant);
        constantInterpAction->setData( QVariant(actionData) );
        QObject::connect( constantInterpAction, SIGNAL(triggered()), this, SLOT(onInterpolationActionTriggered()) );
        interpolationMenu->addAction(constantInterpAction);
    }

    {
        QAction* linearInterpAction = new QAction(tr("Linear"), interpolationMenu);
        QList<QVariant> actionData = genericActionData;
        actionData.push_back((int)eKeyframeTypeLinear);
        linearInterpAction->setData( QVariant(actionData) );
        QObject::connect( linearInterpAction, SIGNAL(triggered()), this, SLOT(onInterpolationActionTriggered()) );
        interpolationMenu->addAction(linearInterpAction);
    }
    {
        QAction* smoothInterpAction = new QAction(tr("Smooth"), interpolationMenu);
        QList<QVariant> actionData = genericActionData;
        actionData.push_back((int)eKeyframeTypeSmooth);
        smoothInterpAction->setData( QVariant(actionData) );
        QObject::connect( smoothInterpAction, SIGNAL(triggered()), this, SLOT(onInterpolationActionTriggered()) );
        interpolationMenu->addAction(smoothInterpAction);
    }
    {
        QAction* catmullRomInterpAction = new QAction(tr("Catmull-Rom"), interpolationMenu);
        QList<QVariant> actionData = genericActionData;
        actionData.push_back((int)eKeyframeTypeCatmullRom);
        catmullRomInterpAction->setData( QVariant(actionData) );
        QObject::connect( catmullRomInterpAction, SIGNAL(triggered()), this, SLOT(onInterpolationActionTriggered()) );
        interpolationMenu->addAction(catmullRomInterpAction);
    }
    {
        QAction* cubicInterpAction = new QAction(tr("Cubic"), interpolationMenu);
        QList<QVariant> actionData = genericActionData;
        actionData.push_back((int)eKeyframeTypeCubic);
        cubicInterpAction->setData( QVariant(actionData) );
        QObject::connect( cubicInterpAction, SIGNAL(triggered()), this, SLOT(onInterpolationActionTriggered()) );
        interpolationMenu->addAction(cubicInterpAction);
    }
    {
        QAction* horizInterpAction = new QAction(tr("Horizontal"), interpolationMenu);
        QList<QVariant> actionData = genericActionData;
        actionData.push_back((int)eKeyframeTypeHorizontal);
        horizInterpAction->setData( QVariant(actionData) );
        QObject::connect( horizInterpAction, SIGNAL(triggered()), this, SLOT(onInterpolationActionTriggered()) );
        interpolationMenu->addAction(horizInterpAction);
    }
    menu->addAction( interpolationMenu->menuAction() );
    
    return interpolationMenu;
}

void
KnobGui::getDimViewFromActionData(const QAction* action, ViewSetSpec* view, DimSpec* dimension)
{
    QList<QVariant> actionData = action->data().toList();
    if (actionData.size() != 2) {
        return;
    }
    QList<QVariant>::iterator it = actionData.begin();
    *dimension = DimSpec(it->toInt());
    ++it;
    *view = ViewSetSpec(it->toInt());
}


bool
KnobGui::getAllViewsVisible() const
{
    if (!_imp->viewUnfoldArrow) {
        return true;
    }
    return _imp->viewUnfoldArrow->isChecked();
}

void
KnobGui::createAnimationMenu(QMenu* menu, DimSpec dimensionIn, ViewSetSpec viewIn)
{
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    DimSpec dimension;
    ViewSetSpec view;
    knob->convertDimViewArgAccordingToKnobState(dimensionIn, viewIn, &dimension, &view);

    menu->clear();

    CurveTypeEnum curveType = knob->getKeyFrameDataType();

    bool dimensionHasKeyframeAtTime = false;
    bool hasAllKeyframesAtTime = true;
    int nDims = knob->getNDimensions();
    bool hasAnimation = false;
    bool dimensionHasAnimation = false;
    bool isEnabled = knob->isEnabled();
    bool dimensionHasExpression = false;
    bool hasExpression = false;
    std::list<ViewIdx> views, knobViews;
    knobViews = getKnob()->getViewsList();
    if (view.isAll()) {
        views = knobViews;
    } else {
        views.push_back(ViewIdx(view));
    }

    bool hasSharedDimension = false;
    KnobDimViewKeySet dimensionSharedKnobs;
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        for (int i = 0; i < nDims; ++i) {

            if (i > 0 && !knob->getAllDimensionsVisible(*it)) {
                continue;
            }
            AnimationLevelEnum lvl = knob->getAnimationLevel(DimIdx(i), knob->getHolder()->getTimelineCurrentTime(), *it);
            if (lvl != eAnimationLevelOnKeyframe) {
                hasAllKeyframesAtTime = false;
            } else if ( (dimension == i || nDims == 1) &&
                       (view == *it || views.size() == 1)
                       && (lvl == eAnimationLevelOnKeyframe) ) {
                dimensionHasKeyframeAtTime = true;
            }

            if (knob->getKeyFramesCount(*it, DimIdx(i)) > 0) {
                hasAnimation = true;
                if ((dimension == i || nDims == 1) &&
                    (view == *it || views.size() == 1)) {
                    dimensionHasAnimation = true;
                }
            }

            if (!knob->getExpression(DimIdx(i), *it).empty()) {
                hasExpression = true;
                if ((dimension == i || nDims == 1) &&
                    (view == *it || views.size() == 1)) {
                    dimensionHasExpression = true;
                }
            }

            KnobDimViewKeySet sharedKnobs;
            knob->getSharedValues(DimIdx(i), *it, &sharedKnobs);
            if (!sharedKnobs.empty()) {
                hasSharedDimension = true;
                if ((dimension == i || nDims == 1) &&
                    (view == *it || views.size() == 1)) {
                    dimensionSharedKnobs = sharedKnobs;
                }
            }
        }
    }

    bool isAllViewsAction = views.size() > 1;

    bool isAppKnob = knob->getHolder() && knob->getHolder()->getApp();

    if ( (knob->getNDimensions() > 1 || views.size() > 1) && knob->isAnimationEnabled() && !hasExpression && isAppKnob ) {
        ///Multi-dim actions
        if (!hasAllKeyframesAtTime) {
            QString label;
            if (isAllViewsAction) {
                label = tr("Set Key (all views)");
            } else {
                label = tr("Set Key (all dimensions)");
            }
            QAction* setKeyAction = new QAction(label, menu);
            QList<QVariant> actionData;
            actionData.push_back((int)DimSpec::all());
            actionData.push_back((int)view);
            setKeyAction->setData(actionData);
            QObject::connect( setKeyAction, SIGNAL(triggered()), this, SLOT(onSetKeyActionTriggered()) );
            menu->addAction(setKeyAction);
            if (!isEnabled) {
                setKeyAction->setEnabled(false);
            }
        } else {
            QString label;
            if (isAllViewsAction) {
                label = tr("Remove Key (all views)");
            } else {
                label = tr("Remove Key (all dimensions)");
            }
            QAction* removeKeyAction = new QAction(label, menu);
            QList<QVariant> actionData;
            actionData.push_back((int)DimSpec::all());
            actionData.push_back((int)view);
            removeKeyAction->setData(actionData);
            QObject::connect( removeKeyAction, SIGNAL(triggered()), this, SLOT(onRemoveKeyActionTriggered()) );
            menu->addAction(removeKeyAction);
            if (!isEnabled) {
                removeKeyAction->setEnabled(false);
            }
        }

        if (hasAnimation) {
            QString label;
            if (isAllViewsAction) {
                label = tr("Remove Animation (all views)");
            } else {
                label = tr("Remove Animation (all dimensions)");
            }
            QAction* removeAnyAnimationAction = new QAction(label, menu);
            QList<QVariant> actionData;
            actionData.push_back((int)DimSpec::all());
            actionData.push_back((int)view);
            removeAnyAnimationAction->setData(actionData);
            QObject::connect( removeAnyAnimationAction, SIGNAL(triggered()), this, SLOT(onRemoveAnimationActionTriggered()) );
            if (!isEnabled) {
                removeAnyAnimationAction->setEnabled(false);
            }
            menu->addAction(removeAnyAnimationAction);
        }
    }
    if ( !isAllViewsAction && ( (!dimension.isAll()) || (knob->getNDimensions() == 1) ) && knob->isAnimationEnabled() && !dimensionHasExpression && isAppKnob ) {
        if ( !menu->isEmpty() ) {
            menu->addSeparator();
        }
        {
            ///Single dim action
            if (!dimensionHasKeyframeAtTime) {
                QAction* setKeyAction = new QAction(tr("Set Key"), menu);
                QList<QVariant> actionData;
                actionData.push_back((int)dimension);
                actionData.push_back((int)view);
                setKeyAction->setData(actionData);
                QObject::connect( setKeyAction, SIGNAL(triggered()), this, SLOT(onSetKeyActionTriggered()) );
                menu->addAction(setKeyAction);
                if (!isEnabled) {
                    setKeyAction->setEnabled(false);
                }
            } else {
                QAction* removeKeyAction = new QAction(tr("Remove Key"), menu);
                QList<QVariant> actionData;
                actionData.push_back((int)dimension);
                actionData.push_back((int)view);
                removeKeyAction->setData(actionData);
                QObject::connect( removeKeyAction, SIGNAL(triggered()), this, SLOT(onRemoveKeyActionTriggered()) );
                menu->addAction(removeKeyAction);
                if (!isEnabled) {
                    removeKeyAction->setEnabled(false);
                }
            }

            if (dimensionHasAnimation) {
                QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"), menu);
                QList<QVariant> actionData;
                actionData.push_back((int)dimension);
                actionData.push_back((int)view);
                removeAnyAnimationAction->setData(actionData);
                QObject::connect( removeAnyAnimationAction, SIGNAL(triggered()), this, SLOT(onRemoveAnimationActionTriggered()) );
                menu->addAction(removeAnyAnimationAction);
                if (!isEnabled) {
                    removeAnyAnimationAction->setEnabled(false);
                }
            }
        }
    }
    if ( !menu->isEmpty() ) {
        menu->addSeparator();
    }

    if (hasAnimation && isAppKnob) {
        QAction* showInCurveEditorAction = new QAction(tr("Show in Animation Editor"), menu);
        QList<QVariant> actionData;
        actionData.push_back((int)dimension);
        actionData.push_back((int)view);
        showInCurveEditorAction->setData(actionData);
        QObject::connect( showInCurveEditorAction, SIGNAL(triggered()), this, SLOT(onShowInCurveEditorActionTriggered()) );
        menu->addAction(showInCurveEditorAction);
        if (!isEnabled) {
            showInCurveEditorAction->setEnabled(false);
        }

        // Only allow interpolation change for double/int
        if (curveType == eCurveTypeInt || curveType == eCurveTypeDouble) {
            if ( (knob->getNDimensions() > 1) && !hasExpression ) {
                Menu* interpMenu = createInterpolationMenu(menu, DimSpec::all(), view, isEnabled);
                Q_UNUSED(interpMenu);
            }
            if (dimensionHasAnimation && !hasExpression) {
                if ( !isAllViewsAction && (!dimension.isAll() || (knob->getNDimensions() == 1)) ) {
                    Menu* interpMenu = createInterpolationMenu(menu, !dimension.isAll() ? dimension : DimSpec(0), view, isEnabled);
                    Q_UNUSED(interpMenu);
                }
            }
        }
    }
    

    {
        Menu* copyMenu = new Menu(menu);
        copyMenu->setTitle( tr("Copy") );

        if (!isAllViewsAction && (!dimension.isAll() || nDims == 1)) {
            assert(!isAllViewsAction);
            if (hasAnimation && isAppKnob && !dimensionHasExpression) {
                QAction* copyAnimationAction = new QAction(tr("Copy Animation"), copyMenu);
                QList<QVariant> actionData;
                actionData.push_back((int)dimension);
                actionData.push_back((int)view);
                copyAnimationAction->setData(actionData);
                QObject::connect( copyAnimationAction, SIGNAL(triggered()), this, SLOT(onCopyAnimationActionTriggered()) );
                copyMenu->addAction(copyAnimationAction);
            }


            {
                QAction* copyValuesAction = new QAction(tr("Copy Value"), copyMenu);
                QList<QVariant> actionData;
                actionData.push_back((int)dimension);
                actionData.push_back((int)view);
                copyValuesAction->setData(actionData);
                copyMenu->addAction(copyValuesAction);
                QObject::connect( copyValuesAction, SIGNAL(triggered()), this, SLOT(onCopyValuesActionTriggered()) );
            }

            if (isAppKnob && !dimensionHasExpression) {
                QAction* copyLinkAction = new QAction(tr("Copy Link"), copyMenu);
                QList<QVariant> actionData;
                actionData.push_back((int)dimension);
                actionData.push_back((int)view);
                copyLinkAction->setData(actionData);
                copyMenu->addAction(copyLinkAction);
                QObject::connect( copyLinkAction, SIGNAL(triggered()), this, SLOT(onCopyLinksActionTriggered()) );
            }
        }

        if (knob->getNDimensions() > 1) {
            if (hasAnimation && isAppKnob && !hasExpression) {
                QString label;
                if (isAllViewsAction) {
                    label = tr("Copy Animation (all views)");
                } else {
                    label = tr("Copy Animation (all dimensions)");
                }

                QAction* copyAnimationAction = new QAction(label, copyMenu);
                QList<QVariant> actionData;
                actionData.push_back((int)DimSpec::all());
                actionData.push_back((int)view);
                copyAnimationAction->setData(actionData);
                QObject::connect( copyAnimationAction, SIGNAL(triggered()), this, SLOT(onCopyAnimationActionTriggered()) );
                copyMenu->addAction(copyAnimationAction);
            }
            {
                QString label;
                if (isAllViewsAction) {
                    label = tr("Copy Values (all views)");
                } else {
                    label = tr("Copy Values (all dimensions)");
                }

                QAction* copyValuesAction = new QAction(label, copyMenu);
                QList<QVariant> actionData;
                actionData.push_back((int)DimSpec::all());
                actionData.push_back((int)view);
                copyValuesAction->setData(actionData);
                copyMenu->addAction(copyValuesAction);
                QObject::connect( copyValuesAction, SIGNAL(triggered()), this, SLOT(onCopyValuesActionTriggered()) );
            }

            if (isAppKnob && !hasExpression) {
                QString label;
                if (isAllViewsAction) {
                    label = tr("Copy Link (all views)");
                } else {
                    label = tr("Copy Link (all dimensions)");
                }
                QAction* copyLinkAction = new QAction(label, copyMenu);
                QList<QVariant> actionData;
                actionData.push_back((int)DimSpec::all());
                actionData.push_back((int)view);
                copyLinkAction->setData(actionData);
                copyMenu->addAction(copyLinkAction);
                QObject::connect( copyLinkAction, SIGNAL(triggered()), this, SLOT(onCopyLinksActionTriggered()) );
            }

        }

        menu->addAction( copyMenu->menuAction() );
    }

    ///If the clipboard is either empty or has no animation, disable the Paste animation action.
    KnobIPtr fromKnob;
    KnobClipBoardType type;

    //cbDim is ignored for now
    DimSpec cbDim;
    ViewSetSpec cbView;
    appPTR->getKnobClipBoard(&type, &fromKnob, &cbDim, &cbView);


    if (fromKnob && (fromKnob != knob) && isAppKnob) {
        if ( fromKnob->typeName() == knob->typeName() ) {
            QString titlebase;
            if (type == eKnobClipBoardTypeCopyValue) {
                titlebase = tr("Paste Value");
            } else if (type == eKnobClipBoardTypeCopyAnim) {
                titlebase = tr("Paste Animation");
            } else if (type == eKnobClipBoardTypeCopyLink) {
                titlebase = tr("Paste Link");
            }

            bool ignorePaste = (!knob->isAnimationEnabled() && type == eKnobClipBoardTypeCopyAnim) ||
            ( (dimension.isAll() || cbDim.isAll()) && knob->getNDimensions() != fromKnob->getNDimensions() ) ||
            (type == eKnobClipBoardTypeCopyLink && hasExpression);
            if (!ignorePaste) {
                if ( cbDim.isAll() && ( fromKnob->getNDimensions() == knob->getNDimensions() ) && !hasExpression ) {
                    QString title = titlebase;
                    if (isAllViewsAction) {
                        title += QLatin1Char(' ');
                        title += tr("(all views)");
                    } else {
                        if (knob->getNDimensions() > 1) {
                            title += QLatin1Char(' ');
                            title += tr("(all dimensions)");

                        }
                    }
                    QAction* pasteAction = new QAction(title, menu);
                    QList<QVariant> actionData;
                    actionData.push_back((int)DimSpec::all());
                    actionData.push_back((int)view);
                    pasteAction->setData(actionData);
                    QObject::connect( pasteAction, SIGNAL(triggered()), this, SLOT(onPasteActionTriggered()) );
                    menu->addAction(pasteAction);
                    if (!isEnabled) {
                        pasteAction->setEnabled(false);
                    }
                }

                if ( !isAllViewsAction && ( !dimension.isAll() || (knob->getNDimensions() == 1) ) && !dimensionHasExpression ) {
                    QAction* pasteAction = new QAction(titlebase, menu);
                    QList<QVariant> actionData;
                    actionData.push_back(dimension.isAll() ? 0 : (int)dimension);
                    actionData.push_back((int)view);
                    pasteAction->setData(actionData);
                    QObject::connect( pasteAction, SIGNAL(triggered()), this, SLOT(onPasteActionTriggered()) );
                    menu->addAction(pasteAction);
                    if (!isEnabled) {
                        pasteAction->setEnabled(false);
                    }
                }
            }
        }
    }

    if ( (knob->getNDimensions() > 1) ) {
        QString label;
        if (isAllViewsAction) {
            label = tr("Reset to default (all views)");
        } else {
            label = tr("Reset to default (all dimensions)");
        }
        QAction* resetDefaultAction = new QAction(label, _imp->copyRightClickMenu);
        QList<QVariant> actionData;
        actionData.push_back((int)DimSpec::all());
        actionData.push_back((int)view);
        resetDefaultAction->setData(actionData);
        QObject::connect( resetDefaultAction, SIGNAL(triggered()), this, SLOT(onResetDefaultValuesActionTriggered()) );
        menu->addAction(resetDefaultAction);
        if (!isEnabled) {
            resetDefaultAction->setEnabled(false);
        }
    }
    if ( !isAllViewsAction && ( (dimension != -1) || (knob->getNDimensions() == 1) ) ) {
        QAction* resetDefaultAction = new QAction(tr("Reset to default"), _imp->copyRightClickMenu);
        QList<QVariant> actionData;
        actionData.push_back((int)dimension);
        actionData.push_back((int)view);
        resetDefaultAction->setData(actionData);
        QObject::connect( resetDefaultAction, SIGNAL(triggered()), this, SLOT(onResetDefaultValuesActionTriggered()) );
        menu->addAction(resetDefaultAction);
        if (!isEnabled) {
            resetDefaultAction->setEnabled(false);
        }
    }

    if ( !menu->isEmpty() ) {
        menu->addSeparator();
    }


    if ( (knob->getNDimensions() > 1) && isAppKnob && !hasSharedDimension ) {
        {
            QString label;
            if (isAllViewsAction) {
                if (hasExpression) {
                    label = tr("Edit Expression (all views)");
                } else {
                    label = tr("Set Expression (all views)");
                }

            } else {
                if (hasExpression) {
                    label = tr("Edit Expression (all dimensions)");
                } else {
                    label = tr("Set Expression (all dimensions)");
                }
            }

            QAction* setExprsAction = new QAction(label, menu );
            QList<QVariant> actionData;
            actionData.push_back((int)DimSpec::all());
            actionData.push_back((int)view);
            setExprsAction->setData(actionData);
            QObject::connect( setExprsAction, SIGNAL(triggered()), this, SLOT(onSetExprActionTriggered()) );
            if (!isEnabled) {
                setExprsAction->setEnabled(false);
            }
            menu->addAction(setExprsAction);
        }
        if (hasExpression) {
            QString label;
            if (isAllViewsAction) {
                label = tr("Clear Expression (all views)");
            } else {
                label = tr("Clear Expression (all dimensions)");
            }

            QAction* clearExprAction = new QAction(label, menu);
            QObject::connect( clearExprAction, SIGNAL(triggered()), this, SLOT(onClearExprActionTriggered()) );
            QList<QVariant> actionData;
            actionData.push_back((int)DimSpec::all());
            actionData.push_back((int)view);
            clearExprAction->setData(actionData);
            if (!isEnabled) {
                clearExprAction->setEnabled(false);
            }
            menu->addAction(clearExprAction);
        }
    }
    if ( !isAllViewsAction && ( (!dimension.isAll()) || (knob->getNDimensions() == 1) ) && isAppKnob && dimensionSharedKnobs.empty() ) {
        {
            QAction* setExprAction = new QAction(dimensionHasExpression ? tr("Edit expression...") : tr("Set expression..."), menu);
            QObject::connect( setExprAction, SIGNAL(triggered()), this, SLOT(onSetExprActionTriggered()) );
            QList<QVariant> actionData;
            actionData.push_back((int)dimension);
            actionData.push_back((int)view);
            setExprAction->setData(actionData);
            if (!isEnabled) {
                setExprAction->setEnabled(false);
            }
            menu->addAction(setExprAction);
        }
        if (dimensionHasExpression) {
            QAction* clearExprAction = new QAction(tr("Clear expression"), menu);
            QObject::connect( clearExprAction, SIGNAL(triggered()), this, SLOT(onClearExprActionTriggered()) );
            QList<QVariant> actionData;
            actionData.push_back((int)dimension);
            actionData.push_back((int)view);
            clearExprAction->setData(actionData);
            if (!isEnabled) {
                clearExprAction->setEnabled(false);
            }
            menu->addAction(clearExprAction);
        }
    }

    KnobHolderPtr holder = knob->getHolder();
    EffectInstancePtr isEffect = toEffectInstance(holder);

    // Add actions to split/unsplit views
    std::vector<std::string> projectViews;
    if (holder && holder->getApp()) {
        projectViews = holder->getApp()->getProject()->getProjectViewNames();
    }

    if (knob->canSplitViews()) {
        std::vector<QAction*> splitActions, unSplitActions;
        for (std::size_t i = 1; i < projectViews.size(); ++i) {
            std::list<ViewIdx>::iterator foundView = std::find(knobViews.begin(), knobViews.end(), ViewIdx(i));
            if (foundView == knobViews.end()) {
                QString label = tr("Split %1 View").arg(QString::fromUtf8(projectViews[i].c_str()));
                QAction* splitAction = new QAction(label, menu);
                QObject::connect( splitAction, SIGNAL(triggered()), this, SLOT(onSplitViewActionTriggered()) );
                splitAction->setData((int)i);
                if (!isEnabled) {
                    splitAction->setEnabled(false);
                }
                splitActions.push_back(splitAction);

            }
        }

        for (KnobGuiPrivate::PerViewWidgetsMap::iterator it = _imp->views.begin(); it != _imp->views.end(); ++it) {
            if (it->first == ViewIdx(0)) {
                continue;
            }
            QString label = tr("Unsplit %1 View").arg(it->second.viewLongName);
            QAction* unSplitAction = new QAction(label, menu);
            QObject::connect( unSplitAction, SIGNAL(triggered()), this, SLOT(onUnSplitViewActionTriggered()) );
            unSplitAction->setData((int)it->first);
            if (!isEnabled) {
                unSplitAction->setEnabled(false);
            }
            unSplitActions.push_back(unSplitAction);
        }

        if (!splitActions.empty() || !unSplitActions.empty()) {
            Menu* viewsMenu = new Menu(menu);
            viewsMenu->setTitle(tr("Multi-View"));
            if (!isEnabled) {
                viewsMenu->menuAction()->setEnabled(false);
            }
            for (std::vector<QAction*>::iterator it = splitActions.begin(); it!=splitActions.end(); ++it) {
                viewsMenu->addAction(*it);
            }
            if (!splitActions.empty() && !unSplitActions.empty()) {
                viewsMenu->addSeparator();
            }
            for (std::vector<QAction*>::iterator it = unSplitActions.begin(); it != unSplitActions.end(); ++it) {
                viewsMenu->addAction(*it);
            }
            menu->addAction(viewsMenu->menuAction());
        }
    } // canSplitViews

    ///find-out to which node that master knob belongs to

    NodeCollectionPtr collec;
    NodeGroupPtr isCollecGroup;
    if (isEffect) {
        collec = isEffect->getNode()->getGroup();
        isCollecGroup = toNodeGroup(collec);
    }


    if ( isAppKnob && ( ( hasSharedDimension && dimension.isAll() ) || !dimensionSharedKnobs.empty() ) ) {
        menu->addSeparator();

        std::string knobName;
        if (!dimensionSharedKnobs.empty()) {
            KnobIPtr firstSharedKnob = dimensionSharedKnobs.begin()->knob.lock();
            if (firstSharedKnob) {
                KnobHolderPtr masterHolder = firstSharedKnob->getHolder();
                assert(masterHolder);
                KnobTableItemPtr isTableItem = toKnobTableItem(masterHolder);
                EffectInstancePtr isEffect = toEffectInstance(masterHolder);
                if (isTableItem) {
                    knobName.append( isTableItem->getModel()->getNode()->getScriptName() );
                    knobName += '.';
                    knobName += isTableItem->getFullyQualifiedName();
                } else if (isEffect) {
                    knobName += isEffect->getScriptName_mt_safe();
                }




                knobName.append(".");
                knobName.append( firstSharedKnob->getName() );
                if (firstSharedKnob->getNDimensions() > 1) {
                    std::string dimName = firstSharedKnob->getDimensionName(dimensionSharedKnobs.begin()->dimension);
                    knobName.append(".");
                    knobName.append(dimName);
                }
            }
        }
        QString actionText;
        actionText.append( tr("Unlink") );
        if ( !knobName.empty() ) {
            actionText.append( QString::fromUtf8(" with ") );
            actionText.append( QString::fromUtf8( knobName.c_str() ) );
        }
        QAction* unlinkAction = new QAction(actionText, menu);
        QList<QVariant> actionData;
        actionData.push_back((int)dimension);
        actionData.push_back((int)view);
        unlinkAction->setData(actionData);
        QObject::connect( unlinkAction, SIGNAL(triggered()), this, SLOT(onUnlinkActionTriggered()) );
        menu->addAction(unlinkAction);
    }

    if ( isCollecGroup) {
        QAction* createMasterOnGroup = new QAction(tr("Create link on group"), menu);
        QObject::connect( createMasterOnGroup, SIGNAL(triggered()), this, SLOT(onCreateAliasOnGroupActionTriggered()) );
        menu->addAction(createMasterOnGroup);
    }
} // createAnimationMenu

KnobIPtr
KnobGui::createDuplicateOnNode(const EffectInstancePtr& effect,
                               bool makeAlias,
                               const KnobPagePtr& page,
                               const KnobGroupPtr& group,
                               int indexInParent)
{
    ///find-out to which node that master knob belongs to
    assert( getKnob()->getHolder()->getApp() );
    KnobIPtr knob = getKnob();

    if (!makeAlias) {
        std::list<ViewIdx> views = knob->getViewsList();
        for (int i = 0; i < knob->getNDimensions(); ++i) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                std::string expr = knob->getExpression(DimIdx(i), *it);
                if ( !expr.empty() ) {
                    StandardButtonEnum rep = Dialogs::questionDialog( tr("Expression").toStdString(), tr("This operation will create "
                                                                                                         "an expression link between this parameter and the new parameter on the group"
                                                                                                         " which will wipe the current expression(s).\n"
                                                                                                         "Continue anyway?").toStdString(), false,
                                                                     StandardButtons(eStandardButtonOk | eStandardButtonCancel) );
                    if (rep != eStandardButtonYes) {
                        return KnobIPtr();
                    }
                }
            }

        }
    }

    EffectInstancePtr isEffect = toEffectInstance( knob->getHolder() );
    if (!isEffect) {
        return KnobIPtr();
    }
    const std::string& nodeScriptName = isEffect->getNode()->getScriptName();
    std::string newKnobName = nodeScriptName +  knob->getName();
    KnobIPtr ret;
    try {
        ret = knob->createDuplicateOnHolder(effect,
                                            page,
                                            group,
                                            indexInParent,
                                            makeAlias ? KnobI::eDuplicateKnobTypeAlias : KnobI::eDuplicateKnobTypeExprLinked,
                                            newKnobName,
                                            knob->getLabel(),
                                            knob->getHintToolTip(),
                                            true /*refreshParamsGui*/,
                                            KnobI::eKnobDeclarationTypeUser);
    } catch (const std::exception& e) {
        Dialogs::errorDialog( tr("Error while creating parameter").toStdString(), e.what() );

        return KnobIPtr();
    }

    if (ret) {
        NodeGuiIPtr groupNodeGuiI = effect->getNode()->getNodeGui();
        NodeGuiPtr groupNodeGui = toNodeGui( groupNodeGuiI );
        assert(groupNodeGui);
        if (groupNodeGui) {
            groupNodeGui->ensurePanelCreated();
        }
    }
    effect->getApp()->triggerAutoSave();

    return ret;
} // KnobGui::createDuplicateOnNode

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGui.cpp"
