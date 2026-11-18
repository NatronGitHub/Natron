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

#ifndef NATRON_GUI_HTTPTOOLAGENTBACKEND_H
#define NATRON_GUI_HTTPTOOLAGENTBACKEND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <memory>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QString>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/AIAgentBackend.h"
#include "Gui/AIConnectionSettings.h"

NATRON_NAMESPACE_ENTER

struct HttpToolAgentBackendPrivate;

/**
 * @brief In-process agent loop: talk to an LLM HTTP API and call Natron MCP tools.
 *
 * Covers Anthropic Messages, OpenAI-compatible chat completions (ChatGPT, Codex
 * API, Ollama, Custom), and Gemini generateContent.
 **/
class HttpToolAgentBackend
    : public AIAgentBackend
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit HttpToolAgentBackend(QObject* parent = 0);

    virtual ~HttpToolAgentBackend();

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

    void onNetworkFinished();

private:

    std::unique_ptr<HttpToolAgentBackendPrivate> _httpImp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_HTTPTOOLAGENTBACKEND_H
