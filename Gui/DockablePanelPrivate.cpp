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


static Page_Knob* getTopLevelPageForKnob(KnobI* knob)
{
    boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
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
    return isTopLevelParentAPage;
}


class NoWheelTabBar : public QTabBar
{
public:
    
    NoWheelTabBar(QWidget* parent) : QTabBar(parent) {}
    
private:
    
    virtual void wheelEvent(QWheelEvent* event) OVERRIDE FINAL
    {
        //ignore wheel events so it doesn't scroll the tabs
        QWidget::wheelEvent(event);
    }
};


DockablePanelTabWidget::DockablePanelTabWidget(Gui* gui,QWidget* parent)
    : QTabWidget(parent)
    , _gui(gui)
{
    setFocusPolicy(Qt::ClickFocus);
    QTabBar* tabbar = new NoWheelTabBar(this);
    tabbar->setObjectName("PanelTabBar");
    tabbar->setFocusPolicy(Qt::ClickFocus);
    setTabBar(tabbar);
    setObjectName("PanelTabBar");
}

void
DockablePanelTabWidget::keyPressEvent(QKeyEvent* event)
{
    Qt::Key key = (Qt::Key)event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();
    
    if (isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key)) {
        if ( _gui->getNodeGraph()->getLastSelectedViewer() ) {
            _gui->getNodeGraph()->getLastSelectedViewer()->previousFrame();
        }
    } else if (isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( _gui->getNodeGraph()->getLastSelectedViewer() ) {
            _gui->getNodeGraph()->getLastSelectedViewer()->nextFrame();
        }
    } else {
        QTabWidget::keyPressEvent(event);
    }
}

QSize
DockablePanelTabWidget::sizeHint() const
{
    return currentWidget() ? currentWidget()->sizeHint() + QSize(0,20) : QSize(300,100);
}

QSize
DockablePanelTabWidget::minimumSizeHint() const
{
    return currentWidget() ? currentWidget()->minimumSizeHint() + QSize(0,20) : QSize(300,100);
}

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
                    Page_Knob* topLevelPage = getTopLevelPageForKnob(knob.get());
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


VerticalColorBar::VerticalColorBar(QWidget* parent)
: QWidget(parent)
, _color(Qt::black)
{
    setFixedWidth(NATRON_VERTICAL_BAR_WIDTH);
}

void
VerticalColorBar::setColor(const QColor& color)
{
    _color = color;
    update();
}

QSize
VerticalColorBar::sizeHint() const
{
    return QWidget::sizeHint();
    //return QSize(5,1000);
}

void
VerticalColorBar::paintEvent(QPaintEvent* /*e*/)
{
    QPainter p(this);
    QPen pen;
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(NATRON_VERTICAL_BAR_WIDTH);
    pen.setColor(_color);
    p.setPen(pen);
    p.drawLine( 0, NATRON_VERTICAL_BAR_WIDTH, 0, height() - NATRON_VERTICAL_BAR_WIDTH);
}

struct TreeItem
{
    QTreeWidgetItem* item;
    boost::shared_ptr<KnobI> knob;
};

struct ManageUserParamsDialogPrivate
{
    DockablePanel* panel;
    
    QHBoxLayout* mainLayout;
    QTreeWidget* tree;
    
    std::list<TreeItem> items;
    
    
    QWidget* buttonsContainer;
    QVBoxLayout* buttonsLayout;
    
    Button* addButton;
    Button* pickButton;
    Button* editButton;
    Button* removeButton;
    Button* upButton;
    Button* downButton;
    Button* closeButton;

    
    ManageUserParamsDialogPrivate(DockablePanel* panel)
    : panel(panel)
    , mainLayout(0)
    , tree(0)
    , items()
    , buttonsContainer(0)
    , buttonsLayout(0)
    , addButton(0)
    , pickButton(0)
    , editButton(0)
    , removeButton(0)
    , upButton(0)
    , downButton(0)
    , closeButton(0)
    {
        
    }
    
    boost::shared_ptr<Page_Knob> getUserPageKnob() const;
    
    void initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs);
    
    void rebuildUserPages();
};

ManageUserParamsDialog::ManageUserParamsDialog(DockablePanel* panel,QWidget* parent)
: QDialog(parent)
, _imp(new ManageUserParamsDialogPrivate(panel))
{
    _imp->mainLayout = new QHBoxLayout(this);
    
    _imp->tree = new QTreeWidget(this);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    //_imp->tree->setRootIsDecorated(false);
    _imp->tree->setItemsExpandable(true);
    _imp->tree->header()->setStretchLastSection(false);
    _imp->tree->setTextElideMode(Qt::ElideMiddle);
    _imp->tree->setContextMenuPolicy(Qt::CustomContextMenu);
    _imp->tree->header()->setStretchLastSection(true);
    _imp->tree->header()->hide();
    
    TreeItem userPageItem;
    userPageItem.item = new QTreeWidgetItem(_imp->tree);
    userPageItem.item->setText(0, NATRON_USER_MANAGED_KNOBS_PAGE);
    userPageItem.item->setExpanded(true);
    _imp->items.push_back(userPageItem);
    
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()),this,SLOT(onSelectionChanged()));
    
    std::list<KnobI*> markedKnobs;
    const std::vector<boost::shared_ptr<KnobI> >& knobs = panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ((*it)->isUserKnob()) {
            
            Page_Knob* page = dynamic_cast<Page_Knob*>(it->get());
            if (page) {
                
                TreeItem pageItem;
                if (page->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                    pageItem.item = userPageItem.item;
                } else {
                    pageItem.item = new QTreeWidgetItem(_imp->tree);
                    pageItem.item->setText(0, page->getName().c_str());
                    pageItem.knob = *it;
                    pageItem.item->setExpanded(true);
                }
                _imp->items.push_back(pageItem);
                
                std::vector<boost::shared_ptr<KnobI> > children = page->getChildren();
                _imp->initializeKnobs(children, pageItem.item, markedKnobs);
                
            }
        }
    }
    
    _imp->mainLayout->addWidget(_imp->tree);
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QVBoxLayout(_imp->buttonsContainer);
    
    _imp->addButton = new Button(tr("Add"),_imp->buttonsContainer);
    _imp->addButton->setToolTip(Natron::convertFromPlainText(tr("Add a new parameter, group or page"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->addButton,SIGNAL(clicked(bool)),this,SLOT(onAddClicked()));
    _imp->buttonsLayout->addWidget(_imp->addButton);
    
    _imp->pickButton = new Button(tr("Pick"),_imp->buttonsContainer);
    _imp->pickButton->setToolTip(Natron::convertFromPlainText(tr("Add a new parameter that is directly copied from/linked to another parameter"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->pickButton,SIGNAL(clicked(bool)),this,SLOT(onPickClicked()));
    _imp->buttonsLayout->addWidget(_imp->pickButton);
    
    _imp->editButton = new Button(tr("Edit"),_imp->buttonsContainer);
    _imp->editButton->setToolTip(Natron::convertFromPlainText(tr("Edit the selected parameter"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->editButton,SIGNAL(clicked(bool)),this,SLOT(onEditClicked()));
    _imp->buttonsLayout->addWidget(_imp->editButton);
    
    _imp->removeButton = new Button(tr("Delete"),_imp->buttonsContainer);
    _imp->removeButton->setToolTip(Natron::convertFromPlainText(tr("Delete the selected parameter"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->removeButton,SIGNAL(clicked(bool)),this,SLOT(onDeleteClicked()));
    _imp->buttonsLayout->addWidget(_imp->removeButton);
    
    _imp->upButton = new Button(tr("Up"),_imp->buttonsContainer);
    _imp->upButton->setToolTip(Natron::convertFromPlainText(tr("Move the selected parameter one level up in the layout"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->upButton,SIGNAL(clicked(bool)),this,SLOT(onUpClicked()));
    _imp->buttonsLayout->addWidget(_imp->upButton);
    
    _imp->downButton = new Button(tr("Down"),_imp->buttonsContainer);
    _imp->downButton->setToolTip(Natron::convertFromPlainText(tr("Move the selected parameter one level down in the layout"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->downButton,SIGNAL(clicked(bool)),this,SLOT(onDownClicked()));
    _imp->buttonsLayout->addWidget(_imp->downButton);
    
    _imp->closeButton = new Button(tr("Close"),_imp->buttonsContainer);
    _imp->closeButton->setToolTip(Natron::convertFromPlainText(tr("Close this dialog"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->closeButton,SIGNAL(clicked(bool)),this,SLOT(onCloseClicked()));
    _imp->buttonsLayout->addWidget(_imp->closeButton);
    
    _imp->mainLayout->addWidget(_imp->buttonsContainer);
    onSelectionChanged();
    _imp->panel->setUserPageActiveIndex();

}

ManageUserParamsDialog::~ManageUserParamsDialog()
{
//    for (std::list<TreeItem>::iterator it = _imp->items.begin() ; it != _imp->items.end(); ++it) {
//        delete it->item;
//    }
}

void
ManageUserParamsDialogPrivate::initializeKnobs(const std::vector<boost::shared_ptr<KnobI> >& knobs,QTreeWidgetItem* parent,std::list<KnobI*>& markedKnobs)
{
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2!=knobs.end(); ++it2) {
        
        if (std::find(markedKnobs.begin(),markedKnobs.end(),it2->get()) != markedKnobs.end()) {
            continue;
        }
        
        markedKnobs.push_back(it2->get());
        
        TreeItem i;
        i.knob = *it2;
        i.item = new QTreeWidgetItem(parent);
        i.item->setText(0, (*it2)->getName().c_str());
        i.item->setExpanded(true);
        items.push_back(i);
        
        Group_Knob* isGrp = dynamic_cast<Group_Knob*>(it2->get());
        if (isGrp) {
            std::vector<boost::shared_ptr<KnobI> > children = isGrp->getChildren();
            initializeKnobs(children, i.item, markedKnobs);
        }
    }
}


boost::shared_ptr<Page_Knob>
ManageUserParamsDialogPrivate::getUserPageKnob() const
{
    return panel->getUserPageKnob();
}


void
ManageUserParamsDialogPrivate::rebuildUserPages()
{
    panel->scanForNewKnobs();
}

void
ManageUserParamsDialog::onPickClicked()
{
    PickKnobDialog dialog(_imp->panel,this);
    if (dialog.exec()) {
        bool useExpr;
        KnobGui* selectedKnob = dialog.getSelectedKnob(&useExpr);
        if (!selectedKnob) {
            return;
        }
        
        NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(_imp->panel);
        assert(nodePanel);
        boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
        assert(nodeGui);
        NodePtr node = nodeGui->getNode();
        assert(node);
        selectedKnob->createDuplicateOnNode(node->getLiveInstance(), useExpr);
    }
}

void
ManageUserParamsDialog::onAddClicked()
{
    AddKnobDialog dialog(_imp->panel,boost::shared_ptr<KnobI>(),this);
    if (dialog.exec()) {
        //Ensure the user page knob exists
        boost::shared_ptr<Page_Knob> userPageKnob = _imp->panel->getUserPageKnob();
        
        boost::shared_ptr<KnobI> knob = dialog.getKnob();
        QTreeWidgetItem* parent = 0;
        
        boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
        if (parentKnob) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->knob.get() == parentKnob.get()) {
                    parent = it->item;
                    break;
                }
            }
        }
        if (!parent) {
            Page_Knob* isPage = dynamic_cast<Page_Knob*>(knob.get());
            if (!isPage) {
                //Default to user page
                for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                    if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                        parent = it->item;
                        break;
                    }
                }
            }

        }
        TreeItem i;
        i.knob = knob;
        i.item = new QTreeWidgetItem;
        if (parent) {
            parent->addChild(i.item);
            parent->setExpanded(true);
        } else {
            _imp->tree->addTopLevelItem(i.item);
        }
        i.item->setText(0, knob->getName().c_str());
        _imp->items.push_back(i);
    }
    _imp->rebuildUserPages();
    _imp->panel->getGui()->getApp()->triggerAutoSave();
}

void
ManageUserParamsDialog::onDeleteClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    it->knob->getHolder()->removeDynamicKnob(it->knob.get());
                    delete it->item;
                    _imp->items.erase(it);
                    
                    boost::shared_ptr<Page_Knob> userPage = _imp->getUserPageKnob();
                    if (userPage->getChildren().empty()) {
                        userPage->getHolder()->removeDynamicKnob(userPage.get());
                    }
                    _imp->panel->getGui()->getApp()->triggerAutoSave();
                    break;
                }
            }
        }
    }
}

void
ManageUserParamsDialog::onEditClicked()
{
    
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    
                    std::list<boost::shared_ptr<KnobI> > listeners;
                    it->knob->getListeners(listeners);
                    if (!listeners.empty()) {
                        Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Edit parameter").toStdString(),
                                                                                tr("This parameter has one or several "
                                                                                   "parameters from which other parameters "
                                                                                   "of the project rely on through expressions "
                                                                                   "or links. Editing this parameter may "
                                                                                   "remove these expressions if the script-name is changed.\n"
                                                                                   "Do you want to continue?").toStdString(), false);
                        if (rep == Natron::eStandardButtonNo) {
                            return;
                        }
                    }
                    
                    AddKnobDialog dialog(_imp->panel,it->knob,this);
                    if (dialog.exec()) {
                        it->knob = dialog.getKnob();
                        it->item->setText(0, it->knob->getName().c_str());
                        _imp->rebuildUserPages();
                        _imp->panel->getGui()->getApp()->triggerAutoSave();
                        break;
                    }
                }
                
            }
        }
    }
    
}

