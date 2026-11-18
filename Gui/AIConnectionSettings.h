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

#ifndef NATRON_GUI_AICONNECTIONSETTINGS_H
#define NATRON_GUI_AICONNECTIONSETTINGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QString>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief How the user authenticated a provider for this machine.
 **/
enum AIConnectionMethodEnum
{
    eAIConnectionMethodNone = 0,
    eAIConnectionMethodCli,
    eAIConnectionMethodApiKey,
    eAIConnectionMethodCustom
};

/**
 * @brief Machine-local connection choice for one AI provider session.
 *
 * API keys are stored only in QSettings (never in the project). Callers must
 * never log or append apiKey into the chat transcript.
 **/
struct AIConnectionConfig
{
    QString providerId;
    AIConnectionMethodEnum method;
    QString apiKey;
    QString baseUrl;
    QString model;
    QString cliPath;
    /// True after the user successfully connected at least once with this config.
    bool autoConnect;

    AIConnectionConfig()
        : providerId()
        , method(eAIConnectionMethodNone)
        , apiKey()
        , baseUrl()
        , model()
        , cliPath()
        , autoConnect(false)
    {
    }
};

/**
 * @brief Load/save AI provider prefs under QSettings group "Natron/AI".
 **/
class AIConnectionSettings
{
public:

    static AIConnectionConfig load();

    static AIConnectionConfig loadForProvider(const QString& providerId);

    static void save(const AIConnectionConfig& config);

    static void clearApiKey(const QString& providerId);

    static QString methodLabel(AIConnectionMethodEnum method);

    static AIConnectionMethodEnum methodFromString(const QString& s);

    static QString methodToString(AIConnectionMethodEnum method);

    /// Global preference: reconnect when the panel opens / provider changes.
    static bool isAutoConnectEnabled();

    static void setAutoConnectEnabled(bool enabled);

    /**
     * @brief If method is unset, pick CLI when the binary exists, else API when
     * a key is saved. Returns true when a usable method is available.
     **/
    static bool resolveAutoMethod(AIConnectionConfig& config);
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_AICONNECTIONSETTINGS_H
