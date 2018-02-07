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

#include "PreferencesPanel.h"

#include <map>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/make_shared.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QStyle>
#include <QPainter>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EffectDescription.h"
#include "Engine/KnobTypes.h"
#include "Engine/KeybindShortcut.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/ActionShortcuts.h"
#include "Gui/GuiDefines.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/Gui.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/LineEdit.h"
#include "Gui/Label.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Splitter.h"
#include "Gui/TableModelView.h"

#define COL_PLUGIN_LABEL 0
#define COL_PLUGINID COL_PLUGIN_LABEL + 1
#define COL_VERSION COL_PLUGINID + 1
#define COL_ENABLED COL_VERSION + 1
#define COL_RS_ENABLED COL_ENABLED + 1
#define COL_MT_ENABLED COL_RS_ENABLED + 1
#define COL_GL_ENABLED COL_MT_ENABLED + 1

NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

struct PreferenceTab
{
    PreferenceTab()
        : treeItem(NULL)
        , tab(NULL)
        , page()
    {
    }

    QTreeWidgetItem* treeItem;
    QFrame* tab;
    KnobPageGuiWPtr page;
};

struct PluginTreeNode
{
    PluginTreeNode()
        : item(NULL)
        , enabledCheckbox(NULL)
        , rsCheckbox(NULL)
        , mtCheckbox(NULL)
        , glCheckbox(NULL)
        , plugin()
    {
    }

    QTreeWidgetItem* item;
    AnimatedCheckBox* enabledCheckbox;
    AnimatedCheckBox* rsCheckbox;
    AnimatedCheckBox* mtCheckbox;
    AnimatedCheckBox* glCheckbox;
    PluginWPtr plugin;
};

typedef std::list<PluginTreeNode> PluginTreeNodeList;


struct GuiBoundAction
{
    GuiBoundAction()
        : item(NULL)
        , action()
    {
    }

    QTreeWidgetItem* item;
    KeybindShortcut action;
};

struct GuiShortCutGroup
{
    GuiShortCutGroup()
        : actions()
        , item(NULL)
    {
    }
    QString grouping;
    std::list<GuiBoundAction> actions;
    QTreeWidgetItem* item;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

static QString
keybindToString(const KeyboardModifiers & modifiers,
                Key key)
{
    Qt::Key qKey = QtEnumConvert::toQtKey(key);
    Qt::KeyboardModifiers qMods = QtEnumConvert::toQtModifiers(modifiers);
    return makeKeySequence(qMods, qKey).toString(QKeySequence::NativeText);
}


typedef std::list<GuiShortCutGroup> GuiAppShorcuts;

///A small hack to the QTreeWidget class to make 2 fuctions public so we can use them in the ShortcutDelegate class
class HackedTreeWidget
    : public QTreeWidget
{
public:

    HackedTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
    {
    }

    QModelIndex indexFromItem_natron(QTreeWidgetItem * item,
                                     int column = 0) const
    {
        return indexFromItem(item, column);
    }

    QTreeWidgetItem* itemFromIndex_natron(const QModelIndex & index) const
    {
        return itemFromIndex(index);
    }
};


static void
makeItemShortCutText(const KeybindShortcut& action,
                     bool useDefault,
                     QString* shortcutStr)
{

    if (useDefault) {
        *shortcutStr = keybindToString(action.defaultModifiers, action.defaultShortcut);
    } else {
        *shortcutStr = keybindToString(action.modifiers, action.currentShortcut);
    }

} // makeItemShortCutText

static void
setItemShortCutText(QTreeWidgetItem* item,
                    const KeybindShortcut& action,
                    bool useDefault)
{
    QString sc;
    makeItemShortCutText(action, useDefault, &sc);
    item->setText( 1,  sc);
}

class ShortcutDelegate
    : public QStyledItemDelegate
{
    HackedTreeWidget* tree;

public:

    ShortcutDelegate(HackedTreeWidget* parent)
        : QStyledItemDelegate(parent)
        , tree(parent)
    {
    }

private:

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

struct PreferencesPanelPrivate
{
    Q_DECLARE_TR_FUNCTIONS(PreferencesPanel)

public:

    PreferencesPanel* _p;
    Gui* gui;
    QVBoxLayout* mainLayout;
    Splitter* splitter;
    QTreeWidget* tree;
    std::vector<PreferenceTab> tabs;
    int currentTabIndex;
    QWidget* warningContainer;
    Label* warningLabelIcon;
    Label* warningLabelDesc;
    DialogButtonBox* buttonBox;
    Button* restorePageDefaultsButton;
    Button* restoreAllDefaultsButton;
    Button* prefsHelp;
    Button* closeButton;
    std::vector<KnobIPtr> changedKnobs;
    bool pluginSettingsChanged;
    Label* pluginFilterLabel;
    LineEdit* pluginFilterEdit;
    QTreeWidget* pluginsView;
    PluginTreeNodeList pluginsList;
    QFrame* shortcutsFrame;
    QVBoxLayout* shortcutsLayout;
    HackedTreeWidget* shortcutsTree;
    QWidget* shortcutGroup;
    QHBoxLayout* shortcutGroupLayout;
    Label* shortcutLabel;
    KeybindRecorder* shortcutEditor;
    Button* validateShortcutButton;
    Button* clearShortcutButton;
    Button* resetShortcutButton;
    QWidget* shortcutButtonsContainer;
    QHBoxLayout* shortcutButtonsLayout;
    Button* restoreShortcutsDefaultsButton;
    GuiAppShorcuts appShortcuts;


    PreferencesPanelPrivate(PreferencesPanel* p,
                            Gui *parent)
        : _p(p)
        , gui(parent)
        , mainLayout(0)
        , splitter(0)
        , tree(0)
        , tabs()
        , currentTabIndex(-1)
        , warningContainer(0)
        , warningLabelIcon(0)
        , warningLabelDesc(0)
        , buttonBox(0)
        , restorePageDefaultsButton(0)
        , restoreAllDefaultsButton(0)
        , prefsHelp(0)
        , closeButton(0)
        , changedKnobs(0)
        , pluginSettingsChanged(false)
        , pluginFilterLabel(0)
        , pluginFilterEdit(0)
        , pluginsView(0)
        , pluginsList()
        , shortcutsFrame(0)
        , shortcutsLayout(0)
        , shortcutsTree(0)
        , shortcutGroup(0)
        , shortcutGroupLayout(0)
        , shortcutLabel(0)
        , shortcutEditor(0)
        , validateShortcutButton(0)
        , clearShortcutButton(0)
        , resetShortcutButton(0)
        , shortcutButtonsContainer(0)
        , shortcutButtonsLayout(0)
        , restoreShortcutsDefaultsButton(0)
        , appShortcuts()
    {
    }

    void createPreferenceTab(const KnobPageGuiPtr& page, PreferenceTab* tab);

    void setVisiblePage(int index);

    PluginTreeNodeList::iterator buildPluginGroupHierarchy(const QStringList& grouping);
    KeybindShortcut getActionForTreeItem(QTreeWidgetItem* item) const
    {
        for (GuiAppShorcuts::const_iterator it = appShortcuts.begin(); it != appShortcuts.end(); ++it) {
            if (it->item == item) {
                return KeybindShortcut();
            }
            for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
                if (it2->item == item) {
                    return it2->action;
                }
            }
        }

        return KeybindShortcut();
    }

    QTreeWidgetItem* getItemForAction(const KeybindShortcut& action) const
    {
        for (GuiAppShorcuts::const_iterator it = appShortcuts.begin(); it != appShortcuts.end(); ++it) {
            if (it->grouping.toStdString() == action.grouping) {
                for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
                    if (it2->action.actionID == action.actionID) {
                        return it2->item;
                    }
                }
                break;
            }
        }

        return NULL;
    }

