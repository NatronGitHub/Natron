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

#ifndef ACTIONSHORTCUTS_H
#define ACTIONSHORTCUTS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

/**
 * @brief In this file all Natron's actions that can have their shortcut edited should be listed.
 **/

#include <map>
#include <list>
#include <vector>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtCore/QString>
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/KeySymbols.h"

#include "Engine/PluginActionShortcut.h"
#include "Engine/KeybindShortcut.h"

#include "Gui/GuiFwd.h"



NATRON_NAMESPACE_ENTER

inline
QKeySequence
makeKeySequence(const Qt::KeyboardModifiers & modifiers,
                Qt::Key key)
{
    int keys = 0;

    if ( modifiers.testFlag(Qt::ControlModifier) ) {
        keys |= Qt::CTRL;
    }
    if ( modifiers.testFlag(Qt::ShiftModifier) ) {
        keys |= Qt::SHIFT;
    }
    if ( modifiers.testFlag(Qt::AltModifier) ) {
        keys |= Qt::ALT;
    }
    if ( modifiers.testFlag(Qt::MetaModifier) ) {
        keys |= Qt::META;
    }
    if (key != (Qt::Key)0) {
        keys |= key;
    }

    return QKeySequence(keys);
}

///This is tricky to do, what we do is we try to find the native strings of the modifiers
///in the sequence native's string. If we find them, we remove them. The last character
///is then the key symbol, we just have to call seq[0] to retrieve it.
inline void
extractKeySequence(const QKeySequence & seq,
                   Qt::KeyboardModifiers & modifiers,
                   Qt::Key & symbol)
{
    const QString nativeMETAStr = QKeySequence(Qt::META).toString(QKeySequence::NativeText);
    const QString nativeCTRLStr = QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText);
    const QString nativeSHIFTStr = QKeySequence(Qt::SHIFT).toString(QKeySequence::NativeText);
    const QString nativeALTStr = QKeySequence(Qt::ALT).toString(QKeySequence::NativeText);
    QString nativeSeqStr = seq.toString(QKeySequence::NativeText);

    if (nativeSeqStr.indexOf(nativeMETAStr) != -1) {
        modifiers |= Qt::MetaModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeMETAStr);
    }
    if (nativeSeqStr.indexOf(nativeCTRLStr) != -1) {
        modifiers |= Qt::ControlModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeCTRLStr);
    }
    if (nativeSeqStr.indexOf(nativeSHIFTStr) != -1) {
        modifiers |= Qt::ShiftModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeSHIFTStr);
    }
    if (nativeSeqStr.indexOf(nativeALTStr) != -1) {
        modifiers |= Qt::AltModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeALTStr);
    }

    ///The nativeSeqStr now contains only the symbol
    QKeySequence newSeq(nativeSeqStr, QKeySequence::NativeText);
    if (newSeq.count() > 0) {
        symbol = (Qt::Key)newSeq[0];
    } else {
        symbol = (Qt::Key)0;
    }
}


class ActionWithShortcut
    : public QAction
    , public KeybindListenerI
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    QString _group;

protected:

    std::vector<std::pair<QString, QKeySequence> > _shortcuts;

public:

    ActionWithShortcut(const std::string & group,
                       const std::string & actionID,
                       const std::string & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);


    ActionWithShortcut(const std::string & group,
                       const std::list<std::string> & actionIDs,
                       const std::string & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);


    const std::vector<std::pair<QString, QKeySequence> >& getShortcuts() const
    {
        return _shortcuts;
    }

    virtual ~ActionWithShortcut();

    // Overriden from KeybindListenerI
    virtual void onShortcutChanged(const std::string& actionID,
                                   Key symbol,
                                   const KeyboardModifiers& modifiers) OVERRIDE;

};

/**
 * @brief Set the widget's tooltip and append in the tooltip the shortcut associated to the action.
 * This will be dynamically changed when the user edits the shortcuts from the editor.
 **/
#define setToolTipWithShortcut(group, actionID, tooltip, widget) ( widget->addAction( new ToolTipActionShortcut(group, actionID, tooltip, widget) ) )
#define setToolTipWithShortcut2(group, actionIDs, tooltip, widget) ( widget->addAction( new ToolTipActionShortcut(group, actionIDs, tooltip, widget) ) )

class ToolTipActionShortcut
    : public ActionWithShortcut
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    QWidget* _widget;
    QString _originalToolTip;
    bool _tooltipSetInternally;

public:

    /**
     * @brief Set a dynamic shortcut in the tooltip. Reference it with %1 where you want to place the shortcut.
     **/
    ToolTipActionShortcut(const std::string & group,
                          const std::string & actionID,
                          const std::string & toolip,
                          QWidget* parent);

    /**
     * @brief Same as above except that the tooltip can contain multiple shortcuts.
     * In that case the tooltip should reference shortcuts by doing so %1, %2 etc... where
     * %1 references the first actionID, %2 the second ,etc...
     **/
    ToolTipActionShortcut(const std::string & group,
                          const std::list<std::string> & actionIDs,
                          const std::string & toolip,
                          QWidget* parent);

    virtual ~ToolTipActionShortcut()
    {
    }

    // Overriden from KeybindListenerI
    virtual void onShortcutChanged(const std::string& actionID,
                                   Key symbol,
                                   const KeyboardModifiers& modifiers) OVERRIDE FINAL;

private:

    virtual bool eventFilter(QObject* watched, QEvent* event) OVERRIDE FINAL;


    void setToolTipFromOriginalToolTip();
};


NATRON_NAMESPACE_EXIT

#endif // ACTIONSHORTCUTS_H
