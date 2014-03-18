//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#ifndef TOOLBUTTON_H
#define TOOLBUTTON_H

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
#include <QIcon>
CLANG_DIAG_ON(deprecated)

#include <boost/scoped_ptr.hpp>

class PluginGroupNode;
class AppInstance;

class QMenu;
class QAction;

struct ToolButtonPrivate;
class ToolButton : public QObject {
    Q_OBJECT
    
public:
    
    ToolButton(AppInstance* app,
               PluginGroupNode* pluginToolButton,
               const QString& pluginID,
               const QString& label,
               QIcon icon = QIcon());
    
    virtual ~ToolButton();
    
    const QString& getID() const;
    
    const QString& getLabel() const;
    
    const QIcon& getIcon() const;
    
    bool hasChildren() const;
    
    QMenu* getMenu() const;
    
    void setMenu(QMenu* menu );
    
    void tryAddChild(ToolButton* child);
    
    const std::vector<ToolButton*>& getChildren() const;
    
    QAction* getAction() const;
    
    void setAction(QAction* action);
    
    PluginGroupNode* getPluginToolButton() const;
    
public slots:
    
    void onTriggered();
    
private:
    
    boost::scoped_ptr<ToolButtonPrivate> _imp;
};

#endif // TOOLBUTTON_H
