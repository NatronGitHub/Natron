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

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QWidget>
CLANG_DIAG_ON(deprecated)

#include "Global/Macros.h"

class Settings;
class DockablePanel;
class QVBoxLayout;
class QHBoxLayout;
class Button;
class Gui;
class PreferencesPanel : public QWidget
{
    Q_OBJECT
    
public:
    PreferencesPanel(boost::shared_ptr<Settings> settings,Gui* parent);
    ~PreferencesPanel() OVERRIDE {}

public slots:
    
    void cancelChanges();
    
    void saveChanges();
private:
    
    virtual void showEvent(QShowEvent* e) OVERRIDE;
    
    virtual void closeEvent(QCloseEvent* e) OVERRIDE;
    
    // FIXME: PIMPL
    Gui* _gui;
    QVBoxLayout* _mainLayout;
    DockablePanel* _panel;
    QWidget* _buttonsContainer;
    QHBoxLayout* _buttonsLayout;
    Button* _cancelB;
    Button* _okB;
    boost::shared_ptr<Settings> _settings;
};

#endif // PREFERENCESPANEL_H