void
ManageUserParamsDialog::onUpClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    
                    if (dynamic_cast<Page_Knob*>(it->knob.get())) {
                        break;
                    }
                    QTreeWidgetItem* parent = 0;
                    boost::shared_ptr<KnobI> parentKnob = it->knob->getParentKnob();
                    if (parentKnob) {
                        if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        } else {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->knob == parentKnob) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        }
                    }
                    
                    int index;
                    if (!parent) {
                        index = _imp->tree->indexOfTopLevelItem(it->item);
                    } else {
                        index = parent->indexOfChild(it->item);
                    }
                    if (index == 0) {
                        break;
                    }
                    _imp->panel->getHolder()->moveKnobOneStepUp(it->knob.get());
                    QList<QTreeWidgetItem*> children = it->item->takeChildren();
                    delete it->item;
                    
                    it->item = new QTreeWidgetItem;
                    it->item->setText(0, it->knob->getName().c_str());
                    if (!parent) {
                        _imp->tree->insertTopLevelItem(index - 1, it->item);
                    } else {
                        parent->insertChild(index - 1, it->item);
                    }
                    it->item->insertChildren(0, children);
                    it->item->setExpanded(true);
                    _imp->tree->clearSelection();
                    it->item->setSelected(true);
                    break;
                }
            }
            
        }
        _imp->rebuildUserPages();
    }
}

void
ManageUserParamsDialog::onDownClicked()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    if (!selection.isEmpty()) {
        for (int i = 0; i < selection.size(); ++i) {
            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                if (it->item == selection[i]) {
                    if (dynamic_cast<Page_Knob*>(it->knob.get())) {
                        break;
                    }

                    QTreeWidgetItem* parent = 0;
                    boost::shared_ptr<KnobI> parentKnob = it->knob->getParentKnob();
                    if (parentKnob) {
                        if (parentKnob->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        } else {
                            for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
                                if (it->knob == parentKnob) {
                                    parent = it->item;
                                    break;
                                }
                            }
                        }
                    }
                    int index;
                    
                    if (!parent) {
                        index = _imp->tree->indexOfTopLevelItem(it->item);
                        if (index == _imp->tree->topLevelItemCount() - 1) {
                            break;
                        }
                    } else {
                        index = parent->indexOfChild(it->item);
                        if (index == parent->childCount() - 1) {
                            break;
                        }
                    }
                    
                    _imp->panel->getHolder()->moveKnobOneStepDown(it->knob.get());
                    QList<QTreeWidgetItem*> children = it->item->takeChildren();
                    delete it->item;
                    
                    
                    it->item = new QTreeWidgetItem;
                    if (parent) {
                        parent->insertChild(index + 1, it->item);
                    } else {
                        _imp->tree->insertTopLevelItem(index + 1, it->item);
                    }
                    it->item->setText(0, it->knob->getName().c_str());
                    it->item->insertChildren(0, children);
                    it->item->setExpanded(true);
                    _imp->tree->clearSelection();
                    it->item->setSelected(true);
                    break;
                }
            }
            
        }
        _imp->rebuildUserPages();
    }
}

void
ManageUserParamsDialog::onCloseClicked()
{
    accept();
}

void
ManageUserParamsDialog::onSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    bool canEdit = true;
    bool canMove = true;
    bool canDelete = true;
    if (!selection.isEmpty()) {
        QTreeWidgetItem* item = selection[0];
        if (item->text(0) == QString(NATRON_USER_MANAGED_KNOBS_PAGE)) {
            canEdit = false;
            canDelete = false;
            canMove = false;
        }
        for (std::list<TreeItem>::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            if (it->item == item) {
                Page_Knob* isPage = dynamic_cast<Page_Knob*>(it->knob.get());
                Group_Knob* isGroup = dynamic_cast<Group_Knob*>(it->knob.get());
                if (isPage) {
                    canMove = false;
                    canEdit = false;
                } else if (isGroup) {
                    canEdit = false;
                }
                break;
            }
        }
    }
    
    _imp->removeButton->setEnabled(selection.size() == 1 && canDelete);
    _imp->editButton->setEnabled(selection.size() == 1 && canEdit);
    _imp->upButton->setEnabled(selection.size() == 1 && canMove);
    _imp->downButton->setEnabled(selection.size() == 1 && canMove);
}

struct PickKnobDialogPrivate
{
    
    QGridLayout* mainLayout;
    Natron::Label* selectNodeLabel;
    CompleterLineEdit* nodeSelectionCombo;
    ComboBox* knobSelectionCombo;
    Natron::Label* useExpressionLabel;
    QCheckBox* useExpressionCheckBox;
    QDialogButtonBox* buttons;
    NodeList allNodes;
    std::map<QString,boost::shared_ptr<KnobI > > allKnobs;
    
    PickKnobDialogPrivate()
    : mainLayout(0)
    , selectNodeLabel(0)
    , nodeSelectionCombo(0)
    , knobSelectionCombo(0)
    , useExpressionLabel(0)
    , useExpressionCheckBox(0)
    , buttons(0)
    , allNodes()
    , allKnobs()
    {
        
    }
};

