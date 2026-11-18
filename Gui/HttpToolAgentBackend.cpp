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

#include "HttpToolAgentBackend.h"

#include "Gui/AIProviderRegistry.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QEventLoop>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

NATRON_NAMESPACE_ENTER

namespace {
enum HttpApiKindEnum
{
    eHttpApiOpenAI = 0,
    eHttpApiAnthropic,
    eHttpApiGemini
};

HttpApiKindEnum
apiKindForProvider(const QString& providerId)
{
    if (providerId == QString::fromUtf8("claude")) {
        return eHttpApiAnthropic;
    }
    if ( ( providerId == QString::fromUtf8("gemini") ) ||
         ( providerId == QString::fromUtf8("antigravity") ) ) {
        return eHttpApiGemini;
    }

    return eHttpApiOpenAI;
}

QString
stripTrailingSlash(QString url)
{
    while ( url.endsWith( QLatin1Char('/') ) ) {
        url.chop(1);
    }

    return url;
}

/// Maps Connect-dialog aliases to Anthropic API model ids. Never lets Fable
/// sneak in as the Natron default (capable + cheap = Sonnet).
QString
resolveAnthropicModelId(const QString& model)
{
    const QString m = model.trimmed().toLower();

    if ( m.isEmpty() ||
         ( m == QString::fromUtf8("sonnet") ) ||
         ( m == QString::fromUtf8("best") ) ||
         m.contains( QString::fromUtf8("fable") ) ) {
        return QString::fromUtf8("claude-sonnet-5");
    }
    if ( m == QString::fromUtf8("haiku") ) {
        return QString::fromUtf8("claude-haiku-4-5");
    }
    if ( m == QString::fromUtf8("opus") ) {
        return QString::fromUtf8("claude-opus-5");
    }

    return model.trimmed();
}
} // namespace

struct HttpToolAgentBackendPrivate
{
    HttpToolAgentBackend* _publicInterface;
    AIConnectionConfig config;
    QString mcpUrl;
    QString mcpToken;
    QNetworkAccessManager* nam;
    QJsonArray messages;
    QJsonArray mcpTools;
    bool running;
    bool cancelRequested;
    bool turnActive;
    int maxToolRounds;

    HttpToolAgentBackendPrivate(HttpToolAgentBackend* publicInterface)
        : _publicInterface(publicInterface)
        , config()
        , mcpUrl()
        , mcpToken()
        , nam(0)
        , messages()
        , mcpTools()
        , running(false)
        , cancelRequested(false)
        , turnActive(false)
        , maxToolRounds(12)
    {
    }

    QByteArray postJson(const QUrl& url,
                        const QByteArray& body,
                        const QList<QPair<QByteArray, QByteArray> >& headers,
                        QString* errorOut);

    bool refreshMcpTools(QString* errorOut);
    QJsonObject callMcpTool(const QString& name,
                            const QJsonObject& arguments,
                            QString* errorOut);

    QJsonArray openaiTools() const;
    QJsonArray anthropicTools() const;
    QJsonArray geminiTools() const;

    void runOpenAITurn(const QString& userText);
    void runAnthropicTurn(const QString& userText);
    void runGeminiTurn(const QString& userText);
};

