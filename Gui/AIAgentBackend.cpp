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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AIAgentBackend.h"

#include "Gui/AIProviderRegistry.h"
#include "Gui/HttpToolAgentBackend.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStandardPaths>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

NATRON_NAMESPACE_ENTER

// How long to wait at each escalation step when shutting the child down.
#define AI_AGENT_TERMINATE_TIMEOUT_MS 3000

struct AIAgentBackendPrivate
{
    AIAgentBackendPrivate() {}
};

AIAgentBackend::AIAgentBackend(QObject* parent)
    : QObject(parent)
    , _imp( new AIAgentBackendPrivate() )
{
}

AIAgentBackend::~AIAgentBackend()
{
}

AIAgentBackend*
AIAgentBackend::create(const AIConnectionConfig& config,
                       QObject* parent)
{
    const QString id = config.providerId;

    if ( ( config.method == eAIConnectionMethodCli ) && ( id == QString::fromUtf8("claude") ) ) {
        ClaudeCodeBackend* backend = new ClaudeCodeBackend(parent);
        backend->configure(config);

        return backend;
    }
    if ( ( config.method == eAIConnectionMethodCli ) && ( id == QString::fromUtf8("codex") ) ) {
        CodexCliBackend* backend = new CodexCliBackend(parent);
        backend->configure(config);

        return backend;
    }
    if ( ( config.method == eAIConnectionMethodCli ) && ( id == QString::fromUtf8("gemini") ) ) {
        GeminiCliBackend* backend = new GeminiCliBackend(parent);
        backend->configure(config);

        return backend;
    }
    if ( ( config.method == eAIConnectionMethodCli ) && ( id == QString::fromUtf8("antigravity") ) ) {
        AntigravityCliBackend* backend = new AntigravityCliBackend(parent);
        backend->configure(config);

        return backend;
    }

    // API key, custom endpoint, Ollama, ChatGPT, or CLI providers falling back to HTTP.
    if ( ( config.method == eAIConnectionMethodApiKey ) ||
         ( config.method == eAIConnectionMethodCustom ) ||
         ( id == QString::fromUtf8("chatgpt") ) ||
         ( id == QString::fromUtf8("ollama") ) ||
         ( id == QString::fromUtf8("custom") ) ||
         ( ( config.method != eAIConnectionMethodCli ) &&
           ( ( id == QString::fromUtf8("claude") ) ||
             ( id == QString::fromUtf8("codex") ) ||
             ( id == QString::fromUtf8("gemini") ) ||
             ( id == QString::fromUtf8("antigravity") ) ) ) ) {
        HttpToolAgentBackend* backend = new HttpToolAgentBackend(parent);
        backend->configure(config);

        return backend;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// ClaudeCodeBackend
// ---------------------------------------------------------------------------

struct ClaudeCodeBackendPrivate
{
    ClaudeCodeBackend* _publicInterface;
    QProcess* process;
    QByteArray stdoutBuffer;
    QString currentToolName;
    AIConnectionConfig config;

    ClaudeCodeBackendPrivate(ClaudeCodeBackend* publicInterface)
        : _publicInterface(publicInterface)
        , process(0)
        , stdoutBuffer()
        , currentToolName()
        , config()
    {
    }

    /// Parses one complete JSONL line from the CLI's stdout.
    void handleEventLine(const QByteArray& line);

    void handleAssistantMessage(const QJsonObject& message);
};

void
ClaudeCodeBackendPrivate::handleAssistantMessage(const QJsonObject& message)
{
    const QJsonArray content = message[QString::fromUtf8("content")].toArray();

    for (int i = 0; i < content.size(); ++i) {
        const QJsonObject block = content.at(i).toObject();
        const QString blockType = block[QString::fromUtf8("type")].toString();

        if ( blockType == QString::fromUtf8("text") ) {
            const QString text = block[QString::fromUtf8("text")].toString();
            if ( !text.isEmpty() ) {
                Q_EMIT _publicInterface->textChunk(text);
            }
        } else if ( blockType == QString::fromUtf8("tool_use") ) {
            const QString name = block[QString::fromUtf8("name")].toString();
            const QJsonObject input = block[QString::fromUtf8("input")].toObject();
            currentToolName = name;
            Q_EMIT _publicInterface->toolCall( name,
                                               QString::fromUtf8( QJsonDocument(input).toJson(QJsonDocument::Compact) ) );
        }
        // Any other block type (thinking, image, ...) is ignored on purpose:
        // the parser must tolerate additions to the event schema rather than
        // break when the CLI is updated.
    }
}

void
ClaudeCodeBackendPrivate::handleEventLine(const QByteArray& line)
{
    const QByteArray trimmed = line.trimmed();

    if ( trimmed.isEmpty() ) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);

    if ( ( parseError.error != QJsonParseError::NoError ) || !doc.isObject() ) {
        // Not JSON: the CLI occasionally prints diagnostics on stdout. Ignore
        // rather than surfacing noise as an error.
        return;
    }

    const QJsonObject event = doc.object();
    const QString type = event[QString::fromUtf8("type")].toString();

    if ( type == QString::fromUtf8("assistant") ) {
        handleAssistantMessage( event[QString::fromUtf8("message")].toObject() );

        return;
    }

    if ( type == QString::fromUtf8("stream_event") ) {
        // --include-partial-messages: incremental text deltas, so the chat
        // bubble fills in progressively instead of appearing all at once.
        const QJsonObject sub = event[QString::fromUtf8("event")].toObject();
        if ( sub[QString::fromUtf8("type")].toString() == QString::fromUtf8("content_block_delta") ) {
            const QJsonObject delta = sub[QString::fromUtf8("delta")].toObject();
            if ( delta[QString::fromUtf8("type")].toString() == QString::fromUtf8("text_delta") ) {
                const QString text = delta[QString::fromUtf8("text")].toString();
                if ( !text.isEmpty() ) {
                    Q_EMIT _publicInterface->textChunk(text);
                }
            }
        }

        return;
    }

    if ( type == QString::fromUtf8("user") ) {
        // Tool results come back as user-role tool_result blocks.
        const QJsonObject message = event[QString::fromUtf8("message")].toObject();
        const QJsonArray content = message[QString::fromUtf8("content")].toArray();
        for (int i = 0; i < content.size(); ++i) {
            const QJsonObject block = content.at(i).toObject();
            if ( block[QString::fromUtf8("type")].toString() == QString::fromUtf8("tool_result") ) {
                const bool isError = block[QString::fromUtf8("is_error")].toBool(false);
                Q_EMIT _publicInterface->toolResult(currentToolName, !isError);
            }
        }

        return;
    }

    if ( type == QString::fromUtf8("result") ) {
        const bool isError = event[QString::fromUtf8("is_error")].toBool(false);
        if (isError) {
            QString msg = event[QString::fromUtf8("result")].toString();
            if ( msg.isEmpty() ) {
                msg = ClaudeCodeBackend::tr("The agent reported an error.");
            }
            Q_EMIT _publicInterface->errorOccurred(msg);
        }
        Q_EMIT _publicInterface->turnFinished();

        return;
    }

    // "system" (init//compact notices) and anything unrecognized: ignored.
}

ClaudeCodeBackend::ClaudeCodeBackend(QObject* parent)
    : AIAgentBackend(parent)
    , _claudeImp( new ClaudeCodeBackendPrivate(this) )
{
}

ClaudeCodeBackend::~ClaudeCodeBackend()
{
    stop();
}

void
ClaudeCodeBackend::configure(const AIConnectionConfig& config)
{
    _claudeImp->config = config;
}

QString
ClaudeCodeBackend::displayName() const
{
    return QString::fromUtf8("Claude");
}

QString
ClaudeCodeBackend::providerId() const
{
    return QString::fromUtf8("claude");
}

QString
ClaudeCodeBackend::connectionMethodLabel() const
{
    return AIConnectionSettings::methodLabel(eAIConnectionMethodCli);
}

QString
ClaudeCodeBackend::findExecutable() const
{
    if ( !_claudeImp->config.cliPath.isEmpty() ) {
        QFileInfo info(_claudeImp->config.cliPath);
        if ( info.exists() && info.isExecutable() ) {
            return info.absoluteFilePath();
        }
    }

    return AIProviderRegistry::findCliExecutable( QString::fromUtf8("claude") );
}

bool
ClaudeCodeBackend::start(const QString& cwd,
                         const QString& mcpUrl,
                         const QString& token)
{
    if ( isRunning() ) {
        return true;
    }

    const QString exe = findExecutable();
    if ( exe.isEmpty() ) {
        Q_EMIT errorOccurred( tr("Claude CLI not found. Install Claude Code, or click Connect… and use an API key.") );

        return false;
    }

    // The MCP server descriptor is passed as a JSON *string*, not a file:
    // --mcp-config accepts either, and passing it inline keeps the session token
    // off disk entirely, where it could otherwise outlive a crash.
    QJsonObject natronServer;
    natronServer[QString::fromUtf8("type")] = QString::fromUtf8("http");
    natronServer[QString::fromUtf8("url")] = mcpUrl;
    {
        QJsonObject headers;
        headers[QString::fromUtf8("Authorization")] = QString::fromUtf8("Bearer ") + token;
        natronServer[QString::fromUtf8("headers")] = headers;
    }

    QJsonObject servers;
    servers[QString::fromUtf8("natron")] = natronServer;

    QJsonObject mcpConfig;
    mcpConfig[QString::fromUtf8("mcpServers")] = servers;

    const QString mcpConfigJson =
        QString::fromUtf8( QJsonDocument(mcpConfig).toJson(QJsonDocument::Compact) );

    QStringList args;
    args << QString::fromUtf8("-p");
    args << QString::fromUtf8("--output-format") << QString::fromUtf8("stream-json");
    args << QString::fromUtf8("--input-format") << QString::fromUtf8("stream-json");
    args << QString::fromUtf8("--include-partial-messages");
    args << QString::fromUtf8("--verbose");
    args << QString::fromUtf8("--mcp-config") << mcpConfigJson;
    // Use only Natron's server: without this the agent would inherit whatever
    // MCP servers the user has configured globally, which may reach files,
    // network or a shell well outside this session.
    args << QString::fromUtf8("--strict-mcp-config");
    // The CLI must not be the gatekeeper here.
    //
    // It has no permission-callback mechanism (no --permission-prompt-tool in
    // 2.x), and in non-interactive -p mode there is no UI to approve anything,
    // so any tool left outside the allowlist is blocked *permanently* -- the
    // request never reaches Natron and our own confirmation dialog never gets
    // asked. Gating therefore lives entirely on the Natron side, in
    // AIMcpServer, where the dialog can show real node names.
    //
    // --strict-mcp-config above is what keeps this bounded: the agent sees our
    // server and nothing else the user may have configured globally.
    args << QString::fromUtf8("--permission-mode") << QString::fromUtf8("bypassPermissions");

    // Pin the model explicitly. Without --model, Claude Code falls back to the
    // account / settings default (or the "best" alias), which can resolve to
    // Fable -- expensive and not what we want for Natron comps. Sonnet is the
    // capable + cheap default; the Connect dialog can override via config.model.
    {
        QString model = _claudeImp->config.model.trimmed();
        if ( model.isEmpty() ||
             ( model == QString::fromUtf8("best") ) ||
             model.contains( QString::fromUtf8("fable"), Qt::CaseInsensitive ) ) {
            model = QString::fromUtf8("sonnet");
        }
        args << QString::fromUtf8("--model") << model;
    }

    _claudeImp->process = new QProcess(this);
    _claudeImp->process->setProgram(exe);
    _claudeImp->process->setArguments(args);
    if ( !cwd.isEmpty() ) {
        _claudeImp->process->setWorkingDirectory(cwd);
    }

    // Inherit the user's environment so the CLI finds its own OAuth credentials.
    // When the user pasted an API key for the CLI method, inject it for this
    // child only -- never log the value.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if ( !_claudeImp->config.apiKey.isEmpty() ) {
        env.insert( QString::fromUtf8("ANTHROPIC_API_KEY"), _claudeImp->config.apiKey );
    }
    _claudeImp->process->setProcessEnvironment(env);

    connect( _claudeImp->process, SIGNAL( readyReadStandardOutput() ),
             this, SLOT( onReadyReadStandardOutput() ) );
    connect( _claudeImp->process, SIGNAL( readyReadStandardError() ),
             this, SLOT( onReadyReadStandardError() ) );
    connect( _claudeImp->process, SIGNAL( finished(int, QProcess::ExitStatus) ),
             this, SLOT( onProcessFinished(int) ) );
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    connect( _claudeImp->process, SIGNAL( errorOccurred(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#else
    connect( _claudeImp->process, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#endif

    _claudeImp->process->start();

    if ( !_claudeImp->process->waitForStarted(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
        Q_EMIT errorOccurred( tr("Could not start '%1'.").arg(exe) );
        delete _claudeImp->process;
        _claudeImp->process = 0;

        return false;
    }

    return true;
}

void
ClaudeCodeBackend::send(const QString& text)
{
    if ( !isRunning() ) {
        Q_EMIT errorOccurred( tr("The agent is not running.") );

        return;
    }

    // stream-json input: one complete JSON object per line.
    QJsonObject textBlock;
    textBlock[QString::fromUtf8("type")] = QString::fromUtf8("text");
    textBlock[QString::fromUtf8("text")] = text;

    QJsonArray content;
    content.push_back(textBlock);

    QJsonObject message;
    message[QString::fromUtf8("role")] = QString::fromUtf8("user");
    message[QString::fromUtf8("content")] = content;

    QJsonObject event;
    event[QString::fromUtf8("type")] = QString::fromUtf8("user");
    event[QString::fromUtf8("message")] = message;

    QByteArray line = QJsonDocument(event).toJson(QJsonDocument::Compact);
    line += "\n";

    _claudeImp->process->write(line);
}

void
ClaudeCodeBackend::interrupt()
{
    if ( !isRunning() ) {
        return;
    }

    // Closing stdin ends the current turn without killing the process outright;
    // stop() escalates from here if the child does not settle.
    _claudeImp->process->closeWriteChannel();
}

void
ClaudeCodeBackend::stop()
{
    if (!_claudeImp->process) {
        return;
    }

    QProcess* process = _claudeImp->process;

    // Detach the slots first: the teardown below will emit finished/error and we
    // do not want to re-enter stop() through them.
    process->disconnect(this);

    if ( process->state() != QProcess::NotRunning ) {
        process->closeWriteChannel();
        if ( !process->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
            process->terminate();
            if ( !process->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
                // Last resort, so we never leave an orphan behind.
                process->kill();
                process->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS);
            }
        }
    }

    _claudeImp->process = 0;
    _claudeImp->stdoutBuffer.clear();
    process->deleteLater();

    Q_EMIT finished();
}

bool
ClaudeCodeBackend::isRunning() const
{
    return _claudeImp->process && ( _claudeImp->process->state() == QProcess::Running );
}

void
ClaudeCodeBackend::onReadyReadStandardOutput()
{
    if (!_claudeImp->process) {
        return;
    }

    _claudeImp->stdoutBuffer += _claudeImp->process->readAllStandardOutput();

    // Consume only whole lines; a partial trailing line stays buffered until the
    // rest arrives. Reading incrementally like this is what keeps the UI
    // responsive instead of blocking on the child.
    int newline = _claudeImp->stdoutBuffer.indexOf('\n');
    while (newline >= 0) {
        const QByteArray line = _claudeImp->stdoutBuffer.left(newline);
        _claudeImp->stdoutBuffer.remove(0, newline + 1);
        _claudeImp->handleEventLine(line);
        newline = _claudeImp->stdoutBuffer.indexOf('\n');
    }
}

void
ClaudeCodeBackend::onReadyReadStandardError()
{
    if (!_claudeImp->process) {
        return;
    }

    const QByteArray err = _claudeImp->process->readAllStandardError();
    if ( !err.trimmed().isEmpty() ) {
        Q_EMIT errorOccurred( QString::fromUtf8( err.trimmed() ) );
    }
}

void
ClaudeCodeBackend::onProcessFinished(int exitCode)
{
    Q_UNUSED(exitCode);

    _claudeImp->stdoutBuffer.clear();
    Q_EMIT finished();
}

void
ClaudeCodeBackend::onProcessError()
{
    if (!_claudeImp->process) {
        return;
    }

    Q_EMIT errorOccurred( _claudeImp->process->errorString() );
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AIAgentBackend.cpp"