    GuiAppShorcuts::iterator buildShortcutsGroupHierarchy(QString grouping);

    void makeGuiActionForShortcut(GuiAppShorcuts::iterator guiGroupIterator, const KeybindShortcut& action);
};

PreferencesPanel::PreferencesPanel(Gui *parent)
    : QWidget(parent)
    , KnobGuiContainerHelper( appPTR->getCurrentSettings(), boost::shared_ptr<QUndoStack>() )
    , _imp( new PreferencesPanelPrivate(this, parent) )
{
}

PreferencesPanel::~PreferencesPanel()
{
}

PluginTreeNodeList::iterator
PreferencesPanelPrivate::buildPluginGroupHierarchy(const QStringList& groupingSplit)
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
    group.item = groupParent;
    foundGuiGroup = pluginsList.insert(pluginsList.end(), group);

    return foundGuiGroup;
} // PreferencesPanelPrivate::buildPluginGroupHierarchy

void
PreferencesPanel::createPluginsView(QGridLayout* pluginsFrameLayout)
{
    _imp->pluginsView = new QTreeWidget(this);
    _imp->pluginsView->setAttribute(Qt::WA_MacShowFocusRect, 0);

    Label* restartWarningLabel = new Label(tr("<i>Changing any plug-in property requires a restart of %1</i>").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ), this);
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
    treeHeader->setToolTip(COL_PLUGIN_LABEL, tr("The label of the plug-in"));
    treeHeader->setText( COL_PLUGIN_LABEL, tr("Label") );
    treeHeader->setToolTip(COL_PLUGINID, tr("The ID of the plug-in"));
    treeHeader->setText( COL_PLUGINID, tr("ID") );
    treeHeader->setToolTip(COL_VERSION, tr("The version of the plug-in"));
    treeHeader->setText( COL_VERSION, tr("Version") );
    treeHeader->setToolTip(COL_ENABLED, tr("If unchecked, the user will not be able to use a node with this plug-in"));
    treeHeader->setText( COL_ENABLED, tr("Enabled") );
    treeHeader->setToolTip(COL_RS_ENABLED, tr("If unchecked, renders will always be made at full image resolution for this plug-in"));
    treeHeader->setText( COL_RS_ENABLED, tr("R-S") );
    treeHeader->setToolTip(COL_MT_ENABLED, tr("If unchecked, there can only be a single render issued at a time for a node of this plug-in. This can alter performances a lot."));
    treeHeader->setText( COL_MT_ENABLED, tr("M-T") );
    treeHeader->setToolTip(COL_GL_ENABLED, tr("If unchecked, OpenGL rendering is disabled for any node with this plug-in. If the checkbox is disabled, the plug-in does not support OpenGL rendering"));
    treeHeader->setText( COL_GL_ENABLED, tr("OpenGL") );
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

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            PluginPtr plugin = *itver;
            assert(plugin);
            if (plugin->getPropertyUnsafe<bool>(kNatronPluginPropIsInternalOnly)) {
                continue;
            }

            PluginTreeNodeList::iterator foundParent = _imp->buildPluginGroupHierarchy( plugin->getGroupingAsQStringList() );
            PluginTreeNode node;
            node.plugin = plugin;
            if ( foundParent == _imp->pluginsList.end() ) {
                node.item = new QTreeWidgetItem(_imp->pluginsView);
            } else {
                node.item = new QTreeWidgetItem(foundParent->item);
            }
            node.item->setText( COL_PLUGIN_LABEL, QString::fromUtf8(plugin->getLabelWithoutSuffix().c_str()) );
            node.item->setText( COL_PLUGINID, QString::fromUtf8(plugin->getPluginID().c_str()) );
            QString versionString = QString::number( plugin->getMajorVersion() ) + QString::fromUtf8(".") + QString::number( plugin->getMinorVersion() );
            node.item->setText(COL_VERSION, versionString);

            {
                QWidget *checkboxContainer = new QWidget(0);
                QHBoxLayout* checkboxLayout = new QHBoxLayout(checkboxContainer);
                AnimatedCheckBox* checkbox = new AnimatedCheckBox(checkboxContainer);
                checkboxLayout->addWidget(checkbox, Qt::AlignLeft | Qt::AlignVCenter);
                checkboxLayout->setContentsMargins(0, 0, 0, 0);
                checkboxLayout->setSpacing(0);
                checkbox->setFixedSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
                checkbox->setChecked( plugin->isEnabled() );
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
                checkbox->setChecked( plugin->isRenderScaleEnabled() );
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
                checkbox->setChecked( plugin->isMultiThreadingEnabled() );
                QObject::connect( checkbox, SIGNAL(clicked(bool)), this, SLOT(onMTEnabledCheckBoxChecked(bool)) );
                _imp->pluginsView->setItemWidget(node.item, COL_MT_ENABLED, checkbox);
                node.mtCheckbox = checkbox;
            }
            {
                QWidget *checkboxContainer = new QWidget(0);
                QHBoxLayout* checkboxLayout = new QHBoxLayout(checkboxContainer);
                AnimatedCheckBox* checkbox = new AnimatedCheckBox(checkboxContainer);
                checkboxLayout->addWidget(checkbox, Qt::AlignLeft | Qt::AlignVCenter);
                checkboxLayout->setContentsMargins(0, 0, 0, 0);
                checkboxLayout->setSpacing(0);
                checkbox->setFixedSize( TO_DPIX(NATRON_SMALL_BUTTON_SIZE), TO_DPIY(NATRON_SMALL_BUTTON_SIZE) );
                checkbox->setChecked(plugin->isOpenGLEnabled());
                QObject::connect( checkbox, SIGNAL(clicked(bool)), this, SLOT(onGLEnabledCheckBoxChecked(bool)) );
                _imp->pluginsView->setItemWidget(node.item, COL_GL_ENABLED, checkbox);
                if (plugin->getEffectDescriptor()->getPropertyUnsafe<PluginOpenGLRenderSupport>(kEffectPropSupportsOpenGLRendering) == ePluginOpenGLRenderSupportNone) {
                    checkbox->setChecked(false);
                    checkbox->setReadOnly(true);
                }
                node.glCheckbox = checkbox;
            }

            _imp->pluginsList.push_back(node);
        }
    }
} // PreferencesPanel::createPluginsView

