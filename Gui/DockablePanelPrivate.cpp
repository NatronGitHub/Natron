/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "DockablePanelPrivate.h"

#include <vector>
#include <utility>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QGridLayout>
#include <QUndoStack>
#include <QDebug>
#include <QScrollArea>

#include "Engine/KnobTypes.h"
#include "Engine/Node.h" // NATRON_PARAMETER_PAGE_NAME_INFO
#include "Gui/ClickableLabel.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiGroup.h" // for KnobGuiGroup
#include "Gui/Label.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/TabGroup.h"

#define NATRON_FORM_LAYOUT_LINES_SPACING 0
#define NATRON_SETTINGS_VERTICAL_SPACING_PIXELS 3

using std::make_pair;
NATRON_NAMESPACE_ENTER;



static void findKnobsOnSameLine(const KnobsVec& knobs,
                                const KnobPtr& ref,
                                KnobsVec& knobsOnSameLine)
{
    int idx = -1;
    for (U32 k = 0; k < knobs.size() ; ++k) {
        if (knobs[k] == ref) {
            idx = k;
            break;
        }
    }
    assert(idx != -1);
    if (idx < 0) {
        return;
    }
    ///find all knobs backward that are on the same line.
    int k = idx - 1;
    KnobPtr parent = ref->getParentKnob();
    
    while ( k >= 0 && !knobs[k]->isNewLineActivated()) {
        if (parent) {
            assert(knobs[k]->getParentKnob() == parent);
            knobsOnSameLine.push_back(knobs[k]);
        } else {
            if (!knobs[k]->getParentKnob() &&
                !dynamic_cast<KnobPage*>(knobs[k].get()) &&
                !dynamic_cast<KnobGroup*>(knobs[k].get())) {
                knobsOnSameLine.push_back(knobs[k]);
            }
        }
        --k;
    }
    
    ///find all knobs forward that are on the same line.
    k = idx;
    while ( k < (int)(knobs.size() - 1) && !knobs[k]->isNewLineActivated()) {
        if (parent) {
            assert(knobs[k + 1]->getParentKnob() == parent);
            knobsOnSameLine.push_back(knobs[k + 1]);
        } else {
            if (!knobs[k + 1]->getParentKnob() &&
                !dynamic_cast<KnobPage*>(knobs[k + 1].get()) &&
                !dynamic_cast<KnobGroup*>(knobs[k + 1].get())) {
                knobsOnSameLine.push_back(knobs[k + 1]);
            }
        }
        ++k;
    }

}

DockablePanelPrivate::DockablePanelPrivate(DockablePanel* publicI,
                                           Gui* gui,
                                           KnobHolder* holder,
                                           QVBoxLayout* container,
                                           DockablePanel::HeaderModeEnum headerMode,
                                           bool useScrollAreasForTabs,
                                           const QString & defaultPageName,
                                           const QString& helpToolTip,
                                           const boost::shared_ptr<QUndoStack>& stack)
: _publicInterface(publicI)
,_gui(gui)
,_container(container)
,_mainLayout(NULL)
,_headerWidget(NULL)
,_headerLayout(NULL)
,_nameLineEdit(NULL)
,_nameLabel(NULL)
, _horizLayout(0)
, _horizContainer(0)
, _verticalColorBar(0)
, _rightContainer(0)
, _rightContainerLayout(0)
,_tabWidget(NULL)
, _centerNodeButton(NULL)
, _enterInGroupButton(NULL)
,_helpButton(NULL)
,_minimize(NULL)
,_hideUnmodifiedButton(NULL)
,_floatButton(NULL)
,_cross(NULL)
,_overlayColor()
,_hasOverlayColor(false)
,_colorButton(NULL)
,_overlayButton(NULL)
,_undoButton(NULL)
,_redoButton(NULL)
,_restoreDefaultsButton(NULL)
,_minimized(false)
,_undoStack(stack)
,_floating(false)
,_floatingWidget(NULL)
,_knobs()
,_knobsVisibilityBeforeHideModif()
,_holder(holder)
,_pages()
,_defaultPageName(defaultPageName)
,_useScrollAreasForTabs(useScrollAreasForTabs)
,_mode(headerMode)
,_isClosedMutex()
,_isClosed(false)
,_helpToolTip(helpToolTip)
,_pluginID()
,_pluginVersionMajor(0)
,_pluginVersionMinor(0)
,_pagesEnabled(true)
,_iconLabel(0)
{
    if (!_undoStack) {
        _undoStack.reset(new QUndoStack());
    }
}

