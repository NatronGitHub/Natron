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

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTemporaryDir>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

NATRON_NAMESPACE_ENTER

#define AI_AGENT_TERMINATE_TIMEOUT_MS 3000

struct AntigravityCliBackendPrivate
{
    AntigravityCliBackend* _publicInterface;
    QProcess* process;
    QByteArray stdoutBuffer;
    QString currentToolName;
    AIConnectionConfig config;
    QString cwd;
    QTemporaryDir* tempDir;
    QString agentsMcpPath;
    QString savedAgentsMcpBackup;
    bool wroteAgentsMcp;
    QString apiKeyHome;

    AntigravityCliBackendPrivate(AntigravityCliBackend* publicInterface)
        : _publicInterface(publicInterface)
        , process(0)
        , stdoutBuffer()
        , currentToolName()
        , config()
        , cwd()
        , tempDir(0)
        , agentsMcpPath()
        , savedAgentsMcpBackup()
        , wroteAgentsMcp(false)
        , apiKeyHome()
    {
    }

    bool writeWorkspaceMcpConfig(const QString& mcpUrl,
                                 const QString& token);
    bool writeApiKeySettings();
    void restoreWorkspaceMcpConfig();
    void handleEventLine(const QByteArray& line);
};

bool
AntigravityCliBackendPrivate::writeWorkspaceMcpConfig(const QString& mcpUrl,
                                                       const QString& token)
{
    // Antigravity reads workspace MCP from <cwd>/.agents/mcp_config.json.
    QDir agentsDir(cwd);
    if ( !agentsDir.exists( QString::fromUtf8(".agents") ) ) {
        if ( !agentsDir.mkdir( QString::fromUtf8(".agents") ) ) {
            return false;
        }
    }

    agentsMcpPath = cwd + QString::fromUtf8("/.agents/mcp_config.json");
    QFile existing(agentsMcpPath);
    if ( existing.exists() && existing.open(QIODevice::ReadOnly) ) {
        savedAgentsMcpBackup = QString::fromUtf8( existing.readAll() );
        existing.close();
    } else {
        savedAgentsMcpBackup.clear();
    }

    QJsonObject natronServer;
    natronServer[QString::fromUtf8("serverUrl")] = mcpUrl;
    {
        QJsonObject headers;
        headers[QString::fromUtf8("Authorization")] = QString::fromUtf8("Bearer ") + token;
        natronServer[QString::fromUtf8("headers")] = headers;
    }

    QJsonObject servers;
    servers[QString::fromUtf8("natron")] = natronServer;
    QJsonObject root;
    root[QString::fromUtf8("mcpServers")] = servers;

    QFile out(agentsMcpPath);
    if ( !out.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
        return false;
    }
    out.write( QJsonDocument(root).toJson(QJsonDocument::Compact) );
    out.close();
    wroteAgentsMcp = true;

    return true;
}

bool
AntigravityCliBackendPrivate::writeApiKeySettings()
{
    if (!tempDir) {
        return false;
    }

    const QString home = tempDir->path();
    QDir dir(home);
    if ( !dir.mkpath( QString::fromUtf8(".gemini/antigravity-cli") ) ) {
        return false;
    }

    QJsonObject settings;
    settings[QString::fromUtf8("modelProvider")] = QString::fromUtf8("gemini");

    const QString path = home + QString::fromUtf8("/.gemini/antigravity-cli/settings.json");
    QFile file(path);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
        return false;
    }
    file.write( QJsonDocument(settings).toJson(QJsonDocument::Compact) );
    file.close();
    apiKeyHome = home;

    return true;
}

void
AntigravityCliBackendPrivate::restoreWorkspaceMcpConfig()
{
    if (!wroteAgentsMcp || agentsMcpPath.isEmpty()) {
        return;
    }

    if ( savedAgentsMcpBackup.isEmpty() ) {
        QFile::remove(agentsMcpPath);
        QDir cwdDir(cwd);
        cwdDir.rmdir( QString::fromUtf8(".agents") );
    } else {
        QFile out(agentsMcpPath);
        if ( out.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
            out.write( savedAgentsMcpBackup.toUtf8() );
            out.close();
        }
    }

    wroteAgentsMcp = false;
    agentsMcpPath.clear();
    savedAgentsMcpBackup.clear();
}