void
PreferencesPanel::createShortcutEditor(QTreeWidgetItem* uiPageTreeItem)
{
    _imp->shortcutsFrame = dynamic_cast<QFrame*>( createPageMainWidget(this) );
    _imp->shortcutsLayout = new QVBoxLayout(_imp->shortcutsFrame);
    _imp->shortcutsTree = new HackedTreeWidget(_imp->shortcutsFrame);
    _imp->shortcutsTree->setColumnCount(3);
    QStringList headers;
    headers << tr("Command") << tr("Shortcut");
    _imp->shortcutsTree->setHeaderLabels(headers);
    _imp->shortcutsTree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->shortcutsTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _imp->shortcutsTree->setSortingEnabled(false);
    _imp->shortcutsTree->setToolTip( NATRON_NAMESPACE::convertFromPlainText(
                                         tr("In this table is represented each action of the application that can have a possible keybind/mouse shortcut."
                                            " Note that this table also have some special assignments which also involve the mouse. "
                                            "You cannot assign a keybind to a shortcut involving the mouse and vice versa. "
                                            "Note that internally %1 does an emulation of a three-button mouse "
                                            "if your computer doesn't have one, that is: \n"
                                            "---> Middle mouse button is emulated by holding down Options (alt) coupled with a left click.\n "
                                            "---> Right mouse button is emulated by holding down Command (cmd) coupled with a left click.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->shortcutsTree->setItemDelegate( new ShortcutDelegate(_imp->shortcutsTree) );

    _imp->shortcutsTree->resizeColumnToContents(0);
    QObject::connect( _imp->shortcutsTree, SIGNAL(itemSelectionChanged()), this, SLOT(onShortcutsSelectionChanged()) );

    _imp->shortcutsLayout->addWidget(_imp->shortcutsTree);

    _imp->shortcutGroup = new QWidget(_imp->shortcutsFrame);
    _imp->shortcutsLayout->addWidget(_imp->shortcutGroup);

    _imp->shortcutGroupLayout = new QHBoxLayout(_imp->shortcutGroup);
    _imp->shortcutGroupLayout->setContentsMargins(0, 0, 0, 0);

    _imp->shortcutLabel = new Label(_imp->shortcutGroup);
    _imp->shortcutLabel->setText( tr("Sequence:") );
    _imp->shortcutGroupLayout->addWidget(_imp->shortcutLabel);

    _imp->shortcutEditor = new KeybindRecorder(_imp->shortcutGroup);
    _imp->shortcutEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );
    _imp->shortcutGroupLayout->addWidget(_imp->shortcutEditor);


    _imp->validateShortcutButton = new Button(tr("Validate"), _imp->shortcutGroup);
    _imp->validateShortcutButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Validates the shortcut on the field editor and set the selected shortcut."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->shortcutGroupLayout->addWidget(_imp->validateShortcutButton);
    QObject::connect( _imp->validateShortcutButton, SIGNAL(clicked(bool)), this, SLOT(onValidateShortcutButtonClicked()) );

    _imp->clearShortcutButton = new Button(tr("Clear"), _imp->shortcutGroup);
    QObject::connect( _imp->clearShortcutButton, SIGNAL(clicked(bool)), this, SLOT(onClearShortcutButtonClicked()) );
    _imp->shortcutGroupLayout->addWidget(_imp->clearShortcutButton);

    _imp->resetShortcutButton = new Button(tr("Reset"), _imp->shortcutGroup);
    QObject::connect( _imp->resetShortcutButton, SIGNAL(clicked(bool)), this, SLOT(onResetShortcutButtonClicked()) );
    _imp->shortcutGroupLayout->addWidget(_imp->resetShortcutButton);

    _imp->shortcutButtonsContainer = new QWidget(this);
    _imp->shortcutButtonsLayout = new QHBoxLayout(_imp->shortcutButtonsContainer);

    _imp->shortcutsLayout->addWidget(_imp->shortcutButtonsContainer);

    _imp->restoreShortcutsDefaultsButton = new Button(tr("Restore Default Shortcuts"), _imp->restoreShortcutsDefaultsButton);
    QObject::connect( _imp->restoreShortcutsDefaultsButton, SIGNAL(clicked(bool)), this, SLOT(onRestoreDefaultShortcutsButtonClicked()) );
    _imp->shortcutButtonsLayout->addWidget(_imp->restoreShortcutsDefaultsButton);
    _imp->shortcutButtonsLayout->addStretch();

    KnobPageGuiPtr page = boost::make_shared<KnobPageGui>();
    page->gridLayout = 0;
    page->tab = _imp->shortcutsFrame;
    PreferenceTab tab;
    tab.tab = _imp->shortcutsFrame;
    tab.tab->hide();
    tab.treeItem = new QTreeWidgetItem(uiPageTreeItem);
    tab.treeItem->setText( 0, tr("Shortcut Editor") );
    tab.page = page;
    _imp->tabs.push_back(tab);
    
    refreshShortcutsFromSettings();

    connect(appPTR->getCurrentSettings().get(), SIGNAL(shortcutsChanged()), this, SLOT(refreshShortcutsFromSettings()));
} // PreferencesPanel::createShortcutEditor

Gui*
PreferencesPanel::getGui() const
{
    return _imp->gui;
}

NodeGuiPtr
PreferencesPanel::getNodeGui() const
{
    return NodeGuiPtr();
}

void
PreferencesPanel::createGui()
{
    //setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setWindowFlags(Qt::Window);
    setWindowTitle( tr("Preferences") );
    _imp->mainLayout = new QVBoxLayout(this);

    _imp->splitter = new Splitter(Qt::Horizontal, _imp->gui, this);
    _imp->splitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    _imp->tree = new QTreeWidget(_imp->splitter);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setColumnCount(1);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _imp->tree->header()->close();
    QObject::connect( _imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()) );

    initializeKnobs();

    _imp->splitter->addWidget(_imp->tree);


    int maxLength = 0;
    QFontMetrics fm = fontMetrics();
    QGridLayout* pluginsFrameLayout = 0;
    QTreeWidgetItem* uiTabTreeItem = 0;
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        KnobPagePtr pageKnob = _imp->tabs[i].page.lock()->pageKnob.lock();
        if (pageKnob->getName() == "pluginsPage") {
            pluginsFrameLayout = _imp->tabs[i].page.lock()->gridLayout;
        } else if (pageKnob->getName() == "uiPage") {
            uiTabTreeItem = _imp->tabs[i].treeItem;
        }
        QString label = QString::fromUtf8( pageKnob->getLabel().c_str() );
        int w = fm.width(label);
        maxLength = std::max(w, maxLength);
    }
    _imp->tree->setFixedWidth(maxLength + 100);



    _imp->warningContainer = new QWidget(this);
    QHBoxLayout* warningsContainerLayout = new QHBoxLayout(_imp->warningContainer);
    warningsContainerLayout->setSpacing(TO_DPIX(5));
    warningsContainerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->warningLabelIcon = new Label(_imp->warningContainer);

    {
        int pixSize = TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE);
        QIcon ic = style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, _imp->warningLabelIcon);
        QPixmap pix = ic.pixmap(QSize(pixSize, pixSize));
        _imp->warningLabelIcon->setPixmap(pix);
    }
    warningsContainerLayout->addWidget(_imp->warningLabelIcon);


    _imp->warningLabelDesc = new Label(_imp->warningContainer);
    _imp->warningLabelDesc->setIsBold(true);
    _imp->warningLabelDesc->setIsModified(true);
    _imp->warningLabelDesc->setText(tr("One or multiple setting(s) modification requires a restart of %1 to take effect.").arg(QString::fromUtf8(NATRON_APPLICATION_NAME)));
    warningsContainerLayout->addWidget(_imp->warningLabelDesc);
    _imp->warningContainer->hide();

    _imp->buttonBox = new DialogButtonBox(Qt::Horizontal, this);

    _imp->restorePageDefaultsButton = new Button( tr("Restore Defaults"), _imp->buttonBox );
    _imp->restorePageDefaultsButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Restore default values for the selected tab"), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->restoreAllDefaultsButton = new Button( tr("Restore All Defaults"), _imp->buttonBox );
    _imp->restoreAllDefaultsButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Restore default values for all preferences"), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->prefsHelp = new Button( tr("Help"), _imp->buttonBox );
    _imp->prefsHelp->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Display help for preferences in an external browser"), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->closeButton = new Button( tr("Close"), _imp->buttonBox );
    _imp->closeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Closes the window."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonBox->addButton(_imp->restorePageDefaultsButton, QDialogButtonBox::ResetRole);
    _imp->buttonBox->addButton(_imp->restoreAllDefaultsButton, QDialogButtonBox::ResetRole);
    _imp->buttonBox->addButton(_imp->prefsHelp, QDialogButtonBox::HelpRole);
    _imp->buttonBox->addButton(_imp->closeButton, QDialogButtonBox::ApplyRole);

    _imp->mainLayout->addWidget(_imp->splitter);
    _imp->mainLayout->addWidget(_imp->warningContainer);
    _imp->mainLayout->addWidget(_imp->buttonBox);

    QObject::connect( _imp->restoreAllDefaultsButton, SIGNAL(clicked()), this, SLOT(onRestoreAllDefaultsClicked()) );
    QObject::connect( _imp->restorePageDefaultsButton, SIGNAL(clicked()), this, SLOT(onRestoreCurrentTabDefaultsClicked()) );
    QObject::connect( _imp->prefsHelp, SIGNAL(clicked()), this, SLOT(openHelp()) );
    QObject::connect( _imp->closeButton, SIGNAL(clicked()), this, SLOT(closeDialog()) );
    QObject::connect( appPTR->getCurrentSettings().get(), SIGNAL(settingChanged(KnobIPtr,ValueChangedReasonEnum)), this, SLOT(onSettingChanged(KnobIPtr,ValueChangedReasonEnum)) );


    // Create plug-ins view
    if (pluginsFrameLayout) {
        createPluginsView(pluginsFrameLayout);
    }


    // Create the shortcut Editor
    if (uiTabTreeItem) {
        createShortcutEditor(uiTabTreeItem);
    }


    if (!_imp->tabs.empty()) {
        _imp->tabs[0].treeItem->setSelected(true);
    }

    resize( TO_DPIX(900), TO_DPIY(600) );
} // PreferencesPanel::createGui

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
            if ( it->plugin.lock() && QString::fromUtf8(it->plugin.lock()->getLabelWithoutSuffix().c_str()).contains(expr) ) {
                itemsToDisplay.push_back(it->item);
            } else {
                it->item->setExpanded(false);
                it->item->setHidden(true);
            }
        }
        // Expand recursively
        for (std::list<QTreeWidgetItem*>::iterator it = itemsToDisplay.begin(); it != itemsToDisplay.end(); ++it) {
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
    AnimatedCheckBox* cb = qobject_cast<AnimatedCheckBox*>( sender() );

    if (!cb) {
        return;
    }
    for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it != _imp->pluginsList.end(); ++it) {
        if (it->enabledCheckbox == cb) {
            it->plugin.lock()->setEnabled(checked);
            _imp->pluginSettingsChanged = true;
            break;
        }
    }
}

