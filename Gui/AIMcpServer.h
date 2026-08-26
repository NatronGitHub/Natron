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

#ifndef NATRON_GUI_AIMCPSERVER_H
#define NATRON_GUI_AIMCPSERVER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <memory>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QObject>
#include <QString>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct AIMcpServerPrivate;

/**
 * @brief A minimal Model Context Protocol (MCP) server exposing the *running*
 * Natron instance to an external agent process.
 *
 * Transport is JSON-RPC 2.0 over HTTP, bound strictly to the loopback interface
 * on an ephemeral port. Every request must carry "Authorization: Bearer <token>"
 * with the token generated at start(); without it the connection is dropped.
 *
 * Threading: the node graph is GUI-thread affine (Engine/Node.cpp carries ~28
 * assert(QThread::currentThread() == qApp->thread()) sites, all compiled out in
 * release builds, so a violation corrupts silently rather than crashing).
 * Every tool that touches nodes or knobs is therefore marshalled onto the GUI
 * thread before it runs -- see AIMcpServerPrivate::runOnGuiThread().
 */
class AIMcpServer
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit AIMcpServer(Gui* gui,
                         QObject* parent = 0);

    virtual ~AIMcpServer();

    /**
     * @brief Binds to 127.0.0.1 on an ephemeral port and generates a fresh
     * session token. Returns false if the socket could not be bound.
     **/
    bool start();

    /**
     * @brief Closes the listening socket, drops every client and invalidates the
     * token. Safe to call when not started.
     **/
    void stop();

    bool isRunning() const;

    /// Port chosen by the system, or 0 when not running.
    quint16 port() const;

    /// Bearer token for this session, regenerated on every start().
    QString token() const;

    /// Convenience: "http://127.0.0.1:<port>/mcp"
    QString url() const;

    /**
     * @brief Opens an undo macro on the node graph's stack, so that every
     * mutation the agent makes until the matching endAgentTransaction() collapses
     * into a single Ctrl+Z entry labelled "AI: <label>".
     *
     * Re-entrant: nested calls are counted and only the outermost pair actually
     * opens and closes the macro.
     *
     * Prefer AIUndoTransaction over calling this directly -- an unmatched
     * beginMacro leaves the stack wedged open for the rest of the session.
     **/
    void beginAgentTransaction(const QString& label);

    /// Closes the macro opened by beginAgentTransaction(). Safe if none is open.
    void endAgentTransaction();

    /// True while an agent transaction is open.
    bool isInAgentTransaction() const;

Q_SIGNALS:

    /**
     * @brief Emitted just before a tool marked destructive is executed. A slot
     * connected with Qt::DirectConnection may set *allowed to false to veto it.
     * This is how approval dialogs are raised inside Natron rather than being
     * delegated to the agent CLI, which has no callback mechanism for it.
     **/
    void destructiveToolRequested(const QString& toolName,
                                  const QString& humanReadableSummary,
                                  bool* allowed);

    /// Emitted after every tool call, for the chat panel's activity log.
    void toolCallFinished(const QString& toolName,
                          bool ok);

private Q_SLOTS:

    void onNewConnection();

    void onSocketReadyRead();

    void onSocketDisconnected();

private:

    /**
     * @brief Executes one JSON-RPC request and returns the serialized response.
     *
     * Q_INVOKABLE so it can be reached with Qt::BlockingQueuedConnection from a
     * non-GUI thread. Never call it directly from one -- go through
     * AIMcpServerPrivate::dispatch(), which picks direct invocation when already
     * on the GUI thread (a BlockingQueuedConnection onto one's own thread
     * deadlocks).
     **/
    Q_INVOKABLE QString dispatchOnGuiThread(const QString& requestJson);

    std::unique_ptr<AIMcpServerPrivate> _imp;

    friend struct AIMcpServerPrivate;
};

/**
 * @brief RAII guard around AIMcpServer::beginAgentTransaction().
 *
 * This is not a convenience: QUndoStack::beginMacro() without a matching
 * endMacro() leaves the stack open permanently -- Undo and Redo stay disabled and
 * every later command is swallowed into the orphaned macro. An early return or a
 * thrown ToolError between the two would do exactly that, so the close must
 * happen in a destructor.
 */
class AIUndoTransaction
{
public:

    AIUndoTransaction(AIMcpServer* server,
                      const QString& label)
        : _server(server)
    {
        if (_server) {
            _server->beginAgentTransaction(label);
        }
    }

    ~AIUndoTransaction()
    {
        if (_server) {
            _server->endAgentTransaction();
        }
    }

private:

    AIUndoTransaction(const AIUndoTransaction&) = delete;
    AIUndoTransaction& operator=(const AIUndoTransaction&) = delete;

    AIMcpServer* _server;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_AIMCPSERVER_H
