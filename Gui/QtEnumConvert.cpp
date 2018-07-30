/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "QtEnumConvert.h"

#include <QtCore/QDebug>
#include <stdexcept>

NATRON_NAMESPACE_ENTER



StandardButtonEnum
QtEnumConvert::fromQtStandardButton(QMessageBox::StandardButton b)
{
    switch (b) {
    case QMessageBox::NoButton:

        return eStandardButtonNoButton;
        break;
    case QMessageBox::Escape:

        return eStandardButtonEscape;
        break;
    case QMessageBox::Ok:

        return eStandardButtonOk;
        break;
    case QMessageBox::Save:

        return eStandardButtonSave;
        break;
    case QMessageBox::SaveAll:

        return eStandardButtonSaveAll;
        break;
    case QMessageBox::Open:

        return eStandardButtonOpen;
        break;
    case QMessageBox::Yes:

        return eStandardButtonYes;
        break;
    case QMessageBox::YesToAll:

        return eStandardButtonYesToAll;
        break;
    case QMessageBox::No:

        return eStandardButtonNo;
        break;
    case QMessageBox::NoToAll:

        return eStandardButtonNoToAll;
        break;
    case QMessageBox::Abort:

        return eStandardButtonAbort;
        break;
    case QMessageBox::Retry:

        return eStandardButtonRetry;
        break;
    case QMessageBox::Ignore:

        return eStandardButtonIgnore;
        break;
    case QMessageBox::Close:

        return eStandardButtonClose;
        break;
    case QMessageBox::Cancel:

        return eStandardButtonCancel;
        break;
    case QMessageBox::Discard:

        return eStandardButtonDiscard;
        break;
    case QMessageBox::Help:

        return eStandardButtonHelp;
        break;
    case QMessageBox::Apply:

        return eStandardButtonApply;
        break;
    case QMessageBox::Reset:

        return eStandardButtonReset;
        break;
    case QMessageBox::RestoreDefaults:

        return eStandardButtonRestoreDefaults;
        break;
    default:

        return eStandardButtonNoButton;
        break;
    } // switch
} // fromQtStandardButton

QMessageBox::StandardButton
QtEnumConvert::toQtStandardButton(StandardButtonEnum b)
{
    switch (b) {
    case eStandardButtonNoButton:

        return QMessageBox::NoButton;
        break;
    case eStandardButtonEscape:

        return QMessageBox::Escape;
        break;
    case eStandardButtonOk:

        return QMessageBox::Ok;
        break;
    case eStandardButtonSave:

        return QMessageBox::Save;
        break;
    case eStandardButtonSaveAll:

        return QMessageBox::SaveAll;
        break;
    case eStandardButtonOpen:

        return QMessageBox::Open;
        break;
    case eStandardButtonYes:

        return QMessageBox::Yes;
        break;
    case eStandardButtonYesToAll:

        return QMessageBox::YesToAll;
        break;
    case eStandardButtonNo:

        return QMessageBox::No;
        break;
    case eStandardButtonNoToAll:

        return QMessageBox::NoToAll;
        break;
    case eStandardButtonAbort:

        return QMessageBox::Abort;
        break;
    case eStandardButtonRetry:

        return QMessageBox::Retry;
        break;
    case eStandardButtonIgnore:

        return QMessageBox::Ignore;
        break;
    case eStandardButtonClose:

        return QMessageBox::Close;
        break;
    case eStandardButtonCancel:

        return QMessageBox::Cancel;
        break;
    case eStandardButtonDiscard:

        return QMessageBox::Discard;
        break;
    case eStandardButtonHelp:

        return QMessageBox::Help;
        break;
    case eStandardButtonApply:

        return QMessageBox::Apply;
        break;
    case eStandardButtonReset:

        return QMessageBox::Reset;
        break;
    case eStandardButtonRestoreDefaults:

        return QMessageBox::RestoreDefaults;
        break;
    default:

        return QMessageBox::NoButton;
        break;
    } // switch
} // toQtStandardButton

QMessageBox::StandardButtons
QtEnumConvert::toQtStandarButtons(StandardButtons buttons)
{
    QMessageBox::StandardButtons ret;

    if ( buttons.testFlag(eStandardButtonNoButton) ) {
        ret |= QMessageBox::NoButton;
    }
    if ( buttons.testFlag(eStandardButtonEscape) ) {
        ret |= QMessageBox::Escape;
    }
    if ( buttons.testFlag(eStandardButtonOk) ) {
        ret |= QMessageBox::Ok;
    }
    if ( buttons.testFlag(eStandardButtonSave) ) {
        ret |= QMessageBox::Save;
    }
    if ( buttons.testFlag(eStandardButtonSaveAll) ) {
        ret |= QMessageBox::SaveAll;
    }
    if ( buttons.testFlag(eStandardButtonOpen) ) {
        ret |= QMessageBox::Open;
    }
    if ( buttons.testFlag(eStandardButtonYes) ) {
        ret |= QMessageBox::Yes;
    }
    if ( buttons.testFlag(eStandardButtonYesToAll) ) {
        ret |= QMessageBox::YesToAll;
    }
    if ( buttons.testFlag(eStandardButtonNo) ) {
        ret |= QMessageBox::No;
    }
    if ( buttons.testFlag(eStandardButtonNoToAll) ) {
        ret |= QMessageBox::NoToAll;
    }
    if ( buttons.testFlag(eStandardButtonAbort) ) {
        ret |= QMessageBox::Abort;
    }
    if ( buttons.testFlag(eStandardButtonIgnore) ) {
        ret |= QMessageBox::Ignore;
    }
    if ( buttons.testFlag(eStandardButtonRetry) ) {
        ret |= QMessageBox::Retry;
    }
    if ( buttons.testFlag(eStandardButtonClose) ) {
        ret |= QMessageBox::Close;
    }
    if ( buttons.testFlag(eStandardButtonCancel) ) {
        ret |= QMessageBox::Cancel;
    }
    if ( buttons.testFlag(eStandardButtonDiscard) ) {
        ret |= QMessageBox::Discard;
    }
    if ( buttons.testFlag(eStandardButtonHelp) ) {
        ret |= QMessageBox::Help;
    }
    if ( buttons.testFlag(eStandardButtonApply) ) {
        ret |= QMessageBox::Apply;
    }
    if ( buttons.testFlag(eStandardButtonReset) ) {
        ret |= QMessageBox::Reset;
    }
    if ( buttons.testFlag(eStandardButtonRestoreDefaults) ) {
        ret |= QMessageBox::RestoreDefaults;
    }

    return ret;
} // toQtStandarButtons


NATRON_NAMESPACE_EXIT

