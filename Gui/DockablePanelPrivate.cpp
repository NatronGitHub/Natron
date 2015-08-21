//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "DockablePanelPrivate.h"

#include <cfloat>
#include <iostream>
#include <fstream>
#include <QLayout>
#include <QAction>
#include <QApplication>
#include <QTabWidget>
#include <QStyle>
#include <QUndoStack>
#include <QGridLayout>
#include <QUndoCommand>
#include <QFormLayout>
#include <QDebug>
#include <QToolTip>
#include <QHeaderView>
#include <QMutex>
#include <QTreeWidget>
#include <QCheckBox>
#include <QHeaderView>
#include <QColorDialog>
#include <QTimer>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QPaintEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QPainter>
#include <QImage>
#include <QToolButton>
#include <QDialogButtonBox>

#include <ofxNatron.h>

GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/serialization/utility.hpp>
GCC_DIAG_ON(unused-parameter)

#include "Engine/BackDrop.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/NoOp.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanelTabWidget.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/KnobGuiTypes.h"
#include "Gui/KnobGuiTypes.h" // for Group_KnobGui
#include "Gui/KnobUndoCommand.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PickKnobDialog.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoPanel.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/TabWidget.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#define NATRON_FORM_LAYOUT_LINES_SPACING 0
#define NATRON_SETTINGS_VERTICAL_SPACING_PIXELS 3

#define NATRON_VERTICAL_BAR_WIDTH 4
using std::make_pair;
using namespace Natron;



