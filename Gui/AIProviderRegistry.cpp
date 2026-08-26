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

#include "AIProviderRegistry.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

NATRON_NAMESPACE_ENTER

namespace {
std::vector<AIProviderInfo>
makeCatalog()
{
    std::vector<AIProviderInfo> out;

    {
        AIProviderInfo p;
        p.id = QString::fromUtf8("claude");
        p.displayName = QString::fromUtf8("Claude");
        p.supportsCli = true;
        p.supportsApi = true;
        p.supportsCustomEndpoint = false;
        p.defaultModel = QString::fromUtf8("sonnet");
        p.suggestedModels = QStringList()
                            << QString::fromUtf8("sonnet")
                            << QString::fromUtf8("opus")
                            << QString::fromUtf8("haiku")
                            << QString::fromUtf8("claude-sonnet-4-5")
                            << QString::fromUtf8("claude-opus-4-5")
                            << QString::fromUtf8("claude-haiku-4-5");
        p.defaultBaseUrl = QString::fromUtf8("https://api.anthropic.com");
        p.apiKeyEnvVar = QString::fromUtf8("ANTHROPIC_API_KEY");
        p.cliBinaryName = QString::fromUtf8("claude");
        p.installHint = QString::fromUtf8("Install Claude Code, then run 'claude' once in a terminal to sign in.");
        out.push_back(p);
    }
    {
        AIProviderInfo p;
        p.id = QString::fromUtf8("chatgpt");
        p.displayName = QString::fromUtf8("ChatGPT");
        p.supportsCli = false;
        p.supportsApi = true;
        p.supportsCustomEndpoint = false;
        p.defaultModel = QString::fromUtf8("gpt-4.1");
        p.suggestedModels = QStringList()
                            << QString::fromUtf8("gpt-4.1")
                            << QString::fromUtf8("gpt-4o")
                            << QString::fromUtf8("o3")
                            << QString::fromUtf8("o4-mini")
                            << QString::fromUtf8("gpt-4.1-mini");
        p.defaultBaseUrl = QString::fromUtf8("https://api.openai.com/v1");
        p.apiKeyEnvVar = QString::fromUtf8("OPENAI_API_KEY");
        p.cliBinaryName = QString();
        p.installHint = QString::fromUtf8("Paste an OpenAI API key, or use the Codex provider with the Codex CLI.");
        out.push_back(p);
    }
    {
        AIProviderInfo p;
        p.id = QString::fromUtf8("codex");
        p.displayName = QString::fromUtf8("Codex");
        p.supportsCli = true;
        p.supportsApi = true;
        p.supportsCustomEndpoint = false;
        p.defaultModel = QString::fromUtf8("gpt-5");
        p.suggestedModels = QStringList()
                            << QString::fromUtf8("gpt-5")
                            << QString::fromUtf8("gpt-4.1")
                            << QString::fromUtf8("o3")
                            << QString::fromUtf8("o4-mini");
        p.defaultBaseUrl = QString::fromUtf8("https://api.openai.com/v1");
        p.apiKeyEnvVar = QString::fromUtf8("CODEX_API_KEY");
        p.cliBinaryName = QString::fromUtf8("codex");
        p.installHint = QString::fromUtf8("Install the Codex CLI and run 'codex login', or paste a CODEX_API_KEY / OpenAI key.");
        out.push_back(p);
    }
    {
        AIProviderInfo p;
        p.id = QString::fromUtf8("gemini");
        p.displayName = QString::fromUtf8("Gemini");
        p.supportsCli = true;
        p.supportsApi = true;
        p.supportsCustomEndpoint = false;
        p.defaultModel = QString::fromUtf8("gemini-2.5-flash");
        p.suggestedModels = QStringList()
                            << QString::fromUtf8("gemini-2.5-flash")
                            << QString::fromUtf8("gemini-2.5-pro")
                            << QString::fromUtf8("gemini-2.0-flash");
        p.defaultBaseUrl = QString::fromUtf8("https://generativelanguage.googleapis.com/v1beta");
        p.apiKeyEnvVar = QString::fromUtf8("GEMINI_API_KEY");
        p.cliBinaryName = QString::fromUtf8("gemini");
        p.installHint = QString::fromUtf8("Install the Gemini CLI, or paste a GEMINI_API_KEY. Prefer Antigravity for the current Google agent CLI.");
        out.push_back(p);
    }
    {
        AIProviderInfo p;
        p.id = QString::fromUtf8("antigravity");
        p.displayName = QString::fromUtf8("Antigravity");
        p.supportsCli = true;
        p.supportsApi = true;
        p.supportsCustomEndpoint = false;
        p.defaultModel = QString::fromUtf8("gemini-2.5-flash");
        p.suggestedModels = QStringList()
                            << QString::fromUtf8("gemini-2.5-flash")
                            << QString::fromUtf8("gemini-2.5-pro")
                            << QString::fromUtf8("gemini-2.0-flash");
        p.defaultBaseUrl = QString::fromUtf8("https://generativelanguage.googleapis.com/v1beta");
        p.apiKeyEnvVar = QString::fromUtf8("GEMINI_API_KEY");
        p.cliBinaryName = QString::fromUtf8("agy");
        p.installHint = QString::fromUtf8(
            "Install Antigravity CLI (agy). Connect with: (1) Google account — run agy once to sign in, then Use CLI; "
            "(2) Gemini API key — paste key and Use API key via CLI; "
            "(3) Direct Gemini HTTP — same key through the HTTP agent.");
        out.push_back(p);
    }
    {
        AIProviderInfo p;
        p.id = QString::fromUtf8("ollama");
        p.displayName = QString::fromUtf8("Ollama");
        p.supportsCli = false;
        p.supportsApi = true;
        p.supportsCustomEndpoint = true;
        p.defaultModel = QString::fromUtf8("llama3.2");
        p.suggestedModels = QStringList()
                            << QString::fromUtf8("llama3.2")
                            << QString::fromUtf8("llama3.1")
                            << QString::fromUtf8("mistral")
                            << QString::fromUtf8("qwen2.5")
                            << QString::fromUtf8("codellama");
        p.defaultBaseUrl = QString::fromUtf8("http://127.0.0.1:11434/v1");
        p.apiKeyEnvVar = QString::fromUtf8("OLLAMA_API_KEY");
        p.cliBinaryName = QString();
        p.installHint = QString::fromUtf8("Run Ollama locally. API key is optional for the local server.");
        out.push_back(p);
    }
    {
        AIProviderInfo p;
        p.id = QString::fromUtf8("custom");
        p.displayName = QString::fromUtf8("Custom");
        p.supportsCli = false;
        p.supportsApi = true;
        p.supportsCustomEndpoint = true;
        p.defaultModel = QString::fromUtf8("gpt-4.1");
        p.suggestedModels = QStringList()
                            << QString::fromUtf8("gpt-4.1")
                            << QString::fromUtf8("gpt-4o");
        p.defaultBaseUrl = QString::fromUtf8("http://127.0.0.1:8000/v1");
        p.apiKeyEnvVar = QString();
        p.cliBinaryName = QString();
        p.installHint = QString::fromUtf8("Any OpenAI-compatible HTTP endpoint (base URL + model + optional key).");
        out.push_back(p);
    }

    return out;
}
} // namespace

