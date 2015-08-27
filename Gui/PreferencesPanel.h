/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef PREFERENCESPANEL_H
#define PREFERENCESPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
