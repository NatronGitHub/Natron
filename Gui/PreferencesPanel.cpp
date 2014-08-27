//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "PreferencesPanel.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QTextDocument> // for Qt::convertFromPlainText
#include <QDialogButtonBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Settings.h"
#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"

PreferencesPanel::PreferencesPanel(boost::shared_ptr<Settings> settings,
                                   Gui *parent)
    : QWidget(parent)
      , _gui(parent)
      , _settings(settings)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setWindowTitle( tr("Preferences") );
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0,0,0,0);
    _mainLayout->setSpacing(0);

    _panel = new DockablePanel(_gui,_settings.get(),_mainLayout,DockablePanel::NO_HEADER,true,
                               "","",false,"",this);
    _mainLayout->addWidget(_panel);

    _buttonBox = new QDialogButtonBox(Qt::Horizontal);
    _applyB = new Button( tr("Apply") );
    _applyB->setToolTip( Qt::convertFromPlainText(tr("Apply changes without closing the window."),Qt::WhiteSpaceNormal) );
    _restoreDefaultsB = new Button( tr("Restore defaults") );
    _restoreDefaultsB->setToolTip( Qt::convertFromPlainText(tr("Restore default values for all preferences."),Qt::WhiteSpaceNormal) );
    _cancelB = new Button( tr("Cancel") );
    _cancelB->setToolTip( Qt::convertFromPlainText(tr("Cancel changes that were not applied and close the window."),Qt::WhiteSpaceNormal) );
    _okB = new Button( tr("OK") );
    _okB->setToolTip( Qt::convertFromPlainText(tr("Apply changes and close the window."),Qt::WhiteSpaceNormal) );
    _buttonBox->addButton(_applyB, QDialogButtonBox::ApplyRole);
    _buttonBox->addButton(_restoreDefaultsB, QDialogButtonBox::ResetRole);
    _buttonBox->addButton(_cancelB, QDialogButtonBox::RejectRole);
    _buttonBox->addButton(_okB, QDialogButtonBox::AcceptRole);

    _mainLayout->addStretch();
    _mainLayout->addWidget(_buttonBox);

    QObject::connect( _restoreDefaultsB, SIGNAL( clicked() ), this, SLOT( restoreDefaults() ) );
    QObject::connect( _applyB, SIGNAL( clicked() ), this, SLOT( applyChanges() ) );
    QObject::connect( _buttonBox, SIGNAL( rejected() ), this, SLOT( cancelChanges() ) );
    QObject::connect( _buttonBox, SIGNAL( accepted() ), this, SLOT( applyChangesAndClose() ) );

    _panel->initializeKnobs();

    resize(700, 400);
}

void
PreferencesPanel::restoreDefaults()
{
    Natron::StandardButton reply = Natron::questionDialog( tr("Preferences").toStdString(),
                                                           tr("Restoring the settings will delete any custom configuration, are you sure you want to do this?").toStdString() );

    if (reply == Natron::Yes) {
        _settings->restoreDefault();
    }
}

void
PreferencesPanel::applyChanges()
{
    _settings->saveSettings();
}

void
PreferencesPanel::cancelChanges()
{
    close();
}

void
PreferencesPanel::applyChangesAndClose()
{
    _settings->saveSettings();
    close();
}

void
PreferencesPanel::showEvent(QShowEvent* /*e*/)
{
    QDesktopWidget* desktop = QApplication::desktop();
    const QRect rect = desktop->screenGeometry();

    move( QPoint(rect.width() / 2 - width() / 2,rect.height() / 2 - height() / 2) );
    _settings->resetWereChangesMadeSinceLastSave();
}

void
PreferencesPanel::closeEvent(QCloseEvent*)
{
    if ( _settings->wereChangesMadeSinceLastSave() ) {
        _settings->restoreSettings();
    }
}

