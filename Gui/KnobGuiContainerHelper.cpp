/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "KnobGuiContainerHelper.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QApplication>
#include <QUndoStack>
#include <QUndoCommand>
#include <QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"

#include "Gui/KnobGui.h"
#include "Gui/ClickableLabel.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/KnobItemsTableGui.h"
#include "Gui/NodeGui.h"
#include "Gui/Gui.h"
#include "Gui/GuiDefines.h"
#include "Gui/UndoCommand_qt.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/TabGroup.h"
#include "Gui/TableModelView.h"



NATRON_NAMESPACE_ENTER

struct KnobGuiContainerHelperPrivate
{
    KnobGuiContainerHelper* _p;
    KnobHolderWPtr holder;

    // Stores our KnobGui referred to by the internal Knob
    KnobsGuiMapping knobsMap;
    KnobPageGuiWPtr currentPage;
    PagesMap pages;
    KnobPageGuiPtr unPagedContainer; // used when container is not paged

    // When set as a dialog, this is a pointer to the group knob representing the dialog
    KnobGroupWPtr dialogKnob;

    boost::shared_ptr<QUndoStack> undoStack; /*!< undo/redo stack*/
    QUndoCommand* cmdBeingPushed;
    bool clearedStackDuringPush;
    boost::scoped_ptr<KnobGuiContainerSignalsHandler> signals;

    std::map<std::string, KnobItemsTableGuiPtr> tables;


    KnobGuiContainerHelperPrivate(KnobGuiContainerHelper* p,
                                  const KnobHolderPtr& holder,
                                  const boost::shared_ptr<QUndoStack>& stack)
    : _p(p)
    , holder(holder)
    , knobsMap()
    , currentPage()
    , pages()
    , unPagedContainer()
    , dialogKnob()
    , undoStack()
    , cmdBeingPushed(0)
    , clearedStackDuringPush(false)
    , signals( new KnobGuiContainerSignalsHandler(p) )
    , tables()
    {
        if (stack) {
            undoStack = stack;
        } else {
            undoStack = boost::make_shared<QUndoStack>();
        }
    }

    KnobGuiPtr createKnobGui(const KnobIPtr &knob);

    KnobsGuiMapping::iterator findKnobGui(const KnobIPtr& knob);

    void refreshPagesEnabledness();
};

KnobGuiContainerHelper::KnobGuiContainerHelper(const KnobHolderPtr& holder,
                                               const boost::shared_ptr<QUndoStack>& stack)
    : KnobGuiContainerI()
    , DockablePanelI()
    , _imp( new KnobGuiContainerHelperPrivate(this, holder, stack) )
{
}

KnobGuiContainerHelper::~KnobGuiContainerHelper()
{
    ///Delete the knob gui if they weren't before
    ///normally the onKnobDeletion() function should have cleared them
    for (KnobsGuiMapping::const_iterator it = _imp->knobsMap.begin(); it != _imp->knobsMap.end(); ++it) {
        if (it->second) {
            KnobIPtr knob = it->first.lock();
            it->second->setGuiRemoved();
        }
    }
}

const PagesMap&
KnobGuiContainerHelper::getPages() const
{
    return _imp->pages;
}

KnobGuiPtr
KnobGuiContainerHelper::getKnobGui(const KnobIPtr& knob) const
{
    for (KnobsGuiMapping::const_iterator it = _imp->knobsMap.begin(); it != _imp->knobsMap.end(); ++it) {
        if (it->first.lock() == knob) {
            return it->second;
        }
    }

    return KnobGuiPtr();
}

int
KnobGuiContainerHelper::getItemsSpacingOnSameLine() const
{
    return 15;
}

KnobGuiPtr
KnobGuiContainerHelperPrivate::createKnobGui(const KnobIPtr &knob)
{
    KnobsGuiMapping::iterator found = findKnobGui(knob);

    if ( found != knobsMap.end() ) {
        return found->second;
    }

    KnobGuiPtr ret = KnobGui::create(knob, KnobGui::eKnobLayoutTypePage, _p);
    knobsMap.push_back( std::make_pair(knob, ret) );

    return ret;
}

KnobsGuiMapping::iterator
KnobGuiContainerHelperPrivate::findKnobGui(const KnobIPtr& knob)
{
    for (KnobsGuiMapping::iterator it = knobsMap.begin(); it != knobsMap.end(); ++it) {
        if (it->first.lock() == knob) {
            return it;
        }
    }

    return knobsMap.end();
}

KnobPageGuiPtr
KnobGuiContainerHelper::getOrCreateDefaultPage()
{
    const std::vector<KnobIPtr> & knobs = getInternalKnobs();

    // Find in all knobs a page param to set this param into
    std::list<KnobPagePtr> pagesNotDeclaredByPlugin;
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobPagePtr p = toKnobPage( knobs[i]);
        if (p) {
            if (p->getKnobDeclarationType() == KnobI::eKnobDeclarationTypePlugin) {
                return getOrCreatePage(p);
            } else {
                pagesNotDeclaredByPlugin.push_back(p);
            }
        }
    }
    if (!pagesNotDeclaredByPlugin.empty()) {
        return getOrCreatePage(pagesNotDeclaredByPlugin.front());
    }
    // The plug-in didn't specify any page, it should have been caught before in Node::getOrCreateMainPage for nodes
    // Anyway create one.

    KnobHolderPtr holder = _imp->holder.lock();
    KnobPagePtr mainPage = holder->createKnob<KnobPage>("settingsPage");
    mainPage->setLabel(KnobHolder::tr("Settings"));
    if (mainPage) {
        return getOrCreatePage(mainPage);
    }

    return KnobPageGuiPtr();
}

KnobPageGuiPtr
KnobGuiContainerHelper::getCurrentPage() const
{
    return _imp->currentPage.lock();
}

void
KnobGuiContainerHelper::setCurrentPage(const KnobPageGuiPtr& curPage)
{
    _imp->currentPage = curPage;
    _imp->refreshPagesEnabledness();
}