void
DockablePanelPrivate::initializeKnobVector(const std::vector< boost::shared_ptr< KnobI> > & knobs,
                                           QWidget* lastRowWidget)
{
    for (U32 i = 0; i < knobs.size(); ++i) {

        bool makeNewLine = true;
        KnobGroup *isGroup = dynamic_cast<KnobGroup*>(knobs[i].get());
        KnobPage *isPage = dynamic_cast<KnobPage*>(knobs[i].get());
        
        ////The knob  will have a vector of all other knobs on the same line.
        std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;
        
        
        //If the knob is dynamic (i:e created after the initial creation of knobs)
        //it can be added as part of a group defined earlier hence we have to insert it at the proper index.
        KnobPtr parentKnob = knobs[i]->getParentKnob();
        KnobGroup* isParentGroup = dynamic_cast<KnobGroup*>(parentKnob.get());

        
        if (!isPage && !isGroup) {
            if ( (i > 0) && !knobs[i - 1]->isNewLineActivated() ) {
                makeNewLine = false;
            }
            
            KnobPage* isParentPage = dynamic_cast<KnobPage*>(parentKnob.get());
            if (isParentPage) {
                KnobsVec  children = isParentPage->getChildren();
                findKnobsOnSameLine(children, knobs[i], knobsOnSameLine);
            } else if (isParentGroup) {
                KnobsVec  children = isParentGroup->getChildren();
                findKnobsOnSameLine(children, knobs[i], knobsOnSameLine);
            } else {
                findKnobsOnSameLine(knobs, knobs[i], knobsOnSameLine);
            }
            
        }
        
        KnobGui* newGui = findKnobGuiOrCreate(knobs[i],makeNewLine,lastRowWidget,knobsOnSameLine);
        ///childrens cannot be on the same row than their parent
        if (!isGroup && newGui) {
            lastRowWidget = newGui->getFieldContainer();
        }
    }
    
    _publicInterface->refreshTabWidgetMaxHeight();
}

KnobGui*
DockablePanelPrivate::createKnobGui(const KnobPtr &knob)
{
    boost::weak_ptr<KnobI> k = knob;
    std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator found = _knobs.find(k);

    if ( found != _knobs.end() ) {
        return found->second;
    }

    KnobGui* ret =  appPTR->createGuiForKnob(knob,_publicInterface);
    if (!ret) {
        qDebug() << "Failed to create Knob GUI";

        return NULL;
    }
    _knobs.insert( make_pair(knob, ret) );

    return ret;
}

