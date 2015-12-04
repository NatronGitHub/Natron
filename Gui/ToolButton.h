/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef TOOLBUTTON_H
#define TOOLBUTTON_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"


struct ToolButtonPrivate;

class ToolButton
    : public QObject
{
    Q_OBJECT

public:


    ToolButton( GuiAppInstance* app,
                const boost::shared_ptr<PluginGroupNode>& pluginToolButton,
                const QString & pluginID,
                int major,
                int minor,
                const QString & label,
               QIcon toolbuttonIcon = QIcon(),
               QIcon menuIcon = QIcon());

    virtual ~ToolButton();

    const QString & getID() const;
    
    int getPluginMajor() const;
    
    int getPluginMinor() const;
    
    const QString & getLabel() const;
    const QIcon & getToolButtonIcon() const;
    const QIcon & getMenuIcon() const;

    bool hasChildren() const;

    QMenu* getMenu() const;

    void setMenu(QMenu* menu );

    void tryAddChild(ToolButton* child);
    
    void sortChildren();

    const std::vector<ToolButton*> & getChildren() const;
    QAction* getAction() const;

    void setAction(QAction* action);

    boost::shared_ptr<PluginGroupNode> getPluginToolButton() const;

public Q_SLOTS:

    void onTriggered();

private:

    boost::scoped_ptr<ToolButtonPrivate> _imp;
};

#endif // TOOLBUTTON_H
