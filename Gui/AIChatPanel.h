/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef NATRON_GUI_AICHATPANEL_H
#define NATRON_GUI_AICHATPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <memory>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QUrl>
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/QtCompat.h"

#include "Gui/AIConnectionSettings.h"
#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct AIChatPanelPrivate;

/**
 * @brief Dockable "AI Assistant" panel: a native Qt chat driving an external
 * agent CLI or HTTP tool-agent, which in turn drives Natron through AIMcpServer.
 **/
class AIChatPanel
    : public QWidget, public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    AIChatPanel(Gui* gui);

    virtual ~AIChatPanel();

    /// Starts the MCP server; starts the agent only when a saved connection exists.
    void ensureStarted();

    virtual void onPanelMadeCurrent() OVERRIDE FINAL;

public Q_SLOTS:

    void onSendClicked();

    void onStopClicked();

    void onConnectClicked();

    void onAutoConnectToggled(bool checked);

    void onModelComboChanged(const QString& model);

    void onModelEditingFinished();

    void onTranscriptAnchorClicked(const QUrl& url);

    void onProviderComboChanged(int index);

    void onBackendTextChunk(const QString& text);

    void onBackendToolCall(const QString& name,
                           const QString& argsJson);

    void onBackendToolResult(const QString& name,
                             bool ok);

    void onBackendTurnFinished();

    void onBackendError(const QString& message);

    void onBackendFinished();

    void onDestructiveToolRequested(const QString& toolName,
                                    const QString& summary,
                                    bool* allowed);

private:

    void applyConnection(const AIConnectionConfig& config,
                         bool startBackend);

    /// Start MCP + auto-connect when the preference allows it.
    bool tryAutoConnect();

    void refreshModelCombo();

    void applySelectedModel(bool reconnectIfNeeded);

    void connectBackendSignals();

    void updateProviderFooter();

    QString projectCwd() const;

    virtual bool eventFilter(QObject* watched,
                             QEvent* event) OVERRIDE FINAL;

    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QtCompat::QEnterEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;

    virtual QUndoStack* getUndoStack() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    std::unique_ptr<AIChatPanelPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_AICHATPANEL_H