KnobPageGuiPtr
KnobGuiContainerHelper::getOrCreatePage(const KnobPagePtr& page)
{

    if (!page || page->getIsToolBar()) {
        return KnobPageGuiPtr();
    }
    if ( !isPagingEnabled()) {
        return _imp->unPagedContainer;
    }

    // If the page is already created, return it
    assert(page);
    PagesMap::iterator found = _imp->pages.find(page);
    if ( found != _imp->pages.end() ) {
        return found->second;
    }


    QWidget* newTab;
    QWidget* layoutContainer;

    // The widget parent of the page
    QWidget* pagesContainer = getPagesContainer();
    assert(pagesContainer);

    // Check If the page main widget should be a scrollarea
    if ( useScrollAreaForTabs() ) {
        QScrollArea* sa = new QScrollArea(pagesContainer);
        layoutContainer = new QWidget(sa);
        layoutContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sa->setWidgetResizable(true);
        sa->setWidget(layoutContainer);
        newTab = sa;
    } else {
        // Otherwise let the derived class create the main widget
        newTab = createPageMainWidget(pagesContainer);
        layoutContainer = newTab;
    }

    // The container layout is always a grid layout
    QGridLayout *tabLayout = new QGridLayout(layoutContainer);
    layoutContainer->setLayout(tabLayout);
    tabLayout->setColumnStretch(1, 1);
    tabLayout->setSpacing( TO_DPIY(NATRON_FORM_LAYOUT_LINES_SPACING) );

    // Create the page gui
    KnobPageGuiPtr pageGui( new KnobPageGui() );
    pageGui->tab = newTab;
    pageGui->pageKnob = page;
    pageGui->groupAsTab = 0;
    pageGui->gridLayout = tabLayout;

    KnobSignalSlotHandlerPtr handler = page->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(labelChanged()), _imp->signals.get(), SLOT(onPageLabelChangedInternally()) );
    QObject::connect( handler.get(), SIGNAL(secretChanged()), _imp->signals.get(), SLOT(onPageSecretnessChanged()) );

    // Add the page to the container (most likely a tab widget)
    if (!page->getIsSecret()) {
        addPageToPagesContainer(pageGui);
    } else {
        newTab->hide();
    }

    pageGui->tab->setToolTip( QString::fromUtf8( page->getHintToolTip().c_str() ) );

    _imp->pages.insert( std::make_pair(page, pageGui) );

    return pageGui;
} // KnobGuiContainerHelper::getOrCreatePage

QPixmap
KnobGuiContainerHelper::getStandardIcon(QMessageBox::Icon icon,
                int size,
                QWidget* widget)
{
    QStyle *style = widget ? widget->style() : QApplication::style();
    QIcon tmpIcon;

    switch (icon) {
    case QMessageBox::Information:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxInformation, 0, widget);
        break;
    case QMessageBox::Warning:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxWarning, 0, widget);
        break;
    case QMessageBox::Critical:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxCritical, 0, widget);
        break;
    case QMessageBox::Question:
        tmpIcon = style->standardIcon(QStyle::SP_MessageBoxQuestion, 0, widget);
    default:
        break;
    }
    if ( !tmpIcon.isNull() ) {
        return tmpIcon.pixmap(size, size);
    }

    return QPixmap();
}



const KnobsVec&
KnobGuiContainerHelper::getInternalKnobs() const
{
    return _imp->holder.lock()->getKnobs();
}

const KnobsGuiMapping&
KnobGuiContainerHelper::getKnobsMapping() const
{
    return _imp->knobsMap;
}

std::list<KnobItemsTableGuiPtr>
KnobGuiContainerHelper::getAllKnobItemsTables() const
{
    std::list<KnobItemsTableGuiPtr> ret;
    for (std::map<std::string, KnobItemsTableGuiPtr>::const_iterator it = _imp->tables.begin(); it != _imp->tables.end(); ++it) {
        ret.push_back(it->second);
    }
    return ret;
}

KnobItemsTableGuiPtr
KnobGuiContainerHelper::getKnobItemsTable(const std::string& tableName) const
{
    std::map<std::string, KnobItemsTableGuiPtr>::const_iterator it = _imp->tables.find(tableName);
    if (it != _imp->tables.end()) {
        return it->second;
    }
    return KnobItemsTableGuiPtr();
}

void
KnobGuiContainerHelper::initializeKnobs()
{
    initializeKnobVector( _imp->holder.lock()->getKnobs() );
    _imp->refreshPagesEnabledness();
    refreshCurrentPage();

    onKnobsInitialized();

    if (!_imp->dialogKnob.lock()) {
        // Add the table if not done before
        KnobHolderPtr holder = _imp->holder.lock();
        std::list<KnobItemsTablePtr> tables = holder->getAllItemsTables();
        for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it != tables.end(); ++it) {
            const KnobItemsTablePtr& table = *it;

            std::string tableName = table->getTableIdentifier();
            std::map<std::string, KnobItemsTableGuiPtr>::iterator foundGuiTable = _imp->tables.find(tableName);
            if (foundGuiTable != _imp->tables.end()) {
                continue;
            }

            std::string previousKnobTableName = holder->getItemsTablePreviousKnobScriptName(tableName);
            KnobHolder::KnobItemsTablePositionEnum knobTablePosition = holder->getItemsTablePosition(tableName);
            switch (knobTablePosition) {
                case KnobHolder::eKnobItemsTablePositionAfterKnob: {
                    KnobIPtr foundKnob = holder->getKnobByName(previousKnobTableName);
                    KnobPagePtr page = toKnobPage(foundKnob);
                    KnobPageGuiPtr guiPage;
                    if (!page) {
                        // Look for the first page available
                        if (!_imp->pages.empty()) {
                            guiPage = _imp->pages.begin()->second;
                        }
                    } else {
                        PagesMap::const_iterator found = _imp->pages.find(page);
                        if (found != _imp->pages.end()) {
                            guiPage = found->second;
                        }
                    }
                    if (guiPage) {
                        KnobItemsTableGuiPtr guiTable = createKnobItemsTable(table, guiPage->tab);
                        guiTable->addWidgetsToLayout(guiPage->gridLayout);
                        _imp->tables.insert(std::make_pair(tableName, guiTable));
                    }

                }   break;
                case KnobHolder::eKnobItemsTablePositionBottomOfAllPages: {
                    QWidget* container = getMainContainer();
                    QLayout* mainLayout = getMainContainerLayout();
                    KnobItemsTableGuiPtr guiTable = createKnobItemsTable(table, container);
                    guiTable->addWidgetsToLayout(mainLayout);
                    _imp->tables.insert(std::make_pair(tableName, guiTable));
                }   break;
            }

            KnobsVec tableControlKnobs = table->getTableControlKnobs();
            initializeKnobVectorInternal(tableControlKnobs, 0);
        }

    }
} // initializeKnobs

