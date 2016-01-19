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

#include "ActionShortcuts.h"
#include <QWidget>
#include "Gui/GuiApplicationManager.h"

NATRON_NAMESPACE_USING

ActionWithShortcut::ActionWithShortcut(const QString & group,
                                       const QString & actionID,
                                       const QString & actionDescription,
                                       QObject* parent,
                                       bool setShortcutOnAction)
: QAction(parent)
, _group(group)
, _shortcuts()
{
    std::list<QKeySequence> seq = getKeybind(group, actionID);
    if (seq.empty()) {
        seq.push_back(QKeySequence());
    }
    _shortcuts.push_back(std::make_pair(actionID, seq.front()));
    assert ( !group.isEmpty() && !actionID.isEmpty() );
    if (setShortcutOnAction) {
        setShortcut(seq.front());
    }
    appPTR->addShortcutAction(group, actionID, this);
    setShortcutContext(Qt::WindowShortcut);
    setText( QObject::tr( actionDescription.toStdString().c_str() ) );
}

ActionWithShortcut::ActionWithShortcut(const QString & group,
                                       const QStringList & actionIDs,
                                       const QString & actionDescription,
                                       QObject* parent,
                                       bool setShortcutOnAction)
: QAction(parent)
, _group(group)
, _shortcuts()
{
    QKeySequence seq0;
    for (int i = 0; i < actionIDs.size(); ++i) {
        std::list<QKeySequence> seq = getKeybind(group, actionIDs[i]);
        if (seq.empty()) {
            seq.push_back(QKeySequence());
        }
        _shortcuts.push_back(std::make_pair(actionIDs[i], seq.front()));
        if (i == 0) {
            seq0 = seq.front();
        }
        appPTR->addShortcutAction(group, actionIDs[i], this);
    }
    assert ( !group.isEmpty() && !actionIDs.isEmpty() );
    if (setShortcutOnAction) {
        setShortcut(seq0);
    }
    
    setShortcutContext(Qt::WindowShortcut);
    setText( QObject::tr( actionDescription.toStdString().c_str() ) );
}

ActionWithShortcut::~ActionWithShortcut()
{
    assert ( !_group.isEmpty() && !_shortcuts.empty() );
    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        appPTR->removeShortcutAction(_group, _shortcuts[i].first, this);
    }
}

void
ActionWithShortcut::setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut)
{
    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        if (_shortcuts[i].first == actionID) {
            _shortcuts[i].second = shortcut;
        }
    }
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

TooltipActionShortcut::TooltipActionShortcut(const QString & group,
                                             const QStringList & actionIDs,
                                             const QString & toolip,
                                             QWidget* parent)
: ActionWithShortcut(group,actionIDs,"",parent, false)
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
    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        finalToolTip = finalToolTip.arg(_shortcuts[i].second.toString(QKeySequence::NativeText));
    }

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
TooltipActionShortcut::setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut)
{
    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        if (_shortcuts[i].first == actionID) {
            _shortcuts[i].second = shortcut;
        }
    }
    setTooltipFromOriginalTooltip();
}

#include "moc_ActionShortcuts.cpp"
