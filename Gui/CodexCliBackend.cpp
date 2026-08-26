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
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

NATRON_NAMESPACE_ENTER

#define AI_AGENT_TERMINATE_TIMEOUT_MS 3000

struct CodexCliBackendPrivate
{
    CodexCliBackend* _publicInterface;
    QProcess* process;
    QByteArray stdoutBuffer;
    QString currentToolName;
    AIConnectionConfig config;
    QString mcpUrl;
    QString mcpToken;
    QString cwd;
    QTemporaryDir* tempDir;
    bool sessionReady;
    bool turnRunning;

    CodexCliBackendPrivate(CodexCliBackend* publicInterface)
        : _publicInterface(publicInterface)
        , process(0)
        , stdoutBuffer()
        , currentToolName()
        , config()
        , mcpUrl()
        , mcpToken()
        , cwd()
        , tempDir(0)
        , sessionReady(false)
        , turnRunning(false)
    {
    }

    QString writeTempConfig() const;
    void handleEventLine(const QByteArray& line);
    void killProcess();
};

QString
CodexCliBackendPrivate::writeTempConfig() const
{
    if (!tempDir) {
        return QString();
    }

    const QString path = tempDir->filePath( QString::fromUtf8("config.toml") );
    QFile file(path);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) ) {
        return QString();
    }

    QTextStream out(&file);
    out << QString::fromUtf8("[mcp_servers.natron]\n");
    out << QString::fromUtf8("url = \"%1\"\n").arg(mcpUrl);
    out << QString::fromUtf8("bearer_token_env_var = \"NATRON_MCP_TOKEN\"\n");
    out << QString::fromUtf8("required = true\n");
    file.close();

    return path;
}

