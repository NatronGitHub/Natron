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

#include "PreferencesPanel.h"

#include <map>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"
#include "Engine/Settings.h"

#include "Gui/GuiDefines.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/LineEdit.h"
#include "Gui/Label.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Utils.h"
#include "Gui/Splitter.h"
#include "Gui/TableModelView.h"

#define COL_PLUGIN_LABEL 0
#define COL_PLUGINID COL_PLUGIN_LABEL + 1
#define COL_VERSION COL_PLUGINID + 1
#define COL_ENABLED COL_VERSION + 1
#define COL_RS_ENABLED COL_ENABLED + 1
#define COL_MT_ENABLED COL_RS_ENABLED + 1

NATRON_NAMESPACE_ENTER;

struct PreferenceTab
{
    QTreeWidgetItem* treeItem;
    QFrame* tab;
    boost::weak_ptr<KnobPageGui> page;
};

struct PluginTreeNode
{
    QTreeWidgetItem* item;
    AnimatedCheckBox* enabledCheckbox;
    AnimatedCheckBox* rsCheckbox;
    AnimatedCheckBox* mtCheckbox;
    Plugin* plugin;
};

typedef std::list<PluginTreeNode> PluginTreeNodeList;

struct PreferencesPanelPrivate
{
    PreferencesPanel* _p;
    Gui* gui;
    QVBoxLayout* mainLayout;

    Splitter* splitter;
    QTreeWidget* tree;
    std::vector<PreferenceTab> tabs;
    int currentTabIndex;

    QDialogButtonBox* buttonBox;
    Button* restoreDefaultsB;
    Button* prefsHelp;
    Button* cancelB;
    Button* okB;
    std::vector<KnobI*> changedKnobs;
    bool pluginSettingsChanged;
    bool closeIsOK;

    Label* pluginFilterLabel;
    LineEdit* pluginFilterEdit;
    QTreeWidget* pluginsView;
    PluginTreeNodeList pluginsList;

    PreferencesPanelPrivate(PreferencesPanel* p, Gui *parent)
    : _p(p)
    , gui(parent)
    , mainLayout(0)
    , splitter(0)
    , tree(0)
    , tabs()
    , currentTabIndex(-1)
    , buttonBox(0)
    , restoreDefaultsB(0)
    , prefsHelp(0)
    , cancelB(0)
    , okB(0)
    , changedKnobs(0)
    , pluginSettingsChanged(false)
    , closeIsOK(false)
    , pluginFilterLabel(0)
    , pluginFilterEdit(0)
    , pluginsView(0)
    {
        
    }

    void createPreferenceTab(const KnobPageGuiPtr& page, PreferenceTab* tab);

    void setVisiblePage(int index);

    PluginTreeNodeList::iterator buildGroupHierarchy(const QStringList& grouping);
};

PreferencesPanel::PreferencesPanel(Gui *parent)
    : QWidget(parent)
      , KnobGuiContainerHelper(appPTR->getCurrentSettings().get(), boost::shared_ptr<QUndoStack>())
      , _imp(new PreferencesPanelPrivate(this, parent))
{

}

PreferencesPanel::~PreferencesPanel()
{
    
}

PluginTreeNodeList::iterator
PreferencesPanelPrivate::buildGroupHierarchy(const QStringList& groupingSplit)
{
    if ( groupingSplit.isEmpty() ) {
        return pluginsList.end();
    }

    const QString & lastGroupName = groupingSplit.back();

    // Find out whether the deepest parent group of the action already exist
    PluginTreeNodeList::iterator foundGuiGroup = pluginsList.end();
    for (PluginTreeNodeList::iterator it2 = pluginsList.begin(); it2 != pluginsList.end(); ++it2) {
        if (it2->item->text(0) == lastGroupName) {
            foundGuiGroup = it2;
            break;
        }
    }

    QTreeWidgetItem* groupParent;
    if ( foundGuiGroup != pluginsList.end() ) {
        groupParent = foundGuiGroup->item;
    } else {
        groupParent = 0;
        for (int i = 0; i < groupingSplit.size(); ++i) {
            QTreeWidgetItem* groupingItem;
            bool existAlready = false;
            if (groupParent) {
                for (int j = 0; j < groupParent->childCount(); ++j) {
                    QTreeWidgetItem* child = groupParent->child(j);
                    if (child->text(0) == groupingSplit[i]) {
                        groupingItem = child;
                        existAlready = true;
                        break;
                    }
                }
                if (!existAlready) {
                    groupingItem = new QTreeWidgetItem(groupParent);
                    groupParent->addChild(groupingItem);
                }
            } else {
                for (int j = 0; j < pluginsView->topLevelItemCount(); ++j) {
                    QTreeWidgetItem* topLvlItem = pluginsView->topLevelItem(j);
                    if (topLvlItem->text(0) == groupingSplit[i]) {
                        groupingItem = topLvlItem;
                        existAlready = true;
                        break;
                    }
                }
                if (!existAlready) {
                    groupingItem = new QTreeWidgetItem(pluginsView);
                    pluginsView->addTopLevelItem(groupingItem);
                }
            }
            if (!existAlready) {
                groupingItem->setExpanded(false);
                groupingItem->setFlags(Qt::ItemIsEnabled);
                groupingItem->setText(COL_PLUGIN_LABEL, groupingSplit[i]);
            }
            groupParent = groupingItem;
        }
    }
    PluginTreeNode group;
    group.plugin = 0;
    group.item = groupParent;
    foundGuiGroup = pluginsList.insert(pluginsList.end(), group);

    return foundGuiGroup;
} // PreferencesPanelPrivate::buildGroupHierarchy


