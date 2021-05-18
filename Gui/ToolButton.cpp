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

#include "ToolButton.h"

#include <algorithm>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
#include <QMenu>
#include <QtCore/QtAlgorithms>
CLANG_DIAG_ON(deprecated)

#include <boost/weak_ptr.hpp>
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_ENTER

struct ToolButtonPrivate
{
    GuiAppInstanceWPtr _app;
    QString _id;
    int _major, _minor;
    QString _label;
    QIcon _toolbuttonIcon, _menuIcon;
    QMenu* _menu;
    std::vector<ToolButton*> _children;
    QAction* _action;
    PluginGroupNodeWPtr _pluginToolButton;

    ToolButtonPrivate(const GuiAppInstancePtr& app,
                      const PluginGroupNodePtr& pluginToolButton,
                      const QString & pluginID,
                      int major,
                      int minor,
                      const QString & label,
                      const QIcon& toolbuttonIcon,
                      const QIcon& menuIcon)
        : _app(app)
        , _id(pluginID)
        , _major(major)
        , _minor(minor)
        , _label(label)
        , _toolbuttonIcon(toolbuttonIcon)
        , _menuIcon(menuIcon)
        , _menu(NULL)
        , _children()
        , _action(NULL)
        , _pluginToolButton(pluginToolButton)
    {
    }
};

ToolButton::ToolButton(const GuiAppInstancePtr& app,
                       const PluginGroupNodePtr& pluginToolButton,
                       const QString & pluginID,
                       int major,
                       int minor,
                       const QString & label,
                       QIcon toolbuttonIcon,
                       QIcon menuIcon)
    : _imp( new ToolButtonPrivate(app, pluginToolButton, pluginID, major, minor, label, toolbuttonIcon, menuIcon) )
{
}

ToolButton::~ToolButton()
{
}

const QString &
ToolButton::getID() const
{
    return _imp->_id;
}

int
ToolButton::getPluginMajor() const
{
    return _imp->_major;
}

int
ToolButton::getPluginMinor() const
{
    return _imp->_minor;
}

const QString &
ToolButton::getLabel() const
{
    return _imp->_label;
};
const QIcon &
ToolButton::getToolButtonIcon() const
{
    return _imp->_toolbuttonIcon;
};
const QIcon &
ToolButton::getMenuIcon() const
{
    return _imp->_menuIcon;
}

bool
ToolButton::hasChildren() const
{
    return !( _imp->_children.empty() );
}

QMenu*
ToolButton::getMenu() const
{
    assert(_imp->_menu);

    return _imp->_menu;
}

void
ToolButton::setMenu(QMenu* menu )
{
    _imp->_menu = menu;
}

void
ToolButton::tryAddChild(ToolButton* child)
{
    assert(_imp->_menu);
    // insert if not present
    std::vector<ToolButton*> &children = _imp->_children;
    if ( std::find(children.begin(), children.end(), child) == children.end() ) {
        children.push_back(child);
        _imp->_menu->addAction( child->getAction() );
    }
}

const std::vector<ToolButton*> &
ToolButton::getChildren() const
{
    return _imp->_children;
}

QAction*
ToolButton::getAction() const
{
    return _imp->_action;
}

void
ToolButton::setAction(QAction* action)
{
    _imp->_action = action;
}

PluginGroupNodePtr
ToolButton::getPluginToolButton() const
{
    return _imp->_pluginToolButton.lock();
}

void
ToolButton::onTriggered()
{
    GuiAppInstancePtr app = _imp->_app.lock();

    if (!app) {
        return;
    }
    NodeCollectionPtr group = app->getGui()->getLastSelectedNodeCollection();

    assert(group);
    CreateNodeArgs args(_imp->_id.toStdString(), group);
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, _imp->_major, 0);
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, _imp->_minor, 1);
    app->createNode(args);
}

struct ToolButtonChildrenSortFunctor
{
    bool operator() (const QAction* lhs,
                     const QAction* rhs)
    {
        return lhs->text() < rhs->text();
    }
};

void
ToolButton::sortChildren()
{
    if ( _imp->_children.empty() ) {
        return;
    }
    assert(_imp->_menu);
    QList<QAction*> actions = _imp->_menu->actions();
    qSort( actions.begin(), actions.end(), ToolButtonChildrenSortFunctor() );
    _imp->_menu->clear();

    std::vector<ToolButton*> sortedChildren;
    for (QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it) {
        /// find toolbutton if possible
        for (U32 i = 0; i < _imp->_children.size(); ++i) {
            if (_imp->_children[i]->getAction() == *it) {
                sortedChildren.push_back(_imp->_children[i]);
                break;
            }
        }
        _imp->_menu->addAction(*it);
    }
    _imp->_children = sortedChildren;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ToolButton.cpp"
