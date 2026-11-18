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

#include "AIChatPanel.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTextBrowser>
#include <QUrl>
#include <QHBoxLayout>
#include <QVBoxLayout>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"

#include "Gui/AIAgentBackend.h"
#include "Gui/AIMcpServer.h"
#include "Gui/AIProviderConnectDialog.h"
#include "Gui/AIProviderRegistry.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"

NATRON_NAMESPACE_ENTER

struct AIChatPanelPrivate
{
    AIChatPanel* _publicInterface;
    Gui* gui;

    QVBoxLayout* mainLayout;
    QLabel* statusLabel;
    QTextBrowser* transcript;
    QPlainTextEdit* input;
    QPushButton* sendButton;
    QPushButton* stopButton;
    QComboBox* providerCombo;
    QComboBox* modelCombo;
    QPushButton* connectButton;
    QLabel* providerLabel;
    QCheckBox* autoConnectBox;
    QCheckBox* autoApprove;

    AIMcpServer* server;
    AIAgentBackend* backend;
    AIConnectionConfig connection;
    bool agentConnected;
    bool userStopped;

    bool turnInProgress;
    bool assistantBubbleOpen;
    bool ignoreProviderChange;
    bool ignoreModelChange;
    bool welcomeShown;

    AIChatPanelPrivate(AIChatPanel* publicInterface,
                       Gui* g)
        : _publicInterface(publicInterface)
        , gui(g)
        , mainLayout(0)
        , statusLabel(0)
        , transcript(0)
        , input(0)
        , sendButton(0)
        , stopButton(0)
        , providerCombo(0)
        , modelCombo(0)
        , connectButton(0)
        , providerLabel(0)
        , autoConnectBox(0)
        , autoApprove(0)
        , server(0)
        , backend(0)
        , connection()
        , agentConnected(false)
        , userStopped(false)
        , turnInProgress(false)
        , assistantBubbleOpen(false)
        , ignoreProviderChange(false)
        , ignoreModelChange(false)
        , welcomeShown(false)
    {
    }

    void appendHtml(const QString& html);
    void appendUserBubble(const QString& text);
    void openAssistantBubble();
    void setStatus(const QString& text);

    QString visibleContext() const;

    static QString escape(const QString& text);
};

QString
AIChatPanelPrivate::escape(const QString& text)
{
    QString out = text;

    out.replace( QString::fromUtf8("&"), QString::fromUtf8("&amp;") );
    out.replace( QString::fromUtf8("<"), QString::fromUtf8("&lt;") );
    out.replace( QString::fromUtf8(">"), QString::fromUtf8("&gt;") );

    return out;
}

void
AIChatPanelPrivate::appendHtml(const QString& html)
{
    transcript->moveCursor(QTextCursor::End);
    transcript->insertHtml(html);
    transcript->moveCursor(QTextCursor::End);

    QScrollBar* bar = transcript->verticalScrollBar();
    if (bar) {
        bar->setValue( bar->maximum() );
    }
}

void
AIChatPanelPrivate::appendUserBubble(const QString& text)
{
    appendHtml( QString::fromUtf8("<p><b>%1</b><br/>%2</p>")
                .arg( AIChatPanel::tr("You") )
                .arg( escape(text).replace( QString::fromUtf8("\n"), QString::fromUtf8("<br/>") ) ) );
    assistantBubbleOpen = false;
}

void
AIChatPanelPrivate::openAssistantBubble()
{
    if (assistantBubbleOpen) {
        return;
    }
    appendHtml( QString::fromUtf8("<p><b>%1</b><br/>")
                .arg( backend ? backend->displayName() : AIChatPanel::tr("Assistant") ) );
    assistantBubbleOpen = true;
}

void
AIChatPanelPrivate::setStatus(const QString& text)
{
    if (statusLabel) {
        statusLabel->setText(text);
    }
}

