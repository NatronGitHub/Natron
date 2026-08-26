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

#include "AIProviderConnectDialog.h"

#include "Gui/AIProviderRegistry.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

NATRON_NAMESPACE_ENTER

struct AIProviderConnectDialogPrivate
{
    QString providerId;
    const AIProviderInfo* info;
    AIConnectionConfig config;

    QLabel* titleLabel;
    QLabel* hintLabel;
    QLabel* cliStatusLabel;
    QPushButton* recheckCliButton;
    QPushButton* useCliButton;
    QLineEdit* apiKeyEdit;
    QComboBox* modelCombo;
    QLineEdit* baseUrlEdit;
    QPushButton* useApiButton;
    QPushButton* useCliWithKeyButton;
    QPushButton* useCustomButton;
    QPushButton* clearKeyButton;

    AIProviderConnectDialogPrivate()
        : providerId()
        , info(0)
        , config()
        , titleLabel(0)
        , hintLabel(0)
        , cliStatusLabel(0)
        , recheckCliButton(0)
        , useCliButton(0)
        , apiKeyEdit(0)
        , modelCombo(0)
        , baseUrlEdit(0)
        , useApiButton(0)
        , useCliWithKeyButton(0)
        , useCustomButton(0)
        , clearKeyButton(0)
    {
    }
};