void
PreferencesPanel::onRSEnabledCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* cb = qobject_cast<AnimatedCheckBox*>( sender() );

    if (!cb) {
        return;
    }
    for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it != _imp->pluginsList.end(); ++it) {
        if (it->rsCheckbox == cb) {
            it->plugin.lock()->setRenderScaleEnabled(checked);
            _imp->pluginSettingsChanged = true;
            break;
        }
    }
}

void
PreferencesPanel::onMTEnabledCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* cb = qobject_cast<AnimatedCheckBox*>( sender() );

    if (!cb) {
        return;
    }
    for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it != _imp->pluginsList.end(); ++it) {
        if (it->mtCheckbox == cb) {
            it->plugin.lock()->setMultiThreadingEnabled(checked);
            _imp->pluginSettingsChanged = true;
            break;
        }
    }
}

void
PreferencesPanel::onGLEnabledCheckBoxChecked(bool checked)
{
    AnimatedCheckBox* cb = qobject_cast<AnimatedCheckBox*>(sender());
    if (!cb) {
        return;
    }
    for (PluginTreeNodeList::iterator it = _imp->pluginsList.begin(); it!=_imp->pluginsList.end(); ++it) {
        if (it->mtCheckbox == cb) {
            it->plugin.lock()->setOpenGLEnabled(checked);
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
    if ( selection.empty() ) {
        setCurrentPage( KnobPageGuiPtr() );
        _imp->setVisiblePage(-1);

        return;
    }

    QTreeWidgetItem* selectedItem = selection.front();

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].treeItem == selectedItem) {
            setCurrentPage( _imp->tabs[i].page.lock() );
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
    if ( selection.empty() ) {
        setCurrentPage( KnobPageGuiPtr() );

        return;
    }

    QTreeWidgetItem* selectedItem = selection.front();

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].treeItem == selectedItem) {
            setCurrentPage( _imp->tabs[i].page.lock() );
            _imp->setVisiblePage(i);

            return;
        }
    }
}