PickKnobDialog::PickKnobDialog(DockablePanel* panel, QWidget* parent)
: QDialog(parent)
, _imp(new PickKnobDialogPrivate())
{
    NodeSettingsPanel* nodePanel = dynamic_cast<NodeSettingsPanel*>(panel);
    assert(nodePanel);
    boost::shared_ptr<NodeGui> nodeGui = nodePanel->getNode();
    NodePtr node = nodeGui->getNode();
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(node->getLiveInstance());
    boost::shared_ptr<NodeCollection> collec = node->getGroup();
    NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
    NodeList collectNodes = collec->getNodes();
    for (NodeList::iterator it = collectNodes.begin(); it != collectNodes.end(); ++it) {
        if (!(*it)->getParentMultiInstance() && (*it)->isActivated() && (*it)->getKnobs().size() > 0) {
            _imp->allNodes.push_back(*it);
        }
    }
    if (isCollecGroup) {
        _imp->allNodes.push_back(isCollecGroup->getNode());
    }
    if (isGroup) {
        NodeList groupnodes = isGroup->getNodes();
        for (NodeList::iterator it = groupnodes.begin(); it != groupnodes.end(); ++it) {
            if (!(*it)->getParentMultiInstance() && (*it)->isActivated() && (*it)->getKnobs().size() > 0) {
                _imp->allNodes.push_back(*it);
            }
        }
    }
    QStringList nodeNames;
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        QString name( (*it)->getLabel().c_str() );
        nodeNames.push_back(name);
    }
    nodeNames.sort();

    _imp->mainLayout = new QGridLayout(this);
    _imp->selectNodeLabel = new Natron::Label(tr("Node:"));
    _imp->nodeSelectionCombo = new CompleterLineEdit(nodeNames,nodeNames,false,this);
    _imp->nodeSelectionCombo->setToolTip(Natron::convertFromPlainText(tr("Input the name of a node in the current project."), Qt::WhiteSpaceNormal));
    _imp->nodeSelectionCombo->setFocus(Qt::PopupFocusReason);
    
    _imp->knobSelectionCombo = new ComboBox(this);
    
    _imp->useExpressionLabel = new Natron::Label(tr("Also create an expression to control the parameter:"),this);
    _imp->useExpressionCheckBox = new QCheckBox(this);
    _imp->useExpressionCheckBox->setChecked(true);
    
    QObject::connect( _imp->nodeSelectionCombo,SIGNAL( itemCompletionChosen() ),this,SLOT( onNodeComboEditingFinished() ) );
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal,this);
    QObject::connect( _imp->buttons, SIGNAL( accepted() ), this, SLOT( accept() ) );
    QObject::connect( _imp->buttons, SIGNAL( rejected() ), this, SLOT( reject() ) );
    
    _imp->mainLayout->addWidget(_imp->selectNodeLabel, 0, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->nodeSelectionCombo, 0, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->knobSelectionCombo, 0, 2, 1, 1);
    _imp->mainLayout->addWidget(_imp->useExpressionLabel, 1, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->useExpressionCheckBox, 1, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->buttons, 2, 0, 1, 3);
    
    QTimer::singleShot( 25, _imp->nodeSelectionCombo, SLOT( showCompleter() ) );

}

PickKnobDialog::~PickKnobDialog()
{
    
}

void
PickKnobDialog::onNodeComboEditingFinished()
{
    QString index = _imp->nodeSelectionCombo->text();
    _imp->knobSelectionCombo->clear();
    _imp->allKnobs.clear();
    boost::shared_ptr<Natron::Node> selectedNode;
    std::string currentNodeName = index.toStdString();
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ((*it)->getLabel() == currentNodeName) {
            selectedNode = *it;
            break;
        }
    }
    if (!selectedNode) {
        return;
    }
    const std::vector< boost::shared_ptr<KnobI> > & knobs = selectedNode->getKnobs();
    for (U32 j = 0; j < knobs.size(); ++j) {
        if (!knobs[j]->getIsSecret()) {
            Button_Knob* isButton = dynamic_cast<Button_Knob*>( knobs[j].get() );
            Page_Knob* isPage = dynamic_cast<Page_Knob*>( knobs[j].get() );
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knobs[j].get() );
            if (!isButton && !isPage && !isGroup) {
                QString name( knobs[j]->getDescription().c_str() );
                
                bool canInsertKnob = true;
                for (int k = 0; k < knobs[j]->getDimension(); ++k) {
                    if ( knobs[j]->isSlave(k) || !knobs[j]->isEnabled(k) || name.isEmpty() ) {
                        canInsertKnob = false;
                    }
                }
                if (canInsertKnob) {
                    _imp->allKnobs.insert(std::make_pair( name, knobs[j]));
                    _imp->knobSelectionCombo->addItem(name);
                }
            }
            
        }
    }
}

KnobGui*
PickKnobDialog::getSelectedKnob(bool* useExpressionLink) const
{
    QString index = _imp->nodeSelectionCombo->text();
    boost::shared_ptr<Natron::Node> selectedNode;
    std::string currentNodeName = index.toStdString();
    for (NodeList::iterator it = _imp->allNodes.begin(); it != _imp->allNodes.end(); ++it) {
        if ((*it)->getLabel() == currentNodeName) {
            selectedNode = *it;
            break;
        }
    }
    if (!selectedNode) {
        return 0;
    }
    
    QString str = _imp->knobSelectionCombo->itemText( _imp->knobSelectionCombo->activeIndex() );
    std::map<QString,boost::shared_ptr<KnobI> >::const_iterator it = _imp->allKnobs.find(str);
    
    boost::shared_ptr<KnobI> selectedKnob;
    if ( it != _imp->allKnobs.end() ) {
        selectedKnob = it->second;
    } else {
        return 0;
    }
    
    boost::shared_ptr<NodeGuiI> selectedNodeGuiI = selectedNode->getNodeGui();
    assert(selectedNodeGuiI);
    NodeGui* selectedNodeGui = dynamic_cast<NodeGui*>(selectedNodeGuiI.get());
    assert(selectedNodeGui);
    NodeSettingsPanel* selectedPanel = selectedNodeGui->getSettingPanel();
    bool hadPanelVisible = selectedPanel && !selectedPanel->isClosed();
    if (!selectedPanel) {
        selectedNodeGui->ensurePanelCreated();
    }
    if (!selectedPanel) {
        return 0;
    }
    if (!hadPanelVisible && selectedPanel) {
        selectedPanel->setClosed(true);
    }
    const std::map<boost::weak_ptr<KnobI>,KnobGui*>& knobsMap = selectedPanel->getKnobs();
    std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator found = knobsMap.find(selectedKnob);
    if (found != knobsMap.end()) {
        *useExpressionLink = _imp->useExpressionCheckBox->isChecked();
        return found->second;
    }
    return 0;
}

struct AddKnobDialogPrivate
{
    boost::shared_ptr<KnobI> knob;
    boost::shared_ptr<KnobSerialization> originalKnobSerialization;
    
    DockablePanel* panel;
    
    QVBoxLayout* vLayout;
    
    QWidget* mainContainer;
    QFormLayout* mainLayout;
    
    Natron::Label* typeLabel;
    ComboBox* typeChoice;
    Natron::Label* nameLabel;
    LineEdit* nameLineEdit;

    
    Natron::Label* labelLabel;
    LineEdit* labelLineEdit;
    
    Natron::Label* hideLabel;
    QCheckBox* hideBox;
    
    Natron::Label* startNewLineLabel;
    QCheckBox* startNewLineBox;
    
    Natron::Label* animatesLabel;
    QCheckBox* animatesCheckbox;
    
    Natron::Label* evaluatesLabel;
    QCheckBox* evaluatesOnChange;
    
    Natron::Label* tooltipLabel;
    QTextEdit* tooltipArea;
    
    Natron::Label* minLabel;
    SpinBox* minBox;
    
    Natron::Label* maxLabel;
    SpinBox* maxBox;
    
    Natron::Label* dminLabel;
    SpinBox* dminBox;
    
    Natron::Label* dmaxLabel;
    SpinBox* dmaxBox;
    
    enum DefaultValueType
    {
        eDefaultValueTypeInt,
        eDefaultValueTypeDouble,
        eDefaultValueTypeBool,
        eDefaultValueTypeString
    };
    
    
    Natron::Label* defaultValueLabel;
    SpinBox* default0;
    SpinBox* default1;
    SpinBox* default2;
    SpinBox* default3;
    LineEdit* defaultStr;
    QCheckBox* defaultBool;
    
    Natron::Label* menuItemsLabel;
    QTextEdit* menuItemsEdit;
    
    Natron::Label* multiLineLabel;
    QCheckBox* multiLine;
    
    Natron::Label* richTextLabel;
    QCheckBox* richText;
    
    Natron::Label* sequenceDialogLabel;
    QCheckBox* sequenceDialog;
    
    Natron::Label* multiPathLabel;
    QCheckBox* multiPath;
    
    Natron::Label* groupAsTabLabel;
    QCheckBox* groupAsTab;
    
    Natron::Label* parentGroupLabel;
    ComboBox* parentGroup;
    
    Natron::Label* parentPageLabel;
    ComboBox* parentPage;
    
    std::list<Group_Knob*> userGroups;
    std::list<Page_Knob*> userPages; //< all user pages except the "User" one
    