Gui*
PreferencesPanel::getGui() const
{
    return _imp->gui;
}

void
PreferencesPanel::createGui()
{
    //setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setWindowFlags(Qt::Window);
    setWindowTitle( tr("Preferences") );
    _imp->mainLayout = new QVBoxLayout(this);
    //_imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    //_imp->mainLayout->setSpacing(0);

    _imp->splitter = new Splitter(Qt::Horizontal, this);
    //_imp->centerLayout->setContentsMargins(0, 0, 0, 0);
    //_imp->centerLayout->setSpacing(0);

    _imp->tree = new QTreeWidget(_imp->splitter);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setColumnCount(1);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _imp->tree->header()->close();
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));

    initializeKnobs();

    _imp->splitter->addWidget(_imp->tree);

    _imp->tabs[0].treeItem->setSelected(true);
    onItemSelectionChanged();

    int maxLength = 0;
    QFontMetrics fm = fontMetrics();

    QGridLayout* pluginsFrameLayout = 0;
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        boost::shared_ptr<KnobPage> pageKnob = _imp->tabs[i].page.lock()->pageKnob.lock();
        if (pageKnob->getName() == "plugins") {
            pluginsFrameLayout = _imp->tabs[i].page.lock()->gridLayout;
        }
        QString label = QString::fromUtf8(pageKnob->getLabel().c_str());
        int w = fm.width(label);
        maxLength = std::max(w, maxLength);
    }
    assert(pluginsFrameLayout);

    _imp->tree->setFixedWidth(maxLength + 100);

    _imp->buttonBox = new QDialogButtonBox(Qt::Horizontal);
    _imp->restoreDefaultsB = new Button( tr("Restore defaults") );
    _imp->restoreDefaultsB->setToolTip( GuiUtils::convertFromPlainText(tr("Restore default values for all preferences."), Qt::WhiteSpaceNormal) );

    _imp->prefsHelp = new Button( tr("Help") );
    _imp->prefsHelp->setToolTip( tr("Show help for preference") );

    _imp->cancelB = new Button( tr("Discard") );
    _imp->cancelB->setToolTip( GuiUtils::convertFromPlainText(tr("Cancel changes that were not saved and close the window."), Qt::WhiteSpaceNormal) );
    _imp->okB = new Button( tr("Save") );
    _imp->okB->setToolTip( GuiUtils::convertFromPlainText(tr("Save changes on disk and close the window."), Qt::WhiteSpaceNormal) );
    _imp->buttonBox->addButton(_imp->restoreDefaultsB, QDialogButtonBox::ResetRole);
    _imp->buttonBox->addButton(_imp->prefsHelp, QDialogButtonBox::ResetRole);
    _imp->buttonBox->addButton(_imp->cancelB, QDialogButtonBox::RejectRole);
    _imp->buttonBox->addButton(_imp->okB, QDialogButtonBox::AcceptRole);

    _imp->mainLayout->addWidget(_imp->splitter);
    _imp->mainLayout->addWidget(_imp->buttonBox);

    QObject::connect( _imp->restoreDefaultsB, SIGNAL(clicked()), this, SLOT(restoreDefaults()) );
    QObject::connect( _imp->prefsHelp, SIGNAL(clicked()), this, SLOT(openHelp()) );
    QObject::connect( _imp->buttonBox, SIGNAL(rejected()), this, SLOT(cancelChanges()) );
    QObject::connect( _imp->buttonBox, SIGNAL(accepted()), this, SLOT(saveChangesAndClose()) );
    QObject::connect( appPTR->getCurrentSettings().get(), SIGNAL(settingChanged(KnobI*)), this, SLOT(onSettingChanged(KnobI*)) );


    // Create plug-ins view

    _imp->pluginsView = new QTreeWidget(this);
    _imp->pluginsView->setAttribute(Qt::WA_MacShowFocusRect, 0);

    Label* restartWarningLabel = new Label(tr("<i>Changing any plug-in property requires a restart of %1</i>").arg(QString::fromUtf8(NATRON_APPLICATION_NAME)), this);
    pluginsFrameLayout->addWidget(restartWarningLabel, pluginsFrameLayout->rowCount(), 0, 1, 2);

    _imp->pluginFilterLabel = new Label(tr("Plugin Filter"), this);

    QString filterTt = tr("Display only plug-ins labels containing the letters in the filter much like the Tab menu in the Node Graph");
    _imp->pluginFilterLabel->setToolTip(filterTt);

    _imp->pluginFilterEdit = new LineEdit(this);
    _imp->pluginFilterEdit->setToolTip(filterTt);
    int row = pluginsFrameLayout->rowCount();
    pluginsFrameLayout->addWidget(_imp->pluginFilterLabel, row, 0, 1, 1);
    pluginsFrameLayout->addWidget(_imp->pluginFilterEdit, row, 1, 1, 1);
    connect( _imp->pluginFilterEdit, SIGNAL(textChanged(QString)), this, SLOT(filterPlugins(QString)) );

    pluginsFrameLayout->addWidget(_imp->pluginsView, pluginsFrameLayout->rowCount(), 0, 1, 2);

    QTreeWidgetItem* treeHeader = new QTreeWidgetItem;
    treeHeader->setText(COL_PLUGIN_LABEL, tr("Plugin Label") );
    treeHeader->setText(COL_PLUGINID, tr("Plugin ID") );
    treeHeader->setText(COL_VERSION, tr("Plugin Version") );
    treeHeader->setText(COL_ENABLED, tr("Enabled"));
    treeHeader->setText(COL_RS_ENABLED, tr("Render-Scale Enabled"));
    treeHeader->setText(COL_MT_ENABLED, tr("Multi-threading Enabled"));
    _imp->pluginsView->setHeaderItem(treeHeader);
    _imp->pluginsView->setSelectionMode(QAbstractItemView::NoSelection);