QWidget*
PreferencesPanel::getMainContainer() const
{
    return const_cast<PreferencesPanel*>(this);
}

QLayout*
PreferencesPanel::getMainContainerLayout() const
{
    return _imp->mainLayout;
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
PreferencesPanel::refreshUndoRedoButtonsEnabledNess(bool /*canUndo*/,
                                                    bool /*canRedo*/)
{
}

void
PreferencesPanelPrivate::createPreferenceTab(const KnobPageGuiPtr& page,
                                             PreferenceTab* tab)
{
    tab->tab = dynamic_cast<QFrame*>(page->tab);
    assert(tab->tab);
    if (tab->tab) {
        tab->tab->hide();
    }

    QTreeWidgetItem* parentItem = 0;
    KnobPagePtr pageKnob = page->pageKnob.lock();
    if (pageKnob) {
        // In the preferences, there may be sub-pages
        KnobIPtr hasParent = pageKnob->getParentKnob();
        KnobPagePtr parentPage;
        if (hasParent) {
            parentPage = toKnobPage(hasParent);
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

    if (pageKnob) {
        QString label = QString::fromUtf8( pageKnob->getLabel().c_str() );
        tab->treeItem->setText(0, label);
    }
    tab->page = page;
}

void
PreferencesPanel::setPagesOrder(const std::list<KnobPageGuiPtr>& order,
                                const KnobPageGuiPtr& curPage,
                                bool restorePageIndex)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        delete _imp->tabs[i].treeItem;
    }
    _imp->tabs.clear();

    int i = 0;
    for (std::list<KnobPageGuiPtr>::const_iterator it = order.begin(); it != order.end(); ++it, ++i) {
        PreferenceTab tab;
        _imp->createPreferenceTab(*it, &tab);
        _imp->tabs.push_back(tab);

        if ( (*it != curPage) || !restorePageIndex ) {
            tab.treeItem->setSelected(false);
        } else {
            tab.treeItem->setSelected(true);
            setCurrentPage( _imp->tabs[i].page.lock() );
            _imp->setVisiblePage(i);
        }
    }
}

void
PreferencesPanel::onPageLabelChanged(const KnobPageGuiPtr& page)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].tab == page->tab) {
            QString label = QString::fromUtf8( page->pageKnob.lock()->getLabel().c_str() );
            _imp->tabs[i].treeItem->setText(0, label);

            return;
        }
    }
}