const std::vector<AIProviderInfo>&
AIProviderRegistry::all()
{
    static const std::vector<AIProviderInfo> catalog = makeCatalog();

    return catalog;
}

const AIProviderInfo*
AIProviderRegistry::findById(const QString& id)
{
    const std::vector<AIProviderInfo>& catalog = all();

    for (std::size_t i = 0; i < catalog.size(); ++i) {
        if (catalog[i].id == id) {
            return &catalog[i];
        }
    }

    return 0;
}

QString
AIProviderRegistry::defaultProviderId()
{
    return QString::fromUtf8("claude");
}

QStringList
AIProviderRegistry::suggestedModels(const QString& providerId)
{
    const AIProviderInfo* info = findById(providerId);
    if (!info) {
        return QStringList();
    }

    return info->suggestedModels;
}

QString
AIProviderRegistry::findCliExecutable(const QString& binaryName,
                                      const QStringList& extraCandidates)
{
    if ( binaryName.isEmpty() ) {
        return QString();
    }

#ifdef __NATRON_WIN32__
    QString found = QStandardPaths::findExecutable(binaryName + QString::fromUtf8(".exe"));
    if ( found.isEmpty() ) {
        found = QStandardPaths::findExecutable(binaryName + QString::fromUtf8(".cmd"));
    }
#else
    QString found = QStandardPaths::findExecutable(binaryName);
#endif

    if ( !found.isEmpty() ) {
        return found;
    }

    QStringList candidates = extraCandidates;
    const QString home = QDir::homePath();
#ifdef __NATRON_WIN32__
    candidates << home + QString::fromUtf8("/.local/bin/") + binaryName + QString::fromUtf8(".exe");
    candidates << home + QString::fromUtf8("/AppData/Roaming/npm/") + binaryName + QString::fromUtf8(".cmd");
    candidates << home + QString::fromUtf8("/AppData/Local/") + binaryName + QString::fromUtf8("/") + binaryName + QString::fromUtf8(".exe");
#else
    candidates << home + QString::fromUtf8("/.local/bin/") + binaryName;
    candidates << QString::fromUtf8("/usr/local/bin/") + binaryName;
    candidates << QString::fromUtf8("/opt/homebrew/bin/") + binaryName;
#endif

    for (int i = 0; i < candidates.size(); ++i) {
        QFileInfo info( candidates.at(i) );
        if ( info.exists() && info.isExecutable() ) {
            return info.absoluteFilePath();
        }
    }

    return QString();
}

NATRON_NAMESPACE_EXIT
