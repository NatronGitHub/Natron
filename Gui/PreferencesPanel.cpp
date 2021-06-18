/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <QPainter>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/ActionShortcuts.h"
#include "Gui/GuiDefines.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/Gui.h"
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
        , plugin(NULL)
    {
    }

    QTreeWidgetItem* item;
    AnimatedCheckBox* enabledCheckbox;
    AnimatedCheckBox* rsCheckbox;
    AnimatedCheckBox* mtCheckbox;
    AnimatedCheckBox* glCheckbox;
    Plugin* plugin;
};

typedef std::list<PluginTreeNode> PluginTreeNodeList;


struct GuiBoundAction
{
    GuiBoundAction()
        : item(NULL)
        , action(NULL)
    {
    }

    QTreeWidgetItem* item;
    BoundAction* action;
};

struct GuiShortCutGroup
{
    GuiShortCutGroup()
        : actions()
        , item(NULL)
    {
    }

    std::list<GuiBoundAction> actions;
    QTreeWidgetItem* item;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

static QString
keybindToString(const Qt::KeyboardModifiers & modifiers,
                Qt::Key key)
{
    return makeKeySequence(modifiers, key).toString(QKeySequence::NativeText);
}

static QString
mouseShortcutToString(const Qt::KeyboardModifiers & modifiers,
                      Qt::MouseButton button)
{
    QString ret = makeKeySequence(modifiers, (Qt::Key)0).toString(QKeySequence::NativeText);

    switch (button) {
    case Qt::LeftButton:
        ret.append( QCoreApplication::translate("ShortCutEditor", "LeftButton") );
        break;
    case Qt::MiddleButton:
        ret.append( QCoreApplication::translate("ShortCutEditor", "MiddleButton") );
        break;
    case Qt::RightButton:
        ret.append( QCoreApplication::translate("ShortCutEditor", "RightButton") );
        break;
    default:
        break;
    }

    return ret;
}

typedef std::list<GuiShortCutGroup> GuiAppShorcuts;

///A small hack to the QTreeWidget class to make 2 functions public so we can use them in the ShortcutDelegate class
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
makeItemShortCutText(const BoundAction* action,
                     bool useDefault,
                     QString* shortcutStr,
                     QString* altShortcutStr)
{
    const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(action);
    const MouseAction* ma = dynamic_cast<const MouseAction*>(action);

    if (ka) {
        if (useDefault) {
            if ( !ka->defaultModifiers.empty() ) {
                assert( ka->defaultModifiers.size() == ka->defaultShortcut.size() );
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ka->defaultModifiers.begin();
                std::list<Qt::Key>::const_iterator sit = ka->defaultShortcut.begin();
                *shortcutStr = keybindToString(*mit, *sit);
                if (ka->defaultShortcut.size() > 1) {
                    ++mit;
                    ++sit;
                    *altShortcutStr = keybindToString(*mit, *sit);
                }
            }
        } else {
            if ( !ka->modifiers.empty() ) {
                assert( ka->modifiers.size() == ka->currentShortcut.size() );
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ka->modifiers.begin();
                std::list<Qt::Key>::const_iterator sit = ka->currentShortcut.begin();
                *shortcutStr = keybindToString(*mit, *sit);
                if (ka->currentShortcut.size() > 1) {
                    ++mit;
                    ++sit;
                    *altShortcutStr = keybindToString(*mit, *sit);
                }
            }
        }
    } else if (ma) {
        if (useDefault) {
            if ( !ma->defaultModifiers.empty() ) {
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ma->defaultModifiers.begin();
                *shortcutStr = mouseShortcutToString(*mit, ma->button);
                if (ma->defaultModifiers.size() > 1) {
                    ++mit;
                    *altShortcutStr = mouseShortcutToString(*mit, ma->button);
                }
            }
        } else {
            if ( !ma->modifiers.empty() ) {
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ma->modifiers.begin();
                *shortcutStr = mouseShortcutToString(*mit, ma->button);
                if (ma->modifiers.size() > 1) {
                    ++mit;
                    *altShortcutStr = mouseShortcutToString(*mit, ma->button);
                }
            }
        }
    } else {
        assert(false);
    }
} // makeItemShortCutText

static void
setItemShortCutText(QTreeWidgetItem* item,
                    const BoundAction* action,
                    bool useDefault)
{
    QString sc, altSc;

    makeItemShortCutText(action, useDefault, &sc, &altSc);
    item->setText( 1,  sc);
    item->setText( 2,  altSc);
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
    DialogButtonBox* buttonBox;
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
    QFrame* shortcutsFrame;
    QVBoxLayout* shortcutsLayout;
    HackedTreeWidget* shortcutsTree;
    QWidget* shortcutGroup;
    QHBoxLayout* shortcutGroupLayout;
    Label* shortcutLabel;
    KeybindRecorder* shortcutEditor;
    QWidget* altShortcutGroup;
    QHBoxLayout* altShortcutGroupLayout;
    Label* altShortcutLabel;
    KeybindRecorder* altShortcutEditor;
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
        , pluginsList()
        , shortcutsFrame(0)
        , shortcutsLayout(0)
        , shortcutsTree(0)
        , shortcutGroup(0)
        , shortcutGroupLayout(0)
        , shortcutLabel(0)
        , shortcutEditor(0)
        , altShortcutGroup(0)
        , altShortcutGroupLayout(0)
        , altShortcutLabel(0)
        , altShortcutEditor(0)
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
    BoundAction* getActionForTreeItem(QTreeWidgetItem* item) const
    {
        for (GuiAppShorcuts::const_iterator it = appShortcuts.begin(); it != appShortcuts.end(); ++it) {
            if (it->item == item) {
                return NULL;
            }
            for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
                if (it2->item == item) {
                    return it2->action;
                }
            }
        }

        return (BoundAction*)NULL;
    }

