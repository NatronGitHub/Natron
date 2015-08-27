/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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
CLANG_DIAG_OFF(deprecated)
#include <QMenu>
CLANG_DIAG_ON(deprecated)

#include <boost/weak_ptr.hpp>
#include "Gui/GuiAppInstance.h"
#include "Gui/Gui.h"
#include "Engine/Project.h"

struct ToolButtonPrivate
{
    GuiAppInstance* _app;
    QString _id;
    int _major,_minor;
    QString _label;
    QIcon _icon;
    QMenu* _menu;
    std::vector<ToolButton*> _children;
    QAction* _action;
    boost::weak_ptr<PluginGroupNode> _pluginToolButton;

    ToolButtonPrivate(GuiAppInstance* app,
                      const boost::shared_ptr<PluginGroupNode>& pluginToolButton,
                      const QString & pluginID,
                      int major,
                      int minor,
                      const QString & label,
                      QIcon icon)
        : _app(app)
          , _id(pluginID)
          , _major(major)
          , _minor(minor)
          , _label(label)
          , _icon(icon)
          , _menu(NULL)
          , _children()
          , _action(NULL)
          , _pluginToolButton(pluginToolButton)
    {
    }
};


ToolButton::ToolButton(GuiAppInstance* app,
                       const boost::shared_ptr<PluginGroupNode>& pluginToolButton,
                       const QString & pluginID,
                       int major,
                       int minor,
                       const QString & label,
                       QIcon icon)
    : _imp( new ToolButtonPrivate(app,pluginToolButton,pluginID,major,minor,label,icon) )
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
ToolButton::getIcon() const
{
    return _imp->_icon;
};
bool
ToolButton::hasChildren() const
{
    return !( _imp->_children.empty() );
}

QMenu*
ToolButton::getMenu() const
{
    assert(_imp->_menu); return _imp->_menu;
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
    for (unsigned int i = 0; i < _imp->_children.size(); ++i) {
        if (_imp->_children[i] == child) {
            return;
        }
    }
    _imp->_children.push_back(child);
    _imp->_menu->addAction( child->getAction() );
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

boost::shared_ptr<PluginGroupNode>
ToolButton::getPluginToolButton() const
{
    return _imp->_pluginToolButton.lock();
}

void
ToolButton::onTriggered()
{
    boost::shared_ptr<NodeCollection> group = _imp->_app->getGui()->getLastSelectedNodeCollection();
    assert(group);
    CreateNodeArgs args(_imp->_id,
                        "",
                        _imp->_major,_imp->_minor,
                        true,
                        INT_MIN,INT_MIN,
                        true,
                        true,
                        true,
                        QString(),
                        CreateNodeArgs::DefaultValuesList(),
                        group);
    _imp->_app->createNode( args );
}

struct ToolButtonChildrenSortFunctor
{
    bool operator() (const ToolButton* lhs,const ToolButton* rhs) {
        QAction* lAct = lhs->getAction();
        QAction* rAct = rhs->getAction();
        assert(lAct && rAct);
        return lAct->text() < rAct->text();
    }
};

void
ToolButton::sortChildren()
{
    if (_imp->_children.empty()) {
        return;
    }
    assert(_imp->_menu);
    _imp->_menu->clear();
    std::sort(_imp->_children.begin(), _imp->_children.end(),ToolButtonChildrenSortFunctor());
    for (std::size_t i = 0; i < _imp->_children.size(); ++i) {
        _imp->_menu->addAction(_imp->_children[i]->getAction());
    }
}
