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

#include "AIConnectionSettings.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QSettings>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/AIProviderRegistry.h"

NATRON_NAMESPACE_ENTER

namespace {
QString
rootKey(const QString& leaf)
{
    return QString::fromUtf8("Natron/AI/") + leaf;
}

QString
providerKey(const QString& providerId,
            const QString& leaf)
{
    return QString::fromUtf8("Natron/AI/providers/") + providerId + QString::fromUtf8("/") + leaf;
}
} // namespace

QString
AIConnectionSettings::methodToString(AIConnectionMethodEnum method)
{
    switch (method) {
    case eAIConnectionMethodCli:
        return QString::fromUtf8("cli");
    case eAIConnectionMethodApiKey:
        return QString::fromUtf8("api");
    case eAIConnectionMethodCustom:
        return QString::fromUtf8("custom");
    case eAIConnectionMethodNone:
    default:
        return QString::fromUtf8("none");
    }
}

AIConnectionMethodEnum
AIConnectionSettings::methodFromString(const QString& s)
{
    if (s == QString::fromUtf8("cli")) {
        return eAIConnectionMethodCli;
    }
    if (s == QString::fromUtf8("api")) {
        return eAIConnectionMethodApiKey;
    }
    if (s == QString::fromUtf8("custom")) {
        return eAIConnectionMethodCustom;
    }

    return eAIConnectionMethodNone;
}

QString
AIConnectionSettings::methodLabel(AIConnectionMethodEnum method)
{
    switch (method) {
    case eAIConnectionMethodCli:
        return QString::fromUtf8("CLI");
    case eAIConnectionMethodApiKey:
        return QString::fromUtf8("API key");
    case eAIConnectionMethodCustom:
        return QString::fromUtf8("Custom");
    case eAIConnectionMethodNone:
    default:
        return QString::fromUtf8("not connected");
    }
}

AIConnectionConfig
AIConnectionSettings::load()
{
    QSettings s;
    QString providerId = s.value( rootKey( QString::fromUtf8("selectedProvider") ),
                                  AIProviderRegistry::defaultProviderId() ).toString();
    if ( !AIProviderRegistry::findById(providerId) ) {
        providerId = AIProviderRegistry::defaultProviderId();
    }

    return loadForProvider(providerId);
}

AIConnectionConfig
AIConnectionSettings::loadForProvider(const QString& providerId)
{
    QSettings s;
    AIConnectionConfig config;

    config.providerId = providerId;
    if ( !AIProviderRegistry::findById(config.providerId) ) {
        config.providerId = AIProviderRegistry::defaultProviderId();
    }

    config.method = methodFromString( s.value( providerKey(config.providerId, QString::fromUtf8("method") ),
                                               QString::fromUtf8("none") ).toString() );
    config.apiKey = s.value( providerKey(config.providerId, QString::fromUtf8("apiKey") ) ).toString();
    config.baseUrl = s.value( providerKey(config.providerId, QString::fromUtf8("baseUrl") ) ).toString();
    config.model = s.value( providerKey(config.providerId, QString::fromUtf8("model") ) ).toString();
    config.cliPath = s.value( providerKey(config.providerId, QString::fromUtf8("cliPath") ) ).toString();
    config.autoConnect = s.value( providerKey(config.providerId, QString::fromUtf8("autoConnect") ), false ).toBool();

    const AIProviderInfo* info = AIProviderRegistry::findById(config.providerId);
    if (info) {
        if ( config.baseUrl.isEmpty() ) {
            config.baseUrl = info->defaultBaseUrl;
        }
        if ( config.model.isEmpty() ) {
            config.model = info->defaultModel;
        }
    }

    return config;
}

