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

struct GeminiCliBackendPrivate
{
    GeminiCliBackend* _publicInterface;
    QProcess* process;
    QByteArray stdoutBuffer;
    AIConnectionConfig config;
    QString mcpUrl;
    QString mcpToken;
    QString cwd;
    QTemporaryDir* tempDir;
    bool sessionReady;
    bool turnRunning;

    GeminiCliBackendPrivate(GeminiCliBackend* publicInterface)
        : _publicInterface(publicInterface)
        , process(0)
        , stdoutBuffer()
        , config()
        , mcpUrl()
        , mcpToken()
        , cwd()
        , tempDir(0)
        , sessionReady(false)
        , turnRunning(false)
    {
    }

    bool writeTempSettings() const;
    void killProcess();
};

bool
GeminiCliBackendPrivate::writeTempSettings() const
{
    if (!tempDir) {
        return false;
    }

    QJsonObject natronServer;
    natronServer[QString::fromUtf8("httpUrl")] = mcpUrl;
    {
        QJsonObject headers;
        headers[QString::fromUtf8("Authorization")] = QString::fromUtf8("Bearer ") + mcpToken;
        natronServer[QString::fromUtf8("headers")] = headers;
    }
    natronServer[QString::fromUtf8("trust")] = true;

    QJsonObject servers;
    servers[QString::fromUtf8("natron")] = natronServer;

    QJsonObject mcp;
    mcp[QString::fromUtf8("autoAllowInHeadless")] = true;

    QJsonObject root;
    root[QString::fromUtf8("mcpServers")] = servers;
    root[QString::fromUtf8("mcp")] = mcp;

    const QString path = tempDir->filePath( QString::fromUtf8("settings.json") );
    QFile file(path);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
        return false;
    }
    file.write( QJsonDocument(root).toJson(QJsonDocument::Compact) );
    file.close();

    return true;
}

void
GeminiCliBackendPrivate::killProcess()
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

GeminiCliBackend::GeminiCliBackend(QObject* parent)
    : AIAgentBackend(parent)
    , _geminiImp( new GeminiCliBackendPrivate(this) )
{
}

GeminiCliBackend::~GeminiCliBackend()
{
    stop();
}

void
GeminiCliBackend::configure(const AIConnectionConfig& config)
{
    _geminiImp->config = config;
}

QString
GeminiCliBackend::displayName() const
{
    return QString::fromUtf8("Gemini");
}

QString
GeminiCliBackend::providerId() const
{
    return QString::fromUtf8("gemini");
}

QString
GeminiCliBackend::connectionMethodLabel() const
{
    return AIConnectionSettings::methodLabel(eAIConnectionMethodCli);
}

QString
GeminiCliBackend::findExecutable() const
{
    if ( !_geminiImp->config.cliPath.isEmpty() ) {
        QFileInfo info(_geminiImp->config.cliPath);
        if ( info.exists() && info.isExecutable() ) {
            return info.absoluteFilePath();
        }
    }

    QString found = AIProviderRegistry::findCliExecutable( QString::fromUtf8("gemini") );
    if ( found.isEmpty() ) {
        found = AIProviderRegistry::findCliExecutable( QString::fromUtf8("agy") );
    }

    return found;
}

bool
GeminiCliBackend::start(const QString& cwd,
                        const QString& mcpUrl,
                        const QString& token)
{
    const QString exe = findExecutable();
    if ( exe.isEmpty() ) {
        Q_EMIT errorOccurred( tr("Gemini CLI not found. Install Gemini CLI, or click Connect… and use an API key.") );

        return false;
    }

    delete _geminiImp->tempDir;
    _geminiImp->tempDir = new QTemporaryDir();
    _geminiImp->tempDir->setAutoRemove(true);
    if ( !_geminiImp->tempDir->isValid() ) {
        Q_EMIT errorOccurred( tr("Could not create a temporary Gemini settings directory.") );

        return false;
    }

    _geminiImp->cwd = cwd;
    _geminiImp->mcpUrl = mcpUrl;
    _geminiImp->mcpToken = token;
    if ( !_geminiImp->writeTempSettings() ) {
        Q_EMIT errorOccurred( tr("Could not write temporary Gemini MCP settings.") );

        return false;
    }

    _geminiImp->sessionReady = true;

    return true;
}