#if QT_VERSION < 0x050000
    _imp->pluginsView->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->pluginsView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->pluginsView->setSortingEnabled(true);
    const PluginsMap& plugins = appPTR->getPluginsList();


    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        if ( it->first.empty() ) {
            continue;
        }
        assert(it->second.size() > 0);

        for (PluginMajorsOrdered::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            Plugin* plugin  = *it2;
            assert(plugin);
            if ( plugin->getIsForInternalUseOnly() ) {
                continue;
            }


            PluginTreeNodeList::iterator foundParent = _imp->buildGroupHierarchy(plugin->getGrouping());
            PluginTreeNode node;
            node.plugin = plugin;
            if (foundParent == _imp->pluginsList.end()) {
                node.item = new QTreeWidgetItem(_imp->pluginsView);
            } else {
                node.item = new QTreeWidgetItem(foundParent->item);
            }
            node.item->setText(COL_PLUGIN_LABEL, plugin->getLabelWithoutSuffix());
            node.item->setText(COL_PLUGINID, plugin->getPluginID());
            QString versionString = QString::number(plugin->getMajorVersion()) + QString::fromUtf8(".") + QString::number(plugin->getMinorVersion());
            node.item->setText(COL_VERSION, versionString);

            {
                QWidget *checkboxContainer = new QWidget(0);
                QHBoxLayout* checkboxLayout = new QHBoxLayout(checkboxContainer);
                AnimatedCheckBox* checkbox = new AnimatedCheckBox(checkboxContainer);
                checkboxLayout->addWidget(checkbox, Qt::AlignLeft | Qt::AlignVCenter);
                checkboxLayout->setContentsMargins(0, 0, 0, 0);
                checkboxLayout->setSpacing(0);
                checkbox->setFixedSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
                checkbox->setChecked(plugin->isActivated());
                QObject::connect( checkbox, SIGNAL(clicked(bool)), this, SLOT(onItemEnabledCheckBoxChecked(bool)) );
                _imp->pluginsView->setItemWidget(node.item, COL_ENABLED, checkbox);
                node.enabledCheckbox = checkbox;
            }
            {
                QWidget *checkboxContainer = new QWidget(0);
                QHBoxLayout* checkboxLayout = new QHBoxLayout(checkboxContainer);
                AnimatedCheckBox* checkbox = new AnimatedCheckBox(checkboxContainer);
                checkboxLayout->addWidget(checkbox, Qt::AlignLeft | Qt::AlignVCenter);
                checkboxLayout->setContentsMargins(0, 0, 0, 0);
                checkboxLayout->setSpacing(0);
                checkbox->setFixedSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
                checkbox->setChecked(plugin->isActivated());
                QObject::connect( checkbox, SIGNAL(clicked(bool)), this, SLOT(onRSEnabledCheckBoxChecked(bool)) );
                _imp->pluginsView->setItemWidget(node.item, COL_RS_ENABLED, checkbox);
                node.rsCheckbox = checkbox;
            }

            {
                QWidget *checkboxContainer = new QWidget(0);
                QHBoxLayout* checkboxLayout = new QHBoxLayout(checkboxContainer);
                AnimatedCheckBox* checkbox = new AnimatedCheckBox(checkboxContainer);
                checkboxLayout->addWidget(checkbox, Qt::AlignLeft | Qt::AlignVCenter);
                checkboxLayout->setContentsMargins(0, 0, 0, 0);
                checkboxLayout->setSpacing(0);
                checkbox->setFixedSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
                checkbox->setChecked(plugin->isActivated());
                QObject::connect( checkbox, SIGNAL(clicked(bool)), this, SLOT(onMTEnabledCheckBoxChecked(bool)) );
                _imp->pluginsView->setItemWidget(node.item, COL_MT_ENABLED, checkbox);
                node.mtCheckbox = checkbox;
            }
            _imp->pluginsList.push_back(node);
        }
    }

    
    resize( TO_DPIX(900), TO_DPIY(600) );
}

