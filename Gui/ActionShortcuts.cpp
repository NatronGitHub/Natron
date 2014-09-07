//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "ActionShortcuts.h"

#include "Gui/GuiApplicationManager.h"

ActionWithShortcut::ActionWithShortcut(const QString & group,
                   const QString & actionID,
                   const QString & actionDescription,
                   QObject* parent)
: QAction(parent)
, _group(group)
, _actionID(actionID)
{
    assert ( !group.isEmpty() && !actionID.isEmpty() );
    setShortcut( getKeybind(group, actionID) );
    appPTR->addShortcutAction(group, actionID, this);
    setShortcutContext(Qt::WindowShortcut);
    setText( QObject::tr( actionDescription.toStdString().c_str() ) );
}

ActionWithShortcut::~ActionWithShortcut()
{
    assert ( !_group.isEmpty() && !_actionID.isEmpty() );
    appPTR->removeShortcutAction(_group, _actionID, this);
}