static void findKnobsOnSameLine(const std::vector<boost::shared_ptr<KnobI> >& knobs,
                                const boost::shared_ptr<KnobI>& ref,
                                std::vector<boost::shared_ptr<KnobI> >& knobsOnSameLine)
{
    int idx = -1;
    for (U32 k = 0; k < knobs.size() ; ++k) {
        if (knobs[k] == ref) {
            idx = k;
            break;
        }
    }
    assert(idx != -1);

    ///find all knobs backward that are on the same line.
    int k = idx - 1;
    boost::shared_ptr<KnobI> parent = ref->getParentKnob();
    
    while ( k >= 0 && !knobs[k]->isNewLineActivated()) {
        if (parent) {
            assert(knobs[k]->getParentKnob() == parent);
            knobsOnSameLine.push_back(knobs[k]);
        } else {
            if (!knobs[k]->getParentKnob() &&
                !dynamic_cast<Page_Knob*>(knobs[k].get()) &&
                !dynamic_cast<Group_Knob*>(knobs[k].get())) {
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
                !dynamic_cast<Page_Knob*>(knobs[k + 1].get()) &&
                !dynamic_cast<Group_Knob*>(knobs[k + 1].get())) {
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
        Group_Knob *isGroup = dynamic_cast<Group_Knob*>(knobs[i].get());
        Page_Knob *isPage = dynamic_cast<Page_Knob*>(knobs[i].get());
        
        ////The knob  will have a vector of all other knobs on the same line.
        std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;
        
        
        //If the knob is dynamic (i:e created after the initial creation of knobs)
        //it can be added as part of a group defined earlier hence we have to insert it at the proper index.
        boost::shared_ptr<KnobI> parentKnob = knobs[i]->getParentKnob();
        Group_Knob* isParentGroup = dynamic_cast<Group_Knob*>(parentKnob.get());

        
        if (!isPage && !isGroup) {
            if ( (i > 0) && !knobs[i - 1]->isNewLineActivated() ) {
                makeNewLine = false;
            }
            
            Page_Knob* isParentPage = dynamic_cast<Page_Knob*>(parentKnob.get());
            if (isParentPage) {
                std::vector<boost::shared_ptr<KnobI> >  children = isParentPage->getChildren();
                findKnobsOnSameLine(children, knobs[i], knobsOnSameLine);
            } else if (isParentGroup) {
                std::vector<boost::shared_ptr<KnobI> >  children = isParentGroup->getChildren();
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
    
}

KnobGui*
DockablePanelPrivate::createKnobGui(const boost::shared_ptr<KnobI> &knob)
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

boost::shared_ptr<Page_Knob>
DockablePanelPrivate::ensureDefaultPageKnobCreated()
{
    const std::vector< boost::shared_ptr<KnobI> > & knobs = _holder->getKnobs();
    ///find in all knobs a page param to set this param into
    for (U32 i = 0; i < knobs.size(); ++i) {
        boost::shared_ptr<Page_Knob> p = boost::dynamic_pointer_cast<Page_Knob>(knobs[i]);
        if ( p && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_INFO) && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
            addPage(p.get(),  p->getDescription().c_str() );
            return p;
        }
    }

    
    boost::shared_ptr<KnobI> knob = _holder->getKnobByName(_defaultPageName.toStdString());
    boost::shared_ptr<Page_Knob> pk;
    if (!knob) {
        pk = Natron::createKnob<Page_Knob>(_holder, _defaultPageName.toStdString());
    } else {
        pk = boost::dynamic_pointer_cast<Page_Knob>(knob);
    }
    assert(pk);
    addPage(pk.get(), _defaultPageName);
    return pk;
}

PageMap::iterator
DockablePanelPrivate::getDefaultPage(const boost::shared_ptr<KnobI> &knob)
{
    PageMap::iterator page = _pages.end();
    const std::vector< boost::shared_ptr<KnobI> > & knobs = _holder->getKnobs();
    ///find in all knobs a page param to set this param into
    for (U32 i = 0; i < knobs.size(); ++i) {
        Page_Knob* p = dynamic_cast<Page_Knob*>( knobs[i].get() );
        if ( p && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_INFO) && (p->getDescription() != NATRON_PARAMETER_PAGE_NAME_EXTRA) ) {
            page = addPage(p,  p->getDescription().c_str() );
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
            page = addPage(NULL, _defaultPageName);
        }
    }
    return page;
}

KnobGui*
DockablePanelPrivate::findKnobGuiOrCreate(const boost::shared_ptr<KnobI> & knob,
                                          bool makeNewLine,
                                          QWidget* lastRowWidget,
                                          const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine)
{
    assert(knob);
    boost::shared_ptr<Group_Knob> isGroup = boost::dynamic_pointer_cast<Group_Knob>(knob);
    boost::shared_ptr<Page_Knob> isPage = boost::dynamic_pointer_cast<Page_Knob>(knob);
    KnobGui* ret = 0;
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
        QString pageName(isPage->getDescription().c_str());
        addPage(isPage.get(), pageName);

    } else {
        
        
        ret = createKnobGui(knob);

        boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
        boost::shared_ptr<Group_Knob> parentIsGroup = boost::dynamic_pointer_cast<Group_Knob>(parentKnob);
        Group_KnobGui* parentGui = 0;
        /// if this knob is within a group, make sure the group is created so far
        if (parentIsGroup) {
            parentGui = dynamic_cast<Group_KnobGui*>( findKnobGuiOrCreate( parentKnob,true,ret->getFieldContainer() ) );
        }

        ///So far the knob could have no parent, in which case we force it to be in the default page.
        if (!parentKnob) {
            boost::shared_ptr<Page_Knob> defPage = ensureDefaultPageKnobCreated();
            defPage->addKnob(knob);
            parentKnob = defPage;
        }

        ///if widgets for the KnobGui have already been created, don't do the following
        ///For group only create the gui if it is not  a tab.
        if (isGroup  && isGroup->isTab()) {
            
            Page_Knob* parentIsPage = dynamic_cast<Page_Knob*>(parentKnob.get());
            if (!parentKnob || parentIsPage) {
                PageMap::iterator page = _pages.end();
                if (!parentKnob) {
                    page = getDefaultPage(knob);
                } else {
                    page = addPage(parentIsPage, parentIsPage->getDescription().c_str() );
                }
                bool existed = true;
                if (!page->second.groupAsTab) {
                    existed = false;
                    page->second.groupAsTab = new TabGroup(_publicInterface);
                }
                page->second.groupAsTab->addTab(isGroup, isGroup->getDescription().c_str());
                
                ///retrieve the form layout
                QGridLayout* layout;
                if (_useScrollAreasForTabs) {
                    layout = dynamic_cast<QGridLayout*>(
                                                        dynamic_cast<QScrollArea*>(page->second.tab)->widget()->layout() );
                } else {
                    layout = dynamic_cast<QGridLayout*>( page->second.tab->layout() );
                }
                assert(layout);
                if (!existed) {
                    layout->addWidget(page->second.groupAsTab, page->second.currentRow, 0, 1, 2);
                }
            } else {
                assert(parentIsGroup);
                assert(parentGui);
                TabGroup* groupAsTab = parentGui->getOrCreateTabWidget();
                
                groupAsTab->addTab(isGroup, isGroup->getDescription().c_str());
                
                if (parentIsGroup && parentIsGroup->isTab()) {
                    ///insert the tab in the layout of the parent
                    ///Find the page in the parentParent group
                    boost::shared_ptr<KnobI> parentParent = parentKnob->getParentKnob();
                    assert(parentParent);
                    boost::shared_ptr<Group_Knob> parentParentIsGroup = boost::dynamic_pointer_cast<Group_Knob>(parentParent);
                    boost::shared_ptr<Page_Knob> parentParentIsPage = boost::dynamic_pointer_cast<Page_Knob>(parentParent);
                    assert(parentParentIsGroup || parentParentIsPage);
                    TabGroup* parentTabGroup = 0;
                    if (parentParentIsPage) {
                        PageMap::iterator page = addPage(parentParentIsPage.get(),parentParentIsPage->getDescription().c_str());
                        assert(page != _pages.end());
                        parentTabGroup = page->second.groupAsTab;
                    } else {
                        std::map<boost::weak_ptr<KnobI>,KnobGui*>::iterator it = _knobs.find(parentParent);
                        assert(it != _knobs.end());
                        Group_KnobGui* parentParentGroupGui = dynamic_cast<Group_KnobGui*>(it->second);
                        assert(parentParentGroupGui);
                        parentTabGroup = parentParentGroupGui->getOrCreateTabWidget();
                    }
                    
                    QGridLayout* layout = parentTabGroup->addTab(parentIsGroup, parentIsGroup->getDescription().c_str());
                    assert(layout);
                    layout->addWidget(groupAsTab, 0, 0, 1, 2);
                    
                } else {
                    Page_Knob* topLevelPage = knob->getTopLevelPage();
                    PageMap::iterator page = addPage(topLevelPage,topLevelPage->getDescription().c_str());
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
            KnobI* parentKnobTmp = parentKnob.get();
            while (parentKnobTmp) {
                boost::shared_ptr<KnobI> parent = parentKnobTmp->getParentKnob();
                if (!parent) {
                    break;
                } else {
                    parentKnobTmp = parent.get();
                }
            }

            ////find in which page the knob should be
            Page_Knob* isTopLevelParentAPage = dynamic_cast<Page_Knob*>(parentKnobTmp);
            assert(isTopLevelParentAPage);
            
            PageMap::iterator page = addPage(isTopLevelParentAPage, isTopLevelParentAPage->getDescription().c_str());
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
                fieldLayout->setContentsMargins(3,0,0,NATRON_SETTINGS_VERTICAL_SPACING_PIXELS);
                fieldLayout->setSpacing(2);
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
            
            if (ret->showDescriptionLabel() && !knob->getDescription().empty()) {
                label = new ClickableLabel("",page->second.tab);
                label->setText_overload( QString(QString( ret->getKnob()->getDescription().c_str() ) + ":") );
                QObject::connect( label, SIGNAL( clicked(bool) ), ret, SIGNAL( labelClicked(bool) ) );
            }

            /*
             * Find out in which layout the knob should be: either in the layout of the page or in the layout of
             * the nearest parent group tab in the hierarchy
             */
            boost::shared_ptr<Group_Knob> closestParentGroupTab;
            boost::shared_ptr<KnobI> parentTmp = parentKnob;
            assert(parentKnobTmp);
            while (!closestParentGroupTab) {
                boost::shared_ptr<Group_Knob> parentGroup = boost::dynamic_pointer_cast<Group_Knob>(parentTmp);
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
                
                boost::shared_ptr<KnobI> parentParent = closestParentGroupTab->getParentKnob();
                Group_Knob* parentParentIsGroup = dynamic_cast<Group_Knob*>(parentParent.get());
                Page_Knob* parentParentIsPage = dynamic_cast<Page_Knob*>(parentParent.get());
                
                assert(parentParentIsGroup || parentParentIsPage);
                if (parentParentIsGroup) {
                    Group_KnobGui* parentParentGroupGui = dynamic_cast<Group_KnobGui*>(findKnobGuiOrCreate(parentParent, true,
                                                                                                           ret->getFieldContainer()));
                    assert(parentParentGroupGui);
                    TabGroup* groupAsTab = parentParentGroupGui->getOrCreateTabWidget();
                    assert(groupAsTab);
                    layout = groupAsTab->addTab(closestParentGroupTab, closestParentGroupTab->getDescription().c_str());
                    
                } else if (parentParentIsPage) {
                    PageMap::iterator page = addPage(parentParentIsPage, parentParentIsPage->getDescription().c_str());
                    assert(page != _pages.end());
                    assert(page->second.groupAsTab);
                    layout = page->second.groupAsTab->addTab(closestParentGroupTab, closestParentGroupTab->getDescription().c_str());
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
                
                
                if (!label || !ret->showDescriptionLabel() || label->text().isEmpty()) {
                    layout->addWidget(fieldContainer,rowIndex,0, 1, 2);
                } else {
                    
                    layout->addWidget(fieldContainer,rowIndex,1, 1, 1);
                    layout->addWidget(label, rowIndex, 0, 1, 1, Qt::AlignRight);
        
                }
                
                
                if (closestParentGroupTab) {
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
                }
                
            }

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
        
    } // !isPage

    ///if the knob is a group, create all the children
    if (isGroup) {
        std::vector<boost::shared_ptr<KnobI> > children = isGroup->getChildren();
        initializeKnobVector(children, lastRowWidget);
    } else if (isPage) {
        std::vector<boost::shared_ptr<KnobI> > children = isPage->getChildren();
        initializeKnobVector(children, lastRowWidget);
    }

    return ret;
} // findKnobGuiOrCreate

PageMap::iterator
DockablePanelPrivate::addPage(Page_Knob* page,const QString & name)
{
    if (!_pagesEnabled && _pages.size() > 0) {
        return _pages.begin();
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
    tabLayout->setSpacing(NATRON_FORM_LAYOUT_LINES_SPACING);
    
    if (_tabWidget) {
        if (name == NATRON_USER_MANAGED_KNOBS_PAGE_LABEL || (page && page->isUserKnob())) {
            _tabWidget->insertTab(0,newTab,name);
            _tabWidget->setCurrentIndex(0);
        } else {
            _tabWidget->addTab(newTab,name);
        }
    } else {
        if (name == NATRON_USER_MANAGED_KNOBS_PAGE_LABEL || (page && page->isUserKnob())) {
            _horizLayout->insertWidget(0, newTab);
        } else {
            _horizLayout->addWidget(newTab);
        }
    }
    Page p;
    p.tab = newTab;
    p.currentRow = 0;

    return _pages.insert( make_pair(name,p) ).first;
}