QString
AIChatPanelPrivate::visibleContext() const
{
    if (!gui) {
        return QString();
    }

    QStringList parts;

    NodeGraph* graph = gui->getNodeGraph();
    if (graph) {
        const std::list<NodeGuiPtr>& selected = graph->getSelectedNodes();
        QStringList names;
        for (std::list<NodeGuiPtr>::const_iterator it = selected.begin(); it != selected.end(); ++it) {
            NodePtr node = (*it) ? (*it)->getNode() : NodePtr();
            if (node) {
                names.push_back( QString::fromUtf8( node->getScriptName_mt_safe().c_str() ) );
            }
        }
        if ( !names.isEmpty() ) {
            parts.push_back( AIChatPanel::tr("Selected nodes: %1").arg( names.join( QString::fromUtf8(", ") ) ) );
        }
    }

    GuiAppInstancePtr app = gui->getApp();
    if (app) {
        TimeLinePtr timeline = app->getTimeLine();
        if (timeline) {
            parts.push_back( AIChatPanel::tr("Current frame: %1").arg( (int)timeline->currentFrame() ) );
        }
    }

    if ( parts.isEmpty() ) {
        return QString();
    }

    return QString::fromUtf8("[Natron context] ") + parts.join( QString::fromUtf8(" | ") );
}

// ---------------------------------------------------------------------------

AIChatPanel::AIChatPanel(Gui* gui)
    : QWidget(gui)
    , PanelWidget(this, gui)
    , _imp( new AIChatPanelPrivate(this, gui) )
{
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(4, 4, 4, 4);
    _imp->mainLayout->setSpacing(4);

    _imp->statusLabel = new QLabel(this);
    _imp->statusLabel->setText( tr("Not started") );
    _imp->mainLayout->addWidget(_imp->statusLabel);

    QHBoxLayout* providerRow = new QHBoxLayout();
    providerRow->addWidget( new QLabel(tr("Provider:"), this) );
    _imp->providerCombo = new QComboBox(this);
    const std::vector<AIProviderInfo>& providers = AIProviderRegistry::all();
    for (std::size_t i = 0; i < providers.size(); ++i) {
        _imp->providerCombo->addItem(providers[i].displayName, providers[i].id);
    }
    providerRow->addWidget(_imp->providerCombo, 1);
    _imp->connectButton = new QPushButton(tr("Connect..."), this);
    providerRow->addWidget(_imp->connectButton);
    _imp->autoConnectBox = new QCheckBox(tr("Auto-connect"), this);
    _imp->autoConnectBox->setToolTip( tr("When checked, Natron connects automatically when you open this panel "
                                         "or change provider, using the last method (CLI / API key) that works.") );
    _imp->autoConnectBox->setChecked( AIConnectionSettings::isAutoConnectEnabled() );
    providerRow->addWidget(_imp->autoConnectBox);
    _imp->mainLayout->addLayout(providerRow);

    QHBoxLayout* modelRow = new QHBoxLayout();
    modelRow->addWidget( new QLabel(tr("Model:"), this) );
    _imp->modelCombo = new QComboBox(this);
    _imp->modelCombo->setEditable(true);
    _imp->modelCombo->setInsertPolicy(QComboBox::NoInsert);
    _imp->modelCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _imp->modelCombo->setToolTip( tr("Pick a suggested model or type any model id the provider accepts. "
                                     "Saved per provider on this machine.") );
    modelRow->addWidget(_imp->modelCombo, 1);
    _imp->mainLayout->addLayout(modelRow);

    _imp->transcript = new QTextBrowser(this);
    _imp->transcript->setOpenExternalLinks(true);
    _imp->transcript->setReadOnly(true);
    _imp->transcript->setOpenLinks(false);
    QObject::connect( _imp->transcript, SIGNAL( anchorClicked(QUrl) ),
                      this, SLOT( onTranscriptAnchorClicked(QUrl) ) );
    _imp->mainLayout->addWidget(_imp->transcript, 1);

    _imp->input = new QPlainTextEdit(this);
    _imp->input->setPlaceholderText( tr("Ask the assistant to build or change the comp. Enter sends, Shift+Enter adds a line.") );
    _imp->input->setMaximumHeight(90);
    _imp->input->installEventFilter(this);
    _imp->mainLayout->addWidget(_imp->input);

    QHBoxLayout* buttons = new QHBoxLayout();
    _imp->providerLabel = new QLabel(this);
    buttons->addWidget(_imp->providerLabel, 1);

    _imp->autoApprove = new QCheckBox(tr("Allow everything without asking"), this);
    _imp->autoApprove->setToolTip( tr("When checked, destructive operations such as deleting a node run "
                                      "immediately instead of raising a confirmation dialog.") );
    _imp->autoApprove->setChecked(true);
    buttons->addWidget(_imp->autoApprove);

    _imp->stopButton = new QPushButton(tr("Stop"), this);
    _imp->stopButton->setEnabled(false);
    buttons->addWidget(_imp->stopButton);

    _imp->sendButton = new QPushButton(tr("Send"), this);
    buttons->addWidget(_imp->sendButton);

    _imp->mainLayout->addLayout(buttons);

    QObject::connect( _imp->sendButton, SIGNAL( clicked() ), this, SLOT( onSendClicked() ) );
    QObject::connect( _imp->stopButton, SIGNAL( clicked() ), this, SLOT( onStopClicked() ) );
    QObject::connect( _imp->connectButton, SIGNAL( clicked() ), this, SLOT( onConnectClicked() ) );
    QObject::connect( _imp->autoConnectBox, SIGNAL( toggled(bool) ),
                      this, SLOT( onAutoConnectToggled(bool) ) );
    QObject::connect( _imp->providerCombo, SIGNAL( currentIndexChanged(int) ),
                      this, SLOT( onProviderComboChanged(int) ) );
    QObject::connect( _imp->modelCombo, SIGNAL( activated(QString) ),
                      this, SLOT( onModelComboChanged(QString) ) );
    if ( _imp->modelCombo->lineEdit() ) {
        QObject::connect( _imp->modelCombo->lineEdit(), SIGNAL( editingFinished() ),
                          this, SLOT( onModelEditingFinished() ) );
    }

    _imp->connection = AIConnectionSettings::load();
    _imp->ignoreProviderChange = true;
    const int idx = _imp->providerCombo->findData(_imp->connection.providerId);
    if (idx >= 0) {
        _imp->providerCombo->setCurrentIndex(idx);
    }
    _imp->ignoreProviderChange = false;

    refreshModelCombo();
    updateProviderFooter();
}

