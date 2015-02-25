//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "PreferencesPanel.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QTextDocument> // for Qt::convertFromPlainText
#include <QDialogButtonBox>
#include <QKeyEvent>
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
      , _changedKnobs()
      , _closeIsOK(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setWindowFlags(Qt::Window);
    setWindowTitle( tr("Preferences") );
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0,0,0,0);
    _mainLayout->setSpacing(0);

    _panel = new DockablePanel(_gui,_settings.get(), _mainLayout, DockablePanel::eHeaderModeNoHeader,true,
                               "", "", false,"", this);
    _panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _mainLayout->addWidget(_panel);

    _buttonBox = new QDialogButtonBox(Qt::Horizontal);
    _restoreDefaultsB = new Button( tr("Restore defaults") );
    _restoreDefaultsB->setToolTip( Qt::convertFromPlainText(tr("Restore default values for all preferences."), Qt::WhiteSpaceNormal) );
    _cancelB = new Button( tr("Discard") );
    _cancelB->setToolTip( Qt::convertFromPlainText(tr("Cancel changes that were not saved and close the window."), Qt::WhiteSpaceNormal) );
    _okB = new Button( tr("Save") );
    _okB->setToolTip( Qt::convertFromPlainText(tr("Save changes on disk and close the window."), Qt::WhiteSpaceNormal) );
    _buttonBox->addButton(_restoreDefaultsB, QDialogButtonBox::ResetRole);
    _buttonBox->addButton(_cancelB, QDialogButtonBox::RejectRole);
    _buttonBox->addButton(_okB, QDialogButtonBox::AcceptRole);

   // _mainLayout->addStretch();
    _mainLayout->addWidget(_buttonBox);

    QObject::connect( _restoreDefaultsB, SIGNAL( clicked() ), this, SLOT( restoreDefaults() ) );
    QObject::connect( _buttonBox, SIGNAL( rejected() ), this, SLOT( cancelChanges() ) );
    QObject::connect( _buttonBox, SIGNAL( accepted() ), this, SLOT( saveChangesAndClose() ) );
    QObject::connect(_settings.get(), SIGNAL(settingChanged(KnobI*)), this,SLOT(onSettingChanged(KnobI*)));

    _panel->initializeKnobs();

    resize(900, 600);
}

void
PreferencesPanel::onSettingChanged(KnobI* knob)
{
    for (U32 i = 0; i < _changedKnobs.size(); ++i) {
        if (_changedKnobs[i] == knob) {
            return;
        }
    }
    _changedKnobs.push_back(knob);
}

void
PreferencesPanel::restoreDefaults()
{
    Natron::StandardButtonEnum reply = Natron::questionDialog( tr("Preferences").toStdString(),
                                                           tr("Restoring the settings will delete any custom configuration, are you sure you want to do this?").toStdString(), false );

    if (reply == Natron::eStandardButtonYes) {
        _settings->restoreDefault();
    }
}

void
PreferencesPanel::cancelChanges()
{
    _closeIsOK = false;
    close();
}

void
PreferencesPanel::saveChangesAndClose()
{
    ///Steal focus from other widgets so that we are sure all LineEdits and Spinboxes get the focusOut event and their editingFinished
    ///signal is emitted.
    _okB->setFocus();
    _settings->saveSettings(_changedKnobs,true);
    _closeIsOK = true;
    close();
}

void
PreferencesPanel::showEvent(QShowEvent* /*e*/)
{
    
    QDesktopWidget* desktop = QApplication::desktop();
    const QRect rect = desktop->screenGeometry();

    move( QPoint(rect.width() / 2 - width() / 2,rect.height() / 2 - height() / 2) );
    
    _changedKnobs.clear();
}

void
PreferencesPanel::closeEvent(QCloseEvent*)
{
    if ( !_closeIsOK && !_changedKnobs.empty() ) {
        _settings->beginChanges();
        _settings->restoreKnobsFromSettings(_changedKnobs);
        _settings->endChanges();
    }
}

void
PreferencesPanel::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        _closeIsOK = false;
        close();
    } else {
        QWidget::keyPressEvent(e);
    }
}