void
AIConnectionSettings::save(const AIConnectionConfig& config)
{
    QSettings s;

    s.setValue( rootKey( QString::fromUtf8("selectedProvider") ), config.providerId );
    s.setValue( providerKey(config.providerId, QString::fromUtf8("method") ), methodToString(config.method) );
    s.setValue( providerKey(config.providerId, QString::fromUtf8("apiKey") ), config.apiKey );
    s.setValue( providerKey(config.providerId, QString::fromUtf8("baseUrl") ), config.baseUrl );
    s.setValue( providerKey(config.providerId, QString::fromUtf8("model") ), config.model );
    s.setValue( providerKey(config.providerId, QString::fromUtf8("cliPath") ), config.cliPath );
    s.setValue( providerKey(config.providerId, QString::fromUtf8("autoConnect") ), config.autoConnect );
    s.sync();
}

void
AIConnectionSettings::clearApiKey(const QString& providerId)
{
    QSettings s;

    s.remove( providerKey(providerId, QString::fromUtf8("apiKey") ) );
    s.sync();
}

bool
AIConnectionSettings::isAutoConnectEnabled()
{
    QSettings s;

    // Default on: open the panel and reconnect without another Connect click.
    return s.value( rootKey( QString::fromUtf8("autoConnectEnabled") ), true ).toBool();
}

void
AIConnectionSettings::setAutoConnectEnabled(bool enabled)
{
    QSettings s;

    s.setValue( rootKey( QString::fromUtf8("autoConnectEnabled") ), enabled );
    s.sync();
}

bool
AIConnectionSettings::resolveAutoMethod(AIConnectionConfig& config)
{
    const AIProviderInfo* info = AIProviderRegistry::findById(config.providerId);
    if (!info) {
        return false;
    }

    if ( config.baseUrl.isEmpty() ) {
        config.baseUrl = info->defaultBaseUrl;
    }
    if ( config.model.isEmpty() ) {
        config.model = info->defaultModel;
    }

    auto resolveCliPath = [&]() -> QString {
        if ( !config.cliPath.isEmpty() ) {
            return config.cliPath;
        }
        if ( info->cliBinaryName.isEmpty() ) {
            return QString();
        }
        QString found = AIProviderRegistry::findCliExecutable(info->cliBinaryName);
        if ( found.isEmpty() && ( config.providerId == QString::fromUtf8("antigravity") ) ) {
            found = AIProviderRegistry::findCliExecutable( QString::fromUtf8("antigravity") );
        }
        if ( found.isEmpty() && ( config.providerId == QString::fromUtf8("gemini") ) ) {
            found = AIProviderRegistry::findCliExecutable( QString::fromUtf8("agy") );
        }

        return found;
    };

    if ( config.method == eAIConnectionMethodCli ) {
        const QString cli = resolveCliPath();
        if ( !cli.isEmpty() ) {
            config.cliPath = cli;

            return true;
        }
        // CLI missing: fall through to API key if we have one.
        if ( info->supportsApi && !config.apiKey.isEmpty() ) {
            config.method = eAIConnectionMethodApiKey;

            return true;
        }

        return false;
    }

    if ( ( config.method == eAIConnectionMethodApiKey ) ||
         ( config.method == eAIConnectionMethodCustom ) ) {
        if ( ( config.providerId == QString::fromUtf8("ollama") ) ||
             ( config.providerId == QString::fromUtf8("custom") ) ) {
            return !config.baseUrl.isEmpty();
        }

        return !config.apiKey.isEmpty();
    }

    // method == none: prefer CLI, then API key.
    if (info->supportsCli) {
        const QString cli = resolveCliPath();
        if ( !cli.isEmpty() ) {
            config.cliPath = cli;
            config.method = eAIConnectionMethodCli;

            return true;
        }
    }
    if ( info->supportsApi && !config.apiKey.isEmpty() ) {
        config.method = eAIConnectionMethodApiKey;

        return true;
    }
    if ( info->supportsCustomEndpoint && !config.baseUrl.isEmpty() ) {
        config.method = eAIConnectionMethodCustom;

        return true;
    }

    return false;
}

NATRON_NAMESPACE_EXIT