QByteArray
HttpToolAgentBackendPrivate::postJson(const QUrl& url,
                                      const QByteArray& body,
                                      const QList<QPair<QByteArray, QByteArray> >& headers,
                                      QString* errorOut)
{
    if (!nam) {
        if (errorOut) {
            *errorOut = HttpToolAgentBackend::tr("Network manager is not ready.");
        }

        return QByteArray();
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromUtf8("application/json"));
    for (int i = 0; i < headers.size(); ++i) {
        request.setRawHeader(headers.at(i).first, headers.at(i).second);
    }

    QNetworkReply* reply = nam->post(request, body);
    QEventLoop loop;
    QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect( &timeout, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
    timeout.start(120000);
    loop.exec();

    if (cancelRequested) {
        reply->abort();
        reply->deleteLater();
        if (errorOut) {
            *errorOut = HttpToolAgentBackend::tr("Cancelled.");
        }

        return QByteArray();
    }

    if ( !timeout.isActive() ) {
        reply->abort();
        reply->deleteLater();
        if (errorOut) {
            *errorOut = HttpToolAgentBackend::tr("The provider request timed out.");
        }

        return QByteArray();
    }

    const QByteArray response = reply->readAll();
    const QNetworkReply::NetworkError netErr = reply->error();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    if ( ( netErr != QNetworkReply::NoError ) && response.isEmpty() ) {
        if (errorOut) {
            if ( ( netErr == QNetworkReply::ConnectionRefusedError ) ||
                 ( netErr == QNetworkReply::HostNotFoundError ) ||
                 ( netErr == QNetworkReply::RemoteHostClosedError ) ||
                 ( netErr == QNetworkReply::TimeoutError ) ) {
                const bool looksLikeOllama =
                    ( config.providerId == QString::fromUtf8("ollama") ) ||
                    config.baseUrl.contains( QString::fromUtf8("11434") );
                if (looksLikeOllama) {
                    *errorOut = HttpToolAgentBackend::tr(
                        "Ollama is not running. Start Ollama, then click Connect… again.");
                } else {
                    *errorOut = HttpToolAgentBackend::tr(
                        "Cannot reach %1. Check the base URL, then click Connect… again.")
                        .arg( stripTrailingSlash(config.baseUrl) );
                }
            } else {
                *errorOut = HttpToolAgentBackend::tr(
                    "Network error talking to the provider. Click Connect… to retry.");
            }
        }

        return QByteArray();
    }

    if ( (status == 401) || (status == 403) ) {
        if (errorOut) {
            *errorOut = HttpToolAgentBackend::tr(
                "Invalid or missing API key. Click Connect… and paste a valid key.");
        }

        return QByteArray();
    }

    if (status >= 400) {
        if (errorOut) {
            *errorOut = HttpToolAgentBackend::tr(
                "Provider error (HTTP %1). Click Connect… to fix the key, model, or URL.")
                .arg(status);
        }

        return QByteArray();
    }

    return response;
}

bool
HttpToolAgentBackendPrivate::refreshMcpTools(QString* errorOut)
{
    QJsonObject req;
    req[QString::fromUtf8("jsonrpc")] = QString::fromUtf8("2.0");
    req[QString::fromUtf8("id")] = 1;
    req[QString::fromUtf8("method")] = QString::fromUtf8("tools/list");

    QList<QPair<QByteArray, QByteArray> > headers;
    headers.append( qMakePair( QByteArray("Authorization"),
                               QByteArray("Bearer ") + mcpToken.toUtf8() ) );

    QString err;
    const QByteArray raw = postJson( QUrl(mcpUrl),
                                     QJsonDocument(req).toJson(QJsonDocument::Compact),
                                     headers,
                                     &err );
    if ( raw.isEmpty() ) {
        if (errorOut) {
            *errorOut = err.isEmpty() ? HttpToolAgentBackend::tr("Could not list MCP tools.") : err;
        }

        return false;
    }

    const QJsonObject root = QJsonDocument::fromJson(raw).object();
    const QJsonObject result = root[QString::fromUtf8("result")].toObject();
    mcpTools = result[QString::fromUtf8("tools")].toArray();
    if ( mcpTools.isEmpty() ) {
        // Some servers wrap tools differently.
        if ( result.contains( QString::fromUtf8("tools") ) == false &&
             root.contains( QString::fromUtf8("tools") ) ) {
            mcpTools = root[QString::fromUtf8("tools")].toArray();
        }
    }

    return true;
}

QJsonObject
HttpToolAgentBackendPrivate::callMcpTool(const QString& name,
                                         const QJsonObject& arguments,
                                         QString* errorOut)
{
    Q_EMIT _publicInterface->toolCall( name,
                                       QString::fromUtf8( QJsonDocument(arguments).toJson(QJsonDocument::Compact) ) );

    QJsonObject params;
    params[QString::fromUtf8("name")] = name;
    params[QString::fromUtf8("arguments")] = arguments;

    QJsonObject req;
    req[QString::fromUtf8("jsonrpc")] = QString::fromUtf8("2.0");
    req[QString::fromUtf8("id")] = 2;
    req[QString::fromUtf8("method")] = QString::fromUtf8("tools/call");
    req[QString::fromUtf8("params")] = params;

    QList<QPair<QByteArray, QByteArray> > headers;
    headers.append( qMakePair( QByteArray("Authorization"),
                               QByteArray("Bearer ") + mcpToken.toUtf8() ) );

    QString err;
    const QByteArray raw = postJson( QUrl(mcpUrl),
                                     QJsonDocument(req).toJson(QJsonDocument::Compact),
                                     headers,
                                     &err );
    if ( raw.isEmpty() ) {
        Q_EMIT _publicInterface->toolResult(name, false);
        if (errorOut) {
            *errorOut = err;
        }

        return QJsonObject();
    }

    const QJsonObject root = QJsonDocument::fromJson(raw).object();
    const QJsonObject result = root[QString::fromUtf8("result")].toObject();
    const bool isError = result[QString::fromUtf8("isError")].toBool(false);
    Q_EMIT _publicInterface->toolResult(name, !isError);

    return result;
}

QJsonArray
HttpToolAgentBackendPrivate::openaiTools() const
{
    QJsonArray tools;

    for (int i = 0; i < mcpTools.size(); ++i) {
        const QJsonObject t = mcpTools.at(i).toObject();
        QJsonObject fn;
        fn[QString::fromUtf8("name")] = t[QString::fromUtf8("name")];
        fn[QString::fromUtf8("description")] = t[QString::fromUtf8("description")];
        fn[QString::fromUtf8("parameters")] = t[QString::fromUtf8("inputSchema")];
        QJsonObject tool;
        tool[QString::fromUtf8("type")] = QString::fromUtf8("function");
        tool[QString::fromUtf8("function")] = fn;
        tools.push_back(tool);
    }

    return tools;
}

QJsonArray
HttpToolAgentBackendPrivate::anthropicTools() const
{
    QJsonArray tools;

    for (int i = 0; i < mcpTools.size(); ++i) {
        const QJsonObject t = mcpTools.at(i).toObject();
        QJsonObject tool;
        tool[QString::fromUtf8("name")] = t[QString::fromUtf8("name")];
        tool[QString::fromUtf8("description")] = t[QString::fromUtf8("description")];
        tool[QString::fromUtf8("input_schema")] = t[QString::fromUtf8("inputSchema")];
        tools.push_back(tool);
    }

    return tools;
}

QJsonArray
HttpToolAgentBackendPrivate::geminiTools() const
{
    QJsonArray decls;

    for (int i = 0; i < mcpTools.size(); ++i) {
        const QJsonObject t = mcpTools.at(i).toObject();
        QJsonObject decl;
        decl[QString::fromUtf8("name")] = t[QString::fromUtf8("name")];
        decl[QString::fromUtf8("description")] = t[QString::fromUtf8("description")];
        decl[QString::fromUtf8("parameters")] = t[QString::fromUtf8("inputSchema")];
        decls.push_back(decl);
    }

    QJsonObject wrapper;
    wrapper[QString::fromUtf8("functionDeclarations")] = decls;
    QJsonArray tools;
    tools.push_back(wrapper);

    return tools;
}

void
HttpToolAgentBackendPrivate::runOpenAITurn(const QString& userText)
{
    QJsonObject userMsg;
    userMsg[QString::fromUtf8("role")] = QString::fromUtf8("user");
    userMsg[QString::fromUtf8("content")] = userText;
    messages.push_back(userMsg);

    const QString base = stripTrailingSlash(config.baseUrl);
    const QUrl url(base + QString::fromUtf8("/chat/completions"));

    for (int round = 0; round < maxToolRounds; ++round) {
        if (cancelRequested) {
            break;
        }

        QJsonObject body;
        body[QString::fromUtf8("model")] = config.model;
        body[QString::fromUtf8("messages")] = messages;
        body[QString::fromUtf8("tools")] = openaiTools();
        body[QString::fromUtf8("tool_choice")] = QString::fromUtf8("auto");

        QList<QPair<QByteArray, QByteArray> > headers;
        if ( !config.apiKey.isEmpty() ) {
            headers.append( qMakePair( QByteArray("Authorization"),
                                       QByteArray("Bearer ") + config.apiKey.toUtf8() ) );
        }

        QString err;
        const QByteArray raw = postJson( url,
                                         QJsonDocument(body).toJson(QJsonDocument::Compact),
                                         headers,
                                         &err );
        if ( raw.isEmpty() ) {
            Q_EMIT _publicInterface->errorOccurred(err);
            break;
        }

        const QJsonObject root = QJsonDocument::fromJson(raw).object();
        const QJsonArray choices = root[QString::fromUtf8("choices")].toArray();
        if ( choices.isEmpty() ) {
            Q_EMIT _publicInterface->errorOccurred( HttpToolAgentBackend::tr("Provider returned no choices.") );
            break;
        }

        const QJsonObject message = choices.at(0).toObject()[QString::fromUtf8("message")].toObject();
        messages.push_back(message);

        const QString content = message[QString::fromUtf8("content")].toString();
        if ( !content.isEmpty() ) {
            Q_EMIT _publicInterface->textChunk(content);
        }

        const QJsonArray toolCalls = message[QString::fromUtf8("tool_calls")].toArray();
        if ( toolCalls.isEmpty() ) {
            break;
        }

        for (int i = 0; i < toolCalls.size(); ++i) {
            const QJsonObject call = toolCalls.at(i).toObject();
            const QString id = call[QString::fromUtf8("id")].toString();
            const QJsonObject fn = call[QString::fromUtf8("function")].toObject();
            const QString name = fn[QString::fromUtf8("name")].toString();
            QJsonObject args = QJsonDocument::fromJson( fn[QString::fromUtf8("arguments")].toString().toUtf8() ).object();

            QString toolErr;
            const QJsonObject result = callMcpTool(name, args, &toolErr);
            QJsonObject toolMsg;
            toolMsg[QString::fromUtf8("role")] = QString::fromUtf8("tool");
            toolMsg[QString::fromUtf8("tool_call_id")] = id;
            toolMsg[QString::fromUtf8("content")] =
                toolErr.isEmpty() ? QString::fromUtf8( QJsonDocument(result).toJson(QJsonDocument::Compact) )
                                  : toolErr;
            messages.push_back(toolMsg);
        }
    }

    Q_EMIT _publicInterface->turnFinished();
}

void
HttpToolAgentBackendPrivate::runAnthropicTurn(const QString& userText)
{
    QJsonArray content;
    {
        QJsonObject block;
        block[QString::fromUtf8("type")] = QString::fromUtf8("text");
        block[QString::fromUtf8("text")] = userText;
        content.push_back(block);
    }
    QJsonObject userMsg;
    userMsg[QString::fromUtf8("role")] = QString::fromUtf8("user");
    userMsg[QString::fromUtf8("content")] = content;
    messages.push_back(userMsg);

    const QString base = stripTrailingSlash(config.baseUrl);
    const QUrl url(base + QString::fromUtf8("/v1/messages"));

    for (int round = 0; round < maxToolRounds; ++round) {
        if (cancelRequested) {
            break;
        }

        QJsonObject body;
        body[QString::fromUtf8("model")] = resolveAnthropicModelId(config.model);
        body[QString::fromUtf8("max_tokens")] = 4096;
        body[QString::fromUtf8("messages")] = messages;
        body[QString::fromUtf8("tools")] = anthropicTools();

        QList<QPair<QByteArray, QByteArray> > headers;
        headers.append( qMakePair( QByteArray("x-api-key"), config.apiKey.toUtf8() ) );
        headers.append( qMakePair( QByteArray("anthropic-version"), QByteArray("2023-06-01") ) );

        QString err;
        const QByteArray raw = postJson( url,
                                         QJsonDocument(body).toJson(QJsonDocument::Compact),
                                         headers,
                                         &err );
        if ( raw.isEmpty() ) {
            Q_EMIT _publicInterface->errorOccurred(err);
            break;
        }

        const QJsonObject root = QJsonDocument::fromJson(raw).object();
        const QJsonArray blocks = root[QString::fromUtf8("content")].toArray();

        QJsonObject assistantMsg;
        assistantMsg[QString::fromUtf8("role")] = QString::fromUtf8("assistant");
        assistantMsg[QString::fromUtf8("content")] = blocks;
        messages.push_back(assistantMsg);

        QJsonArray toolResults;
        for (int i = 0; i < blocks.size(); ++i) {
            const QJsonObject block = blocks.at(i).toObject();
            const QString type = block[QString::fromUtf8("type")].toString();
            if ( type == QString::fromUtf8("text") ) {
                const QString text = block[QString::fromUtf8("text")].toString();
                if ( !text.isEmpty() ) {
                    Q_EMIT _publicInterface->textChunk(text);
                }
            } else if ( type == QString::fromUtf8("tool_use") ) {
                const QString name = block[QString::fromUtf8("name")].toString();
                const QString id = block[QString::fromUtf8("id")].toString();
                const QJsonObject input = block[QString::fromUtf8("input")].toObject();
                QString toolErr;
                const QJsonObject result = callMcpTool(name, input, &toolErr);
                QJsonObject toolResult;
                toolResult[QString::fromUtf8("type")] = QString::fromUtf8("tool_result");
                toolResult[QString::fromUtf8("tool_use_id")] = id;
                toolResult[QString::fromUtf8("content")] =
                    toolErr.isEmpty() ? QString::fromUtf8( QJsonDocument(result).toJson(QJsonDocument::Compact) )
                                      : toolErr;
                toolResults.push_back(toolResult);
            }
        }

        if ( toolResults.isEmpty() ) {
            break;
        }

        QJsonObject followUp;
        followUp[QString::fromUtf8("role")] = QString::fromUtf8("user");
        followUp[QString::fromUtf8("content")] = toolResults;
        messages.push_back(followUp);
    }

    Q_EMIT _publicInterface->turnFinished();
}

void
HttpToolAgentBackendPrivate::runGeminiTurn(const QString& userText)
{
    QJsonObject part;
    part[QString::fromUtf8("text")] = userText;
    QJsonArray parts;
    parts.push_back(part);
    QJsonObject userMsg;
    userMsg[QString::fromUtf8("role")] = QString::fromUtf8("user");
    userMsg[QString::fromUtf8("parts")] = parts;
    messages.push_back(userMsg);

    const QString base = stripTrailingSlash(config.baseUrl);
    const QString model = config.model.isEmpty() ? QString::fromUtf8("gemini-2.5-flash") : config.model;
    QString urlStr = base + QString::fromUtf8("/models/") + model + QString::fromUtf8(":generateContent");
    if ( !config.apiKey.isEmpty() ) {
        urlStr += QString::fromUtf8("?key=") + QString::fromUtf8( QUrl::toPercentEncoding(config.apiKey) );
    }
    const QUrl url(urlStr);

    for (int round = 0; round < maxToolRounds; ++round) {
        if (cancelRequested) {
            break;
        }

        QJsonObject body;
        body[QString::fromUtf8("contents")] = messages;
        body[QString::fromUtf8("tools")] = geminiTools();

        QList<QPair<QByteArray, QByteArray> > headers;
        QString err;
        const QByteArray raw = postJson( url,
                                         QJsonDocument(body).toJson(QJsonDocument::Compact),
                                         headers,
                                         &err );
        if ( raw.isEmpty() ) {
            Q_EMIT _publicInterface->errorOccurred(err);
            break;
        }

        const QJsonObject root = QJsonDocument::fromJson(raw).object();
        const QJsonArray candidates = root[QString::fromUtf8("candidates")].toArray();
        if ( candidates.isEmpty() ) {
            Q_EMIT _publicInterface->errorOccurred( HttpToolAgentBackend::tr("Gemini returned no candidates.") );
            break;
        }

        const QJsonObject content = candidates.at(0).toObject()[QString::fromUtf8("content")].toObject();
        messages.push_back(content);

        const QJsonArray outParts = content[QString::fromUtf8("parts")].toArray();
        QJsonArray functionResponses;
        for (int i = 0; i < outParts.size(); ++i) {
            const QJsonObject p = outParts.at(i).toObject();
            if ( p.contains( QString::fromUtf8("text") ) ) {
                const QString text = p[QString::fromUtf8("text")].toString();
                if ( !text.isEmpty() ) {
                    Q_EMIT _publicInterface->textChunk(text);
                }
            }
            if ( p.contains( QString::fromUtf8("functionCall") ) ) {
                const QJsonObject fc = p[QString::fromUtf8("functionCall")].toObject();
                const QString name = fc[QString::fromUtf8("name")].toString();
                const QJsonObject args = fc[QString::fromUtf8("args")].toObject();
                QString toolErr;
                const QJsonObject result = callMcpTool(name, args, &toolErr);
                QJsonObject errObj;
                errObj[QString::fromUtf8("error")] = toolErr;
                QJsonObject response;
                response[QString::fromUtf8("name")] = name;
                response[QString::fromUtf8("response")] = toolErr.isEmpty() ? result : errObj;
                QJsonObject frPart;
                frPart[QString::fromUtf8("functionResponse")] = response;
                functionResponses.push_back(frPart);
            }
        }

        if ( functionResponses.isEmpty() ) {
            break;
        }

        QJsonObject follow;
        follow[QString::fromUtf8("role")] = QString::fromUtf8("user");
        follow[QString::fromUtf8("parts")] = functionResponses;
        messages.push_back(follow);
    }

    Q_EMIT _publicInterface->turnFinished();
}

HttpToolAgentBackend::HttpToolAgentBackend(QObject* parent)
    : AIAgentBackend(parent)
    , _httpImp( new HttpToolAgentBackendPrivate(this) )
{
    _httpImp->nam = new QNetworkAccessManager(this);
}

HttpToolAgentBackend::~HttpToolAgentBackend()
{
    stop();
}

void
HttpToolAgentBackend::configure(const AIConnectionConfig& config)
{
    _httpImp->config = config;
    const AIProviderInfo* info = AIProviderRegistry::findById(config.providerId);
    if (info) {
        if ( _httpImp->config.baseUrl.isEmpty() ) {
            _httpImp->config.baseUrl = info->defaultBaseUrl;
        }
        if ( _httpImp->config.model.isEmpty() ) {
            _httpImp->config.model = info->defaultModel;
        }
    }
}

QString
HttpToolAgentBackend::displayName() const
{
    const AIProviderInfo* info = AIProviderRegistry::findById(_httpImp->config.providerId);

    return info ? info->displayName : QString::fromUtf8("HTTP Agent");
}

QString
HttpToolAgentBackend::providerId() const
{
    return _httpImp->config.providerId;
}

QString
HttpToolAgentBackend::connectionMethodLabel() const
{
    return AIConnectionSettings::methodLabel(_httpImp->config.method == eAIConnectionMethodNone
                                             ? eAIConnectionMethodApiKey
                                             : _httpImp->config.method);
}

QString
HttpToolAgentBackend::findExecutable() const
{
    return QString();
}

bool
HttpToolAgentBackend::start(const QString& cwd,
                            const QString& mcpUrl,
                            const QString& token)
{
    Q_UNUSED(cwd);

    const bool keyOptional = ( _httpImp->config.providerId == QString::fromUtf8("ollama") ) ||
                             ( _httpImp->config.providerId == QString::fromUtf8("custom") );
    if ( !keyOptional && _httpImp->config.apiKey.isEmpty() ) {
        Q_EMIT errorOccurred( tr("An API key is required. Click Connect… and paste one.") );

        return false;
    }

    _httpImp->mcpUrl = mcpUrl;
    _httpImp->mcpToken = token;
    _httpImp->messages = QJsonArray();
    _httpImp->cancelRequested = false;

    QString err;
    if ( !_httpImp->refreshMcpTools(&err) ) {
        Q_EMIT errorOccurred(err);

        return false;
    }

    _httpImp->running = true;

    return true;
}

void
HttpToolAgentBackend::send(const QString& text)
{
    if ( !_httpImp->running ) {
        Q_EMIT errorOccurred( tr("The HTTP agent is not connected.") );

        return;
    }
    if (_httpImp->turnActive) {
        Q_EMIT errorOccurred( tr("Already handling a turn.") );

        return;
    }

    _httpImp->turnActive = true;
    _httpImp->cancelRequested = false;

    const HttpApiKindEnum kind = apiKindForProvider(_httpImp->config.providerId);
    if (kind == eHttpApiAnthropic) {
        _httpImp->runAnthropicTurn(text);
    } else if (kind == eHttpApiGemini) {
        _httpImp->runGeminiTurn(text);
    } else {
        _httpImp->runOpenAITurn(text);
    }

    _httpImp->turnActive = false;
}

void
HttpToolAgentBackend::interrupt()
{
    _httpImp->cancelRequested = true;
}

void
HttpToolAgentBackend::stop()
{
    _httpImp->cancelRequested = true;
    _httpImp->running = false;
    _httpImp->turnActive = false;
    _httpImp->messages = QJsonArray();
    Q_EMIT finished();
}

bool
HttpToolAgentBackend::isRunning() const
{
    return _httpImp->running;
}

void
HttpToolAgentBackend::onNetworkFinished()
{
    // Reserved for future async streaming; the sync EventLoop path is used today.
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_HttpToolAgentBackend.cpp"