boost::shared_ptr<KnobPage>
DockablePanelPrivate::ensureDefaultPageKnobCreated()
{
    const std::vector< KnobPtr > & knobs = _holder->getKnobs();
    ///find in all knobs a page param to set this param into
    for (U32 i = 0; i < knobs.size(); ++i) {
        boost::shared_ptr<KnobPage> p = boost::dynamic_pointer_cast<KnobPage>(knobs[i]);
        if ( p && (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_INFO) && (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
            getOrCreatePage(p);
            return p;
        }
    }

    
    KnobPtr knob = _holder->getKnobByName(_defaultPageName.toStdString());
    boost::shared_ptr<KnobPage> pk;
    if (!knob) {
        pk = AppManager::createKnob<KnobPage>(_holder, _defaultPageName.toStdString());
    } else {
        pk = boost::dynamic_pointer_cast<KnobPage>(knob);
    }
    assert(pk);
    getOrCreatePage(pk);
    return pk;
}

PageMap::iterator
DockablePanelPrivate::getDefaultPage(const KnobPtr &knob)
{
    PageMap::iterator page = _pages.end();
    const std::vector< KnobPtr > & knobs = _holder->getKnobs();
    ///find in all knobs a page param to set this param into
    for (U32 i = 0; i < knobs.size(); ++i) {
        boost::shared_ptr<KnobPage> p = boost::dynamic_pointer_cast<KnobPage>( knobs[i]);
        if ( p && (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_INFO) && (p->getLabel() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
            page = getOrCreatePage(p);
            p->addKnob(knob);
            break;
        }
    }
    
    if ( page == _pages.end() ) {
        ///the plug-in didn't specify any page
        ///for this param, put it in the first page that is not the default page.
        ///If there is still no page, put it in the default tab.
        for (PageMap::iterator it = _pages.begin(); it != _pages.end(); ++it) {
            if (it->first != _defaultPageName) {
                page = it;
                break;
            }
        }
        
        ///Last resort: The plug-in didn't specify ANY page, just put it into the default page
        if ( page == _pages.end() ) {
            page = getOrCreatePage(boost::shared_ptr<KnobPage>());
        }
    }
    return page;
}

KnobGui*
DockablePanelPrivate::findKnobGuiOrCreate(const KnobPtr & knob,
                                          bool makeNewLine,
                                          QWidget* lastRowWidget,
                                          const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine)
{
    assert(knob);
    boost::shared_ptr<KnobGroup> isGroup = boost::dynamic_pointer_cast<KnobGroup>(knob);
    boost::shared_ptr<KnobPage> isPage = boost::dynamic_pointer_cast<KnobPage>(knob);
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
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
    
    
    if (isPage) {
        if (isPage->getChildren().empty()) {
            return 0;
        }
        getOrCreatePage(isPage);
        KnobsVec children = isPage->getChildren();
        initializeKnobVector(children, lastRowWidget);
        return 0;
    }
    
    
    KnobGui* ret = createKnobGui(knob);
    if (!ret) {
        return 0;
    }
    
    KnobPtr parentKnob = knob->getParentKnob();
    
    boost::shared_ptr<KnobGroup> parentIsGroup = boost::dynamic_pointer_cast<KnobGroup>(parentKnob);
    KnobGuiGroup* parentGui = 0;
    /// if this knob is within a group, make sure the group is created so far
    if (parentIsGroup) {
        parentGui = dynamic_cast<KnobGuiGroup*>( findKnobGuiOrCreate( parentKnob,true,ret->getFieldContainer() ) );
    }
    
    ///So far the knob could have no parent, in which case we force it to be in the default page.
    if (!parentKnob) {
        boost::shared_ptr<KnobPage> defPage = ensureDefaultPageKnobCreated();
        defPage->addKnob(knob);
        parentKnob = defPage;
    }
    
    ///if widgets for the KnobGui have already been created, don't do the following
    ///For group only create the gui if it is not  a tab.
    if (isGroup  && isGroup->isTab()) {
        
        boost::shared_ptr<KnobPage> parentIsPage = boost::dynamic_pointer_cast<KnobPage>(parentKnob);
        if (!parentKnob || parentIsPage) {
            PageMap::iterator page = _pages.end();
            if (!parentKnob) {
                page = getDefaultPage(knob);
            } else {
                page = getOrCreatePage(parentIsPage);
            }
            bool existed = true;
            if (!page->second.groupAsTab) {
                existed = false;
                page->second.groupAsTab = new TabGroup(_publicInterface);
            }
            page->second.groupAsTab->addTab(isGroup, isGroup->getLabel().c_str());
            
            ///retrieve the form layout
            QGridLayout* layout;
            if (_useScrollAreasForTabs) {
                layout = dynamic_cast<QGridLayout*>(dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout());
            } else {
                layout = dynamic_cast<QGridLayout*>(page->second.tab->layout());
            }
            assert(layout);
            if (!existed) {
                layout->addWidget(page->second.groupAsTab, page->second.currentRow, 0, 1, 2);
            }
        } else {
            assert(parentIsGroup);
            assert(parentGui);
            TabGroup* groupAsTab = parentGui->getOrCreateTabWidget();
            
            groupAsTab->addTab(isGroup, isGroup->getLabel().c_str());
            
            if (parentIsGroup && parentIsGroup->isTab()) {
                ///insert the tab in the layout of the parent
                ///Find the page in the parentParent group
                KnobPtr parentParent = parentKnob->getParentKnob();
                assert(parentParent);
                boost::shared_ptr<KnobGroup> parentParentIsGroup = boost::dynamic_pointer_cast<KnobGroup>(parentParent);
                boost::shared_ptr<KnobPage> parentParentIsPage = boost::dynamic_pointer_cast<KnobPage>(parentParent);
                assert(parentParentIsGroup || parentParentIsPage);
                TabGroup* parentTabGroup = 0;
                if (parentParentIsPage) {
                    PageMap::iterator page = getOrCreatePage(parentParentIsPage);
                    assert(page != _pages.end());
                    parentTabGroup = page->second.groupAsTab;
                } else {
                    std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator it = _knobs.find(parentParent);
                    assert(it != _knobs.end());
                    KnobGuiGroup* parentParentGroupGui = dynamic_cast<KnobGuiGroup*>(it->second);
                    assert(parentParentGroupGui);
                    parentTabGroup = parentParentGroupGui->getOrCreateTabWidget();
                }
                
                QGridLayout* layout = parentTabGroup->addTab(parentIsGroup, parentIsGroup->getLabel().c_str());
                assert(layout);
                layout->addWidget(groupAsTab, 0, 0, 1, 2);
                
            } else {
                boost::shared_ptr<KnobPage> topLevelPage = knob->getTopLevelPage();
                PageMap::iterator page = getOrCreatePage(topLevelPage);
                assert(page != _pages.end());
                ///retrieve the form layout
                QGridLayout* layout;
                if (_useScrollAreasForTabs) {
                    layout = dynamic_cast<QGridLayout*>(
                                                        dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout() );
                } else {
                    layout = dynamic_cast<QGridLayout*>( page->second.tab->layout() );
                }
                assert(layout);
                
                layout->addWidget(groupAsTab, page->second.currentRow, 0, 1, 2);
            }
            
        }
        
    } else if (!ret->hasWidgetBeenCreated()) {
        KnobPtr parentKnobTmp = parentKnob;
        while (parentKnobTmp) {
            KnobPtr parent = parentKnobTmp->getParentKnob();
            if (!parent) {
                break;
            } else {
                parentKnobTmp = parent;
            }
        }
        
        ////find in which page the knob should be
        boost::shared_ptr<KnobPage> isTopLevelParentAPage = boost::dynamic_pointer_cast<KnobPage>(parentKnobTmp);
        assert(isTopLevelParentAPage);
        
        PageMap::iterator page = getOrCreatePage(isTopLevelParentAPage);
        assert( page != _pages.end() );
        
        ///retrieve the form layout
        QGridLayout* layout;
        if (_useScrollAreasForTabs) {
            layout = dynamic_cast<QGridLayout*>(
                                                dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout() );
        } else {
            layout = dynamic_cast<QGridLayout*>( page->second.tab->layout() );
        }
        assert(layout);
        
        
        ///if the knob has specified that it didn't want to trigger a new line, decrement the current row
        /// index of the tab
        
        if (!makeNewLine) {
            --page->second.currentRow;
        }
        
        QWidget* fieldContainer = 0;
        QHBoxLayout* fieldLayout = 0;
        
        if (makeNewLine) {
            ///if new line is not turned off, create a new line
            fieldContainer = new QWidget(page->second.tab);
            fieldLayout = new QHBoxLayout(fieldContainer);
            fieldLayout->setContentsMargins(TO_DPIX(3),0,0,TO_DPIY(NATRON_SETTINGS_VERTICAL_SPACING_PIXELS));
            fieldLayout->setSpacing(TO_DPIY(2));
        } else {
            ///otherwise re-use the last row's widget and layout
            assert(lastRowWidget);
            fieldContainer = lastRowWidget;
            fieldLayout = dynamic_cast<QHBoxLayout*>( fieldContainer->layout() );
        }
        
        assert(fieldContainer);
        assert(fieldLayout);
        
        ///Create the label if needed
        ClickableLabel* label = 0;
        
        std::string descriptionLabel;
        KnobString* isStringKnob = dynamic_cast<KnobString*>(knob.get());
        bool isLabelKnob = isStringKnob && isStringKnob->isLabel();
        if (isLabelKnob) {
            descriptionLabel = isStringKnob->getValue();
        } else {
            descriptionLabel = knob->getLabel();
        }
        if (ret->isLabelVisible() && (isLabelKnob || !descriptionLabel.empty())) {
            label = new ClickableLabel("",page->second.tab);
            QString labelStr(descriptionLabel.c_str());
            labelStr += ":";
            label->setText_overload(labelStr );
            QObject::connect( label, SIGNAL( clicked(bool) ), ret, SIGNAL( labelClicked(bool) ) );
        }
        
        /*
         * Find out in which layout the knob should be: either in the layout of the page or in the layout of
         * the nearest parent group tab in the hierarchy
         */
        boost::shared_ptr<KnobGroup> closestParentGroupTab;
        KnobPtr parentTmp = parentKnob;
        assert(parentKnobTmp);
        while (!closestParentGroupTab) {
            boost::shared_ptr<KnobGroup> parentGroup = boost::dynamic_pointer_cast<KnobGroup>(parentTmp);
            if (parentGroup && parentGroup->isTab()) {
                closestParentGroupTab = parentGroup;
            }
            parentTmp = parentTmp->getParentKnob();
            if (!parentTmp) {
                break;
            }
            
        }
        
        if (closestParentGroupTab) {
            
            /*
             * At this point we know that the parent group (which is a tab in the TabWidget) will have at least 1 knob
             * so ensure it is added to the TabWidget.
             * There are 2 possibilities, either the parent of the group tab is another group, in which case we have to
             * make sure the TabWidget is visible in the parent TabWidget of the group, otherwise we just add the TabWidget
             * to the on of the page.
             */
            
            KnobPtr parentParent = closestParentGroupTab->getParentKnob();
            KnobGroup* parentParentIsGroup = dynamic_cast<KnobGroup*>(parentParent.get());
            boost::shared_ptr<KnobPage> parentParentIsPage = boost::dynamic_pointer_cast<KnobPage>(parentParent);
            
            assert(parentParentIsGroup || parentParentIsPage);
            if (parentParentIsGroup) {
                KnobGuiGroup* parentParentGroupGui = dynamic_cast<KnobGuiGroup*>(findKnobGuiOrCreate(parentParent, true,
                                                                                                     ret->getFieldContainer()));
                assert(parentParentGroupGui);
                TabGroup* groupAsTab = parentParentGroupGui->getOrCreateTabWidget();
                assert(groupAsTab);
                layout = groupAsTab->addTab(closestParentGroupTab, closestParentGroupTab->getLabel().c_str());
                
            } else if (parentParentIsPage) {
                PageMap::iterator page = getOrCreatePage(parentParentIsPage);
                assert(page != _pages.end());
                assert(page->second.groupAsTab);
                layout = page->second.groupAsTab->addTab(closestParentGroupTab, closestParentGroupTab->getLabel().c_str());
            }
            assert(layout);
            
        }
        
        ///fill the fieldLayout with the widgets
        ret->createGUI(layout,fieldContainer,label,fieldLayout,makeNewLine,knobsOnSameLine);
        
        
        ret->setEnabledSlot();
        
        ///Must add the row to the layout before calling setSecret()
        if (makeNewLine) {
            int rowIndex;
            if (closestParentGroupTab) {
                rowIndex = layout->rowCount();
            } else if (parentGui && knob->isDynamicallyCreated()) {
                const std::list<KnobGui*>& children = parentGui->getChildren();
                if (children.empty()) {
                    rowIndex = parentGui->getActualIndexInLayout();
                } else {
                    rowIndex = children.back()->getActualIndexInLayout();
                }
                ++rowIndex;
            } else {
                rowIndex = page->second.currentRow;
            }
            
            fieldContainer->layout()->setAlignment(Qt::AlignLeft);
            
            
            if (!label || !ret->isLabelVisible() || label->text().isEmpty()) {
                layout->addWidget(fieldContainer,rowIndex,0, 1, 2);
            } else {
                
                QWidget *labelContainer = new QWidget(page->second.tab);
                QHBoxLayout *labelLayout = new QHBoxLayout(labelContainer);
                labelLayout->setContentsMargins(TO_DPIX(3),0,0,TO_DPIY(NATRON_SETTINGS_VERTICAL_SPACING_PIXELS));
                labelLayout->setSpacing(TO_DPIY(2));
                labelLayout->addWidget(label);

                layout->addWidget(fieldContainer,rowIndex,1, 1, 1);
                layout->addWidget(labelContainer, rowIndex, 0, 1, 1, Qt::AlignRight);
                
            }
            
            
            //if (closestParentGroupTab) {
            ///See http://stackoverflow.com/questions/14033902/qt-qgridlayout-automatically-centers-moves-items-to-the-middle for
            ///a bug of QGridLayout: basically all items are centered, but we would like to add stretch in the bottom of the layout.
            ///To do this we add an empty widget with an expanding vertical size policy.
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
                if (w->objectName() == "emptyWidget") {
                    foundSpacer = w;
                    break;
                }
            }
            if (foundSpacer) {
                layout->removeWidget(foundSpacer);
            } else {
                foundSpacer = new QWidget(layout->parentWidget());
                foundSpacer->setObjectName("emptyWidget");
                foundSpacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
                
            }
            
            ///And add our stretch
            layout->addWidget(foundSpacer,layout->rowCount(), 0, 1, 2);
            // }
            
        } // makeNewLine
        
        ret->setSecret();
        
        if (knob->isNewLineActivated() && ret->shouldAddStretch()) {
            fieldLayout->addStretch();
        }
        
        
        
        ///increment the row count
        ++page->second.currentRow;
        
        if (parentIsGroup) {
            assert(parentGui);
            parentGui->addKnob(ret);
        }
        
    } //  if ( !ret->hasWidgetBeenCreated() && ( !isGroup || !isGroup->isTab() ) ) {
    


    ///if the knob is a group, create all the children
    if (isGroup) {
        KnobsVec children = isGroup->getChildren();
        initializeKnobVector(children, lastRowWidget);
    }
    return ret;
} // findKnobGuiOrCreate

PageMap::iterator
DockablePanelPrivate::getOrCreatePage(const boost::shared_ptr<KnobPage>& page)
{
    if (!_pagesEnabled && _pages.size() > 0) {
        return _pages.begin();
    }
    
    QString name;
    if (!page) {
        name = _defaultPageName;
    } else {
        name = page->getLabel().c_str();
    }
    
    PageMap::iterator found = _pages.find(name);

    if ( found != _pages.end() ) {
        return found;
    }
    
    
    QWidget* newTab;
    QWidget* layoutContainer;
    if (_useScrollAreasForTabs) {
        assert(_tabWidget);
        QScrollArea* sa = new QScrollArea(_tabWidget);
        layoutContainer = new QWidget(sa);
        layoutContainer->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
        sa->setWidgetResizable(true);
        sa->setWidget(layoutContainer);
        newTab = sa;
    } else {
        QWidget* parent;
        
        if (_tabWidget) {
            parent = _tabWidget;
        } else {
            parent = _publicInterface;
        };
        
        RightClickableWidget* clickableWidget = new RightClickableWidget(_publicInterface,parent);
        QObject::connect(clickableWidget,SIGNAL(rightClicked(QPoint)),_publicInterface,SLOT( onRightClickMenuRequested(QPoint) ) );
        QObject::connect(clickableWidget,SIGNAL(escapePressed()),_publicInterface,SLOT( closePanel() ) );
        clickableWidget->setFocusPolicy(Qt::NoFocus);
        newTab = clickableWidget;
        layoutContainer = newTab;
    }
    QGridLayout *tabLayout = new QGridLayout(layoutContainer);
    tabLayout->setObjectName("formLayout");
    layoutContainer->setLayout(tabLayout);
    //tabLayout->setContentsMargins(1, 1, 1, 1);
    tabLayout->setColumnStretch(1, 1);
    tabLayout->setSpacing(TO_DPIY(NATRON_FORM_LAYOUT_LINES_SPACING));
    
    if (_tabWidget) {
        _tabWidget->addTab(newTab,name);
    } else {
        _horizLayout->addWidget(newTab);
    
    }
    
    
    Page p;
    p.tab = newTab;
    p.currentRow = 0;
    p.pageKnob =  page;
    if (page) {
        boost::shared_ptr<KnobSignalSlotHandler> handler = page->getSignalSlotHandler();
        if (handler) {
            QObject::connect(handler.get(), SIGNAL(labelChanged()), _publicInterface, SLOT(onPageLabelChangedInternally()));
        }
    }
    return _pages.insert( make_pair(name,p) ).first;
}

void
DockablePanelPrivate::refreshPagesSecretness()
{
    assert(_tabWidget);
    std::string stdName = _tabWidget->tabText(_tabWidget->currentIndex()).toStdString();
    const KnobsVec& knobs = _holder->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        KnobPage* isPage = dynamic_cast<KnobPage*>(it->get());
        if (!isPage) {
            continue;
        }
        if (isPage->getLabel() == stdName) {
            if (isPage->getIsSecret()) {
                isPage->setSecret(false);
                isPage->evaluateValueChange(0, isPage->getCurrentTime(), eValueChangedReasonUserEdited);
            }
        } else {
            if (!isPage->getIsSecret()) {
                isPage->setSecret(true);
                isPage->evaluateValueChange(0, isPage->getCurrentTime(), eValueChangedReasonUserEdited);
            }
        }
    }
}

NATRON_NAMESPACE_EXIT;