void
PreferencesPanel::onSettingChanged(const KnobIPtr& knob, ValueChangedReasonEnum reason)
{
    if (reason != eValueChangedReasonUserEdited && reason != eValueChangedReasonUserEdited && reason != eValueChangedReasonRestoreDefault) {
        return;
    }
    if (appPTR->getCurrentSettings()->doesKnobChangeRequireOFXCacheClear(knob)) {
        appPTR->clearPluginsLoadedCache(); // clear the cache for next restart
    }
    if (appPTR->getCurrentSettings()->doesKnobChangeRequireRestart(knob)) {
        _imp->warningContainer->show();
    }
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
    int serverPort = appPTR->getDocumentationServerPort();
    QString localUrl = QString::fromUtf8("http://localhost:") + QString::number(serverPort) + QString::fromUtf8("/_prefs.html");
#ifdef NATRON_DOCUMENTATION_ONLINE
    int docSource = appPTR->getCurrentSettings()->getDocumentationSource();
    QString remoteUrl = QString::fromUtf8(NATRON_DOCUMENTATION_ONLINE);
    if ( (serverPort == 0) && (docSource == 0) ) {
        docSource = 1;
    }

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
#else
    QDesktopServices::openUrl( QUrl(localUrl) );
#endif
}

void
PreferencesPanel::onRestoreCurrentTabDefaultsClicked()
{
    KnobPageGuiPtr currentPage = getCurrentPage();
    if (!currentPage) {
        return;
    }

    appPTR->getCurrentSettings()->restorePageToDefaults(currentPage->pageKnob.lock());

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
        if (it->glCheckbox) {
            it->glCheckbox->setChecked(true);
        }
    }
}

void
PreferencesPanel::onRestoreAllDefaultsClicked()
{
    StandardButtonEnum reply = Dialogs::questionDialog( tr("Preferences").toStdString(),
                                                        tr("Restoring the settings will delete any custom configuration, are you sure you want to do this?").toStdString(), false );
    if (reply != eStandardButtonYes) {
        return;
    }
    for (std::vector<PreferenceTab>::const_iterator it = _imp->tabs.begin(); it != _imp->tabs.end(); ++it) {
        appPTR->getCurrentSettings()->restorePageToDefaults(it->page.lock()->pageKnob.lock());
    }
}

void
PreferencesPanel::closeDialog()
{
    if (!_imp->changedKnobs.empty() || _imp->pluginSettingsChanged) {
        appPTR->getCurrentSettings()->saveSettingsToFile();
    }
    close();
}


void
PreferencesPanel::refreshShortcutsFromSettings()
{
    _imp->appShortcuts.clear();

    KeybindShortcut currentSelection;
    QList<QTreeWidgetItem*> selectedItems = _imp->shortcutsTree->selectedItems();
    if (!selectedItems.empty()) {
        currentSelection = _imp->getActionForTreeItem(selectedItems.front());
    }

    _imp->shortcutsTree->clear();

    const ApplicationShortcutsMap & appShortcuts = appPTR->getCurrentSettings()->getAllShortcuts();
    
    for (ApplicationShortcutsMap::const_iterator it = appShortcuts.begin(); it != appShortcuts.end(); ++it) {
        QString grouping = QString::fromUtf8(it->first.c_str());
        GuiAppShorcuts::iterator foundGuiGroup = _imp->buildShortcutsGroupHierarchy(grouping);
        assert( foundGuiGroup != _imp->appShortcuts.end() );
        
        for (GroupShortcutsMap::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            _imp->makeGuiActionForShortcut(foundGuiGroup, it2->second);
        }
    }

    // Restore selection
    if (!currentSelection.actionID.empty()) {
        QTreeWidgetItem* item = _imp->getItemForAction(currentSelection);
        item->setSelected(true);
    }

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
PreferencesPanel::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        closeDialog();
    } else {
        QWidget::keyPressEvent(e);
    }
}