void
GeminiCliBackend::send(const QString& text)
{
    if ( !_geminiImp->sessionReady ) {
        Q_EMIT errorOccurred( tr("Gemini is not connected.") );

        return;
    }
    if (_geminiImp->turnRunning) {
        Q_EMIT errorOccurred( tr("Gemini is still working on the previous turn.") );

        return;
    }

    const QString exe = findExecutable();
    _geminiImp->killProcess();

    QStringList args;
    args << QString::fromUtf8("-p");
    args << text;
    args << QString::fromUtf8("--yolo");
    if ( !_geminiImp->config.model.trimmed().isEmpty() ) {
        args << QString::fromUtf8("-m") << _geminiImp->config.model.trimmed();
    }

    _geminiImp->process = new QProcess(this);
    _geminiImp->process->setProgram(exe);
    _geminiImp->process->setArguments(args);
    if ( !_geminiImp->cwd.isEmpty() ) {
        _geminiImp->process->setWorkingDirectory(_geminiImp->cwd);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Point Gemini at our temp settings without replacing HOME (that would
    // drop the user's existing CLI login). Prefer an API key when provided.
    const QString settingsPath = _geminiImp->tempDir->filePath( QString::fromUtf8("settings.json") );
    env.insert( QString::fromUtf8("GEMINI_CLI_SYSTEM_SETTINGS_PATH"), settingsPath );
    if ( !_geminiImp->config.apiKey.isEmpty() ) {
        env.insert( QString::fromUtf8("GEMINI_API_KEY"), _geminiImp->config.apiKey );
        env.insert( QString::fromUtf8("GOOGLE_API_KEY"), _geminiImp->config.apiKey );
    }
    _geminiImp->process->setProcessEnvironment(env);

    connect( _geminiImp->process, SIGNAL( readyReadStandardOutput() ),
             this, SLOT( onReadyReadStandardOutput() ) );
    connect( _geminiImp->process, SIGNAL( readyReadStandardError() ),
             this, SLOT( onReadyReadStandardError() ) );
    connect( _geminiImp->process, SIGNAL( finished(int, QProcess::ExitStatus) ),
             this, SLOT( onProcessFinished(int) ) );
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    connect( _geminiImp->process, SIGNAL( errorOccurred(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#else
    connect( _geminiImp->process, SIGNAL( error(QProcess::ProcessError) ),
             this, SLOT( onProcessError() ) );
#endif

    _geminiImp->turnRunning = true;
    _geminiImp->process->start();
    if ( !_geminiImp->process->waitForStarted(AI_AGENT_TERMINATE_TIMEOUT_MS) ) {
        _geminiImp->turnRunning = false;
        Q_EMIT errorOccurred( tr("Could not start '%1'.").arg(exe) );
        _geminiImp->killProcess();
    }
}

void
GeminiCliBackend::interrupt()
{
    _geminiImp->killProcess();
    Q_EMIT turnFinished();
}

void
GeminiCliBackend::stop()
{
    _geminiImp->killProcess();
    _geminiImp->sessionReady = false;
    delete _geminiImp->tempDir;
    _geminiImp->tempDir = 0;
    Q_EMIT finished();
}

bool
GeminiCliBackend::isRunning() const
{
    return _geminiImp->sessionReady;
}

void
GeminiCliBackend::onReadyReadStandardOutput()
{
    if (!_geminiImp->process) {
        return;
    }

    const QByteArray chunk = _geminiImp->process->readAllStandardOutput();
    if ( !chunk.isEmpty() ) {
        Q_EMIT textChunk( QString::fromUtf8(chunk) );
    }
}

void
GeminiCliBackend::onReadyReadStandardError()
{
    if (!_geminiImp->process) {
        return;
    }

    const QByteArray err = _geminiImp->process->readAllStandardError();
    const QString text = QString::fromUtf8( err.trimmed() );
    if ( !text.isEmpty() &&
         ( text.contains( QString::fromUtf8("error"), Qt::CaseInsensitive ) ||
           text.contains( QString::fromUtf8("failed"), Qt::CaseInsensitive ) ) ) {
        Q_EMIT errorOccurred(text);
    }
}

void
GeminiCliBackend::onProcessFinished(int exitCode)
{
    Q_UNUSED(exitCode);

    if (_geminiImp->turnRunning) {
        _geminiImp->turnRunning = false;
        Q_EMIT turnFinished();
    }
}

void
GeminiCliBackend::onProcessError()
{
    if (!_geminiImp->process) {
        return;
    }

    _geminiImp->turnRunning = false;
    Q_EMIT errorOccurred( _geminiImp->process->errorString() );
    Q_EMIT turnFinished();
}

NATRON_NAMESPACE_EXIT
