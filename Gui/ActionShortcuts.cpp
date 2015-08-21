//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ActionShortcuts.h"
#include <QWidget>
#include "Gui/GuiApplicationManager.h"

ActionWithShortcut::ActionWithShortcut(const QString & group,
                                       const QString & actionID,
                                       const QString & actionDescription,
                                       QObject* parent,
                                       bool setShortcutOnAction)
: QAction(parent)
, _group(group)
, _actionID(actionID)
, _shortcut(getKeybind(group, actionID))
{
    assert ( !group.isEmpty() && !actionID.isEmpty() );
    if (setShortcutOnAction) {
        setShortcut(_shortcut);
    }
    appPTR->addShortcutAction(group, actionID, this);
    setShortcutContext(Qt::WindowShortcut);
    setText( QObject::tr( actionDescription.toStdString().c_str() ) );
}

ActionWithShortcut::~ActionWithShortcut()
{
    assert ( !_group.isEmpty() && !_actionID.isEmpty() );
    appPTR->removeShortcutAction(_group, _actionID, this);
}

void
ActionWithShortcut::setShortcutWrapper(const QKeySequence& shortcut)
{
    _shortcut = shortcut;
    setShortcut(shortcut);
}

TooltipActionShortcut::TooltipActionShortcut(const QString & group,
                      const QString & actionID,
                      const QString & toolip,
                      QWidget* parent)
: ActionWithShortcut(group,actionID,"",parent, false)
, _widget(parent)
, _originalTooltip(toolip)
, _tooltipSetInternally(false)
{
    assert(parent);
    setTooltipFromOriginalTooltip();
    _widget->installEventFilter(this);
}

void
TooltipActionShortcut::setTooltipFromOriginalTooltip()
{
    QString finalToolTip = _originalTooltip;
    finalToolTip += "<p><b>";
    finalToolTip += QObject::tr("Keyboard shortcut: ");
    finalToolTip += _shortcut.toString(QKeySequence::NativeText);
    finalToolTip += "</b></p>";
    
    _tooltipSetInternally = true;
    _widget->setToolTip(finalToolTip);
    _tooltipSetInternally = false;
}

bool
TooltipActionShortcut::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != _widget) {
        return false;
    }
    if (event->type() == QEvent::ToolTipChange) {
        if (_tooltipSetInternally) {
            return false;
        }
        _originalTooltip = _widget->toolTip();
        setTooltipFromOriginalTooltip();
    }
    return false;
}

void
TooltipActionShortcut::setShortcutWrapper(const QKeySequence& shortcut)
{
    _shortcut = shortcut;
    setTooltipFromOriginalTooltip();
}
