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

#ifndef NATRON_GUI_AIPROVIDERREGISTRY_H
#define NATRON_GUI_AIPROVIDERREGISTRY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QString>
#include <QStringList>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Static catalog entry for one AI provider shown in the connection menu.
 **/
struct AIProviderInfo
{
    QString id;
    QString displayName;
    bool supportsCli;
    bool supportsApi;
    bool supportsCustomEndpoint;
    QString defaultModel;
    /// Common choices shown in the Model combo (user can still type a custom id).
    QStringList suggestedModels;
    QString defaultBaseUrl;
    QString apiKeyEnvVar;
    QString cliBinaryName;
    QString installHint;
};

/**
 * @brief Built-in provider catalog (Claude, ChatGPT, Codex, Gemini, Ollama, Custom).
 **/
class AIProviderRegistry
{
public:

    static const std::vector<AIProviderInfo>& all();

    static const AIProviderInfo* findById(const QString& id);

    static QString defaultProviderId();

    /// Suggested model ids for the given provider (empty if unknown).
    static QStringList suggestedModels(const QString& providerId);

    /**
     * @brief Locates a CLI binary by name on PATH and known install locations.
     **/
    static QString findCliExecutable(const QString& binaryName,
                                     const QStringList& extraCandidates = QStringList());
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_AIPROVIDERREGISTRY_H
