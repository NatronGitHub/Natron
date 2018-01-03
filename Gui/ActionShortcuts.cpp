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

#include "ActionShortcuts.h"

#include <stdexcept>

#include <QWidget>
#include "Engine/Settings.h"

#include "Gui/QtEnumConvert.h"
#include "Gui/GuiApplicationManager.h"

NATRON_NAMESPACE_ENTER

ActionWithShortcut::ActionWithShortcut(const std::string & group,
                                       const std::string & actionID,
                                       const std::string & actionDescription,
                                       QObject* parent,
                                       bool setShortcutOnAction)
    : QAction(parent)
    , _group( QString::fromUtf8( group.c_str() ) )
    , _shortcuts()
{
    // insert here the output of:
    // fgrep "#define kShortcutAction" KeybindShortcut.h | sed -e 's/#define /(void)QT_TR_NOOP(/' -e 's/ .*/);/'
    (void)QT_TR_NOOP(kShortcutActionNewProject);
    (void)QT_TR_NOOP(kShortcutActionOpenProject);
    (void)QT_TR_NOOP(kShortcutActionCloseProject);
    (void)QT_TR_NOOP(kShortcutActionReloadProject);
    (void)QT_TR_NOOP(kShortcutActionSaveProject);
    (void)QT_TR_NOOP(kShortcutActionSaveAsProject);
    (void)QT_TR_NOOP(kShortcutActionSaveAndIncrVersion);
    (void)QT_TR_NOOP(kShortcutActionPreferences);
    (void)QT_TR_NOOP(kShortcutActionQuit);
    (void)QT_TR_NOOP(kShortcutActionProjectSettings);
    (void)QT_TR_NOOP(kShortcutActionShowErrorLog);
    (void)QT_TR_NOOP(kShortcutActionNewViewer);
    (void)QT_TR_NOOP(kShortcutActionFullscreen);
    (void)QT_TR_NOOP(kShortcutActionShowWindowsConsole);
    (void)QT_TR_NOOP(kShortcutActionClearPluginsLoadCache);
    (void)QT_TR_NOOP(kShortcutActionClearCache);
    (void)QT_TR_NOOP(kShortcutActionShowAbout);
    (void)QT_TR_NOOP(kShortcutActionRenderSelected);
    (void)QT_TR_NOOP(kShortcutActionEnableRenderStats);
    (void)QT_TR_NOOP(kShortcutActionRenderAll);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput1);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput2);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput3);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput4);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput5);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput6);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput7);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput8);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput9);
    (void)QT_TR_NOOP(kShortcutActionConnectViewerToInput10);
    (void)QT_TR_NOOP(kShortcutActionShowPaneFullScreen);
    (void)QT_TR_NOOP(kShortcutActionImportLayout);
    (void)QT_TR_NOOP(kShortcutActionExportLayout);
    (void)QT_TR_NOOP(kShortcutActionDefaultLayout);
    (void)QT_TR_NOOP(kShortcutActionNextTab);
    (void)QT_TR_NOOP(kShortcutActionPrevTab);
    (void)QT_TR_NOOP(kShortcutActionCloseTab);
    (void)QT_TR_NOOP(kShortcutActionGraphRearrangeNodes);
    (void)QT_TR_NOOP(kShortcutActionGraphRemoveNodes);
    (void)QT_TR_NOOP(kShortcutActionGraphShowExpressions);
    (void)QT_TR_NOOP(kShortcutActionGraphSelectUp);
    (void)QT_TR_NOOP(kShortcutActionGraphSelectDown);
    (void)QT_TR_NOOP(kShortcutActionGraphSelectAll);
    (void)QT_TR_NOOP(kShortcutActionGraphSelectAllVisible);
    (void)QT_TR_NOOP(kShortcutActionGraphAutoHideInputs);
    (void)QT_TR_NOOP(kShortcutActionGraphHideInputs);
    (void)QT_TR_NOOP(kShortcutActionGraphSwitchInputs);
    (void)QT_TR_NOOP(kShortcutActionGraphCopy);
    (void)QT_TR_NOOP(kShortcutActionGraphPaste);
    (void)QT_TR_NOOP(kShortcutActionGraphClone);
    (void)QT_TR_NOOP(kShortcutActionGraphDeclone);
    (void)QT_TR_NOOP(kShortcutActionGraphCut);
    (void)QT_TR_NOOP(kShortcutActionGraphDuplicate);
    (void)QT_TR_NOOP(kShortcutActionGraphDisableNodes);
    (void)QT_TR_NOOP(kShortcutActionGraphToggleAutoPreview);
    (void)QT_TR_NOOP(kShortcutActionGraphToggleAutoTurbo);
    (void)QT_TR_NOOP(kShortcutActionGraphTogglePreview);
    (void)QT_TR_NOOP(kShortcutActionGraphForcePreview);
    (void)QT_TR_NOOP(kShortcutActionGraphShowCacheSize);
    (void)QT_TR_NOOP(kShortcutActionGraphFrameNodes);
    (void)QT_TR_NOOP(kShortcutActionGraphFindNode);
    (void)QT_TR_NOOP(kShortcutActionGraphRenameNode);
    (void)QT_TR_NOOP(kShortcutActionGraphExtractNode);
    (void)QT_TR_NOOP(kShortcutActionGraphMakeGroup);
    (void)QT_TR_NOOP(kShortcutActionGraphExpandGroup);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleRemoveKeys);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleConstant);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleLinear);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleSmooth);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleCatmullrom);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleCubic);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleHorizontal);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleBreak);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleSelectAll);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleCenter);
    (void)QT_TR_NOOP(kShortcutActionAnimationModuleCopy);
    (void)QT_TR_NOOP(kShortcutActionAnimationModulePasteKeyframes);
    (void)QT_TR_NOOP(kShortcutActionAnimationModulePasteKeyframesAbsolute);
    (void)QT_TR_NOOP(kShortcutActionScriptEditorPrevScript);
    (void)QT_TR_NOOP(kShortcutActionScriptEditorNextScript);
    (void)QT_TR_NOOP(kShortcutActionScriptEditorClearHistory);
    (void)QT_TR_NOOP(kShortcutActionScriptExecScript);
    (void)QT_TR_NOOP(kShortcutActionScriptClearOutput);
    (void)QT_TR_NOOP(kShortcutActionScriptShowOutput);

    QString actionIDStr = QString::fromUtf8( actionID.c_str() );
    QKeySequence seq = getKeybind(_group, actionIDStr);

    _shortcuts.push_back( std::make_pair( actionIDStr, seq) );
    assert ( !_group.isEmpty() && !actionIDStr.isEmpty() );
    if (setShortcutOnAction) {
        setShortcut( seq );
    }
    appPTR->getCurrentSettings()->addShortcutAction(_group.toStdString(), actionIDStr.toStdString(), this);
    setShortcutContext(Qt::WindowShortcut);
    setText( tr( actionDescription.c_str() ) );
}