void
PreferencesPanel::filterPlugins(const QString & txt)
{


    if ( txt.isEmpty() ) {
        for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it != _imp->pluginsList.end(); ++it) {
            it->item->setHidden(false);
            it->item->setExpanded(false);
        }
    } else {
        QString pattern;
        for (int i = 0; i < txt.size(); ++i) {
            pattern.push_back( QLatin1Char('*') );
            pattern.push_back(txt[i]);
        }
        pattern.push_back( QLatin1Char('*') );
        QRegExp expr(pattern, Qt::CaseInsensitive, QRegExp::WildcardUnix);

        std::list<QTreeWidgetItem*> itemsToDisplay;
        for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it != _imp->pluginsList.end(); ++it) {
            if (it->plugin && it->plugin->getLabelWithoutSuffix().contains(expr)) {
                itemsToDisplay.push_back(it->item);
            } else {
                it->item->setExpanded(false);
                it->item->setHidden(true);
            }
        }
        // Expand recursively
        for (std::list<QTreeWidgetItem*>::iterator it = itemsToDisplay.begin(); it!=itemsToDisplay.end(); ++it) {
            (*it)->setHidden(false);
            QTreeWidgetItem* parent = (*it)->parent();
            while (parent) {
                parent->setExpanded(true);
                parent->setHidden(false);
                parent = parent->parent();
            }
        }
    }

}

void
PreferencesPanel::onItemEnabledCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* cb = qobject_cast<AnimatedCheckBox*>(sender());
    if (!cb) {
        return;
    }
    for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it!=_imp->pluginsList.end(); ++it) {
        if (it->enabledCheckbox == cb) {
            it->plugin->setActivated(checked);
            _imp->pluginSettingsChanged = true;
            break;
        }
    }
}

void
PreferencesPanel::onRSEnabledCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* cb = qobject_cast<AnimatedCheckBox*>(sender());
    if (!cb) {
        return;
    }
    for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it!=_imp->pluginsList.end(); ++it) {
        if (it->rsCheckbox == cb) {
            it->plugin->setRenderScaleEnabled(checked);
            _imp->pluginSettingsChanged = true;
            break;
        }
    }
}