    AddKnobDialogPrivate(DockablePanel* panel)
    : knob()
    , originalKnobSerialization()
    , panel(panel)
    , vLayout(0)
    , mainContainer(0)
    , mainLayout(0)
    , typeLabel(0)
    , typeChoice(0)
    , nameLabel(0)
    , nameLineEdit(0)
    , labelLabel(0)
    , labelLineEdit(0)
    , hideLabel(0)
    , hideBox(0)
    , startNewLineLabel(0)
    , startNewLineBox(0)
    , animatesLabel(0)
    , animatesCheckbox(0)
    , evaluatesLabel(0)
    , evaluatesOnChange(0)
    , tooltipLabel(0)
    , tooltipArea(0)
    , minLabel(0)
    , minBox(0)
    , maxLabel(0)
    , maxBox(0)
    , dminLabel(0)
    , dminBox(0)
    , dmaxLabel(0)
    , dmaxBox(0)
    , defaultValueLabel(0)
    , default0(0)
    , default1(0)
    , default2(0)
    , default3(0)
    , defaultStr(0)
    , defaultBool(0)
    , menuItemsLabel(0)
    , menuItemsEdit(0)
    , multiLineLabel(0)
    , multiLine(0)
    , richTextLabel(0)
    , richText(0)
    , sequenceDialogLabel(0)
    , sequenceDialog(0)
    , multiPathLabel(0)
    , multiPath(0)
    , groupAsTabLabel(0)
    , groupAsTab(0)
    , parentGroupLabel(0)
    , parentGroup(0)
    , parentPageLabel(0)
    , parentPage(0)
    , userGroups()
    , userPages()
    {
        
    }
    
    void setVisibleMinMax(bool visible);
    
    void setVisibleMenuItems(bool visible);
    
    void setVisibleAnimates(bool visible);
    
    void setVisibleEvaluate(bool visible);
    
    void setVisibleStartNewLine(bool visible);
    
    void setVisibleHide(bool visible);
    
    void setVisibleMultiLine(bool visible);
    
    void setVisibleRichText(bool visible);
    
    void setVisibleSequence(bool visible);
    
    void setVisibleMultiPath(bool visible);
    
    void setVisibleGrpAsTab(bool visible);
    
    void setVisibleParent(bool visible);
    
    void setVisiblePage(bool visible);
    
    void setVisibleDefaultValues(bool visible,AddKnobDialogPrivate::DefaultValueType type,int dimensions);
    
    void createKnobFromSelection(int type,int optionalGroupIndex = -1);
    
    Group_Knob* getSelectedGroup() const;
};

static int getChoiceIndexFromKnobType(KnobI* knob)
{
    
    int dim = knob->getDimension();
    
    Int_Knob* isInt = dynamic_cast<Int_Knob*>(knob);
    Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob);
    Color_Knob* isColor = dynamic_cast<Color_Knob*>(knob);
    Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob);
    Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob);
    String_Knob* isStr = dynamic_cast<String_Knob*>(knob);
    File_Knob* isFile = dynamic_cast<File_Knob*>(knob);
    OutputFile_Knob* isOutputFile = dynamic_cast<OutputFile_Knob*>(knob);
    Path_Knob* isPath = dynamic_cast<Path_Knob*>(knob);
    Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob);
    Page_Knob* isPage = dynamic_cast<Page_Knob*>(knob);
    Button_Knob* isBtn = dynamic_cast<Button_Knob*>(knob);
    
    if (isInt) {
        if (dim == 1) {
            return 0;
        } else if (dim == 2) {
            return 1;
        } else if (dim == 3) {
            return 2;
        }
    } else if (isDbl) {
        if (dim == 1) {
            return 3;
        } else if (dim == 2) {
            return 4;
        } else if (dim == 3) {
            return 5;
        }
    } else if (isColor) {
        if (dim == 3) {
            return 6;
        } else if (dim == 4) {
            return 7;
        }
    } else if (isChoice) {
        return 8;
    } else if (isBool) {
        return 9;
    } else if (isStr) {
        if (isStr->isLabel()) {
            return 10;
        } else  {
            return 11;
        }
    } else if (isFile) {
        return 12;
    } else if (isOutputFile) {
        return 13;
    } else if (isPath) {
        return 14;
    } else if (isGrp) {
        return 15;
    } else if (isPage) {
        return 16;
    } else if (isBtn) {
        return 17;
    }
    return -1;
}