void
KnobGuiContainerHelper::initializeKnobVectorInternal(const KnobsVec& siblingsVec,
                                                     KnobsVec* regularKnobsVec)
{


    // Hold a pointer to the previous child if the previous child did not want to create a new line
    for (KnobsVec::const_iterator it2 = siblingsVec.begin(); it2 != siblingsVec.end(); ++it2) {

        // Create this knob
        KnobGuiPtr newGui = findKnobGuiOrCreate(*it2);
        (void)newGui;

        // Remove it from the "regularKnobs" to mark it as created as we will use them later
        if (regularKnobsVec) {
            KnobsVec::iterator foundRegular = std::find(regularKnobsVec->begin(), regularKnobsVec->end(), *it2);
            if ( foundRegular != regularKnobsVec->end() ) {
                regularKnobsVec->erase(foundRegular);
            }
        }
    }
} // KnobGuiContainerHelper::initializeKnobVectorInternal

void
KnobGuiContainerHelper::initializeKnobVector(const KnobsVec& knobs)
{
    // Extract pages first and initialize them recursively
    // All other knobs are put in regularKnobs
    std::list<KnobPagePtr> pages;
    KnobsVec regularKnobs;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobPagePtr isPage = toKnobPage(knobs[i]);
        if (isPage) {
            if (!isPage->getIsToolBar()) {
                pages.push_back(isPage);
            }
        } else {
            regularKnobs.push_back(knobs[i]);
        }
    }
    for (std::list<KnobPagePtr>::iterator it = pages.begin(); it != pages.end(); ++it) {
        // Create page
        KnobGuiPtr knobGui = findKnobGuiOrCreate(*it);
        Q_UNUSED(knobGui);

        // Create its children
        KnobsVec children = (*it)->getChildren();
        initializeKnobVectorInternal(children, &regularKnobs);
    }

    // Remove from the regularKnobs left the knobs that want to be with the knobsTable (if any)
    std::list<KnobItemsTablePtr> tables = _imp->holder.lock()->getAllItemsTables();

    KnobsVec tmp;
    for (KnobsVec::const_iterator it = regularKnobs.begin(); it != regularKnobs.end(); ++it) {
        bool isTableControl = false;
        for (std::list<KnobItemsTablePtr>::const_iterator it2 = tables.begin(); it2 != tables.end(); ++it2) {
            if ((*it2)->isTableControlKnob(*it)) {
                isTableControl = true;
                break;
            }

        }
        if (!isTableControl) {
            tmp.push_back(*it);
        }
    }
    regularKnobs = tmp;


    // For knobs that did not belong to a page,  create them
    initializeKnobVectorInternal(regularKnobs, 0);
    refreshTabWidgetMaxHeight();
}

