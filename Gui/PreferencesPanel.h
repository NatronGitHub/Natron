//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef PREFERENCESPANEL_H
#define PREFERENCESPANEL_H

#include <QWidget>
#include <boost/shared_ptr.hpp>

class Settings;
class DockablePanel;
class QVBoxLayout;
class PreferencesPanel : public QWidget
{
    QVBoxLayout* _mainLayout;
    DockablePanel* _panel;
    boost::shared_ptr<Settings> _settings;

public:
    PreferencesPanel(boost::shared_ptr<Settings> settings,QWidget* parent= NULL);
};

#endif // PREFERENCESPANEL_H