AddKnobDialog::AddKnobDialog(DockablePanel* panel,const boost::shared_ptr<KnobI>& knob,QWidget* parent)
: QDialog(parent)
, _imp(new AddKnobDialogPrivate(panel))
{
    
    _imp->knob = knob;
    assert(!knob || knob->isUserKnob());
    
    //QFont font(NATRON_FONT,NATRON_FONT_SIZE_11);
    
    _imp->vLayout = new QVBoxLayout(this);
    _imp->vLayout->setContentsMargins(0, 0, 15, 0);
    
    _imp->mainContainer = new QWidget(this);
    _imp->mainLayout = new QFormLayout(_imp->mainContainer);
    _imp->mainLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    _imp->mainLayout->setFormAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _imp->mainLayout->setSpacing(3);
    _imp->mainLayout->setContentsMargins(0, 0, 15, 0);
    
    _imp->vLayout->addWidget(_imp->mainContainer);
    
    {
        QWidget* firstRowContainer = new QWidget(this);
        QHBoxLayout* firstRowLayout = new QHBoxLayout(firstRowContainer);
        firstRowLayout->setContentsMargins(0, 0, 0, 0);
        
        _imp->nameLabel = new Natron::Label(tr("Script name:"),this);
        _imp->nameLineEdit = new LineEdit(firstRowContainer);
        _imp->nameLineEdit->setToolTip(Natron::convertFromPlainText(tr("The name of the parameter as it will be used in Python scripts"), Qt::WhiteSpaceNormal));
        
        if (knob) {
            _imp->nameLineEdit->setText(knob->getName().c_str());
        }
        firstRowLayout->addWidget(_imp->nameLineEdit);
        firstRowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->nameLabel, firstRowContainer);
        
    }
    
    {
        QWidget* secondRowContainer = new QWidget(this);
        QHBoxLayout* secondRowLayout = new QHBoxLayout(secondRowContainer);
        secondRowLayout->setContentsMargins(0, 0, 15, 0);
        _imp->labelLabel = new Natron::Label(tr("Label:"),secondRowContainer);
        _imp->labelLineEdit = new LineEdit(secondRowContainer);
        _imp->labelLineEdit->setToolTip(Natron::convertFromPlainText(tr("The label of the parameter as displayed on the graphical user interface"), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->labelLineEdit->setText(knob->getDescription().c_str());
        }
        secondRowLayout->addWidget(_imp->labelLineEdit);
        _imp->hideLabel = new Natron::Label(tr("Hide:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->hideLabel);
        _imp->hideBox = new QCheckBox(secondRowContainer);
        _imp->hideBox->setToolTip(Natron::convertFromPlainText(tr("If checked the parameter will not be visible on the user interface"), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->hideBox->setChecked(knob->getIsSecret());
        }
        secondRowLayout->addWidget(_imp->hideBox);
        _imp->startNewLineLabel = new Natron::Label(tr("Start new line:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->startNewLineLabel);
        _imp->startNewLineBox = new QCheckBox(secondRowContainer);
        _imp->startNewLineBox->setToolTip(Natron::convertFromPlainText(tr("If unchecked the parameter will be on the same line as the previous parameter"), Qt::WhiteSpaceNormal));
        if (knob) {
            
            // get the flag on the previous knob
            bool startNewLine = true;
            boost::shared_ptr<KnobI> parentKnob = _imp->knob->getParentKnob();
            if (parentKnob) {
                Group_Knob* parentIsGrp = dynamic_cast<Group_Knob*>(parentKnob.get());
                Page_Knob* parentIsPage = dynamic_cast<Page_Knob*>(parentKnob.get());
                assert(parentIsGrp || parentIsPage);
                std::vector<boost::shared_ptr<KnobI> > children;
                if (parentIsGrp) {
                    children = parentIsGrp->getChildren();
                } else if (parentIsPage) {
                    children = parentIsPage->getChildren();
                }
                for (U32 i = 0; i < children.size(); ++i) {
                    if (children[i] == _imp->knob) {
                        if (i > 0) {
                            startNewLine = children[i - 1]->isNewLineActivated();
                        }
                        break;
                    }
                }
            }

            
            _imp->startNewLineBox->setChecked(startNewLine);
        }
        secondRowLayout->addWidget(_imp->startNewLineBox);
        secondRowLayout->addStretch();
        
        _imp->mainLayout->addRow(_imp->labelLabel, secondRowContainer);
    }
    
    {
        QWidget* thirdRowContainer = new QWidget(this);
        QHBoxLayout* thirdRowLayout = new QHBoxLayout(thirdRowContainer);
        thirdRowLayout->setContentsMargins(0, 0, 15, 0);
        
        if (!knob) {
            _imp->typeLabel = new Natron::Label(tr("Type:"),thirdRowContainer);
            _imp->typeChoice = new ComboBox(thirdRowContainer);
            _imp->typeChoice->setToolTip(Natron::convertFromPlainText(tr("The data type of the parameter."), Qt::WhiteSpaceNormal));
            _imp->typeChoice->addItem("Integer");
            _imp->typeChoice->addItem("Integer 2D");
            _imp->typeChoice->addItem("Integer 3D");
            _imp->typeChoice->addItem("Floating point");
            _imp->typeChoice->addItem("Floating point 2D");
            _imp->typeChoice->addItem("Floating point 3D");
            _imp->typeChoice->addItem("Color RGB");
            _imp->typeChoice->addItem("Color RGBA");
            _imp->typeChoice->addItem("Choice (Pulldown)");
            _imp->typeChoice->addItem("Checkbox");
            _imp->typeChoice->addItem("Label");
            _imp->typeChoice->addItem("Text input");
            _imp->typeChoice->addItem("Input file");
            _imp->typeChoice->addItem("Output file");
            _imp->typeChoice->addItem("Directory");
            _imp->typeChoice->addItem("Group");
            _imp->typeChoice->addItem("Page");
            _imp->typeChoice->addItem("Button");
            QObject::connect(_imp->typeChoice, SIGNAL(currentIndexChanged(int)),this, SLOT(onTypeCurrentIndexChanged(int)));
            
            thirdRowLayout->addWidget(_imp->typeChoice);
        }
        _imp->animatesLabel = new Natron::Label(tr("Animates:"),thirdRowContainer);

        if (!knob) {
            thirdRowLayout->addWidget(_imp->animatesLabel);
        }
        _imp->animatesCheckbox = new QCheckBox(thirdRowContainer);
        _imp->animatesCheckbox->setToolTip(Natron::convertFromPlainText(tr("When checked this parameter will be able to animate with keyframes."), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->animatesCheckbox->setChecked(knob->isAnimationEnabled());
        }
        thirdRowLayout->addWidget(_imp->animatesCheckbox);
        _imp->evaluatesLabel = new Natron::Label(Natron::convertFromPlainText(tr("Render on change:"), Qt::WhiteSpaceNormal),thirdRowContainer);
        thirdRowLayout->addWidget(_imp->evaluatesLabel);
        _imp->evaluatesOnChange = new QCheckBox(thirdRowContainer);
        _imp->evaluatesOnChange->setToolTip(Natron::convertFromPlainText(tr("If checked, when the value of this parameter changes a new render will be triggered."), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->evaluatesOnChange->setChecked(knob->getEvaluateOnChange());
        }
        thirdRowLayout->addWidget(_imp->evaluatesOnChange);
        thirdRowLayout->addStretch();
        
        if (!knob) {
            _imp->mainLayout->addRow(_imp->typeLabel, thirdRowContainer);
        } else {
            _imp->mainLayout->addRow(_imp->animatesLabel, thirdRowContainer);
        }
    }
    {
        _imp->tooltipLabel = new Natron::Label(tr("Tooltip:"),this);
        _imp->tooltipArea = new QTextEdit(this);
        _imp->tooltipArea->setToolTip(Natron::convertFromPlainText(tr("The help tooltip that will appear when hovering the parameter with the mouse."), Qt::WhiteSpaceNormal));
        _imp->mainLayout->addRow(_imp->tooltipLabel,_imp->tooltipArea);
        if (knob) {
            _imp->tooltipArea->setPlainText(knob->getHintToolTip().c_str());
        }
    }
    {
        _imp->menuItemsLabel = new Natron::Label(tr("Menu items:"),this);
        _imp->menuItemsEdit = new QTextEdit(this);
        QString tt = Natron::convertFromPlainText(tr("The entries that will be available in the drop-down menu. \n"
                                                 "Each line defines a new menu entry. You can specify a specific help tooltip for each entry "
                                                 "by separating the entry text from the help with the following characters on the line: "
                                                 "<?> \n\n"
                                                 "E.g: Special function<?>Will use our very own special function."), Qt::WhiteSpaceNormal);
        _imp->menuItemsEdit->setToolTip(tt);
        _imp->mainLayout->addRow(_imp->menuItemsLabel,_imp->menuItemsEdit);
        
        Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob.get());
        if (isChoice) {
            std::vector<std::string> entries = isChoice->getEntries_mt_safe();
            std::vector<std::string> entriesHelp = isChoice->getEntriesHelp_mt_safe();
            QString data;
            for (U32 i = 0; i < entries.size(); ++i) {
                QString line(entries[i].c_str());
                if (i < entriesHelp.size() && !entriesHelp[i].empty()) {
                    line.append("<?>");
                    line.append(entriesHelp[i].c_str());
                }
                data.append(line);
                data.append('\n');
            }
            _imp->menuItemsEdit->setPlainText(data);
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->multiLineLabel = new Natron::Label(tr("Multi-line:"),optContainer);
        _imp->multiLine = new QCheckBox(optContainer);
        _imp->multiLine->setToolTip(Natron::convertFromPlainText(tr("Should this text be multi-line or single-line ?"), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->multiLine);
        _imp->mainLayout->addRow(_imp->multiLineLabel, optContainer);
        
        String_Knob* isStr = dynamic_cast<String_Knob*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiLine()) {
                _imp->multiLine->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->richTextLabel = new Natron::Label(tr("Rich text:"),optContainer);
        _imp->richText = new QCheckBox(optContainer);
        QString tt = Natron::convertFromPlainText(tr("If checked, the text area will be able to use rich text encoding with a sub-set of html.\n "
                                                 "This property is only valid for multi-line input text only."), Qt::WhiteSpaceNormal);

        _imp->richText->setToolTip(tt);
        optLayout->addWidget(_imp->richText);
        _imp->mainLayout->addRow(_imp->richTextLabel, optContainer);
        
        String_Knob* isStr = dynamic_cast<String_Knob*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiLine() && isStr->usesRichText()) {
                _imp->richText->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->sequenceDialogLabel = new Natron::Label(tr("Use sequence dialog:"),optContainer);
        _imp->sequenceDialog = new QCheckBox(optContainer);
        _imp->sequenceDialog->setToolTip(Natron::convertFromPlainText(tr("If checked the file dialog for this parameter will be able to decode image sequences."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->sequenceDialog);
        _imp->mainLayout->addRow(_imp->sequenceDialogLabel, optContainer);
        
        File_Knob* isFile = dynamic_cast<File_Knob*>(knob.get());
        OutputFile_Knob* isOutFile = dynamic_cast<OutputFile_Knob*>(knob.get());
        if (isFile) {
            if (isFile->isInputImageFile()) {
                _imp->sequenceDialog->setChecked(true);
            }
        } else if (isOutFile) {
            if (isOutFile->isOutputImageFile()) {
                _imp->sequenceDialog->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->multiPathLabel = new Natron::Label(Natron::convertFromPlainText(tr("Multiple paths:"), Qt::WhiteSpaceNormal),optContainer);
        _imp->multiPath = new QCheckBox(optContainer);
        _imp->multiPath->setToolTip(Natron::convertFromPlainText(tr("If checked the parameter will be a table where each entry points to a different path."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->multiPath);
        _imp->mainLayout->addRow(_imp->multiPathLabel, optContainer);
        
        Path_Knob* isStr = dynamic_cast<Path_Knob*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiPath()) {
                _imp->multiPath->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->groupAsTabLabel = new Natron::Label(tr("Group as tab:"),optContainer);
        _imp->groupAsTab = new QCheckBox(optContainer);
        _imp->groupAsTab->setToolTip(Natron::convertFromPlainText(tr("If checked the group will be a tab instead."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->groupAsTab);
        _imp->mainLayout->addRow(_imp->groupAsTabLabel, optContainer);
        
        Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob.get());
        if (isGrp) {
            if (isGrp && isGrp->isTab()) {
                _imp->groupAsTab->setChecked(true);
            }
        }

    }
    {
        QWidget* minMaxContainer = new QWidget(this);
        QWidget* dminMaxContainer = new QWidget(this);
        QHBoxLayout* minMaxLayout = new QHBoxLayout(minMaxContainer);
        QHBoxLayout* dminMaxLayout = new QHBoxLayout(dminMaxContainer);
        minMaxLayout->setContentsMargins(0, 0, 0, 0);
        dminMaxLayout->setContentsMargins(0, 0, 0, 0);
        _imp->minLabel = new Natron::Label(tr("Minimum:"),minMaxContainer);

        _imp->minBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->minBox->setToolTip(Natron::convertFromPlainText(tr("Set the minimum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval."), Qt::WhiteSpaceNormal));
        minMaxLayout->addWidget(_imp->minBox);
        
        _imp->maxLabel = new Natron::Label(tr("Maximum:"),minMaxContainer);
        _imp->maxBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->maxBox->setToolTip(Natron::convertFromPlainText(tr("Set the maximum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval."), Qt::WhiteSpaceNormal));
        minMaxLayout->addWidget(_imp->maxLabel);
        minMaxLayout->addWidget(_imp->maxBox);
        minMaxLayout->addStretch();
        
        _imp->dminLabel = new Natron::Label(tr("Display Minimum:"),dminMaxContainer);
        _imp->dminBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dminBox->setToolTip(Natron::convertFromPlainText(tr("Set the display minimum value for the parameter. This is a hint that is typically "
                                                              "used to set the range of the slider."), Qt::WhiteSpaceNormal));
        dminMaxLayout->addWidget(_imp->dminBox);
        
        _imp->dmaxLabel = new Natron::Label(tr("Display Maximum:"),dminMaxContainer);
        _imp->dmaxBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dmaxBox->setToolTip(Natron::convertFromPlainText(tr("Set the display maximum value for the parameter. This is a hint that is typically "
                                                              "used to set the range of the slider."), Qt::WhiteSpaceNormal));
        dminMaxLayout->addWidget(_imp->dmaxLabel);
        dminMaxLayout->addWidget(_imp->dmaxBox);
       
        dminMaxLayout->addStretch();
        
        Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob.get());
        Int_Knob* isInt = dynamic_cast<Int_Knob*>(knob.get());
        Color_Knob* isColor = dynamic_cast<Color_Knob*>(knob.get());
        

        if (isDbl) {
            double min = isDbl->getMinimum(0);
            double max = isDbl->getMaximum(0);
            double dmin = isDbl->getDisplayMinimum(0);
            double dmax = isDbl->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);
        } else if (isInt) {
            int min = isInt->getMinimum(0);
            int max = isInt->getMaximum(0);
            int dmin = isInt->getDisplayMinimum(0);
            int dmax = isInt->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);

        } else if (isColor) {
            double min = isColor->getMinimum(0);
            double max = isColor->getMaximum(0);
            double dmin = isColor->getDisplayMinimum(0);
            double dmax = isColor->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);

        }
        
        _imp->mainLayout->addRow(_imp->minLabel, minMaxContainer);
        _imp->mainLayout->addRow(_imp->dminLabel, dminMaxContainer);
    }
    
    {
        QWidget* defValContainer = new QWidget(this);
        QHBoxLayout* defValLayout = new QHBoxLayout(defValContainer);
        defValLayout->setContentsMargins(0, 0, 0, 0);
        _imp->defaultValueLabel = new Natron::Label(tr("Default value:"),defValContainer);

        _imp->default0 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default0->setValue(0);
        _imp->default0->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 0)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default0);
        
        _imp->default1 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default1->setValue(0);
        _imp->default1->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 1)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default1);
        
        _imp->default2 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default2->setValue(0);
        _imp->default2->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 2)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default2);
        
        _imp->default3 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default3->setValue(0);
        _imp->default3->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 3)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default3);

        
        _imp->defaultStr = new LineEdit(defValContainer);
        _imp->defaultStr->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->defaultStr);
        
        
        _imp->defaultBool = new QCheckBox(defValContainer);
        _imp->defaultBool->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter."), Qt::WhiteSpaceNormal));
        _imp->defaultBool->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
        defValLayout->addWidget(_imp->defaultBool);

        defValLayout->addStretch();
        
        _imp->mainLayout->addRow(_imp->defaultValueLabel, defValContainer);
        
        
        Knob<double>* isDbl = dynamic_cast<Knob<double>*>(knob.get());
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
        Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob.get());
        Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(knob.get());
        
        if (isDbl) {
            _imp->default0->setValue(isDbl->getDefaultValue(0));
            if (isDbl->getDimension() >= 2) {
                _imp->default1->setValue(isDbl->getDefaultValue(1));
            }
            if (isDbl->getDimension() >= 3) {
                _imp->default2->setValue(isDbl->getDefaultValue(2));
            }
            if (isDbl->getDimension() >= 4) {
                _imp->default3->setValue(isDbl->getDefaultValue(3));
            }
        } else if (isInt) {
            _imp->default0->setValue(isInt->getDefaultValue(0));
            if (isInt->getDimension() >= 2) {
                _imp->default1->setValue(isInt->getDefaultValue(1));
            }
            if (isInt->getDimension() >= 3) {
                _imp->default2->setValue(isInt->getDefaultValue(2));
            }

        } else if (isBool) {
            _imp->defaultBool->setChecked(isBool->getDefaultValue(0));
        } else if (isStr) {
            _imp->defaultStr->setText(isStr->getDefaultValue(0).c_str());
        }
        
    }
    
    
    const std::map<boost::weak_ptr<KnobI>,KnobGui*>& knobs = _imp->panel->getKnobs();
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        if (knob->isUserKnob()) {
            Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob.get());
            if (isGrp) {
                _imp->userGroups.push_back(isGrp);
            }
        }
    }
    
    
    
    if (!_imp->userGroups.empty()) {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->parentGroupLabel = new Natron::Label(tr("Group:"),optContainer);
        _imp->parentGroup = new ComboBox(optContainer);
        
        _imp->parentGroup->setToolTip(Natron::convertFromPlainText(tr("The name of the group under which this parameter will appear."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->parentGroup);
        
        _imp->mainLayout->addRow(_imp->parentGroupLabel, optContainer);
    }
    
    if (!knob) {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        _imp->parentPageLabel = new Natron::Label(tr("Page:"),optContainer);
        _imp->parentPage = new ComboBox(optContainer);
        _imp->parentPage->addItem(NATRON_USER_MANAGED_KNOBS_PAGE);
        QObject::connect(_imp->parentPage,SIGNAL(currentIndexChanged(int)),this,SLOT(onPageCurrentIndexChanged(int)));
        const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->panel->getHolder()->getKnobs();
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin() ; it != knobs.end(); ++it) {
            if ((*it)->isUserKnob()) {
                Page_Knob* isPage = dynamic_cast<Page_Knob*>(it->get());
                if (isPage && isPage->getName() != NATRON_USER_MANAGED_KNOBS_PAGE) {
                    _imp->userPages.push_back(isPage);
                }
            }
        }
        
        for (std::list<Page_Knob*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
            _imp->parentPage->addItem((*it)->getName().c_str());
        }
        _imp->parentPage->setToolTip(Natron::convertFromPlainText(tr("The tab under which this parameter will appear."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->parentPage);
        if (knob) {
            ////find in which page the knob should be
            Page_Knob* isTopLevelParentAPage = getTopLevelPageForKnob(knob.get());
            assert(isTopLevelParentAPage);
            if (isTopLevelParentAPage->getName() != NATRON_USER_MANAGED_KNOBS_PAGE) {
                int index = 1; // 1 because of the "User" item
                bool found = false;
                for (std::list<Page_Knob*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it, ++index) {
                    if ((*it) == isTopLevelParentAPage) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    _imp->parentPage->setCurrentIndex(index);
                }
            }
            
            
        }
        
        _imp->mainLayout->addRow(_imp->parentPageLabel, optContainer);
        onPageCurrentIndexChanged(0);
    } else { // if(!knob)
        
        if (_imp->parentGroup) {
            Page_Knob* topLvlPage = getTopLevelPageForKnob(knob.get());
            assert(topLvlPage);
            boost::shared_ptr<KnobI> parent = knob->getParentKnob();
            Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
            _imp->parentGroup->addItem("-");
            int idx = 1;
            for (std::list<Group_Knob*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it, ++idx) {
                Page_Knob* page = getTopLevelPageForKnob(*it);
                assert(page);
                
                ///add only grps whose parent page is the selected page
                if (page == topLvlPage) {
                    _imp->parentGroup->addItem((*it)->getName().c_str());
                    if (isParentGrp && isParentGrp == *it) {
                        _imp->parentGroup->setCurrentIndex(idx);
                    }
                }
                
            }
        }
    }
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),Qt::Horizontal,this);
    QObject::connect(buttons,SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(buttons,SIGNAL(accepted()), this, SLOT(onOkClicked()));
    _imp->vLayout->addWidget(buttons);
    
    int type;
    if (!knob) {
        type = _imp->typeChoice->activeIndex();
    } else {
        type = getChoiceIndexFromKnobType(knob.get());
        assert(type != -1);
    }
    onTypeCurrentIndexChanged(type);
    _imp->panel->setUserPageActiveIndex();
    
    if (knob) {
        _imp->originalKnobSerialization.reset(new KnobSerialization(knob));
    }
}

void
AddKnobDialog::onPageCurrentIndexChanged(int index)
{
    if (!_imp->parentGroup) {
        return;
    }
    _imp->parentGroup->clear();
    _imp->parentGroup->addItem("-");
    
    std::string selectedPage = _imp->parentPage->itemText(index).toStdString();
    Page_Knob* parentPage = 0;
    
    if (selectedPage == NATRON_USER_MANAGED_KNOBS_PAGE) {
        parentPage = _imp->panel->getUserPageKnob().get();
    } else {
        for (std::list<Page_Knob*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
            if ((*it)->getName() == selectedPage) {
                parentPage = *it;
                break;
            }
        }
    }
    
    for (std::list<Group_Knob*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it) {
        Page_Knob* page = getTopLevelPageForKnob(*it);
        assert(page);
        
        ///add only grps whose parent page is the selected page
        if (page == parentPage) {
            _imp->parentGroup->addItem((*it)->getName().c_str());
        }
        
    }
}

void
AddKnobDialog::onTypeCurrentIndexChanged(int index)
{
    _imp->setVisiblePage(index != 16);
    switch (index) {
        case 0: // int
        case 1: // int 2D
        case 2: // int 3D
        case 3: // fp
        case 4: // fp 2D
        case 5: // fp 3D
        case 6: // RGB
        case 7: // RGBA
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(true);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            if (index == 0 || index == 3) {
                _imp->setVisibleDefaultValues(true,
                                              index == 0 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              1);
            } else if (index == 1 || index == 4) {
                _imp->setVisibleDefaultValues(true,
                                              index == 1 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              2);
            } else if (index == 2 || index == 5 || index == 6) {
                _imp->setVisibleDefaultValues(true,
                                              index == 2 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              3);
            } else if (index == 7) {
                _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeDouble,4);
            }
            break;
        case 8: // choice
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(true);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 9: // bool
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeBool,1);
            break;
        case 10: // label
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 11: // text input
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(true);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(true);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 12: // input file
        case 13: // output file
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(true);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 14: // path
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(true);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 15: // grp
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(true);
            _imp->setVisibleParent(false);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 16: // page
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(false);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(false);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 17: // button
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(false);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        default:
            break;
    }
}