AIChatPanel::~AIChatPanel()
{
    if (_imp->backend) {
        _imp->backend->stop();
    }
    if (_imp->server) {
        _imp->server->stop();
    }
}

QUndoStack*
AIChatPanel::getUndoStack() const
{
    if (!_imp->gui) {
        return 0;
    }

    NodeGraph* graph = _imp->gui->getNodeGraph();

    return graph ? graph->getUndoStack() : 0;
}

QString
AIChatPanel::projectCwd() const
{
    QString cwd;
    GuiAppInstancePtr app = _imp->gui ? _imp->gui->getApp() : GuiAppInstancePtr();
    if (app) {
        ProjectPtr project = app->getProject();
        if (project) {
            const QString path = project->getProjectPath();
            if ( !path.isEmpty() && QDir(path).exists() ) {
                cwd = path;
            }
        }
    }
    if ( cwd.isEmpty() ) {
        cwd = QDir::homePath();
    }

    return cwd;
}

void
AIChatPanel::connectBackendSignals()
{
    if (!_imp->backend) {
        return;
    }

    QObject::connect( _imp->backend, SIGNAL( textChunk(QString) ),
                      this, SLOT( onBackendTextChunk(QString) ) );
    QObject::connect( _imp->backend, SIGNAL( toolCall(QString, QString) ),
                      this, SLOT( onBackendToolCall(QString, QString) ) );
    QObject::connect( _imp->backend, SIGNAL( toolResult(QString, bool) ),
                      this, SLOT( onBackendToolResult(QString, bool) ) );
    QObject::connect( _imp->backend, SIGNAL( turnFinished() ),
                      this, SLOT( onBackendTurnFinished() ) );
    QObject::connect( _imp->backend, SIGNAL( errorOccurred(QString) ),
                      this, SLOT( onBackendError(QString) ) );
    QObject::connect( _imp->backend, SIGNAL( finished() ),
                      this, SLOT( onBackendFinished() ) );
}

void
AIChatPanel::updateProviderFooter()
{
    const AIProviderInfo* info = AIProviderRegistry::findById(_imp->connection.providerId);
    const QString name = info ? info->displayName : tr("Provider");
    if (_imp->agentConnected && _imp->backend) {
        const QString model = _imp->connection.model.trimmed();
        _imp->providerLabel->setText(
            tr("powered by %1 (%2%3) - your account; conversation and project content are sent to the provider")
            .arg( _imp->backend->displayName() )
            .arg( _imp->backend->connectionMethodLabel() )
            .arg( model.isEmpty() ? QString()
                                  : ( QString::fromUtf8(" · ") + model ) ) );
        _imp->setStatus( tr("%1 · %2%3 connected")
                         .arg( _imp->backend->displayName() )
                         .arg( _imp->backend->connectionMethodLabel() )
                         .arg( model.isEmpty() ? QString()
                                               : ( QString::fromUtf8(" · ") + model ) ) );
    } else {
        _imp->providerLabel->setText(
            tr("%1 - not connected. Click Connect… to use CLI, an API key, or a custom endpoint.")
            .arg(name) );
        _imp->setStatus( tr("%1 · not connected").arg(name) );
    }
}