GuiAppShorcuts::iterator
PreferencesPanelPrivate::buildShortcutsGroupHierarchy(QString grouping)
{
    ///Do not allow empty grouping, make them under the Global shortcut
    if ( grouping.isEmpty() ) {
        grouping = QString::fromUtf8(kShortcutGroupGlobal);
    }

    ///Groups are separated by a '/'
    QStringList groupingSplit = grouping.split( QLatin1Char('/') );
    assert(groupingSplit.size() > 0);
    const QString & lastGroupName = groupingSplit.back();

    ///Find out whether the deepest parent group of the action already exist
    GuiAppShorcuts::iterator foundGuiGroup = appShortcuts.end();
    for (GuiAppShorcuts::iterator it2 = appShortcuts.begin(); it2 != appShortcuts.end(); ++it2) {
        if (it2->item->text(0) == lastGroupName) {
            foundGuiGroup = it2;
            break;
        }
    }

    QTreeWidgetItem* groupItem;
    if ( foundGuiGroup != appShortcuts.end() ) {
        groupItem = foundGuiGroup->item;
    } else {
        groupItem = 0;
        for (int i = 0; i < groupingSplit.size(); ++i) {
            QTreeWidgetItem* groupingItem;
            bool existAlready = false;
            if (groupItem) {
                for (int j = 0; j < groupItem->childCount(); ++j) {
                    QTreeWidgetItem* child = groupItem->child(j);
                    if (child->text(0) == groupingSplit[i]) {
                        groupingItem = child;
                        existAlready = true;
                        break;
                    }
                }
                if (!existAlready) {
                    groupingItem = new QTreeWidgetItem(groupItem);
                    groupItem->addChild(groupingItem);
                }
            } else {
                for (int j = 0; j < shortcutsTree->topLevelItemCount(); ++j) {
                    QTreeWidgetItem* topLvlItem = shortcutsTree->topLevelItem(j);
                    if (topLvlItem->text(0) == groupingSplit[i]) {
                        groupingItem = topLvlItem;
                        existAlready = true;
                        break;
                    }
                }
                if (!existAlready) {
                    groupingItem = new QTreeWidgetItem(shortcutsTree);
                    shortcutsTree->addTopLevelItem(groupingItem);
                }
            }
            if (!existAlready) {
                /* expand only top-level groups */
                groupingItem->setExpanded(i == 0);
                groupingItem->setFlags(Qt::ItemIsEnabled);
                groupingItem->setText(0, groupingSplit[i]);
            }
            groupItem = groupingItem;
        }
    }
    GuiShortCutGroup group;
    group.item = groupItem;
    group.grouping = lastGroupName;
    foundGuiGroup = appShortcuts.insert(appShortcuts.end(), group);

    return foundGuiGroup;
} // PreferencesPanel::buildShortcutGroupHierarchy

void
PreferencesPanelPrivate::makeGuiActionForShortcut(GuiAppShorcuts::iterator guiGroupIterator,
                                                  const KeybindShortcut& action)
{
    GuiBoundAction guiAction;

    guiAction.action = action;
    guiAction.item = new QTreeWidgetItem(guiGroupIterator->item);
    guiAction.item->setText(0, QString::fromUtf8(action.actionLabel.c_str()));
    QString shortcutStr = keybindToString(action.modifiers, action.currentShortcut);
    
    if (!action.editable) {
        guiAction.item->setToolTip( 0, tr("This action is standard and its shortcut cannot be edited.") );
        guiAction.item->setToolTip( 1, tr("This action is standard and its shortcut cannot be edited.") );
        guiAction.item->setDisabled(true);
    }
    guiAction.item->setExpanded(true);
    guiAction.item->setText(1, shortcutStr);
    guiGroupIterator->actions.push_back(guiAction);
    guiGroupIterator->item->addChild(guiAction.item);
}

void
ShortcutDelegate::paint(QPainter * painter,
                        const QStyleOptionViewItem & option,
                        const QModelIndex & index) const
{
    QTreeWidgetItem* item = tree->itemFromIndex_natron(index);

    if (!item) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    ///Determine whether the item is top level or not
    bool isTopLevel = false;
    int topLvlCount = tree->topLevelItemCount();
    for (int i = 0; i < topLvlCount; ++i) {
        if (tree->topLevelItem(i) == item) {
            isTopLevel = true;
            break;
        }
    }

    QFont font = painter->font();
    QPen pen;

    if (isTopLevel) {
        font.setBold(true);
        font.setPixelSize(13);
        pen.setColor(Qt::black);
    } else {
        font.setBold(false);
        font.setPixelSize(11);
        if ( item->isDisabled() ) {
            pen.setColor(Qt::black);
        } else {
            pen.setColor( QColor(200, 200, 200) );
        }
    }
    painter->setFont(font);
    painter->setPen(pen);

    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);

    ///Draw the item name column
    if (option.state & QStyle::State_Selected) {
        painter->fillRect( geom, option.palette.highlight() );
    }
    QRect r;
    painter->drawText(geom, Qt::TextSingleLine, item->data(index.column(), Qt::DisplayRole).toString(), &r);
} // paint

KeybindRecorder::KeybindRecorder(QWidget* parent)
    : LineEdit(parent)
{
}

KeybindRecorder::~KeybindRecorder()
{
}