AddKnobDialog::~AddKnobDialog()
{
    
}

boost::shared_ptr<KnobI>
AddKnobDialog::getKnob() const
{
    return _imp->knob;
}

void
AddKnobDialogPrivate::createKnobFromSelection(int index,int optionalGroupIndex)
{
    assert(!knob);
    std::string label = labelLineEdit->text().toStdString();
    
    switch (index) {
        case 0:
        case 1:
        case 2: {
            //int
            int dim = index + 1;
            boost::shared_ptr<Int_Knob> k = Natron::createKnob<Int_Knob>(panel->getHolder(), label, dim, false);
            std::vector<int> mins(dim),dmins(dim);
            std::vector<int> maxs(dim),dmaxs(dim);
            
            for (int i = 0; i < dim; ++i) {
                mins[i] = std::floor(minBox->value() + 0.5);
                dmins[i] = std::floor(dminBox->value() + 0.5);
                maxs[i] = std::floor(maxBox->value() + 0.5);
                dmaxs[i] = std::floor(dmaxBox->value() + 0.5);
            }
            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            std::vector<int> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }
            knob = k;
        } break;
        case 3:
        case 4:
        case 5: {
            //double
            int dim = index - 2;
            boost::shared_ptr<Double_Knob> k = Natron::createKnob<Double_Knob>(panel->getHolder(), label, dim, false);
            std::vector<double> mins(dim),dmins(dim);
            std::vector<double> maxs(dim),dmaxs(dim);
            for (int i = 0; i < dim; ++i) {
                mins[i] = minBox->value();
                dmins[i] = dminBox->value();
                maxs[i] = maxBox->value();
                dmaxs[i] = dmaxBox->value();
            }
            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            std::vector<double> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }

            
            knob = k;
        } break;
        case 6:
        case 7: {
            // color
            int dim = index - 3;
            boost::shared_ptr<Color_Knob> k = Natron::createKnob<Color_Knob>(panel->getHolder(), label, dim, false);
            std::vector<double> mins(dim),dmins(dim);
            std::vector<double> maxs(dim),dmaxs(dim);
            for (int i = 0; i < dim; ++i) {
                mins[i] = minBox->value();
                dmins[i] = dminBox->value();
                maxs[i] = maxBox->value();
                dmaxs[i] = dmaxBox->value();
            }
            std::vector<double> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            if (dim >= 4) {
                defValues.push_back(default3->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }

            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            knob = k;
        }  break;
        case 8: {

            boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(panel->getHolder(), label, 1, false);
            QString entriesRaw = menuItemsEdit->toPlainText();
            QTextStream stream(&entriesRaw);
            std::vector<std::string> entries,helps;

            while (!stream.atEnd()) {
                QString line = stream.readLine();
                int foundHelp = line.indexOf("<?>");
                if (foundHelp != -1) {
                    QString entry = line.mid(0,foundHelp);
                    QString help = line.mid(foundHelp + 3,-1);
                    for (int i = 0; i < (int)entries.size() - (int)helps.size(); ++i) {
                        helps.push_back("");
                    }
                    entries.push_back(entry.toStdString());
                    helps.push_back(help.toStdString());
                } else {
                    entries.push_back(line.toStdString());
                    if (!helps.empty()) {
                        helps.push_back("");
                    }
                }
            }
            k->populateChoices(entries,helps);
            
            int defValue = default0->value();
            if (defValue < (int)entries.size() && defValue >= 0) {
                k->setDefaultValue(defValue);
            }
            
            knob = k;
        } break;
        case 9: {
            boost::shared_ptr<Bool_Knob> k = Natron::createKnob<Bool_Knob>(panel->getHolder(), label, 1, false);
            bool defValue = defaultBool->isChecked();
            k->setDefaultValue(defValue);
            knob = k;
        }   break;
        case 10:
        case 11: {
            boost::shared_ptr<String_Knob> k = Natron::createKnob<String_Knob>(panel->getHolder(), label, 1, false);
            if (multiLine->isChecked()) {
                k->setAsMultiLine();
                if (richText->isChecked()) {
                    k->setUsesRichText(true);
                }
            } else {
                if (index == 10) {
                    k->setAsLabel();
                }
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        }   break;
        case 12: {
            boost::shared_ptr<File_Knob> k = Natron::createKnob<File_Knob>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsInputImage();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 13: {
            boost::shared_ptr<OutputFile_Knob> k = Natron::createKnob<OutputFile_Knob>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsOutputImageFile();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 14: {
            boost::shared_ptr<Path_Knob> k = Natron::createKnob<Path_Knob>(panel->getHolder(), label, 1, false);
            if (multiPath->isChecked()) {
                k->setMultiPath(true);
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 15: {
            boost::shared_ptr<Group_Knob> k = Natron::createKnob<Group_Knob>(panel->getHolder(), label, 1, false);
            if (groupAsTab->isChecked()) {
                k->setAsTab();
            }
            k->setDefaultValue(true); //< default to opened
            knob = k;
        } break;
        case 16: {
            boost::shared_ptr<Page_Knob> k = Natron::createKnob<Page_Knob>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        case 17: {
            boost::shared_ptr<Button_Knob> k = Natron::createKnob<Button_Knob>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        default:
            break;
    }
    
    
    assert(knob);
    knob->setAsUserKnob();
    if (knob->canAnimate()) {
        knob->setAnimationEnabled(animatesCheckbox->isChecked());
    }
    knob->setEvaluateOnChange(evaluatesOnChange->isChecked());
    
    
    
    knob->setSecret(hideBox->isChecked());
    knob->setName(nameLineEdit->text().toStdString());
    knob->setHintToolTip(tooltipArea->toPlainText().toStdString());
    bool addedInGrp = false;
    Group_Knob* selectedGrp = getSelectedGroup();
    if (selectedGrp) {
        if (optionalGroupIndex != -1) {
            selectedGrp->insertKnob(optionalGroupIndex, knob);
        } else {
            selectedGrp->addKnob(knob);
        }
        addedInGrp = true;
    }
    
    
    if (index != 16 && parentPage && !addedInGrp) {
        std::string selectedItem = parentPage->getCurrentIndexText().toStdString();
        if (selectedItem == NATRON_USER_MANAGED_KNOBS_PAGE) {
            boost::shared_ptr<Page_Knob> userPage = panel->getUserPageKnob();
            userPage->addKnob(knob);
            panel->setUserPageActiveIndex();
        } else {
            for (std::list<Page_Knob*>::iterator it = userPages.begin(); it != userPages.end(); ++it) {
                if ((*it)->getName() == selectedItem) {
                    (*it)->addKnob(knob);
                    break;
                }
            }

        }
    }
    
    
    KnobHolder* holder = panel->getHolder();
    assert(holder);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    assert(isEffect);
    isEffect->getNode()->declarePythonFields();
}

Group_Knob*
AddKnobDialogPrivate::getSelectedGroup() const
{
    if (parentGroup) {
        std::string selectedItem = parentGroup->getCurrentIndexText().toStdString();
        if (selectedItem != "-") {
            for (std::list<Group_Knob*>::const_iterator it = userGroups.begin(); it != userGroups.end(); ++it) {
                if ((*it)->getName() == selectedItem) {
                    return *it;
                }
            }
        }
    }
    return 0;
}

void
AddKnobDialog::onOkClicked()
{
    QString name = _imp->nameLineEdit->text();
    bool badFormat = false;
    if (name.isEmpty()) {
        badFormat = true;
    }
    if (!badFormat && !name[0].isLetter()) {
        badFormat = true;
    }
    
    if (!badFormat) {
        //make sure everything is alphaNumeric without spaces
        for (int i = 0; i < name.size(); ++i) {
            if (name[i] == QChar('_')) {
                continue;
            }
            if (name[i] == QChar(' ') || !name[i].isLetterOrNumber()) {
                badFormat = true;
                break;
            }
        }
    }
    
    //check if another knob has the same script name
    std::string stdName = name.toStdString();
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin() ; it != knobs.end(); ++it) {
        if ((*it)->getName() == stdName && (*it) != _imp->knob) {
            badFormat = true;
            break;
        }
    }
    
    if (badFormat) {
        Natron::errorDialog(tr("Error").toStdString(), tr("A parameter must have a unique script name composed only of characters from "
                                                          "[a - z / A- Z] and digits [0 - 9]. This name cannot contain spaces for scripting purposes.")
                            .toStdString());
        return;
        
    } else {
        ///Remove the previous knob, and recreate it.
        
        ///Index of the type in the combo
        int index;
        
        ///Remember the old page in which to insert the knob
        Page_Knob* oldParentPage = 0;
        
        ///If the knob was in a group, we need to place it at the same index
        int oldIndexInGroup = -1;
        
        std::vector<std::pair<std::string,bool> > expressions;
        std::map<boost::shared_ptr<KnobI>,std::vector<std::pair<std::string,bool> > > listenersExpressions;
        
        if (_imp->knob) {
            
            oldParentPage = getTopLevelPageForKnob(_imp->knob.get());
            index = getChoiceIndexFromKnobType(_imp->knob.get());
            boost::shared_ptr<KnobI> parent = _imp->knob->getParentKnob();
            Group_Knob* isParentGrp = dynamic_cast<Group_Knob*>(parent.get());
            if (isParentGrp && isParentGrp == _imp->getSelectedGroup()) {
                std::vector<boost::shared_ptr<KnobI> > children = isParentGrp->getChildren();
                for (U32 i = 0; i < children.size(); ++i) {
                    if (children[i] == _imp->knob) {
                        oldIndexInGroup = i;
                        break;
                    }
                }
            } else {
                std::vector<boost::shared_ptr<KnobI> > children = oldParentPage->getChildren();
                for (U32 i = 0; i < children.size(); ++i) {
                    if (children[i] == _imp->knob) {
                        oldIndexInGroup = i;
                        break;
                    }
                }
            }
            expressions.resize(_imp->knob->getDimension());
            for (std::size_t i = 0 ; i < expressions.size(); ++i) {
                std::string expr = _imp->knob->getExpression(i);
                bool useRetVar = _imp->knob->isExpressionUsingRetVariable(i);
                expressions[i] = std::make_pair(expr,useRetVar);
            }
            
            //Since removing this knob will also remove all expressions from listeners, conserve them and try
            //to recover them afterwards
            std::list<boost::shared_ptr<KnobI> > listeners;
            _imp->knob->getListeners(listeners);
            for (std::list<boost::shared_ptr<KnobI> >::iterator it = listeners.begin(); it != listeners.end(); ++it) {
                std::vector<std::pair<std::string,bool> > exprs;
                for (int i = 0; i < (*it)->getDimension(); ++i) {
                    std::pair<std::string,bool> e;
                    e.first = (*it)->getExpression(i);
                    e.second = (*it)->isExpressionUsingRetVariable(i);
                    exprs.push_back(e);
                }
                listenersExpressions[*it] = exprs;
            }
            
            _imp->panel->getHolder()->removeDynamicKnob(_imp->knob.get());
            
            _imp->knob.reset();
        } else {
            assert(_imp->typeChoice);
            index = _imp->typeChoice->activeIndex();
        }
        _imp->createKnobFromSelection(index, oldIndexInGroup);
        assert(_imp->knob);
        
        if (oldParentPage && !_imp->knob->getParentKnob()) {
            if (oldIndexInGroup == -1) {
                oldParentPage->addKnob(_imp->knob);
            } else {
                oldParentPage->insertKnob(oldIndexInGroup, _imp->knob);
            }
        }
        
        //If startsNewLine is false, set the flag on the previous knob
        bool startNewLine = _imp->startNewLineBox->isChecked();
        boost::shared_ptr<KnobI> parentKnob = _imp->knob->getParentKnob();
        if (parentKnob) {
            Group_Knob* parentIsGrp = dynamic_cast<Group_Knob*>(parentKnob.get());
            Page_Knob* parentIsPage = dynamic_cast<Page_Knob*>(parentKnob.get());
            assert(parentIsGrp || parentIsPage);
            std::vector<boost::shared_ptr<KnobI> > children;
            if (parentIsGrp) {
                children = parentIsGrp->getChildren();
            } else if (parentIsPage) {
                children = parentIsPage->getChildren();
            }
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    if (i > 0) {
                        children[i - 1]->setAddNewLine(startNewLine);
                    }
                    break;
                }
            }
        }
        
        
        if (_imp->originalKnobSerialization) {
            _imp->knob->clone(_imp->originalKnobSerialization->getKnob().get());
        }
        //Recover expressions
        if (!expressions.empty()) {
            try {
                for (std::size_t i = 0 ; i < expressions.size(); ++i) {
                    if (!expressions[i].first.empty()) {
                        _imp->knob->setExpression(i, expressions[i].first, expressions[i].second);
                    }
                }
            } catch (...) {
                
            }
        }
        
        //Recover listeners expressions
        if (!listenersExpressions.empty()) {
            
            for (std::map<boost::shared_ptr<KnobI>,std::vector<std::pair<std::string,bool> > >::iterator it = listenersExpressions.begin();
                 it != listenersExpressions.end(); ++it) {
                assert(it->first->getDimension() == (int)it->second.size());
                for (int i = 0; i < it->first->getDimension(); ++i) {
                    try {
                        it->first->setExpression(i, it->second[i].first, it->second[i].second);
                    } catch (...) {
                        
                    }
                }
            }
            
        }
    }
    
    accept();
}

void
AddKnobDialogPrivate::setVisibleMinMax(bool visible)
{
    minLabel->setVisible(visible);
    minBox->setVisible(visible);
    maxLabel->setVisible(visible);
    maxBox->setVisible(visible);
    dminLabel->setVisible(visible);
    dminBox->setVisible(visible);
    dmaxLabel->setVisible(visible);
    dmaxBox->setVisible(visible);
    if (typeChoice) {
        int type = typeChoice->activeIndex();
        
        if (type == 6 || type == 7) {
            // color range to 0-1
            minBox->setValue(INT_MIN);
            maxBox->setValue(INT_MAX);
            dminBox->setValue(0.);
            dmaxBox->setValue(1.);
        } else {
            minBox->setValue(INT_MIN);
            maxBox->setValue(INT_MAX);
            dminBox->setValue(0);
            dmaxBox->setValue(100);
        }
    }
}

void
AddKnobDialogPrivate::setVisibleMenuItems(bool visible)
{
    menuItemsLabel->setVisible(visible);
    menuItemsEdit->setVisible(visible);
}

void
AddKnobDialogPrivate::setVisibleAnimates(bool visible)
{
    animatesLabel->setVisible(visible);
    animatesCheckbox->setVisible(visible);
    if (!knob) {
        animatesCheckbox->setChecked(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleEvaluate(bool visible)
{
    evaluatesLabel->setVisible(visible);
    evaluatesOnChange->setVisible(visible);
    if (!knob) {
        evaluatesOnChange->setChecked(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleStartNewLine(bool visible)
{
    startNewLineLabel->setVisible(visible);
    startNewLineBox->setVisible(visible);
    if (!knob) {
        startNewLineBox->setChecked(true);
    }
}

void
AddKnobDialogPrivate::setVisibleHide(bool visible)
{
    hideLabel->setVisible(visible);
    hideBox->setVisible(visible);
    if (!knob) {
        hideBox->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleMultiLine(bool visible)
{
    multiLineLabel->setVisible(visible);
    multiLine->setVisible(visible);
    if (!knob) {
        multiLine->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleRichText(bool visible)
{
    richTextLabel->setVisible(visible);
    richText->setVisible(visible);
    if (!knob) {
        richText->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleSequence(bool visible)
{
    sequenceDialogLabel->setVisible(visible);
    sequenceDialog->setVisible(visible);
    if (!knob) {
        sequenceDialog->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleMultiPath(bool visible)
{
    multiPathLabel->setVisible(visible);
    multiPath->setVisible(visible);
    if (!knob) {
        multiPath->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleGrpAsTab(bool visible)
{
    groupAsTabLabel->setVisible(visible);
    groupAsTab->setVisible(visible);
    if (!knob) {
        groupAsTab->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleParent(bool visible)
{
    if (!userGroups.empty()) {
        assert(parentGroup);
        parentGroup->setVisible(visible);
        parentGroupLabel->setVisible(visible);
    }
}

void
AddKnobDialogPrivate::setVisiblePage(bool visible)
{
    if (parentPage) {
        parentPage->setVisible(visible);
        parentPageLabel->setVisible(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleDefaultValues(bool visible,AddKnobDialogPrivate::DefaultValueType type,int dimensions)
{
    if (!visible) {
        defaultStr->setVisible(false);
        default0->setVisible(false);
        default1->setVisible(false);
        default2->setVisible(false);
        default3->setVisible(false);
        defaultValueLabel->setVisible(false);
    } else {
        defaultValueLabel->setVisible(true);

        if (type == eDefaultValueTypeInt || type == eDefaultValueTypeDouble) {
            defaultStr->setVisible(false);
            defaultBool->setVisible(false);
            if (dimensions == 1) {
                default0->setVisible(true);
                default1->setVisible(false);
                default2->setVisible(false);
                default3->setVisible(false);
            } else if (dimensions == 2) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(false);
                default3->setVisible(false);
            } else if (dimensions == 3) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(true);
                default3->setVisible(false);
            } else if (dimensions == 4) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(true);
                default3->setVisible(true);
            } else {
                assert(false);
            }
            if (type == eDefaultValueTypeDouble) {
                default0->setType(SpinBox::eSpinBoxTypeDouble);
                default1->setType(SpinBox::eSpinBoxTypeDouble);
                default2->setType(SpinBox::eSpinBoxTypeDouble);
                default3->setType(SpinBox::eSpinBoxTypeDouble);
            } else {
                default0->setType(SpinBox::eSpinBoxTypeInt);
                default1->setType(SpinBox::eSpinBoxTypeInt);
                default2->setType(SpinBox::eSpinBoxTypeInt);
                default3->setType(SpinBox::eSpinBoxTypeInt);
            }
        } else if (type == eDefaultValueTypeString) {
            defaultStr->setVisible(true);
            default0->setVisible(false);
            default1->setVisible(false);
            default2->setVisible(false);
            default3->setVisible(false);
            defaultBool->setVisible(false);
        } else {
            defaultStr->setVisible(false);
            default0->setVisible(false);
            default1->setVisible(false);
            default2->setVisible(false);
            default3->setVisible(false);
            defaultBool->setVisible(true);
        }
    }
}