void
AntigravityCliBackendPrivate::handleEventLine(const QByteArray& line)
{
    // Mirror Claude's JSONL reader, but for Antigravity's event schema:
    //   {"event":"init"| "step_update"| "result", ...}
    const QByteArray trimmed = line.trimmed();

    if ( trimmed.isEmpty() ) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);
    if ( ( parseError.error != QJsonParseError::NoError ) || !doc.isObject() ) {
        return;
    }

    const QJsonObject root = doc.object();
    // Docs use "event"; tolerate "type" if a build diverges.
    QString kind = root[QString::fromUtf8("event")].toString();
    if ( kind.isEmpty() ) {
        kind = root[QString::fromUtf8("type")].toString();
    }

    if ( kind == QString::fromUtf8("step_update") ) {
        const QJsonObject step = root[QString::fromUtf8("step_update")].toObject();
        const QString stepType = step[QString::fromUtf8("step_type")].toString();
        const QString state = step[QString::fromUtf8("state")].toString();

        const QString textDelta = step[QString::fromUtf8("text_delta")].toString();
        if ( !textDelta.isEmpty() &&
             ( ( stepType == QString::fromUtf8("agent_response") ) ||
               stepType.contains( QString::fromUtf8("response") ) ||
               stepType.contains( QString::fromUtf8("message") ) ) ) {
            Q_EMIT _publicInterface->textChunk(textDelta);
        }

        // Tool / MCP activity (names vary by agy version).
        if ( stepType.contains( QString::fromUtf8("tool") ) ||
             stepType.contains( QString::fromUtf8("mcp") ) ) {
            QString name = step[QString::fromUtf8("tool_name")].toString();
            if ( name.isEmpty() ) {
                name = step[QString::fromUtf8("name")].toString();
            }
            if ( name.isEmpty() ) {
                name = stepType;
            }
            if ( state == QString::fromUtf8("ACTIVE") ||
                 state == QString::fromUtf8("STARTED") ||
                 state == QString::fromUtf8("RUNNING") ) {
                currentToolName = name;
                Q_EMIT _publicInterface->toolCall(
                    name,
                    QString::fromUtf8( QJsonDocument(step).toJson(QJsonDocument::Compact) ) );
            } else if ( state == QString::fromUtf8("DONE") ||
                        state == QString::fromUtf8("COMPLETED") ||
                        state == QString::fromUtf8("ERROR") ) {
                const bool ok = ( state != QString::fromUtf8("ERROR") );
                Q_EMIT _publicInterface->toolResult( currentToolName.isEmpty() ? name : currentToolName, ok );
            }
        }

        return;
    }

    if ( kind == QString::fromUtf8("result") ) {
        const QJsonObject result = root[QString::fromUtf8("result")].toObject();
        const QString status = result[QString::fromUtf8("status")].toString();
        if ( status == QString::fromUtf8("ERROR") ||
             status.contains( QString::fromUtf8("FAIL"), Qt::CaseInsensitive ) ) {
            QString err = result[QString::fromUtf8("error")].toString();
            if ( err.isEmpty() ) {
                err = AntigravityCliBackend::tr("Antigravity reported an error.");
            }
            Q_EMIT _publicInterface->errorOccurred(err);
        } else {
            // If we missed streaming deltas, surface the final turn response once.
            const QString response = result[QString::fromUtf8("response")].toString();
            if ( !response.isEmpty() ) {
                // Prefer streamed text_delta; only emit full response when nothing
                // was streamed for this turn (empty currentToolName is a weak signal,
                // so we skip dumping the whole response to avoid duplicates).
            }
        }
        Q_EMIT _publicInterface->turnFinished();

        return;
    }

    // init and unrecognized events: ignore (same tolerance as Claude's parser).
}

AntigravityCliBackend::AntigravityCliBackend(QObject* parent)
    : AIAgentBackend(parent)
    , _agyImp( new AntigravityCliBackendPrivate(this) )
{
}