void
AIChatPanel::applyConnection(const AIConnectionConfig& config,
                             bool startBackend)
{
    if (_imp->backend) {
        _imp->backend->disconnect(this);
        _imp->backend->stop();
        _imp->backend->deleteLater();
        _imp->backend = 0;
    }

    _imp->connection = config;
    _imp->agentConnected = false;
    AIConnectionSettings::save(_imp->connection);

    _imp->ignoreProviderChange = true;
    const int idx = _imp->providerCombo->findData(config.providerId);
    if (idx >= 0) {
        _imp->providerCombo->setCurrentIndex(idx);
    }
    _imp->ignoreProviderChange = false;

    refreshModelCombo();

    if (!startBackend || ( config.method == eAIConnectionMethodNone )) {
        updateProviderFooter();

        return;
    }

    // Ensure MCP is up without going through ensureStarted() (avoids recursion
    // when ensureStarted auto-connects via this method).
    if (!_imp->server) {
        _imp->server = new AIMcpServer(_imp->gui, this);
        QObject::connect( _imp->server, SIGNAL( destructiveToolRequested(QString, QString, bool*) ),
                          this, SLOT( onDestructiveToolRequested(QString, QString, bool*) ),
                          Qt::DirectConnection );
        if ( !_imp->server->start() ) {
            _imp->setStatus( tr("Could not open the local MCP port.") );
            updateProviderFooter();

            return;
        }
        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#888;\"><i>%1</i><br/>"
                                            "MCP: <code>%2</code><br/>"
                                            "Token: <code>%3</code></p>")
                          .arg( tr("Local MCP server started (loopback only).") )
                          .arg( _imp->server->url() )
                          .arg( _imp->server->token() ) );
    }

    _imp->backend = AIAgentBackend::create(config, this);
    if (!_imp->backend) {
        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#c44;\">%1</p>")
                          .arg( tr("Could not create a backend for this provider/method.") ) );
        updateProviderFooter();

        return;
    }

    connectBackendSignals();

    if ( _imp->backend->start( projectCwd(), _imp->server->url(), _imp->server->token() ) ) {
        _imp->agentConnected = true;
        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#888;\"><i>%1</i></p>")
                          .arg( tr("Connected to %1 via %2.")
                                .arg( _imp->backend->displayName() )
                                .arg( _imp->backend->connectionMethodLabel() ) ) );
    } else {
        _imp->agentConnected = false;
        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#c44;\">%1 "
                                            "<a href=\"#connect\">%2</a></p>")
                          .arg( tr("Could not connect.") )
                          .arg( tr("Open Connect…") ) );
        if (_imp->connectButton) {
            _imp->connectButton->setFocus(Qt::OtherFocusReason);
        }
    }

    updateProviderFooter();
}

void
AIChatPanel::refreshModelCombo()
{
    if (!_imp->modelCombo) {
        return;
    }

    const QString providerId = _imp->connection.providerId.isEmpty()
                               ? _imp->providerCombo->currentData().toString()
                               : _imp->connection.providerId;
    const AIProviderInfo* info = AIProviderRegistry::findById(providerId);
    QString current = _imp->connection.model.trimmed();
    if ( current.isEmpty() && info ) {
        current = info->defaultModel;
    }

    _imp->ignoreModelChange = true;
    _imp->modelCombo->clear();
    const QStringList suggested = AIProviderRegistry::suggestedModels(providerId);
    for (int i = 0; i < suggested.size(); ++i) {
        _imp->modelCombo->addItem(suggested.at(i));
    }
    if ( !current.isEmpty() && ( _imp->modelCombo->findText(current) < 0 ) ) {
        _imp->modelCombo->insertItem(0, current);
    }
    if ( !current.isEmpty() ) {
        _imp->modelCombo->setCurrentText(current);
    } else if (_imp->modelCombo->count() > 0) {
        _imp->modelCombo->setCurrentIndex(0);
        current = _imp->modelCombo->currentText();
    }
    if ( _imp->connection.model != current ) {
        _imp->connection.model = current;
    }
    _imp->ignoreModelChange = false;
}