static void
workAroundGridLayoutBug(QGridLayout* layout)
{
    // See http://stackoverflow.com/questions/14033902/qt-qgridlayout-automatically-centers-moves-items-to-the-middle for
    // a bug of QGridLayout: basically all items are centered, but we would like to add stretch in the bottom of the layout.
    // To do this we add an empty widget with an expanding vertical size policy.
    QWidget* foundSpacer = 0;

    for (int i = 0; i < layout->rowCount(); ++i) {
        QLayoutItem* item = layout->itemAtPosition(i, 0);
        if (!item) {
            continue;
        }
        QWidget* w = item->widget();
        if (!w) {
            continue;
        }
        if ( w->objectName() == QString::fromUtf8("emptyWidget") ) {
            foundSpacer = w;
            break;
        }
    }
    if (foundSpacer) {
        layout->removeWidget(foundSpacer);
    } else {
        foundSpacer = new QWidget( layout->parentWidget() );
        foundSpacer->setObjectName( QString::fromUtf8("emptyWidget") );
        foundSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    ///And add our stretch
    layout->addWidget(foundSpacer, layout->rowCount(), 0, 1, 2);
}

bool
KnobGuiContainerHelper::setLabelFromTextAndIcon(KnobClickableLabel* widget, const QString& labelText, const QString& labelIconFilePath, bool setBold)
{
    bool pixmapSet = false;
    if ( !labelIconFilePath.isEmpty() ) {
        QPixmap pix;

        //QFontMetrics fm(widget->font(), 0);
        int pixSize = TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE);

        if (labelIconFilePath == QLatin1String("dialog-warning")) {
            pix = getStandardIcon(QMessageBox::Warning, pixSize, widget);
        } else if (labelIconFilePath == QLatin1String("dialog-question")) {
            pix = getStandardIcon(QMessageBox::Question, pixSize, widget);
        } else if (labelIconFilePath == QLatin1String("dialog-error")) {
            pix = getStandardIcon(QMessageBox::Critical, pixSize, widget);
        } else if (labelIconFilePath == QLatin1String("dialog-information")) {
            pix = getStandardIcon(QMessageBox::Information, pixSize, widget);
        } else {
            pix.load(labelIconFilePath);
            if (pix.width() != pixSize) {
                pix = pix.scaled(pixSize, pixSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
        }
        if ( !pix.isNull() ) {
            pixmapSet = true;
            widget->setPixmap(pix);
            return true;
        } else {
            return false;
        }
    }
    if (!pixmapSet) {
        /*labelStr += ":";*/
        if ( setBold ) {
            widget->setIsBold(true);
        }
        widget->setText(labelText );
        return !labelText.isEmpty();
    }
    return false;

}

void
KnobGuiContainerHelper::createTabedGroupGui(const KnobGroupPtr& knob)
{
    KnobIPtr parentKnob = knob->getParentKnob();
    assert(parentKnob);

    KnobPagePtr parentIsPage = toKnobPage(parentKnob);
    if (parentIsPage) {

        KnobPageGuiPtr page;
        if (isPagingEnabled()) {
            page = getOrCreatePage(parentIsPage);
        } else {
            page = _imp->unPagedContainer;
        }

        // Create the frame for the group that are set as tabs within this tab
        bool existed = true;
        if (!page->groupAsTab) {
            existed = false;
            page->groupAsTab = new TabGroup( getPagesContainer() );
        }
        page->groupAsTab->addTab( knob, QString::fromUtf8( knob->getLabel().c_str() ) );
        if (!existed) {
            page->gridLayout->addWidget(page->groupAsTab, page->gridLayout->rowCount(), 0, 1, 2);
        }

        page->groupAsTab->refreshTabSecretNess( knob );
    } else {

        boost::shared_ptr<KnobGuiGroup> parentGroupGui;
        KnobGuiPtr parentKnobGui;
        KnobGroupPtr parentIsGroup = toKnobGroup(parentKnob);
        // If this knob is within a group, make sure the group is created so far
        if (parentIsGroup) {
            parentKnobGui = findKnobGuiOrCreate(parentKnob);
            if (parentKnobGui) {
                parentGroupGui = boost::dynamic_pointer_cast<KnobGuiGroup>(parentKnobGui->getWidgetsForView(ViewIdx(0)));
            }
        }

        // This is a group inside a group
        if (parentKnobGui) {
            TabGroup* groupAsTab = parentKnobGui->getOrCreateTabWidget();
            assert(groupAsTab);
            groupAsTab->addTab( knob, QString::fromUtf8( knob->getLabel().c_str() ) );
            if ( parentIsGroup && parentIsGroup->isTab() ) {
                // Insert the tab in the layout of the parent
                // Find the page in the parentParent group

                KnobIPtr parentParent = parentKnob->getParentKnob();
                assert(parentParent);
                KnobGroupPtr parentParentIsGroup = toKnobGroup(parentParent);
                KnobPagePtr parentParentIsPage = toKnobPage(parentParent);
                assert(parentParentIsGroup || parentParentIsPage);
                TabGroup* parentTabGroup = 0;
                if (parentParentIsPage) {
                    KnobPageGuiPtr page = getOrCreatePage(parentParentIsPage);
                    assert(page);
                    parentTabGroup = page->groupAsTab;
                } else {
                    KnobsGuiMapping::iterator it = _imp->findKnobGui(parentParent);
                    assert( it != _imp->knobsMap.end() );
                    if (it != _imp->knobsMap.end() && it->second) {
                        parentTabGroup = it->second->getOrCreateTabWidget();
                    }
                }

                QGridLayout* layout = parentTabGroup->addTab( parentIsGroup, QString::fromUtf8( parentIsGroup->getLabel().c_str() ) );
                assert(layout);
                layout->addWidget(groupAsTab, 0, 0, 1, 2);
            } else {
                KnobPagePtr topLevelPage = knob->getTopLevelPage();
                KnobPageGuiPtr page = getOrCreatePage(topLevelPage);
                assert(page);
                page->gridLayout->addWidget(groupAsTab, page->gridLayout->rowCount(), 0, 1, 2);
            }
            groupAsTab->refreshTabSecretNess(knob);
        }
    } // parentIsPage
} // createTabedGroupGui

KnobGuiPtr
KnobGuiContainerHelper::findKnobGuiOrCreate(const KnobIPtr &knob)
{
    assert(knob);

    // Groups and Pages have special cases in the following code as they are containers
    KnobGroupPtr isGroup = toKnobGroup(knob);
    KnobPagePtr isPage = toKnobPage(knob);

    // Is this knob already described in the gui ?
    for (KnobsGuiMapping::const_iterator it = _imp->knobsMap.begin(); it != _imp->knobsMap.end(); ++it) {
        if ( (it->first.lock() == knob) && it->second ) {
            if (isPage) {
                return it->second;
            } else if ( isGroup && ( ( !isGroup->isTab() && it->second->hasWidgetBeenCreated() ) || isGroup->isTab() ) ) {
                return it->second;
            } else if ( it->second->hasWidgetBeenCreated() ) {
                return it->second;
            } else {
                break;
            }
        }
    }


    // For a page, create it if needed and recursively describe its children
    if (isPage) {
        if ( isPage->getChildren().empty() ) {
            return KnobGuiPtr();
        }
        getOrCreatePage(isPage);
        KnobsVec children = isPage->getChildren();
        initializeKnobVector(children);

        return KnobGuiPtr();
    }

    // A group knob that is as a dialog should never be created as a knob.
    if (isGroup && isGroup->getIsDialog()) {
        return KnobGuiPtr();
    }

    // If setup to display the content of a group as a dialog, only create the knobs within that group.
    KnobGroupPtr thisContainerIsGroupDialog = _imp->dialogKnob.lock();
    if (thisContainerIsGroupDialog) {
        if (knob->getParentKnob() != thisContainerIsGroupDialog) {
            return KnobGuiPtr();
        }
    }

    // A knob that is a table control (e.g: "Add Item" "Remove item") does not literally belong to a page and is
    // dependent of the location of the table
    std::list<KnobItemsTablePtr> tables = _imp->holder.lock()->getAllItemsTables();
    KnobItemsTableGuiPtr isKnobTableControl;
    for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it != tables.end(); ++it) {
        if ((*it)->isTableControlKnob(knob)) {
            isKnobTableControl = getKnobItemsTable((*it)->getTableIdentifier());
            break;
        }

    }

    KnobIPtr parentKnob = knob->getParentKnob();

    // If this assert triggers, that means a knob was not added to a KnobPage.
    if (!isKnobTableControl && !parentKnob && isPagingEnabled()) {
        qDebug() << knob->getName().c_str() << "does not belong to any page";
    }

    KnobGroupPtr parentIsGroup = toKnobGroup(parentKnob);

    // If the parent group is a dialog, never create the knob unless we are creating this container specifically to display
    // the content of the group
    if (parentIsGroup && parentIsGroup->getIsDialog() && parentIsGroup != thisContainerIsGroupDialog) {
        return KnobGuiPtr();
    }


    // Create the actual knob gui object
    KnobGuiPtr ret = _imp->createKnobGui(knob);
    if (!ret) {
        return KnobGuiPtr();
    }
    assert(!ret->hasWidgetBeenCreated());


    // So far the knob could have no parent, in which case we force it to be in the default page.
    if (!parentKnob && !isKnobTableControl) {
        KnobPageGuiPtr defPage = getOrCreateDefaultPage();
        defPage->pageKnob.lock()->addKnob(knob);
        parentKnob = defPage->pageKnob.lock();
        assert(parentKnob);

    }


    if ( isGroup  && isGroup->isTab() ) {
        // A group set as a tab must add a tab to the parent tabwidget
        createTabedGroupGui(isGroup);
        KnobsVec children = isGroup->getChildren();
        initializeKnobVector(children);
        return ret;
    }

    // Get the top level parent page
    KnobPageGuiPtr page;
    if (!isKnobTableControl) {
        KnobPagePtr isTopLevelParentAPage = toKnobPage(parentKnob);

        KnobIPtr parentKnobTmp = parentKnob;
        while (parentKnobTmp && !isTopLevelParentAPage) {
            parentKnobTmp = parentKnobTmp->getParentKnob();
            if (parentKnobTmp) {
                isTopLevelParentAPage = toKnobPage(parentKnobTmp);
            }
        }
        assert(isTopLevelParentAPage);

        page = getOrCreatePage(isTopLevelParentAPage);
    }

    if (!page && !isKnobTableControl) {
        return ret;
    }

    // Retrieve the main grid layout of the page
    QGridLayout* gridLayout = 0;
    QBoxLayout* boxLayout = 0;
    QLayout* knobTablesLayout = 0;

    if (!isKnobTableControl) {
        gridLayout = page->gridLayout;
    } else {
        // For a table control, retrieve the layout on the knobs table itself.
        // It can be either a grid or box layout
        knobTablesLayout = isKnobTableControl->getContainerLayout();
        gridLayout = dynamic_cast<QGridLayout*>(knobTablesLayout);
        boxLayout = dynamic_cast<QBoxLayout*>(knobTablesLayout);
    }

    /*
     * Find out in which layout the knob should be: either in the layout of the page or in the layout of
     * the nearest parent group tab in the hierarchy
     */
    KnobGroupPtr closestParentGroupTab;
    if (!isKnobTableControl) {
        KnobIPtr parentTmp = parentKnob;
        while (!closestParentGroupTab && parentTmp) {
            KnobGroupPtr parentGroup = toKnobGroup(parentTmp);
            if ( parentGroup && parentGroup->isTab() ) {
                closestParentGroupTab = parentGroup;
            }
            parentTmp = parentTmp->getParentKnob();
            if (!parentTmp) {
                break;
            }
        }
    }

    if (closestParentGroupTab && !isKnobTableControl) {
        /*
         * At this point we know that the parent group (which is a tab in the TabWidget) will have at least 1 knob
         * so ensure it is added to the TabWidget.
         * There are 2 possibilities, either the parent of the group tab is another group, in which case we have to
         * make sure the TabWidget is visible in the parent TabWidget of the group, otherwise we just add the TabWidget
         * to the on of the page.
         */

        KnobIPtr parentParent = closestParentGroupTab->getParentKnob();
        KnobGroupPtr parentParentIsGroup = toKnobGroup(parentParent);
        KnobPagePtr parentParentIsPage = toKnobPage(parentParent);

        assert(parentParentIsGroup || parentParentIsPage);
        if (parentParentIsGroup) {
            KnobGuiPtr parentParentGui = findKnobGuiOrCreate(parentParent);
            assert(parentParentGui);
            if (parentParentGui) {
                TabGroup* groupAsTab = parentParentGui->getOrCreateTabWidget();
                assert(groupAsTab);
                gridLayout = groupAsTab->addTab( closestParentGroupTab, QString::fromUtf8( closestParentGroupTab->getLabel().c_str() ) );
                groupAsTab->refreshTabSecretNess(closestParentGroupTab);
            }
        } else if (parentParentIsPage) {
            KnobPageGuiPtr page = getOrCreatePage(parentParentIsPage);
            assert(page);
            assert(page->groupAsTab);
            gridLayout = page->groupAsTab->addTab( closestParentGroupTab, QString::fromUtf8( closestParentGroupTab->getLabel().c_str() ) );
            page->groupAsTab->refreshTabSecretNess(closestParentGroupTab);
        }
        assert(gridLayout);
    }

    // Create the actual gui for the knob. If there was a knob decribed previously that had the onNewLine flag
    // set to false, the knob will already be added to the layout
    {
        QWidget* parentWidget = 0;
        if (isKnobTableControl) {
            parentWidget = knobTablesLayout->parentWidget();
        } else {
            assert(page);
            parentWidget = page->tab;
        }
        ret->createGUI(parentWidget);
    }


    // Add the knob to the layotu
    if (ret->isOnNewLine()) {

        const bool labelOnSameColumn = ret->isLabelOnSameColumn();
        Qt::Alignment labelAlignment;
        Qt::Alignment fieldAlignment;

        if (isGroup) {
            labelAlignment = Qt::AlignLeft;
        } else {
            labelAlignment = Qt::AlignRight;
        }

        QWidget* labelContainer = ret->getLabelContainer();
        QWidget* fieldContainer = ret->getFieldContainer();

        if (gridLayout) {
            int rowIndex = gridLayout->rowCount();
            if (!labelContainer) {
                gridLayout->addWidget(fieldContainer, rowIndex, 0, 1, 2);
            } else {
                if (labelOnSameColumn) {
                    labelContainer->layout()->addWidget(fieldContainer);
                    gridLayout->addWidget(labelContainer, rowIndex, 0, 1, 2);
                } else {
                    gridLayout->addWidget(labelContainer, rowIndex, 0, 1, 1, labelAlignment);
                    gridLayout->addWidget(fieldContainer, rowIndex, 1, 1, 1);
                }
            }
            workAroundGridLayoutBug(gridLayout);

        } else if (boxLayout) {
            // Add both label + content to a horizontal wifget and add them to the box layout
            QWidget* container = new QWidget(boxLayout->parentWidget());
            QHBoxLayout* hLayout = new QHBoxLayout(container);
            hLayout->setContentsMargins(0, 0, 0, 0);
            hLayout->setSpacing(0);
            if (labelContainer) {
                hLayout->addWidget(labelContainer);
            }

            hLayout->addWidget(fieldContainer);
            boxLayout->addWidget(container);
            hLayout->addStretch();
        }


    }

    // If this knob is within a group, add this KnobGui to the KnobGroupGui

    {
        boost::shared_ptr<KnobGuiGroup> parentGroupGui;
        KnobGuiPtr parentKnobGui;
        if (parentIsGroup) {
            parentKnobGui = findKnobGuiOrCreate(parentKnob);
            if (parentKnobGui) {
                parentGroupGui = boost::dynamic_pointer_cast<KnobGuiGroup>(parentKnobGui->getWidgetsForView(ViewIdx(0)));
            }
        }
        if (parentGroupGui) {
            parentGroupGui->addKnob(ret);
        }
    }

    // If the node wants a KnobItemsTable after this knob, create it
    KnobHolderPtr holder = _imp->holder.lock();

    for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it != tables.end(); ++it) {
        const KnobItemsTablePtr& table = *it;
        std::string tableName = table->getTableIdentifier();
        std::map<std::string, KnobItemsTableGuiPtr>::iterator foundGuiTable = _imp->tables.find(tableName);
        if (foundGuiTable != _imp->tables.end()) {
            continue;
        }

        std::string previousKnobTableName = holder->getItemsTablePreviousKnobScriptName(tableName);
        KnobHolder::KnobItemsTablePositionEnum knobTablePosition = holder->getItemsTablePosition(tableName);

        if (previousKnobTableName == knob->getName() && knobTablePosition == KnobHolder::eKnobItemsTablePositionAfterKnob) {
            KnobItemsTableGuiPtr guiTable = createKnobItemsTable(table, page->tab);
            guiTable->addWidgetsToLayout(gridLayout);
            _imp->tables.insert(std::make_pair(tableName, guiTable));
            KnobsVec tableControlKnobs = table->getTableControlKnobs();
            initializeKnobVectorInternal(tableControlKnobs, 0);
        }

    }

    // If the knob is a group, create all the children now recursively
    if (isGroup) {
        KnobsVec children = isGroup->getChildren();
        initializeKnobVector(children);
    }

    return ret;
} // findKnobGuiOrCreate