AntigravityCliBackend::~AntigravityCliBackend()
{
    stop();
}

void
AntigravityCliBackend::configure(const AIConnectionConfig& config)
{
    _agyImp->config = config;
}

QString
AntigravityCliBackend::displayName() const
{
    return QString::fromUtf8("Antigravity");
}

QString
AntigravityCliBackend::providerId() const
{
    return QString::fromUtf8("antigravity");
}

QString
AntigravityCliBackend::connectionMethodLabel() const
{
    if ( !_agyImp->config.apiKey.isEmpty() ) {
        return QString::fromUtf8("CLI + API key");
    }

    return AIConnectionSettings::methodLabel(eAIConnectionMethodCli);
}

QString
AntigravityCliBackend::findExecutable() const
{
    if ( !_agyImp->config.cliPath.isEmpty() ) {
        QFileInfo info(_agyImp->config.cliPath);
        if ( info.exists() && info.isExecutable() ) {
            return info.absoluteFilePath();
        }
    }

    QString found = AIProviderRegistry::findCliExecutable( QString::fromUtf8("agy") );
    if ( found.isEmpty() ) {
        found = AIProviderRegistry::findCliExecutable( QString::fromUtf8("antigravity") );
    }

    return found;
}

bool
AntigravityCliBackend::start(const QString& cwd,
                              const QString& mcpUrl,
                              const QString& token)
{
    // Same lifecycle as ClaudeCodeBackend: one long-lived process, prompts on stdin.
    if ( isRunning() ) {
        return true;
    }

    const QString exe = findExecutable();
    if ( exe.isEmpty() ) {
        Q_EMIT errorOccurred( tr("Antigravity CLI (agy) not found. Install it, run 'agy' once to sign in, or use a Gemini API key.") );

        return false;
    }

    _agyImp->cwd = cwd.isEmpty() ? QDir::homePath() : cwd;

    delete _agyImp->tempDir;
    _agyImp->tempDir = new QTemporaryDir();
    _agyImp->tempDir->setAutoRemove(true);
    if ( !_agyImp->tempDir->isValid() ) {
        Q_EMIT errorOccurred( tr("Could not create a temporary Antigravity settings directory.") );

        return false;
    }

    if ( !_agyImp->writeWorkspaceMcpConfig(mcpUrl, token) ) {
        Q_EMIT errorOccurred( tr("Could not write .agents/mcp_config.json for Antigravity MCP.") );

        return false;
    }

    QStringList args;
    // Continuous session like Claude: stream-json in and out (no per-turn -p).
    args << QString::fromUtf8("--input-format") << QString::fromUtf8("stream-json");
    args << QString::fromUtf8("--output-format") << QString::fromUtf8("stream-json");
    args << QString::fromUtf8("--dangerously-skip-permissions");
    if ( !_agyImp->config.model.isEmpty() &&
         ( _agyImp->config.model != QString::fromUtf8("sonnet") ) ) {
        args << QString::fromUtf8("--model") << _agyImp->config.model;
    }

    _agyImp->process = new QProcess(this);
    _agyImp->process->setProgram(exe);
    _agyImp->process->setArguments(args);
    _agyImp->process->setWorkingDirectory(_agyImp->cwd);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if ( !_agyImp->config.apiKey.isEmpty() ) {
        if ( _agyImp->writeApiKeySettings() ) {
            env.insert( QString::fromUtf8("HOME"), _agyImp->apiKeyHome );
#ifdef __NATRON_WIN32__
            env.insert( QString::fromUtf8("USERPROFILE"), _agyImp->apiKeyHome );
#endif
        }
        env.insert( QString::fromUtf8("GEMINI_API_KEY"), _agyImp->config.apiKey );
    }
    _agyImp->process->setProcessEnvironment(env);

    connect( _agyImp->process, SIGNAL( readyReadStandardOutput() ),
             this, SLOT( onReadyReadStandardOutput() ) );
    connect( _agyImp->process, SIGNAL( readyReadStandardError() ),
             this, SLOT( onReadyReadStandardError() ) );
    connect( _agyImp->process, SIGNAL( finished(int, QProcess::ExitStatus) ),
             this, SLOT( onProcessFinished(int) ) );
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    connect( _agyImp->process, SIGNAL( errorOccurred(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#else
    connect( _agyImp->process, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#endif

    _agyImp->process->start();
    if ( !_agyImp->process->waitForStarted(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
        Q_EMIT errorOccurred( tr("Could not start '%1'.").arg(exe) );
        delete _agyImp->process;
        _agyImp->process = 0;
        _agyImp->restoreWorkspaceMcpConfig();

        return false;
    }

    return true;
}

void
AntigravityCliBackend::send(const QString& text)
{
    if ( !isRunning() ) {
        Q_EMIT errorOccurred( tr("Antigravity is not connected.") );

        return;
    }

    // Antigravity stream-json input (docs):
    //   {"event":"user","message":{"content":"..."}}
    QJsonObject message;
    message[QString::fromUtf8("content")] = text;

    QJsonObject event;
    event[QString::fromUtf8("event")] = QString::fromUtf8("user");
    event[QString::fromUtf8("message")] = message;

    QByteArray line = QJsonDocument(event).toJson(QJsonDocument::Compact);
    line += "\n";
    _agyImp->process->write(line);
}

void
AntigravityCliBackend::interrupt()
{
    if ( !isRunning() ) {
        return;
    }

    // Same idea as Claude: close stdin to end the turn without killing the session.
    _agyImp->process->closeWriteChannel();
}

void
AntigravityCliBackend::stop()
{
    if (_agyImp->process) {
        QProcess* process = _agyImp->process;
        process->disconnect(this);
        if ( process->state() != QProcess::NotRunning ) {
            process->closeWriteChannel();
            if ( !process->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
                process->terminate();
                if ( !process->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
                    process->kill();
                    process->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS);
                }
            }
        }
        _agyImp->process = 0;
        _agyImp->stdoutBuffer.clear();
        process->deleteLater();
    }

    _agyImp->restoreWorkspaceMcpConfig();
    delete _agyImp->tempDir;
    _agyImp->tempDir = 0;
    _agyImp->apiKeyHome.clear();
    Q_EMIT finished();
}

bool
AntigravityCliBackend::isRunning() const
{
    return _agyImp->process && ( _agyImp->process->state() == QProcess::Running );
}

void
AntigravityCliBackend::onReadyReadStandardOutput()
{
    if (!_agyImp->process) {
        return;
    }

    _agyImp->stdoutBuffer += _agyImp->process->readAllStandardOutput();

    int newline = _agyImp->stdoutBuffer.indexOf('\n');
    while (newline >= 0) {
        const QByteArray line = _agyImp->stdoutBuffer.left(newline);
        _agyImp->stdoutBuffer.remove(0, newline + 1);
        _agyImp->handleEventLine(line);
        newline = _agyImp->stdoutBuffer.indexOf('\n');
    }
}

void
AntigravityCliBackend::onReadyReadStandardError()
{
    if (!_agyImp->process) {
        return;
    }

    const QByteArray err = _agyImp->process->readAllStandardError();
    const QString text = QString::fromUtf8( err.trimmed() );
    if ( text.isEmpty() ) {
        return;
    }
    if ( text.contains( QString::fromUtf8("authentication required"), Qt::CaseInsensitive ) ) {
        Q_EMIT errorOccurred( tr("Antigravity needs sign-in. Run 'agy' once in a terminal, or paste a Gemini API key and reconnect.") );
    } else if ( text.contains( QString::fromUtf8("error"), Qt::CaseInsensitive ) ||
                text.contains( QString::fromUtf8("failed"), Qt::CaseInsensitive ) ) {
        Q_EMIT errorOccurred(text);
    }
}

void
AntigravityCliBackend::onProcessFinished(int exitCode)
{
    Q_UNUSED(exitCode);

    _agyImp->stdoutBuffer.clear();
    Q_EMIT finished();
}

void
AntigravityCliBackend::onProcessError()
{
    if (!_agyImp->process) {
        return;
    }

    Q_EMIT errorOccurred( _agyImp->process->errorString() );
}

NATRON_NAMESPACE_EXIT