void
AIChatPanel::applySelectedModel(bool reconnectIfNeeded)
{
    if (!_imp->modelCombo) {
        return;
    }

    const QString model = _imp->modelCombo->currentText().trimmed();
    if ( model.isEmpty() || ( model == _imp->connection.model ) ) {
        return;
    }

    _imp->connection.model = model;
    AIConnectionSettings::save(_imp->connection);

    if (reconnectIfNeeded &&
        _imp->agentConnected &&
        ( _imp->connection.method != eAIConnectionMethodNone )) {
        _imp->userStopped = false;
        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#888;\"><i>%1</i></p>")
                          .arg( tr("Switching model to %1…").arg(model) ) );
        applyConnection(_imp->connection, true);
    } else {
        updateProviderFooter();
    }
}

void
AIChatPanel::onModelComboChanged(const QString& /*model*/)
{
    if (_imp->ignoreModelChange) {
        return;
    }
    applySelectedModel(true);
}

void
AIChatPanel::onModelEditingFinished()
{
    if (_imp->ignoreModelChange) {
        return;
    }
    applySelectedModel(true);
}

bool
AIChatPanel::tryAutoConnect()
{
    if ( _imp->autoConnectBox && !_imp->autoConnectBox->isChecked() ) {
        return false;
    }
    if (_imp->userStopped) {
        return false;
    }
    if (_imp->agentConnected && _imp->backend && _imp->backend->isRunning()) {
        return true;
    }

    AIConnectionConfig config = _imp->connection;
    if ( config.providerId.isEmpty() ) {
        config.providerId = _imp->providerCombo->currentData().toString();
    }
    // Merge latest saved prefs for this provider (keys, method).
    AIConnectionConfig saved = AIConnectionSettings::loadForProvider(config.providerId);
    if ( config.apiKey.isEmpty() ) {
        config.apiKey = saved.apiKey;
    }
    if ( config.method == eAIConnectionMethodNone ) {
        config.method = saved.method;
    }
    if ( config.model.isEmpty() ) {
        config.model = saved.model;
    }
    if ( config.baseUrl.isEmpty() ) {
        config.baseUrl = saved.baseUrl;
    }
    if ( config.cliPath.isEmpty() ) {
        config.cliPath = saved.cliPath;
    }

    if ( !AIConnectionSettings::resolveAutoMethod(config) ) {
        return false;
    }

    config.autoConnect = true;
    applyConnection(config, true);

    return _imp->agentConnected;
}

void
AIChatPanel::ensureStarted()
{
    if (!_imp->server) {
        _imp->server = new AIMcpServer(_imp->gui, this);

        QObject::connect( _imp->server, SIGNAL( destructiveToolRequested(QString, QString, bool*) ),
                          this, SLOT( onDestructiveToolRequested(QString, QString, bool*) ),
                          Qt::DirectConnection );

        if ( !_imp->server->start() ) {
            _imp->setStatus( tr("Could not open the local MCP port.") );

            return;
        }

        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#888;\"><i>%1</i><br/>"
                                            "MCP: <code>%2</code><br/>"
                                            "Token: <code>%3</code></p>")
                          .arg( tr("Local MCP server started (loopback only).") )
                          .arg( _imp->server->url() )
                          .arg( _imp->server->token() ) );
    }

    if (_imp->agentConnected && _imp->backend && _imp->backend->isRunning()) {
        return;
    }

    if ( tryAutoConnect() ) {
        return;
    }

    updateProviderFooter();
    if (!_imp->agentConnected && !_imp->welcomeShown) {
        _imp->welcomeShown = true;
        _imp->appendHtml( QString::fromUtf8("<p><i>%1</i></p>")
                          .arg( tr("Auto-connect could not start a provider. Click Connect… or install a CLI / paste an API key.") ) );
    }
}

void
AIChatPanel::onPanelMadeCurrent()
{
    _imp->userStopped = false;
    ensureStarted();
}

