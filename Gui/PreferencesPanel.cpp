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
#include <QScrollArea>
#include "Engine/Settings.h"
#include "Gui/DockablePanel.h"

PreferencesPanel::PreferencesPanel(boost::shared_ptr<Settings> settings,QWidget *parent)
    : QScrollArea(parent)
    , _settings(settings)
{
    
    _viewPort = new QWidget(this);
    _viewPort->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setWindowTitle("Preferences");
    _mainLayout = new QVBoxLayout(_viewPort);
    _mainLayout->setContentsMargins(0,0,0,0);
    _mainLayout->setSpacing(0);

    setWidget(_viewPort);
    setWidgetResizable(true);
    
    _panel = new DockablePanel(_settings.get(),_mainLayout,DockablePanel::NO_HEADER,
                               "","",false,"",this);
    // _panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _mainLayout->addWidget(_panel);

    _panel->initializeKnobs();
    resize(640, 480);
}