    GuiAppShorcuts::iterator buildShortcutsGroupHierarchy(QString grouping);

    void makeGuiActionForShortcut(GuiAppShorcuts::iterator guiGroupIterator, BoundAction* action);
};

PreferencesPanel::PreferencesPanel(Gui *parent)
    : QWidget(parent)
    , KnobGuiContainerHelper( appPTR->getCurrentSettings().get(), QUndoStackPtr() )
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
    group.plugin = 0;
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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
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
            Plugin* plugin  = *itver;
            assert(plugin);
            if ( plugin->getIsForInternalUseOnly() ) {
                continue;
            }


            PluginTreeNodeList::iterator foundParent = _imp->buildPluginGroupHierarchy( plugin->getGrouping() );
            PluginTreeNode node;
            node.plugin = plugin;
            if ( foundParent == _imp->pluginsList.end() ) {
                node.item = new QTreeWidgetItem(_imp->pluginsView);
            } else {
                node.item = new QTreeWidgetItem(foundParent->item);
            }
            node.item->setText( COL_PLUGIN_LABEL, plugin->getLabelWithoutSuffix() );
            node.item->setText( COL_PLUGINID, plugin->getPluginID() );
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
                checkbox->setChecked( plugin->isActivated() );
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
                checkbox->setChecked( plugin->isActivated() );
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
                checkbox->setChecked( plugin->isActivated() );
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
                checkbox->setChecked(plugin->isActivated());
                QObject::connect( checkbox, SIGNAL(clicked(bool)), this, SLOT(onGLEnabledCheckBoxChecked(bool)) );
                _imp->pluginsView->setItemWidget(node.item, COL_GL_ENABLED, checkbox);
                if (plugin->getPluginOpenGLRenderSupport() == ePluginOpenGLRenderSupportNone) {
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
    headers << tr("Command") << tr("Shortcut") << tr("Alt. Shortcut");
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

    const AppShortcuts & appShortcuts = appPTR->getAllShortcuts();

    for (AppShortcuts::const_iterator it = appShortcuts.begin(); it != appShortcuts.end(); ++it) {
        GuiAppShorcuts::iterator foundGuiGroup = _imp->buildShortcutsGroupHierarchy(it->first);
        assert( foundGuiGroup != _imp->appShortcuts.end() );

        for (GroupShortcuts::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            _imp->makeGuiActionForShortcut(foundGuiGroup, it2->second);
        }
    }

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

    _imp->altShortcutGroup = new QWidget(this);
    _imp->shortcutsLayout->addWidget(_imp->altShortcutGroup);

    _imp->altShortcutGroupLayout = new QHBoxLayout(_imp->altShortcutGroup);
    _imp->altShortcutGroupLayout->setContentsMargins(0, 0, 0, 0);

    _imp->altShortcutLabel = new Label(_imp->altShortcutGroup);
    _imp->altShortcutLabel->setText( tr("Alternative Sequence:") );
    _imp->altShortcutGroupLayout->addWidget(_imp->altShortcutLabel);

    _imp->altShortcutEditor = new KeybindRecorder(_imp->altShortcutGroup);
    _imp->altShortcutEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    _imp->altShortcutEditor->setPlaceholderText( tr("Type to set an alternative shortcut") );
    _imp->altShortcutGroupLayout->addWidget(_imp->altShortcutEditor);


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
} // PreferencesPanel::createShortcutEditor

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

    _imp->splitter = new Splitter(Qt::Horizontal, this);

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
        if (pageKnob->getName() == "plugins") {
            pluginsFrameLayout = _imp->tabs[i].page.lock()->gridLayout;
        } else if (pageKnob->getName() == "userInterfacePage") {
            uiTabTreeItem = _imp->tabs[i].treeItem;
        }
        QString label = QString::fromUtf8( pageKnob->getLabel().c_str() );
        int w = fm.width(label);
        maxLength = std::max(w, maxLength);
    }
    assert(pluginsFrameLayout);
    assert(uiTabTreeItem);
    _imp->tree->setFixedWidth(maxLength + 100);

    _imp->buttonBox = new DialogButtonBox(Qt::Horizontal);
    _imp->restoreDefaultsB = new Button( tr("Restore Defaults") );
    _imp->restoreDefaultsB->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Restore default values for all preferences."), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->prefsHelp = new Button( tr("Help") );
    _imp->prefsHelp->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Display help for preferences in an external browser."), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->cancelB = new Button( tr("Discard") );
    _imp->cancelB->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Cancel changes that were not saved and close the window."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->okB = new Button( tr("Save") );
    _imp->okB->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Save changes on disk and close the window."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonBox->addButton(_imp->restoreDefaultsB, QDialogButtonBox::ResetRole);
    _imp->buttonBox->addButton(_imp->prefsHelp, QDialogButtonBox::HelpRole);
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
    createPluginsView(pluginsFrameLayout);


    // Create the shortcut Editor
    createShortcutEditor(uiTabTreeItem);


    _imp->tabs[0].treeItem->setSelected(true);
    onItemSelectionChanged();

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
            if ( it->plugin && it->plugin->getLabelWithoutSuffix().contains(expr) ) {
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
            it->plugin->setActivated(checked);
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
            it->plugin->setRenderScaleEnabled(checked);
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
            it->plugin->setMultiThreadingEnabled(checked);
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
            it->plugin->setOpenGLEnabled(checked);
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
    SettingsPtr settings = appPTR->getCurrentSettings();
    if (settings) {
        settings->saveSettings(_imp->changedKnobs, true, _imp->pluginSettingsChanged);
    }
    appPTR->saveShortcuts();
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
    if ( !_imp->closeIsOK && (!_imp->changedKnobs.empty() || _imp->pluginSettingsChanged) ) {
        SettingsPtr settings = appPTR->getCurrentSettings();
        if ( !_imp->changedKnobs.empty() ) {
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

    QTreeWidgetItem* groupParent;
    if ( foundGuiGroup != appShortcuts.end() ) {
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
            groupParent = groupingItem;
        }
    }
    GuiShortCutGroup group;
    group.item = groupParent;
    foundGuiGroup = appShortcuts.insert(appShortcuts.end(), group);

    return foundGuiGroup;
} // PreferencesPanel::buildShortcutGroupHierarchy

void
PreferencesPanelPrivate::makeGuiActionForShortcut(GuiAppShorcuts::iterator guiGroupIterator,
                                                  BoundAction* action)
{
    GuiBoundAction guiAction;

    guiAction.action = action;
    guiAction.item = new QTreeWidgetItem(guiGroupIterator->item);
    guiAction.item->setText(0, guiAction.action->description);
    const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(action);
    const MouseAction* ma = dynamic_cast<const MouseAction*>(action);
    QString shortcutStr, altShortcutStr;
    if (ka) {
        if ( !ka->modifiers.empty() ) {
            std::list<Qt::KeyboardModifiers>::const_iterator mit = ka->modifiers.begin();
            std::list<Qt::Key>::const_iterator sit = ka->currentShortcut.begin();
            shortcutStr = keybindToString(*mit, *sit);
            if (ka->modifiers.size() > 1) {
                ++mit;
                ++sit;
                altShortcutStr = keybindToString(*mit, *sit);
            }
        }
    } else if (ma) {
        if ( !ma->modifiers.empty() ) {
            std::list<Qt::KeyboardModifiers>::const_iterator mit = ma->modifiers.begin();
            shortcutStr = mouseShortcutToString(*mit, ma->button);
            if (ma->modifiers.size() > 1) {
                ++mit;
                altShortcutStr = mouseShortcutToString(*mit, ma->button);
            }
        }
    } else {
        assert(false);
    }
    if (!action->editable) {
        guiAction.item->setToolTip( 0, tr("This action is standard and its shortcut cannot be edited.") );
        guiAction.item->setToolTip( 1, tr("This action is standard and its shortcut cannot be edited.") );
        guiAction.item->setDisabled(true);
    }
    guiAction.item->setExpanded(true);
    guiAction.item->setText(1, shortcutStr);
    guiAction.item->setText(2, altShortcutStr);
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
        font.setPointSize(13);
        pen.setColor(Qt::gray);
    } else {
        font.setBold(false);
        font.setPointSize(11);
        if ( item->isDisabled() ) {
            pen.setColor(Qt::gray);
        } else {
            pen.setColor(Qt::lightGray);
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
PreferencesPanel::addShortcut(BoundAction* action)
{
    GuiAppShorcuts::iterator foundGuiGroup = _imp->buildShortcutsGroupHierarchy(action->grouping);

    assert( foundGuiGroup != _imp->appShortcuts.end() );

    _imp->makeGuiActionForShortcut(foundGuiGroup, action);
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

    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
    QString sc, altSc;
    makeItemShortCutText(action, false, &sc, &altSc);
    _imp->shortcutEditor->setText(sc);
    _imp->altShortcutEditor->setText(altSc);
}

void
PreferencesPanel::onValidateShortcutButtonClicked()
{
    QString text = _imp->shortcutEditor->text();
    QString altText = _imp->altShortcutEditor->text();

    QList<QTreeWidgetItem*> items = _imp->shortcutsTree->selectedItems();
    if ( (items.size() > 1) || items.empty() ) {
        return;
    }

    QTreeWidgetItem* selection = items.front();
    QKeySequence seq(text, QKeySequence::NativeText);
    QKeySequence altseq(altText, QKeySequence::NativeText);
    BoundAction* action = _imp->getActionForTreeItem(selection);
    QTreeWidgetItem* parent = selection->parent();
    while (parent) {
        QTreeWidgetItem* parentUp = parent->parent();
        if (!parentUp) {
            break;
        }
        parent = parentUp;
    }
    assert(parent);

    //only keybinds can be edited...
    KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(action);
    assert(ka);
    if (!ka) {
        return;
    }
    Qt::KeyboardModifiers modifiers, altmodifiers;
    Qt::Key symbol, altsymbmol;
    extractKeySequence(seq, modifiers, symbol);
    extractKeySequence(altseq, altmodifiers, altsymbmol);

    for (GuiAppShorcuts::iterator it = _imp->appShortcuts.begin(); it != _imp->appShortcuts.end(); ++it) {
        for (std::list<GuiBoundAction>::iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
            if ( (it2->action != action) && ( it->item->text(0) == parent->text(0) ) ) {
                KeyBoundAction* keyAction = dynamic_cast<KeyBoundAction*>(it2->action);
                if (keyAction) {
                    assert( keyAction->modifiers.size() == keyAction->currentShortcut.size() );
                    std::list<Qt::KeyboardModifiers>::const_iterator mit = keyAction->modifiers.begin();
                    for (std::list<Qt::Key>::const_iterator it3 = keyAction->currentShortcut.begin(); it3 != keyAction->currentShortcut.end(); ++it3, ++mit) {
                        if ( (*mit == modifiers) && (*it3 == symbol) ) {
                            QString err = QString::fromUtf8("Cannot bind this shortcut because the following action is already using it: %1")
                                          .arg( it2->item->text(0) );
                            _imp->shortcutEditor->clear();
                            Dialogs::errorDialog( tr("Shortcuts Editor").toStdString(), tr( err.toStdString().c_str() ).toStdString() );

                            return;
                        }
                    }
                }
            }
        }
    }

    selection->setText(1, text);
    action->modifiers.clear();
    if ( !text.isEmpty() ) {
        action->modifiers.push_back(modifiers);
        ka->currentShortcut.push_back(symbol);
    }
    if ( !altText.isEmpty() ) {
        action->modifiers.push_back(altmodifiers);
        ka->currentShortcut.push_back(altsymbmol);
    }

    appPTR->notifyShortcutChanged(ka);
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
        _imp->altShortcutEditor->setText( QString() );
        _imp->altShortcutEditor->setPlaceholderText( tr("Type to set an alternative shortcut") );

        return;
    }

    QTreeWidgetItem* selection = items.front();
    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
    action->modifiers.clear();
    MouseAction* ma = dynamic_cast<MouseAction*>(action);
    KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(action);
    if (ma) {
        ma->button = Qt::NoButton;
    } else if (ka) {
        ka->currentShortcut.clear();
    }

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
    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
    action->modifiers = action->defaultModifiers;
    KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(action);
    if (ka) {
        ka->currentShortcut = ka->defaultShortcut;
        appPTR->notifyShortcutChanged(ka);
    }
    setItemShortCutText(selection, action, true);

    QString sc, altsc;
    makeItemShortCutText(action, true, &sc, &altsc);
    _imp->shortcutEditor->setText(sc);
    _imp->altShortcutEditor->setText(altsc);
}

void
PreferencesPanel::onRestoreDefaultShortcutsButtonClicked()
{
    StandardButtonEnum reply = Dialogs::questionDialog( tr("Restore defaults").toStdString(), tr("Restoring default shortcuts "
                                                                                                 "will wipe all the current configuration "
                                                                                                 "are you sure you want to do this?").toStdString(), false );

    if (reply == eStandardButtonYes) {
        appPTR->restoreDefaultShortcuts();
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
