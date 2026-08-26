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

#ifndef NATRON_GUI_AIAGENTBACKEND_H
#define NATRON_GUI_AIAGENTBACKEND_H

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
#include <QStringList>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/AIConnectionSettings.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct AIAgentBackendPrivate;
struct ClaudeCodeBackendPrivate;
struct CodexCliBackendPrivate;
struct GeminiCliBackendPrivate;
struct AntigravityCliBackendPrivate;

/**
 * @brief Drives an external agent CLI or an HTTP tool-agent and normalizes its
 * output into Natron-level events.
 *
 * For CLI backends, authentication preferably stays with the CLI. API keys may
 * be injected into the child environment when the user chooses that method;
 * Natron never logs or serializes those keys into the project.
 */
class AIAgentBackend
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit AIAgentBackend(QObject* parent = 0);

    virtual ~AIAgentBackend();

    /// Human-readable backend name, shown in the panel ("Claude Code", ...).
    virtual QString displayName() const = 0;

    virtual QString providerId() const = 0;

    virtual QString connectionMethodLabel() const = 0;

    /**
     * @brief Locates the CLI binary. Empty when it is not installed or when the
     * backend does not use a CLI (HTTP agent).
     **/
    virtual QString findExecutable() const = 0;

    /**
     * @brief Starts the child process or marks the HTTP agent ready.
     * @param cwd Working directory -- the project folder, so the agent's own
     * file tools see the artist's comp.
     * @param mcpUrl Loopback URL of Natron's AIMcpServer.
     * @param token Bearer token for that server.
     **/
    virtual bool start(const QString& cwd,
                       const QString& mcpUrl,
                       const QString& token) = 0;

    /// Sends one user turn.
    virtual void send(const QString& text) = 0;

    /// Asks the agent to stop the current turn without killing the process.
    virtual void interrupt() = 0;

    /// Terminates the child: interrupt, then terminate, then kill.
    virtual void stop() = 0;

    virtual bool isRunning() const = 0;

    /**
     * @brief Builds the backend matching @p config (CLI or HTTP).
     * Ownership is transferred to @p parent.
     **/
    static AIAgentBackend* create(const AIConnectionConfig& config,
                                  QObject* parent);

Q_SIGNALS:

    /// A fragment of assistant text. Chunks arrive progressively.
    void textChunk(const QString& text);

    /// The agent invoked a tool. argsJson is compact JSON.
    void toolCall(const QString& name,
                  const QString& argsJson);

    void toolResult(const QString& name,
                    bool ok);

    /// The agent finished a turn and is ready for the next message.
    void turnFinished();

    void errorOccurred(const QString& message);

    /// The child process exited / HTTP agent stopped.
    void finished();

protected:

    std::unique_ptr<AIAgentBackendPrivate> _imp;
};

/**
 * @brief AIAgentBackend for the "claude" CLI (Claude Code).
 **/
class ClaudeCodeBackend
    : public AIAgentBackend
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit ClaudeCodeBackend(QObject* parent = 0);

    virtual ~ClaudeCodeBackend();

    void configure(const AIConnectionConfig& config);

    virtual QString displayName() const OVERRIDE FINAL;
    virtual QString providerId() const OVERRIDE FINAL;
    virtual QString connectionMethodLabel() const OVERRIDE FINAL;
    virtual QString findExecutable() const OVERRIDE FINAL;
    virtual bool start(const QString& cwd,
                       const QString& mcpUrl,
                       const QString& token) OVERRIDE FINAL;
    virtual void send(const QString& text) OVERRIDE FINAL;
    virtual void interrupt() OVERRIDE FINAL;
    virtual void stop() OVERRIDE FINAL;
    virtual bool isRunning() const OVERRIDE FINAL;

private Q_SLOTS:

    void onReadyReadStandardOutput();

    void onReadyReadStandardError();

    void onProcessFinished(int exitCode);

    void onProcessError();