void
KeybindRecorder::keyPressEvent(QKeyEvent* e)
{
    if ( !isEnabled() || isReadOnly() ) {
        return;
    }

    QKeySequence *seq;
    if (e->key() == Qt::Key_Control) {
        seq = new QKeySequence(Qt::CTRL);
    } else if (e->key() == Qt::Key_Shift) {
        seq = new QKeySequence(Qt::SHIFT);
    } else if (e->key() == Qt::Key_Meta) {
        seq = new QKeySequence(Qt::META);
    } else if (e->key() == Qt::Key_Alt) {
        seq = new QKeySequence(Qt::ALT);
    } else {
        seq = new QKeySequence( e->key() );
    }

    QString seqStr = seq->toString(QKeySequence::NativeText);
    delete seq;
    QString txt = text()  + seqStr;
    setText(txt);
}

void
PreferencesPanel::onShortcutsSelectionChanged()
{
    QList<QTreeWidgetItem*> items = _imp->shortcutsTree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }

    if ( items.empty() ) {
        _imp->shortcutEditor->setText( QString() );
        _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );
        _imp->shortcutEditor->setReadOnly_NoFocusRect(true);

        return;
    }
    QTreeWidgetItem* selection = items.front();
    if ( !selection->isDisabled() ) {
        _imp->shortcutEditor->setReadOnly_NoFocusRect(false);
        _imp->clearShortcutButton->setEnabled(true);
        _imp->resetShortcutButton->setEnabled(true);
    } else {
        _imp->shortcutEditor->setReadOnly_NoFocusRect(true);
        _imp->clearShortcutButton->setEnabled(false);
        _imp->resetShortcutButton->setEnabled(false);
    }

    KeybindShortcut action = _imp->getActionForTreeItem(selection);
    if (!action.actionID.empty()) {
        QString sc;
        makeItemShortCutText(action, false, &sc);
        _imp->shortcutEditor->setText(sc);
    }
}

void
PreferencesPanel::onValidateShortcutButtonClicked()
{
    QString text = _imp->shortcutEditor->text();

    QList<QTreeWidgetItem*> items = _imp->shortcutsTree->selectedItems();
    if ( (items.size() > 1) || items.empty() ) {
        return;
    }

    QTreeWidgetItem* selection = items.front();
    QKeySequence seq(text, QKeySequence::NativeText);
    KeybindShortcut action = _imp->getActionForTreeItem(selection);

    QTreeWidgetItem* parent = selection->parent();
    while (parent) {
        QTreeWidgetItem* parentUp = parent->parent();
        if (!parentUp) {
            break;
        }
        parent = parentUp;
    }
    assert(parent);

    Qt::KeyboardModifiers qmodifiers;
    Qt::Key qsymbol;
    extractKeySequence(seq, qmodifiers, qsymbol);

    KeyboardModifiers mods = QtEnumConvert::fromQtModifiers(qmodifiers);
    Key sym = QtEnumConvert::fromQtKey(qsymbol);

    // Check for conflicts: 2 shorcuts within the same group cannot have the same keybind
    for (GuiAppShorcuts::iterator it = _imp->appShortcuts.begin(); it != _imp->appShortcuts.end(); ++it) {
        for (std::list<GuiBoundAction>::iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
            if ( (it2->action.actionID != action.actionID) && ( it->item->text(0) == parent->text(0) ) ) {

                if ( (it2->action.modifiers == mods) && (it2->action.currentShortcut == sym) ) {
                    QString err = tr("Cannot bind this shortcut because the following action is already using it: %1")
                    .arg( it2->item->text(0) );
                    _imp->shortcutEditor->clear();
                    Dialogs::errorDialog( tr("Shortcuts Editor").toStdString(), tr( err.toStdString().c_str() ).toStdString() );

                    return;
                }

                
            }
        }
    }

    // Refresh keybinds
    appPTR->getCurrentSettings()->setShortcutKeybind(action.grouping, action.actionID, mods, sym);

} // PreferencesPanel::onValidateShortcutButtonClicked

void
PreferencesPanel::onClearShortcutButtonClicked()
{
    QList<QTreeWidgetItem*> items = _imp->shortcutsTree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }

    if ( items.empty() ) {
        _imp->shortcutEditor->setText( QString() );
        _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );

        return;
    }

    QTreeWidgetItem* selection = items.front();
    KeybindShortcut action = _imp->getActionForTreeItem(selection);

    appPTR->getCurrentSettings()->setShortcutKeybind(action.grouping, action.actionID, eKeyboardModifierNone, (Key)0);

    selection->setText( 1, QString() );

    _imp->shortcutEditor->setText( QString() );
    _imp->shortcutEditor->setFocus();


}

void
PreferencesPanel::onResetShortcutButtonClicked()
{
    QList<QTreeWidgetItem*> items = _imp->shortcutsTree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }

    if ( items.empty() ) {
        _imp->shortcutEditor->setText( QString() );
        _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );

        return;
    }

    QTreeWidgetItem* selection = items.front();
    KeybindShortcut action = _imp->getActionForTreeItem(selection);
    appPTR->getCurrentSettings()->setShortcutKeybind(action.grouping, action.actionID, action.defaultModifiers, action.defaultShortcut);
}

void
PreferencesPanel::onRestoreDefaultShortcutsButtonClicked()
{
    StandardButtonEnum reply = Dialogs::questionDialog( tr("Restore defaults").toStdString(), tr("Restoring default shortcuts "
                                                                                                 "will wipe all the current configuration "
                                                                                                 "are you sure you want to do this?").toStdString(), false );

    if (reply == eStandardButtonYes) {
        appPTR->getCurrentSettings()->restoreDefaultShortcuts();
        for (GuiAppShorcuts::const_iterator it = _imp->appShortcuts.begin(); it != _imp->appShortcuts.end(); ++it) {
            for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
                setItemShortCutText(it2->item, it2->action, true);
            }
        }
        _imp->shortcutsTree->clearSelection();
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_PreferencesPanel.cpp"