QWidget*
KnobGuiContainerHelper::createKnobHorizontalFieldContainer(QWidget* parent) const
{
    return new QWidget(parent);
}

void
KnobGuiContainerHelper::deleteKnobGui(const KnobIPtr& knob)
{
    KnobPagePtr isPage = toKnobPage(knob);

    if ( isPage && isPagingEnabled() ) {
        // Remove the page and all its children
        PagesMap::iterator found = _imp->pages.find(isPage);
        if ( found != _imp->pages.end() ) {
            _imp->refreshPagesEnabledness();

            KnobsVec children = isPage->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                deleteKnobGui(children[i]);
            }

            found->second->tab->deleteLater();
            _imp->pages.erase(found);
        }
    } else {
        // This is not a page or paging is disabled

        KnobGroupPtr isGrp = toKnobGroup(knob);
        if (isGrp) {
            KnobsVec children = isGrp->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                deleteKnobGui(children[i]);
            }
        }
        if ( isGrp && isGrp->isTab() ) {
            //find parent page
            KnobIPtr parent = knob->getParentKnob();
            KnobPagePtr isParentPage = toKnobPage(parent);
            KnobGroupPtr isParentGroup = toKnobGroup(parent);

            assert(isParentPage || isParentGroup);
            if (isParentPage) {
                PagesMap::iterator page = _imp->pages.find(isParentPage);
                assert( page != _imp->pages.end() );
                TabGroup* groupAsTab = page->second->groupAsTab;
                if (groupAsTab) {
                    groupAsTab->removeTab(isGrp);
                    if ( groupAsTab->isEmpty() ) {
                        delete page->second->groupAsTab;
                        page->second->groupAsTab = 0;
                    }
                }
            } else if (isParentGroup) {
                KnobsGuiMapping::iterator found  = _imp->findKnobGui(knob);
                assert( found != _imp->knobsMap.end() );
                if (found != _imp->knobsMap.end() && found->second) {
                    TabGroup* groupAsTab = found->second->getOrCreateTabWidget();
                    if (groupAsTab) {
                        groupAsTab->removeTab(isGrp);
                        if ( groupAsTab->isEmpty() ) {
                            found->second->removeTabWidget();
                        }
                    }
                }
            }

            KnobsGuiMapping::iterator it  = _imp->findKnobGui(knob);
            if ( it != _imp->knobsMap.end() ) {
                _imp->knobsMap.erase(it);
            }
        } else {
            KnobsGuiMapping::iterator it  = _imp->findKnobGui(knob);
            if ( it != _imp->knobsMap.end() ) {
                it->second->destroyWidgetsLater();
                _imp->knobsMap.erase(it);
            }
        }
    }
} // KnobGuiContainerHelper::deleteKnobGui