void
AIChatPanel::onConnectClicked()
{
    ensureStarted();

    const QString providerId = _imp->providerCombo->currentData().toString();
    AIProviderConnectDialog dialog(providerId, this);
    if ( dialog.exec() != QDialog::Accepted ) {
        return;
    }

    _imp->userStopped = false;
    AIConnectionConfig cfg = dialog.resultConfig();
    cfg.autoConnect = true;
    applyConnection(cfg, true);
}

void
AIChatPanel::onAutoConnectToggled(bool checked)
{
    AIConnectionSettings::setAutoConnectEnabled(checked);
    if (checked) {
        _imp->userStopped = false;
        tryAutoConnect();
    }
}

void
AIChatPanel::onTranscriptAnchorClicked(const QUrl& url)
{
    if ( url.toString() == QString::fromUtf8("#connect") ) {
        onConnectClicked();
    }
}

void
AIChatPanel::onProviderComboChanged(int index)
{
    Q_UNUSED(index);

    if (_imp->ignoreProviderChange) {
        return;
    }

    const QString providerId = _imp->providerCombo->currentData().toString();
    if (providerId == _imp->connection.providerId && _imp->agentConnected) {
        return;
    }

    if (_imp->backend) {
        _imp->backend->disconnect(this);
        _imp->backend->stop();
        _imp->backend->deleteLater();
        _imp->backend = 0;
    }

    _imp->agentConnected = false;
    _imp->userStopped = false;
    _imp->connection = AIConnectionSettings::loadForProvider(providerId);
    _imp->connection.providerId = providerId;
    AIConnectionSettings::save(_imp->connection);
    refreshModelCombo();

    if ( !tryAutoConnect() ) {
        updateProviderFooter();
        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#888;\"><i>%1</i></p>")
                          .arg( tr("Provider changed. Auto-connect unavailable — click Connect…") ) );
    }
}

void
AIChatPanel::onSendClicked()
{
    const QString text = _imp->input->toPlainText().trimmed();

    if ( text.isEmpty() ) {
        return;
    }

    ensureStarted();

    // Commit any typed model before the turn (editingFinished may not have run).
    applySelectedModel(true);

    if ( !_imp->backend || !_imp->backend->isRunning() ) {
        _imp->userStopped = false;
        if ( !tryAutoConnect() ) {
            _imp->appendHtml( QString::fromUtf8("<p style=\"color:#c44;\">%1</p>")
                              .arg( tr("Not connected. Click Connect… to choose CLI or an API key.") ) );

            return;
        }
    }

    _imp->appendUserBubble(text);
    _imp->input->clear();

    if (_imp->server) {
        _imp->server->beginAgentTransaction( tr("assistant turn") );
    }
    _imp->turnInProgress = true;
    _imp->stopButton->setEnabled(true);
    _imp->sendButton->setEnabled(false);

    const QString context = _imp->visibleContext();
    _imp->backend->send( context.isEmpty() ? text : ( context + QString::fromUtf8("\n\n") + text ) );
}

void
AIChatPanel::onStopClicked()
{
    // Interrupt the turn; do not auto-reconnect until the user reopens the
    // panel, toggles Auto-connect, changes provider, or clicks Connect…
    _imp->userStopped = true;
    if (_imp->backend) {
        _imp->backend->interrupt();
    }
    onBackendTurnFinished();
}

void
AIChatPanel::onBackendTextChunk(const QString& text)
{
    _imp->openAssistantBubble();
    _imp->appendHtml( AIChatPanelPrivate::escape(text)
                      .replace( QString::fromUtf8("\n"), QString::fromUtf8("<br/>") ) );
}

void
AIChatPanel::onBackendToolCall(const QString& name,
                               const QString& argsJson)
{
    Q_UNUSED(argsJson);

    QString shortName = name;
    if ( shortName.startsWith( QString::fromUtf8("mcp__natron__") ) ) {
        shortName = shortName.mid( QString::fromUtf8("mcp__natron__").size() );
    }

    _imp->assistantBubbleOpen = false;
    _imp->appendHtml( QString::fromUtf8("<p style=\"color:#888;\">&#9881; %1&#8230;</p>")
                      .arg( AIChatPanelPrivate::escape(shortName) ) );
}