void
PreferencesPanel::onMTEnabledCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* cb = qobject_cast<AnimatedCheckBox*>(sender());
    if (!cb) {
        return;
    }
    for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it!=_imp->pluginsList.end(); ++it) {
        if (it->mtCheckbox == cb) {
            it->plugin->setMultiThreadingEnabled(checked);
            _imp->pluginSettingsChanged = true;
            break;
        }
    }
}

void
PreferencesPanelPrivate::setVisiblePage(int index)
{
    if (currentTabIndex != -1) {
        QFrame* frame = tabs[currentTabIndex].tab;
        //frame->setParent(0);
        //splitter->removeWidget(frame);
        frame->hide();
    }
    if (index  != -1) {
        tabs[index].tab->show();
        splitter->addWidget(tabs[index].tab);
        currentTabIndex = index;
    }
    tree->resizeColumnToContents(0);
}

void
PreferencesPanel::onPageActivated(const KnobPageGuiPtr& page)
{
    if (!page) {
        return;
    }
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].page.lock() == page) {
            _imp->setVisiblePage(i);
        }
    }
}

void
PreferencesPanel::onItemSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    assert(selection.size() <= 1);
    if (selection.empty()) {
        setCurrentPage(KnobPageGuiPtr());
        _imp->setVisiblePage(-1);
        return;
    }

    QTreeWidgetItem* selectedItem = selection.front();

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].treeItem == selectedItem) {
            setCurrentPage(_imp->tabs[i].page.lock());
            _imp->setVisiblePage(i);
            return;
        }
    }

}

void
PreferencesPanel::refreshCurrentPage()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    assert(selection.size() <= 1);
    if (selection.empty()) {
        setCurrentPage(KnobPageGuiPtr());
        return;
    }

    QTreeWidgetItem* selectedItem = selection.front();

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].treeItem == selectedItem) {
            setCurrentPage(_imp->tabs[i].page.lock());
            _imp->setVisiblePage(i);
            return;
        }
    }
}

QWidget*
PreferencesPanel::getPagesContainer() const
{
    return const_cast<PreferencesPanel*>(this);
}

QWidget*
PreferencesPanel::createPageMainWidget(QWidget* parent) const
{
    QFrame* ret = new QFrame(parent);
    ret->setFrameShadow(QFrame::Sunken);
    ret->setFrameShape(QFrame::Box);
    return ret;
}

void
PreferencesPanel::addPageToPagesContainer(const KnobPageGuiPtr& page)
{
    PreferenceTab tab;
    _imp->createPreferenceTab(page, &tab);
    _imp->tabs.push_back(tab);
}

void
PreferencesPanel::removePageFromContainer(const KnobPageGuiPtr& page)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].tab == page->tab) {
            _imp->tabs.erase(_imp->tabs.begin() + i);
            return;
        }
    }
}

void
PreferencesPanel::refreshUndoRedoButtonsEnabledNess(bool /*canUndo*/, bool /*canRedo*/)
{

}

void
PreferencesPanelPrivate::createPreferenceTab(const KnobPageGuiPtr& page, PreferenceTab* tab)
{
    tab->tab = dynamic_cast<QFrame*>(page->tab);
    tab->tab->hide();
    assert(tab->tab);

    QTreeWidgetItem* parentItem = 0;

    boost::shared_ptr<KnobPage> pageKnob = page->pageKnob.lock();
    {

        // In the preferences, there may be sub-pages
        KnobPtr hasParent = pageKnob->getParentKnob();
        boost::shared_ptr<KnobPage> parentPage;
        if (hasParent) {
            parentPage = boost::dynamic_pointer_cast<KnobPage>(hasParent);
            if (parentPage) {

                // look in the tabs if it is created
                for (std::size_t i = 0; i < tabs.size(); ++i) {
                    if (tabs[i].page.lock()->pageKnob.lock() == parentPage) {
                        parentItem = tabs[i].treeItem;
                        break;
                    }
                }
            }
        }
    }
    if (parentItem) {
        tab->treeItem = new QTreeWidgetItem(parentItem);
    } else {
        tab->treeItem = new QTreeWidgetItem(tree);
    }

    tab->treeItem->setExpanded(true);
    
    QString label = QString::fromUtf8(pageKnob->getLabel().c_str());
    tab->treeItem->setText(0, label);
    tab->page = page;
}