void
KnobGuiContainerHelper::refreshGuiForKnobsChanges(bool restorePageIndex)
{
    KnobPageGuiPtr curPage;

    if ( isPagingEnabled() ) {
        if (restorePageIndex) {
            curPage = getCurrentPage();
        }
    }

    // Delete all knobs gui
    {
        KnobsGuiMapping mapping = _imp->knobsMap;
        _imp->knobsMap.clear();
        for (KnobsGuiMapping::iterator it = mapping.begin(); it != mapping.end(); ++it) {
            assert(it->second);
            it->second->destroyWidgetsLater();
            it->second.reset();
        }
    }

    // Now delete all pages
    for (PagesMap::iterator it = _imp->pages.begin(); it != _imp->pages.end(); ++it) {
        removePageFromContainer(it->second);
        it->second->tab->deleteLater();
    }
    _imp->pages.clear();

    //Clear undo/Redo stack so that KnobGui pointers are not lying around
    clearUndoRedoStack();

    recreateKnobsInternal(curPage, restorePageIndex);

    _imp->refreshPagesEnabledness();

}

void
KnobGuiContainerHelper::recreateViewerUIKnobs()
{
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    EffectInstancePtr isEffect = toEffectInstance(_imp->holder.lock());
    if (!isEffect) {
        return;
    }

    NodePtr thisNode = isEffect->getNode();
    if (!thisNode) {
        return;
    }
    NodeGuiPtr thisNodeGui = boost::dynamic_pointer_cast<NodeGui>(thisNode->getNodeGui());
    if (!thisNodeGui) {
        return;
    }
    NodeGuiPtr currentViewerInterface = gui->getCurrentNodeViewerInterface(thisNode->getPlugin());

    if (thisNodeGui->getNode()->isEffectViewerNode()) {
        gui->removeViewerInterface(thisNodeGui, true);
    } else {
        gui->removeNodeViewerInterface(thisNodeGui, true);
    }
    gui->createNodeViewerInterface(thisNodeGui);
    if (currentViewerInterface) {
        gui->setNodeViewerInterface(currentViewerInterface);
    }

}

