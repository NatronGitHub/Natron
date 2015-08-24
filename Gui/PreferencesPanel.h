//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef PREFERENCESPANEL_H
#define PREFERENCESPANEL_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

class Settings;
class DockablePanel;
class QVBoxLayout;
//class QHBoxLayout;
class QDialogButtonBox;
class Button;
class KnobI;
class Gui;
class PreferencesPanel
    : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    PreferencesPanel(boost::shared_ptr<Settings> settings,
                     Gui* parent);
    ~PreferencesPanel() OVERRIDE
    {
    }

public Q_SLOTS:

    void restoreDefaults();

    void cancelChanges();

    void saveChangesAndClose();
    
    void onSettingChanged(KnobI* knob);

private:

    virtual void showEvent(QShowEvent* e) OVERRIDE;
    virtual void closeEvent(QCloseEvent* e) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;

    // FIXME: PIMPL
    Gui* _gui;
    QVBoxLayout* _mainLayout;
    DockablePanel* _panel;
    QDialogButtonBox* _buttonBox;
    Button* _restoreDefaultsB;
    Button* _cancelB;
    Button* _okB;
    boost::shared_ptr<Settings> _settings;
    std::vector<KnobI*> _changedKnobs;
    bool _closeIsOK;
};

#endif // PREFERENCESPANEL_H
