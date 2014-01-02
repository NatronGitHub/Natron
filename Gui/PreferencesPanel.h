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

#include <boost/shared_ptr.hpp>
#include <QScrollArea>

class Settings;
class DockablePanel;
class QVBoxLayout;
class PreferencesPanel : public QScrollArea
{
public:
    PreferencesPanel(boost::shared_ptr<Settings> settings,QWidget* parent= NULL);
    ~PreferencesPanel() OVERRIDE {}

private:
    // FIXME: PIMPL
    QVBoxLayout* _mainLayout;
    QWidget* _viewPort;
    QScrollArea* _sa;
    DockablePanel* _panel;
    boost::shared_ptr<Settings> _settings;
};

#endif // PREFERENCESPANEL_H