void
KnobGuiContainerHelperPrivate::refreshPagesEnabledness()
{
    KnobPageGuiPtr curPage = _p->getCurrentPage();

    for (PagesMap::const_iterator it = pages.begin(); it != pages.end(); ++it) {
        KnobPagePtr page = it->second->pageKnob.lock();
        if (it->second == curPage) {
            if ( !page->isEnabled() ) {
                page->setEnabled(true);
                page->evaluateValueChange(DimSpec::all(), page->getCurrentRenderTime(), ViewSetSpec::all(), eValueChangedReasonUserEdited);
            }
        } else {
            if ( page->isEnabled() ) {
                page->setEnabled(false);
                page->evaluateValueChange(DimSpec::all(), page->getCurrentRenderTime(), ViewSetSpec::all(), eValueChangedReasonUserEdited);
            }
        }
    }
}

void
KnobGuiContainerHelper::refreshPagesOrder(const KnobPageGuiPtr& curTabName,
                                          bool restorePageIndex)
{
    if ( !isPagingEnabled() ) {
        return;
    }
    std::list<KnobPageGuiPtr> orderedPages;
    const KnobsVec& knobs = getInternalKnobs();
    std::list<KnobPagePtr> internalPages;
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr isPage = toKnobPage(*it);
        if (isPage && !isPage->getIsSecret()) {
            internalPages.push_back(isPage);
        }
    }

    for (std::list<KnobPagePtr>::iterator it = internalPages.begin(); it != internalPages.end(); ++it) {
        PagesMap::const_iterator foundPage = _imp->pages.find(*it);
        if ( foundPage != _imp->pages.end() ) {
            orderedPages.push_back(foundPage->second);
        }
    }

    setPagesOrder(orderedPages, curTabName, restorePageIndex);
}

void
KnobGuiContainerHelper::recreateKnobsInternal(const KnobPageGuiPtr& curPage,
                                              bool restorePageIndex)
{
    // Re-create knobs
    const KnobsVec& knobs = getInternalKnobs();

    initializeKnobVector(knobs);

    refreshPagesOrder(curPage, restorePageIndex);
    refreshCurrentPage();
    recreateViewerUIKnobs();


    onKnobsRecreated();
}

void
KnobGuiContainerHelper::recreateUserKnobs(bool restorePageIndex)
{
    const KnobsVec& knobs = getInternalKnobs();
    std::list<KnobPagePtr> userPages;

    getUserPages(userPages);

    KnobPageGuiPtr curPage;
    if ( isPagingEnabled() ) {
        if (restorePageIndex) {
            curPage = getCurrentPage();
        }

        for (std::list<KnobPagePtr>::iterator it = userPages.begin(); it != userPages.end(); ++it) {
            deleteKnobGui( (*it)->shared_from_this() );
        }
    } else {
        for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            deleteKnobGui(*it);
        }
    }
    recreateKnobsInternal(curPage, restorePageIndex);
}

void
KnobGuiContainerHelper::getUserPages(std::list<KnobPagePtr>& userPages) const
{
    return _imp->holder.lock()->getUserPages(userPages);
}

void
KnobGuiContainerHelper::setPageActiveIndex(const KnobPagePtr& page)
{
    PagesMap::const_iterator foundPage = _imp->pages.find(page);

    if ( foundPage == _imp->pages.end() ) {
        return;
    }
    _imp->refreshPagesEnabledness();

    onPageActivated(foundPage->second);
}


