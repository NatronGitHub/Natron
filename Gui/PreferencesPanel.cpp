//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "PreferencesPanel.h"

#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QTextDocument> // for Qt::convertFromPlainText

#include "Engine/Settings.h"
#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"

PreferencesPanel::PreferencesPanel(boost::shared_ptr<Settings> settings,Gui *parent)
    : QWidget(parent)
    , _gui(parent)
    , _settings(settings)
{
    
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setWindowTitle("Preferences");
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(0,0,0,0);
    _mainLayout->setSpacing(0);
    
    _panel = new DockablePanel(_gui,_settings.get(),_mainLayout,DockablePanel::NO_HEADER,true,
                               "","",false,"",this);
    // _panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _mainLayout->addWidget(_panel);

    _buttonsContainer = new QWidget(this);
    _buttonsLayout = new QHBoxLayout(_buttonsContainer);
    _buttonsLayout->addStretch();
    _restoreDefaultsB = new Button("Restore defaults",_buttonsContainer);
    _restoreDefaultsB->setToolTip(Qt::convertFromPlainText("Restore default values for all preferences.",Qt::WhiteSpaceNormal));
    _cancelB = new Button("Cancel",_buttonsContainer);
    _okB = new Button("Save",_buttonsContainer);
    _buttonsLayout->addWidget(_restoreDefaultsB);
    _buttonsLayout->addWidget(_cancelB);
    _buttonsLayout->addWidget(_okB);
    
    _mainLayout->addStretch();
    _mainLayout->addWidget(_buttonsContainer);
    
    QObject::connect(_restoreDefaultsB, SIGNAL(clicked()), this, SLOT(restoreDefaults()));
    QObject::connect(_cancelB, SIGNAL(clicked()), this, SLOT(cancelChanges()));
    QObject::connect(_okB, SIGNAL(clicked()), this, SLOT(saveChanges()));
    
    _panel->initializeKnobs();
    
    resize(500, 400);
    
}

void PreferencesPanel::restoreDefaults() {
    Natron::StandardButton reply = Natron::questionDialog("Preferences",
                                        "Restoring the settings will delete any custom configuration, are you sure you want to do this?");
    if (reply == Natron::Yes) {
        _settings->restoreDefault();
    }
    
}

void PreferencesPanel::cancelChanges() {
    close();
}

void PreferencesPanel::saveChanges() {
    _settings->saveSettings();
    close();
}

void PreferencesPanel::showEvent(QShowEvent* /*e*/) {
    QDesktopWidget* desktop = QApplication::desktop();
    const QRect rect = desktop->screenGeometry();
    move(QPoint(rect.width() / 2 - width() / 2,rect.height() / 2 - height() / 2));
}

void PreferencesPanel::closeEvent(QCloseEvent*) {
    if (_settings->wereChangesMadeSinceLastSave()) {
        _settings->restoreSettings();
    }
}