AIProviderConnectDialog::AIProviderConnectDialog(const QString& providerId,
                                                 QWidget* parent)
    : QDialog(parent)
    , _imp( new AIProviderConnectDialogPrivate() )
{
    _imp->providerId = providerId;
    _imp->info = AIProviderRegistry::findById(providerId);
    _imp->config = AIConnectionSettings::loadForProvider(providerId);

    setWindowTitle( tr("Connect AI provider") );
    resize(520, 420);

    QVBoxLayout* root = new QVBoxLayout(this);

    const QString name = _imp->info ? _imp->info->displayName : providerId;
    _imp->titleLabel = new QLabel( tr("<b>%1</b>").arg(name), this );
    root->addWidget(_imp->titleLabel);

    _imp->hintLabel = new QLabel(_imp->info ? _imp->info->installHint : QString(), this);
    _imp->hintLabel->setWordWrap(true);
    root->addWidget(_imp->hintLabel);

    if (_imp->info && _imp->info->supportsCli) {
        const bool isAgy = (_imp->providerId == QString::fromUtf8("antigravity"));
        QGroupBox* cliBox = new QGroupBox(
            isAgy ? tr("Antigravity CLI (agy) — Google account")
                  : tr("CLI (subscription login)"), this);
        QVBoxLayout* cliLayout = new QVBoxLayout(cliBox);
        _imp->cliStatusLabel = new QLabel(cliBox);
        cliLayout->addWidget(_imp->cliStatusLabel);
        QHBoxLayout* cliButtons = new QHBoxLayout();
        _imp->recheckCliButton = new QPushButton(tr("Recheck"), cliBox);
        _imp->useCliButton = new QPushButton(
            isAgy ? tr("Use CLI (Google login)") : tr("Use CLI"), cliBox);
        cliButtons->addWidget(_imp->recheckCliButton);
        cliButtons->addWidget(_imp->useCliButton);
        cliButtons->addStretch(1);
        cliLayout->addLayout(cliButtons);
        root->addWidget(cliBox);

        connect( _imp->recheckCliButton, SIGNAL( clicked() ), this, SLOT( onRecheckCli() ) );
        connect( _imp->useCliButton, SIGNAL( clicked() ), this, SLOT( onUseCli() ) );
    }

    {
        const bool isAgy = (_imp->providerId == QString::fromUtf8("antigravity"));
        QGroupBox* apiBox = new QGroupBox(
            isAgy ? tr("Gemini API key methods") : tr("API key"), this);
        QFormLayout* apiForm = new QFormLayout(apiBox);
        _imp->apiKeyEdit = new QLineEdit(apiBox);
        _imp->apiKeyEdit->setEchoMode(QLineEdit::Password);
        _imp->apiKeyEdit->setText(_imp->config.apiKey);
        _imp->apiKeyEdit->setPlaceholderText(
            isAgy ? tr("Gemini API key from Google AI Studio")
                  : tr("Paste API key (stored only on this machine)"));
        apiForm->addRow(tr("API key"), _imp->apiKeyEdit);

        _imp->modelCombo = new QComboBox(apiBox);
        _imp->modelCombo->setEditable(true);
        _imp->modelCombo->setInsertPolicy(QComboBox::NoInsert);
        {
            const QStringList suggested = AIProviderRegistry::suggestedModels(_imp->providerId);
            for (int i = 0; i < suggested.size(); ++i) {
                _imp->modelCombo->addItem(suggested.at(i));
            }
            QString model = _imp->config.model;
            if ( model.isEmpty() && _imp->info ) {
                model = _imp->info->defaultModel;
            }
            if ( !model.isEmpty() && ( _imp->modelCombo->findText(model) < 0 ) ) {
                _imp->modelCombo->insertItem(0, model);
            }
            if ( !model.isEmpty() ) {
                _imp->modelCombo->setCurrentText(model);
            }
        }
        apiForm->addRow(tr("Model"), _imp->modelCombo);

        _imp->baseUrlEdit = new QLineEdit(apiBox);
        _imp->baseUrlEdit->setText(_imp->config.baseUrl);
        apiForm->addRow(tr("Base URL"), _imp->baseUrlEdit);

        QHBoxLayout* apiButtons = new QHBoxLayout();
        if (isAgy) {
            _imp->useCliWithKeyButton = new QPushButton(tr("Use key via Antigravity CLI"), apiBox);
            _imp->useApiButton = new QPushButton(tr("Use direct Gemini HTTP"), apiBox);
            apiButtons->addWidget(_imp->useCliWithKeyButton);
            apiButtons->addWidget(_imp->useApiButton);
            connect( _imp->useCliWithKeyButton, SIGNAL( clicked() ), this, SLOT( onUseCliWithApiKey() ) );
        } else {
            _imp->useApiButton = new QPushButton(tr("Use API key"), apiBox);
            apiButtons->addWidget(_imp->useApiButton);
        }
        _imp->clearKeyButton = new QPushButton(tr("Clear saved key"), apiBox);
        apiButtons->addWidget(_imp->clearKeyButton);
        apiButtons->addStretch(1);
        apiForm->addRow(apiButtons);

        if (_imp->info && _imp->info->supportsCustomEndpoint) {
            _imp->useCustomButton = new QPushButton(tr("Use custom endpoint"), apiBox);
            apiButtons->addWidget(_imp->useCustomButton);
            connect( _imp->useCustomButton, SIGNAL( clicked() ), this, SLOT( onUseCustom() ) );
        }

        root->addWidget(apiBox);

        connect( _imp->useApiButton, SIGNAL( clicked() ), this, SLOT( onUseApiKey() ) );
        connect( _imp->clearKeyButton, SIGNAL( clicked() ), this, SLOT( onClearKey() ) );
    }

    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    connect( box, SIGNAL( rejected() ), this, SLOT( reject() ) );
    root->addWidget(box);

    refreshCliStatus();
}

AIProviderConnectDialog::~AIProviderConnectDialog()
{
}

AIConnectionConfig
AIProviderConnectDialog::resultConfig() const
{
    return _imp->config;
}