int
KnobGuiContainerHelper::getPagesCount() const
{
    return getPages().size();
}

const QUndoCommand*
KnobGuiContainerHelper::getLastUndoCommand() const
{
    return _imp->undoStack->command(_imp->undoStack->index() - 1);
}



void
KnobGuiContainerHelper::pushUndoCommand(const UndoCommandPtr& command)
{
    if (!_imp->undoStack) {
        command->redo();
    } else {
        pushUndoCommand( new UndoCommand_qt(command) );
    }
}

void
KnobGuiContainerHelper::pushUndoCommand(UndoCommand* command)
{
    UndoCommandPtr p(command);
    pushUndoCommand(p);
}


void
KnobGuiContainerHelper::pushUndoCommand(QUndoCommand* cmd)
{
    if ( !getGui() ) {
        delete cmd;

        return;
    }
    _imp->undoStack->setActive();
    _imp->cmdBeingPushed = cmd;
    _imp->clearedStackDuringPush = false;
    _imp->undoStack->push(cmd);

    //We may be in a situation where the command was not pushed because the stack was cleared
    if (!_imp->clearedStackDuringPush) {
        _imp->cmdBeingPushed = 0;
    }
    refreshUndoRedoButtonsEnabledNess( _imp->undoStack->canUndo(), _imp->undoStack->canRedo() );
}

QUndoStackPtr
KnobGuiContainerHelper::getUndoStack() const
{
    return _imp->undoStack;
}

void
KnobGuiContainerHelper::clearUndoRedoStack()
{
    if (_imp->undoStack) {
        _imp->undoStack->clear();
        _imp->clearedStackDuringPush = true;
        _imp->signals->s_deleteCurCmdLater();
        refreshUndoRedoButtonsEnabledNess( _imp->undoStack->canUndo(), _imp->undoStack->canRedo() );
    }
}

void
KnobGuiContainerSignalsHandler::onDeleteCurCmdLaterTriggered()
{
    _container->onDeleteCurCmdLater();
}

void
KnobGuiContainerSignalsHandler::onPageSecretnessChanged()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (!handler) {
        return;
    }
    KnobPage* isPage = dynamic_cast<KnobPage*>(handler->getKnob().get());
    if (!isPage) {
        return;
    }
    const PagesMap& pages = _container->getPages();
    for (PagesMap::const_iterator it = pages.begin(); it!=pages.end(); ++it) {
        if (it->first.lock().get() == isPage) {
            if (isPage->getIsSecret()) {
                it->second->tab->setVisible(false);
                _container->removePageFromContainer(it->second);
            } else {
                _container->addPageToPagesContainer(it->second);
                it->second->tab->setVisible(true);
            }

            break;
        }
    }
}

void
KnobGuiContainerHelper::onDeleteCurCmdLater()
{
    if (_imp->cmdBeingPushed) {
        _imp->undoStack->clear();
        _imp->cmdBeingPushed = 0;
    }
}

void
KnobGuiContainerSignalsHandler::onPageLabelChangedInternally()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    if (!handler) {
        return;
    }
    KnobIPtr knob = handler->getKnob();
    KnobPagePtr isPage = toKnobPage(knob);
    if (!isPage) {
        return;
    }
    const PagesMap& pages = _container->getPages();
    PagesMap::const_iterator found = pages.find(isPage);
    if ( found != pages.end() ) {
        _container->onPageLabelChanged(found->second);
    }
}


void
KnobGuiContainerHelper::refreshPageVisibility(const KnobPagePtr& page)
{
    // When all knobs of a page are hidden, if the container is a tabwidget, hide the tab

    QTabWidget* isTabWidget = dynamic_cast<QTabWidget*>(getPagesContainer());
    if (!isTabWidget) {
        return;
    }



    const PagesMap& pages = getPages();

    std::list<KnobPageGuiPtr> pagesToDisplay;
    for (int i = 0; i < isTabWidget->count(); ++i) {
        QWidget* w = isTabWidget->widget(i);
        for (PagesMap::const_iterator it = pages.begin(); it!=pages.end(); ++it) {
            if (it->second->tab == w) {
                if (it->first.lock() != page) {
                    pagesToDisplay.push_back(it->second);
                } else {
                    KnobsVec children = page->getChildren();
                    bool visible = false;
                    for (KnobsVec::const_iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                        visible |= !(*it2)->getIsSecret();
                    }
                    if (visible) {
                        pagesToDisplay.push_back(it->second);
                    }
                }
                break;
            }
        }
    }

    KnobPageGuiPtr curPage = getCurrentPage();
    setPagesOrder(pagesToDisplay, curPage, true);
}

void
KnobGuiContainerHelper::setAsDialogForGroup(const KnobGroupPtr& groupKnobDialog)
{
    assert(groupKnobDialog->getIsDialog());
    turnOffPages();
    _imp->dialogKnob = groupKnobDialog;
}

KnobGroupPtr
KnobGuiContainerHelper::isDialogForGroup() const
{
    return _imp->dialogKnob.lock();
}

void
KnobGuiContainerHelper::turnOffPages()
{
    if (_imp->unPagedContainer) {
        // turnOffPages() was already called.
        return;
    }
    onPagingTurnedOff();

    _imp->unPagedContainer.reset(new KnobPageGui);
    _imp->unPagedContainer->tab = new QWidget(getPagesContainer());
    _imp->unPagedContainer->gridLayout = new QGridLayout(_imp->unPagedContainer->tab);
    _imp->unPagedContainer->gridLayout->setColumnStretch(1, 1);
    _imp->unPagedContainer->gridLayout->setSpacing( TO_DPIY(NATRON_FORM_LAYOUT_LINES_SPACING) );


    addPageToPagesContainer(_imp->unPagedContainer);
}

bool
KnobGuiContainerHelper::isPagingEnabled() const
{
    return _imp->unPagedContainer.get() == 0;
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_KnobGuiContainerHelper.cpp"