ActionWithShortcut::ActionWithShortcut(const std::string & group,
                                       const std::list<std::string> & actionIDs,
                                       const std::string & actionDescription,
                                       QObject* parent,
                                       bool setShortcutOnAction)
    : QAction(parent)
    , _group( QString::fromUtf8( group.c_str() ) )
    , _shortcuts()
{
    QKeySequence seq0;

    for (std::list<std::string>::const_iterator it = actionIDs.begin(); it != actionIDs.end(); ++it) {
        QString actionIDStr = QString::fromUtf8( it->c_str() );
        QKeySequence seq = getKeybind(_group, actionIDStr);

        _shortcuts.push_back( std::make_pair( actionIDStr, seq) );
        if ( it == actionIDs.begin() ) {
            seq0 = seq;
        }
        appPTR->getCurrentSettings()->addShortcutAction(_group.toStdString(), actionIDStr.toStdString(), this);
    }
    assert ( !_group.isEmpty() );
    if (setShortcutOnAction) {
        setShortcut(seq0);
    }

    setShortcutContext(Qt::WindowShortcut);
    setText( tr( actionDescription.c_str() ) );
}

ActionWithShortcut::~ActionWithShortcut()
{
    assert ( !_group.isEmpty() && !_shortcuts.empty() );
    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        appPTR->getCurrentSettings()->removeShortcutAction(_group.toStdString(), _shortcuts[i].first.toStdString(), this);
    }
}

void
ActionWithShortcut::onShortcutChanged(const std::string& actionID,
                                      Key symbol,
                                      const KeyboardModifiers& modifiers)
{
    QKeySequence seq = makeKeySequence(QtEnumConvert::toQtModifiers(modifiers), QtEnumConvert::toQtKey(symbol));
    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        if (_shortcuts[i].first.toStdString() == actionID) {
            _shortcuts[i].second = seq;
        }
    }
    setShortcut(seq);
}

ToolTipActionShortcut::ToolTipActionShortcut(const std::string & group,
                                             const std::string & actionID,
                                             const std::string & toolip,
                                             QWidget* parent)
    : ActionWithShortcut(group, actionID, std::string(), parent, false)
    , _widget(parent)
    , _originalToolTip( QString::fromUtf8( toolip.c_str() ) )
    , _tooltipSetInternally(false)
{
    assert(parent);
    setToolTipFromOriginalToolTip();
    _widget->installEventFilter(this);
}

ToolTipActionShortcut::ToolTipActionShortcut(const std::string & group,
                                             const std::list<std::string> & actionIDs,
                                             const std::string & toolip,
                                             QWidget* parent)
    : ActionWithShortcut(group, actionIDs, std::string(), parent, false)
    , _widget(parent)
    , _originalToolTip( QString::fromUtf8( toolip.c_str() ) )
    , _tooltipSetInternally(false)
{
    assert(parent);
    setToolTipFromOriginalToolTip();
    _widget->installEventFilter(this);
}

void
ToolTipActionShortcut::setToolTipFromOriginalToolTip()
{
    QString finalToolTip = _originalToolTip;

    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        finalToolTip = finalToolTip.arg( _shortcuts[i].second.toString(QKeySequence::NativeText) );
    }

    _tooltipSetInternally = true;
    _widget->setToolTip(finalToolTip);
    _tooltipSetInternally = false;
}

bool
ToolTipActionShortcut::eventFilter(QObject* watched,
                                   QEvent* event)
{
    if (watched != _widget) {
        return false;
    }
    if (event->type() == QEvent::ToolTipChange) {
        if (_tooltipSetInternally) {
            return false;
        }
        _originalToolTip = _widget->toolTip();
        setToolTipFromOriginalToolTip();
    }

    return false;
}


void
ToolTipActionShortcut::onShortcutChanged(const std::string& actionID,
                                      Key symbol,
                                      const KeyboardModifiers& modifiers)
{
    QKeySequence seq = makeKeySequence(QtEnumConvert::toQtModifiers(modifiers), QtEnumConvert::toQtKey(symbol));
    for (std::size_t i = 0; i < _shortcuts.size(); ++i) {
        if (_shortcuts[i].first.toStdString() == actionID) {
            _shortcuts[i].second = seq;
        }
    }
  
    setToolTipFromOriginalToolTip();
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_ActionShortcuts.cpp"