void
AIProviderConnectDialog::refreshCliStatus()
{
    if (!_imp->cliStatusLabel || !_imp->info) {
        return;
    }

    const QString exe = AIProviderRegistry::findCliExecutable(_imp->info->cliBinaryName);
    QString found = exe;
    if ( found.isEmpty() && ( _imp->providerId == QString::fromUtf8("antigravity") ) ) {
        found = AIProviderRegistry::findCliExecutable( QString::fromUtf8("antigravity") );
    }
    if ( found.isEmpty() ) {
        _imp->cliStatusLabel->setText( QString::fromUtf8("<span style=\"color:#c44;\">%1</span><br/><i>%2</i>")
                                       .arg( tr("CLI not found on PATH.") )
                                       .arg( _imp->info->installHint ) );
        if (_imp->useCliButton) {
            _imp->useCliButton->setEnabled(false);
        }
        if (_imp->useCliWithKeyButton) {
            _imp->useCliWithKeyButton->setEnabled(false);
        }
    } else {
        _imp->cliStatusLabel->setText( QString::fromUtf8("<span style=\"color:#4a4;\">%1</span><br/><code>%2</code>")
                                       .arg( tr("CLI found.") )
                                       .arg(found) );
        if (_imp->useCliButton) {
            _imp->useCliButton->setEnabled(true);
        }
        if (_imp->useCliWithKeyButton) {
            _imp->useCliWithKeyButton->setEnabled(true);
        }
    }
}

void
AIProviderConnectDialog::onRecheckCli()
{
    refreshCliStatus();
}

void
AIProviderConnectDialog::onUseCli()
{
    _imp->config.providerId = _imp->providerId;
    _imp->config.method = eAIConnectionMethodCli;
    // Google-login path: do not carry a key unless the user chose "via CLI".
    _imp->config.apiKey.clear();
    _imp->config.model = _imp->modelCombo->currentText().trimmed();
    _imp->config.baseUrl = _imp->baseUrlEdit->text().trimmed();
    _imp->config.autoConnect = true;
    accept();
}

void
AIProviderConnectDialog::onUseCliWithApiKey()
{
    _imp->config.providerId = _imp->providerId;
    _imp->config.method = eAIConnectionMethodCli;
    _imp->config.apiKey = _imp->apiKeyEdit->text().trimmed();
    _imp->config.model = _imp->modelCombo->currentText().trimmed();
    _imp->config.baseUrl = _imp->baseUrlEdit->text().trimmed();
    _imp->config.autoConnect = true;
    if ( _imp->config.apiKey.isEmpty() ) {
        _imp->hintLabel->setText( tr("Paste a Gemini API key first.") );

        return;
    }
    accept();
}

void
AIProviderConnectDialog::onUseApiKey()
{
    _imp->config.providerId = _imp->providerId;
    _imp->config.method = eAIConnectionMethodApiKey;
    _imp->config.apiKey = _imp->apiKeyEdit->text().trimmed();
    _imp->config.model = _imp->modelCombo->currentText().trimmed();
    _imp->config.baseUrl = _imp->baseUrlEdit->text().trimmed();
    _imp->config.autoConnect = true;

    const bool keyOptional = ( _imp->providerId == QString::fromUtf8("ollama") ) ||
                             ( _imp->providerId == QString::fromUtf8("custom") );
    if ( !keyOptional && _imp->config.apiKey.isEmpty() ) {
        _imp->hintLabel->setText( tr("Paste an API key first.") );

        return;
    }

    accept();
}

void
AIProviderConnectDialog::onUseCustom()
{
    _imp->config.providerId = _imp->providerId;
    _imp->config.method = eAIConnectionMethodCustom;
    _imp->config.apiKey = _imp->apiKeyEdit->text().trimmed();
    _imp->config.model = _imp->modelCombo->currentText().trimmed();
    _imp->config.baseUrl = _imp->baseUrlEdit->text().trimmed();
    _imp->config.autoConnect = true;
    if ( _imp->config.baseUrl.isEmpty() ) {
        _imp->hintLabel->setText( tr("Base URL is required for a custom endpoint.") );

        return;
    }
    accept();
}

void
AIProviderConnectDialog::onClearKey()
{
    _imp->apiKeyEdit->clear();
    AIConnectionSettings::clearApiKey(_imp->providerId);
    _imp->config.apiKey.clear();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AIProviderConnectDialog.cpp"