void
PreferencesPanel::setPagesOrder(const std::list<KnobPageGuiPtr>& order, const KnobPageGuiPtr& curPage, bool restorePageIndex)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        delete _imp->tabs[i].treeItem;
    }
    _imp->tabs.clear();

    int i = 0;
    for (std::list<KnobPageGuiPtr>::const_iterator it = order.begin(); it!=order.end(); ++it, ++i) {
        PreferenceTab tab;
        _imp->createPreferenceTab(*it, &tab);
        _imp->tabs.push_back(tab);

        if (*it != curPage || !restorePageIndex) {
            tab.treeItem->setSelected(false);
        } else {
            tab.treeItem->setSelected(true);
            setCurrentPage(_imp->tabs[i].page.lock());
            _imp->setVisiblePage(i);
        }
    }
}

void
PreferencesPanel::onPageLabelChanged(const KnobPageGuiPtr& page)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].tab == page->tab) {
            QString label = QString::fromUtf8(page->pageKnob.lock()->getLabel().c_str());
            _imp->tabs[i].treeItem->setText(0, label);
            return;
        }
    }
}

void
PreferencesPanel::onSettingChanged(KnobI* knob)
{
    for (U32 i = 0; i < _imp->changedKnobs.size(); ++i) {
        if (_imp->changedKnobs[i] == knob) {
            return;
        }
    }
    _imp->changedKnobs.push_back(knob);
}

void
PreferencesPanel::openHelp()
{
    int docSource = appPTR->getCurrentSettings()->getDocumentationSource();
    int serverPort = appPTR->getCurrentSettings()->getServerPort();
    QString localUrl = QString::fromUtf8("http://localhost:") + QString::number(serverPort) + QString::fromUtf8("/_prefs.html");
    QString remoteUrl = QString::fromUtf8(NATRON_DOCUMENTATION_ONLINE);

    switch (docSource) {
    case 0:
        QDesktopServices::openUrl( QUrl(localUrl) );
        break;
    case 1:
        QDesktopServices::openUrl( QUrl(remoteUrl) );
        break;
    case 2:
        Dialogs::informationDialog(tr("Missing documentation").toStdString(), tr("Missing documentation, please go to settings and select local or online documentation source.").toStdString(), true);
        break;
    }
}

void
PreferencesPanel::restoreDefaults()
{
    StandardButtonEnum reply = Dialogs::questionDialog( tr("Preferences").toStdString(),
                                                        tr("Restoring the settings will delete any custom configuration, are you sure you want to do this?").toStdString(), false );

    if (reply == eStandardButtonYes) {
        appPTR->getCurrentSettings()->restoreDefault();

        for (PluginTreeNodeList::const_iterator it = _imp->pluginsList.begin(); it != _imp->pluginsList.end(); ++it) {
            if (it->enabledCheckbox) {
                it->enabledCheckbox->setChecked(true);
            }
            if (it->rsCheckbox) {
                it->rsCheckbox->setChecked(true);
            }
            if (it->mtCheckbox) {
                it->mtCheckbox->setChecked(true);
            }
        }
    }
}

void
PreferencesPanel::cancelChanges()
{
    _imp->closeIsOK = false;
    close();
}

void
PreferencesPanel::saveChangesAndClose()
{
    ///Steal focus from other widgets so that we are sure all LineEdits and Spinboxes get the focusOut event and their editingFinished
    ///signal is emitted.
    _imp->okB->setFocus();
    appPTR->getCurrentSettings()->saveSettings(_imp->changedKnobs, true, _imp->pluginSettingsChanged);
    _imp->closeIsOK = true;
    close();
}

void
PreferencesPanel::showEvent(QShowEvent* /*e*/)
{
    QDesktopWidget* desktop = QApplication::desktop();
    const QRect rect = desktop->screenGeometry();

    move( QPoint(rect.width() / 2 - width() / 2, rect.height() / 2 - height() / 2) );

    _imp->changedKnobs.clear();
    _imp->pluginSettingsChanged = false;
}

void
PreferencesPanel::closeEvent(QCloseEvent*)
{
    if ( !_imp->closeIsOK && (!_imp->changedKnobs.empty() || _imp->pluginSettingsChanged)) {
        boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
        if (!_imp->changedKnobs.empty()) {
            settings->beginChanges();
            settings->restoreKnobsFromSettings(_imp->changedKnobs);
            settings->endChanges();
        }
        if (_imp->pluginSettingsChanged) {
            settings->restorePluginSettings();
        }
    }
}

void
PreferencesPanel::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        _imp->closeIsOK = false;
        close();
    } else {
        QWidget::keyPressEvent(e);
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_PreferencesPanel.cpp"