void
CodexCliBackendPrivate::handleEventLine(const QByteArray& line)
{
    const QByteArray trimmed = line.trimmed();

    if ( trimmed.isEmpty() ) {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(trimmed, &parseError);
    if ( ( parseError.error != QJsonParseError::NoError ) || !doc.isObject() ) {
        return;
    }

    const QJsonObject event = doc.object();
    const QString type = event[QString::fromUtf8("type")].toString();

    if ( type == QString::fromUtf8("item.completed") ||
         type == QString::fromUtf8("item.updated") ||
         type == QString::fromUtf8("item.started") ) {
        const QJsonObject item = event[QString::fromUtf8("item")].toObject();
        const QString itemType = item[QString::fromUtf8("type")].toString();
        if ( itemType == QString::fromUtf8("agent_message") ||
             itemType == QString::fromUtf8("message") ) {
            QString text = item[QString::fromUtf8("text")].toString();
            if ( text.isEmpty() ) {
                text = item[QString::fromUtf8("content")].toString();
            }
            if ( !text.isEmpty() && type == QString::fromUtf8("item.completed") ) {
                Q_EMIT _publicInterface->textChunk(text);
            }
        } else if ( itemType.contains( QString::fromUtf8("mcp") ) ||
                    itemType.contains( QString::fromUtf8("tool") ) ) {
            const QString name = item[QString::fromUtf8("name")].toString();
            if ( !name.isEmpty() ) {
                currentToolName = name;
                if ( type == QString::fromUtf8("item.started") ) {
                    Q_EMIT _publicInterface->toolCall( name,
                                                       QString::fromUtf8( QJsonDocument(item).toJson(QJsonDocument::Compact) ) );
                } else if ( type == QString::fromUtf8("item.completed") ) {
                    Q_EMIT _publicInterface->toolResult(currentToolName, true);
                }
            }
        }

        return;
    }

    if ( type == QString::fromUtf8("turn.completed") ) {
        turnRunning = false;
        Q_EMIT _publicInterface->turnFinished();

        return;
    }

    if ( type == QString::fromUtf8("turn.failed") || type == QString::fromUtf8("error") ) {
        turnRunning = false;
        QString msg = event[QString::fromUtf8("message")].toString();
        if ( msg.isEmpty() ) {
            msg = event[QString::fromUtf8("error")].toString();
        }
        if ( msg.isEmpty() ) {
            msg = CodexCliBackend::tr("Codex reported an error.");
        }
        Q_EMIT _publicInterface->errorOccurred(msg);
        Q_EMIT _publicInterface->turnFinished();
    }
}

void
CodexCliBackendPrivate::killProcess()
{
    if (!process) {
        return;
    }

    QProcess* p = process;
    process = 0;
    p->disconnect(_publicInterface);
    if ( p->state() != QProcess::NotRunning ) {
        p->terminate();
        if ( !p->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
            p->kill();
            p->waitForFinished(AI_AGENT_TERMINATE_TIMEOUT_MS);
        }
    }
    p->deleteLater();
    stdoutBuffer.clear();
    turnRunning = false;
}

CodexCliBackend::CodexCliBackend(QObject* parent)
    : AIAgentBackend(parent)
    , _codexImp( new CodexCliBackendPrivate(this) )
{
}

CodexCliBackend::~CodexCliBackend()
{
    stop();
}

void
CodexCliBackend::configure(const AIConnectionConfig& config)
{
    _codexImp->config = config;
}

QString
CodexCliBackend::displayName() const
{
    return QString::fromUtf8("Codex");
}

QString
CodexCliBackend::providerId() const
{
    return QString::fromUtf8("codex");
}

QString
CodexCliBackend::connectionMethodLabel() const
{
    return AIConnectionSettings::methodLabel(eAIConnectionMethodCli);
}

QString
CodexCliBackend::findExecutable() const
{
    if ( !_codexImp->config.cliPath.isEmpty() ) {
        QFileInfo info(_codexImp->config.cliPath);
        if ( info.exists() && info.isExecutable() ) {
            return info.absoluteFilePath();
        }
    }

    return AIProviderRegistry::findCliExecutable( QString::fromUtf8("codex") );
}

bool
CodexCliBackend::start(const QString& cwd,
                       const QString& mcpUrl,
                       const QString& token)
{
    const QString exe = findExecutable();
    if ( exe.isEmpty() ) {
        Q_EMIT errorOccurred( tr("Codex CLI not found. Install Codex, or click Connect… and use an API key.") );

        return false;
    }

    delete _codexImp->tempDir;
    _codexImp->tempDir = new QTemporaryDir();
    _codexImp->tempDir->setAutoRemove(true);
    if ( !_codexImp->tempDir->isValid() ) {
        Q_EMIT errorOccurred( tr("Could not create a temporary Codex config directory.") );

        return false;
    }

    _codexImp->cwd = cwd;
    _codexImp->mcpUrl = mcpUrl;
    _codexImp->mcpToken = token;
    _codexImp->sessionReady = true;

    return true;
}

void
CodexCliBackend::send(const QString& text)
{
    if ( !_codexImp->sessionReady ) {
        Q_EMIT errorOccurred( tr("Codex is not connected.") );

        return;
    }
    if (_codexImp->turnRunning) {
        Q_EMIT errorOccurred( tr("Codex is still working on the previous turn.") );

        return;
    }

    const QString exe = findExecutable();
    const QString configPath = _codexImp->writeTempConfig();
    if ( configPath.isEmpty() ) {
        Q_EMIT errorOccurred( tr("Could not write the temporary Codex MCP config.") );

        return;
    }

    _codexImp->killProcess();

    QStringList args;
    args << QString::fromUtf8("exec");
    args << QString::fromUtf8("--json");
    args << QString::fromUtf8("--skip-git-repo-check");
    if ( !_codexImp->config.model.trimmed().isEmpty() ) {
        args << QString::fromUtf8("-m") << _codexImp->config.model.trimmed();
    }
    args << text;

    _codexImp->process = new QProcess(this);
    _codexImp->process->setProgram(exe);
    _codexImp->process->setArguments(args);
    if ( !_codexImp->cwd.isEmpty() ) {
        _codexImp->process->setWorkingDirectory(_codexImp->cwd);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert( QString::fromUtf8("CODEX_HOME"), _codexImp->tempDir->path() );
    env.insert( QString::fromUtf8("NATRON_MCP_TOKEN"), _codexImp->mcpToken );
    if ( !_codexImp->config.apiKey.isEmpty() ) {
        env.insert( QString::fromUtf8("CODEX_API_KEY"), _codexImp->config.apiKey );
        env.insert( QString::fromUtf8("OPENAI_API_KEY"), _codexImp->config.apiKey );
    }
    _codexImp->process->setProcessEnvironment(env);

    connect( _codexImp->process, SIGNAL( readyReadStandardOutput() ),
             this, SLOT( onReadyReadStandardOutput() ) );
    connect( _codexImp->process, SIGNAL( readyReadStandardError() ),
             this, SLOT( onReadyReadStandardError() ) );
    connect( _codexImp->process, SIGNAL( finished(int, QProcess::ExitStatus) ),
             this, SLOT( onProcessFinished(int) ) );
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    connect( _codexImp->process, SIGNAL( errorOccurred(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#else
    connect( _codexImp->process, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#endif

    _codexImp->turnRunning = true;
    _codexImp->process->start();
    if ( !_codexImp->process->waitForStarted(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
        _codexImp->turnRunning = false;
        Q_EMIT errorOccurred( tr("Could not start '%1'.").arg(exe) );
        _codexImp->killProcess();
    }
}

void
CodexCliBackend::interrupt()
{
    _codexImp->killProcess();
    Q_EMIT turnFinished();
}

void
CodexCliBackend::stop()
{
    _codexImp->killProcess();
    _codexImp->sessionReady = false;
    delete _codexImp->tempDir;
    _codexImp->tempDir = 0;
    Q_EMIT finished();
}

bool
CodexCliBackend::isRunning() const
{
    return _codexImp->sessionReady;
}

void
CodexCliBackend::onReadyReadStandardOutput()
{
    if (!_codexImp->process) {
        return;
    }

    _codexImp->stdoutBuffer += _codexImp->process->readAllStandardOutput();
    int newline = _codexImp->stdoutBuffer.indexOf('\n');
    while (newline >= 0) {
        const QByteArray line = _codexImp->stdoutBuffer.left(newline);
        _codexImp->stdoutBuffer.remove(0, newline + 1);
        _codexImp->handleEventLine(line);
        newline = _codexImp->stdoutBuffer.indexOf('\n');
    }
}

void
CodexCliBackend::onReadyReadStandardError()
{
    if (!_codexImp->process) {
        return;
    }

    const QByteArray err = _codexImp->process->readAllStandardError();
    // Codex streams progress on stderr; only surface obvious failures.
    const QString text = QString::fromUtf8( err.trimmed() );
    if ( !text.isEmpty() &&
         ( text.contains( QString::fromUtf8("error"), Qt::CaseInsensitive ) ||
           text.contains( QString::fromUtf8("failed"), Qt::CaseInsensitive ) ) ) {
        Q_EMIT errorOccurred(text);
    }
}

void
CodexCliBackend::onProcessFinished(int exitCode)
{
    Q_UNUSED(exitCode);

    if (_codexImp->turnRunning) {
        _codexImp->turnRunning = false;
        Q_EMIT turnFinished();
    }
    _codexImp->stdoutBuffer.clear();
}

void
CodexCliBackend::onProcessError()
{
    if (!_codexImp->process) {
        return;
    }

    _codexImp->turnRunning = false;
    Q_EMIT errorOccurred( _codexImp->process->errorString() );
    Q_EMIT turnFinished();
}

NATRON_NAMESPACE_EXIT