void
AIChatPanel::onBackendToolResult(const QString& name,
                                 bool ok)
{
    QString shortName = name;

    if ( shortName.startsWith( QString::fromUtf8("mcp__natron__") ) ) {
        shortName = shortName.mid( QString::fromUtf8("mcp__natron__").size() );
    }

    _imp->assistantBubbleOpen = false;
    _imp->appendHtml( QString::fromUtf8("<p style=\"color:%1;\">&#9881; %2 %3</p>")
                      .arg( ok ? QString::fromUtf8("#4a4") : QString::fromUtf8("#c44") )
                      .arg( AIChatPanelPrivate::escape(shortName) )
                      .arg( ok ? QString::fromUtf8("&#10003;") : QString::fromUtf8("&#10007;") ) );
}

void
AIChatPanel::onBackendTurnFinished()
{
    if (!_imp->turnInProgress) {
        return;
    }

    _imp->turnInProgress = false;
    _imp->assistantBubbleOpen = false;

    if (_imp->server) {
        _imp->server->endAgentTransaction();
    }

    _imp->stopButton->setEnabled(false);
    _imp->sendButton->setEnabled(true);
}

void
AIChatPanel::onBackendError(const QString& message)
{
    _imp->assistantBubbleOpen = false;
    _imp->agentConnected = _imp->backend && _imp->backend->isRunning();
    updateProviderFooter();
    _imp->appendHtml( QString::fromUtf8("<p style=\"color:#c44;\"><b>%1</b> %2</p>"
                                        "<p><a href=\"#connect\">%3</a></p>")
                      .arg( tr("Error:") )
                      .arg( AIChatPanelPrivate::escape(message) )
                      .arg( tr("Open Connect…") ) );
    if (_imp->connectButton) {
        _imp->connectButton->setFocus(Qt::OtherFocusReason);
    }
}

void
AIChatPanel::onBackendFinished()
{
    onBackendTurnFinished();
    _imp->agentConnected = false;
    _imp->userStopped = true;
    updateProviderFooter();
    _imp->appendHtml( QString::fromUtf8("<p style=\"color:#888;\"><i>%1</i></p>")
                      .arg( tr("Agent stopped. Reopen this panel or click Connect… to reconnect.") ) );
}

void
AIChatPanel::onDestructiveToolRequested(const QString& toolName,
                                        const QString& summary,
                                        bool* allowed)
{
    if (!allowed) {
        return;
    }

    if ( _imp->autoApprove && _imp->autoApprove->isChecked() ) {
        *allowed = true;

        return;
    }

    const QMessageBox::StandardButton answer =
        QMessageBox::question( this,
                               tr("AI Assistant"),
                               tr("The assistant wants to: %1\n\nAllow it?").arg(summary),
                               QMessageBox::Yes | QMessageBox::No,
                               QMessageBox::No );

    *allowed = (answer == QMessageBox::Yes);

    if (!*allowed) {
        _imp->appendHtml( QString::fromUtf8("<p style=\"color:#c44;\">%1</p>")
                          .arg( tr("Declined: %1").arg( AIChatPanelPrivate::escape(toolName) ) ) );
    }
}

bool
AIChatPanel::eventFilter(QObject* watched,
                         QEvent* event)
{
    if ( ( watched == _imp->input ) && ( event->type() == QEvent::KeyPress ) ) {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if ( ( key->key() == Qt::Key_Return ) || ( key->key() == Qt::Key_Enter ) ) {
            if ( key->modifiers() & Qt::ShiftModifier ) {
                return false;
            }
            onSendClicked();

            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void
AIChatPanel::keyPressEvent(QKeyEvent* e)
{
    if ( ( ( e->key() == Qt::Key_Return ) || ( e->key() == Qt::Key_Enter ) ) &&
         !( e->modifiers() & Qt::ShiftModifier ) ) {
        onSendClicked();
        e->accept();

        return;
    }

    handleUnCaughtKeyPressEvent(e);
    QWidget::keyPressEvent(e);
}

void
AIChatPanel::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyUpEvent(e);
    QWidget::keyReleaseEvent(e);
}

void
AIChatPanel::focusInEvent(QFocusEvent* e)
{
    takeClickFocus();
    QWidget::focusInEvent(e);
}

void
AIChatPanel::mousePressEvent(QMouseEvent* e)
{
    takeClickFocus();
    QWidget::mousePressEvent(e);
}

void
AIChatPanel::enterEvent(QtCompat::QEnterEvent* e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
AIChatPanel::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AIChatPanel.cpp"