private:

    std::unique_ptr<ClaudeCodeBackendPrivate> _claudeImp;
};

/**
 * @brief AIAgentBackend for the "codex" CLI (`codex exec --json`).
 **/
class CodexCliBackend
    : public AIAgentBackend
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit CodexCliBackend(QObject* parent = 0);

    virtual ~CodexCliBackend();

    void configure(const AIConnectionConfig& config);

    virtual QString displayName() const OVERRIDE FINAL;
    virtual QString providerId() const OVERRIDE FINAL;
    virtual QString connectionMethodLabel() const OVERRIDE FINAL;
    virtual QString findExecutable() const OVERRIDE FINAL;
    virtual bool start(const QString& cwd,
                       const QString& mcpUrl,
                       const QString& token) OVERRIDE FINAL;
    virtual void send(const QString& text) OVERRIDE FINAL;
    virtual void interrupt() OVERRIDE FINAL;
    virtual void stop() OVERRIDE FINAL;
    virtual bool isRunning() const OVERRIDE FINAL;

private Q_SLOTS:

    void onReadyReadStandardOutput();

    void onReadyReadStandardError();

    void onProcessFinished(int exitCode);

    void onProcessError();

private:

    std::unique_ptr<CodexCliBackendPrivate> _codexImp;
};

/**
 * @brief AIAgentBackend for the "gemini" CLI.
 **/
class GeminiCliBackend
    : public AIAgentBackend
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit GeminiCliBackend(QObject* parent = 0);

    virtual ~GeminiCliBackend();

    void configure(const AIConnectionConfig& config);

    virtual QString displayName() const OVERRIDE FINAL;
    virtual QString providerId() const OVERRIDE FINAL;
    virtual QString connectionMethodLabel() const OVERRIDE FINAL;
    virtual QString findExecutable() const OVERRIDE FINAL;
    virtual bool start(const QString& cwd,
                       const QString& mcpUrl,
                       const QString& token) OVERRIDE FINAL;
    virtual void send(const QString& text) OVERRIDE FINAL;
    virtual void interrupt() OVERRIDE FINAL;
    virtual void stop() OVERRIDE FINAL;
    virtual bool isRunning() const OVERRIDE FINAL;

private Q_SLOTS:

    void onReadyReadStandardOutput();

    void onReadyReadStandardError();

    void onProcessFinished(int exitCode);

    void onProcessError();

private:

    std::unique_ptr<GeminiCliBackendPrivate> _geminiImp;
};

/**
 * @brief AIAgentBackend for Google Antigravity CLI (`agy`).
 *
 * Auth modes: Google account (CLI subscription / cached OAuth) or Gemini API
 * key via modelProvider=gemini + GEMINI_API_KEY. MCP is wired through a
 * workspace `.agents/mcp_config.json` pointing at Natron's loopback server.
 **/
class AntigravityCliBackend
    : public AIAgentBackend
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit AntigravityCliBackend(QObject* parent = 0);

    virtual ~AntigravityCliBackend();

    void configure(const AIConnectionConfig& config);

    virtual QString displayName() const OVERRIDE FINAL;
    virtual QString providerId() const OVERRIDE FINAL;
    virtual QString connectionMethodLabel() const OVERRIDE FINAL;
    virtual QString findExecutable() const OVERRIDE FINAL;
    virtual bool start(const QString& cwd,
                       const QString& mcpUrl,
                       const QString& token) OVERRIDE FINAL;
    virtual void send(const QString& text) OVERRIDE FINAL;
    virtual void interrupt() OVERRIDE FINAL;
    virtual void stop() OVERRIDE FINAL;
    virtual bool isRunning() const OVERRIDE FINAL;

private Q_SLOTS:

    void onReadyReadStandardOutput();

    void onReadyReadStandardError();

    void onProcessFinished(int exitCode);

    void onProcessError();

private:

    std::unique_ptr<AntigravityCliBackendPrivate> _agyImp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_AIAGENTBACKEND_H
