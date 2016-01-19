/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

NATRON_NAMESPACE_USING

///what a painful mapping!
Natron::Key
QtEnumConvert::fromQtKey(Qt::Key k)
{
    switch (k) {
    case Qt::Key_Escape:

        return Natron::Key_Escape;
    case Qt::Key_Tab:

        return Natron::Key_Tab;
    case Qt::Key_Clear:

        return Natron::Key_Clear;
    case Qt::Key_Return:

        return Natron::Key_Return;
    case Qt::Key_Pause:

        return Natron::Key_Pause;
    case Qt::Key_Delete:

        return Natron::Key_Delete;

    case Qt::Key_Multi_key:

        return Natron::Key_Multi_key;
    case Qt::Key_SingleCandidate:

        return Natron::Key_SingleCandidate;
    case Qt::Key_MultipleCandidate:

        return Natron::Key_MultipleCandidate;
    case Qt::Key_PreviousCandidate:

        return Natron::Key_PreviousCandidate;

    case Qt::Key_Kanji:

        return Natron::Key_Kanji;
    case Qt::Key_Muhenkan:

        return Natron::Key_Muhenkan;
    case Qt::Key_Henkan:

        return Natron::Key_Henkan;
    case Qt::Key_Romaji:

        return Natron::Key_Romaji;
    case Qt::Key_Hiragana:

        return Natron::Key_Hiragana;
    case Qt::Key_Katakana:

        return Natron::Key_Katakana;
    case Qt::Key_Hiragana_Katakana:

        return Natron::Key_Hiragana_Katakana;
    case Qt::Key_Zenkaku:

        return Natron::Key_Zenkaku;
    case Qt::Key_Hankaku:

        return Natron::Key_Hankaku;
    case Qt::Key_Zenkaku_Hankaku:

        return Natron::Key_Zenkaku_Hankaku;
    case Qt::Key_Touroku:

        return Natron::Key_Touroku;
    case Qt::Key_Massyo:

        return Natron::Key_Massyo;
    case Qt::Key_Kana_Lock:

        return Natron::Key_Kana_Lock;
    case Qt::Key_Kana_Shift:

        return Natron::Key_Kana_Shift;
    case Qt::Key_Eisu_Shift:

        return Natron::Key_Eisu_Shift;
    case Qt::Key_Eisu_toggle:

        return Natron::Key_Eisu_toggle;

    case Qt::Key_Home:

        return Natron::Key_Home;
    case Qt::Key_Left:

        return Natron::Key_Left;
    case Qt::Key_Up:

        return Natron::Key_Up;
    case Qt::Key_Right:

        return Natron::Key_Right;
    case Qt::Key_Down:

        return Natron::Key_Down;
    case Qt::Key_End:

        return Natron::Key_End;

    case Qt::Key_Select:

        return Natron::Key_Select;
    case Qt::Key_Print:

        return Natron::Key_Print;
    case Qt::Key_Execute:

        return Natron::Key_Execute;
    case Qt::Key_Insert:

        return Natron::Key_Insert;
    case Qt::Key_Menu:

        return Natron::Key_Menu;
    case Qt::Key_Cancel:

        return Natron::Key_Cancel;
    case Qt::Key_Help:

        return Natron::Key_Help;
    case Qt::Key_Mode_switch:

        return Natron::Key_Mode_switch;
    case Qt::Key_F1:

        return Natron::Key_F1;
    case Qt::Key_F2:

        return Natron::Key_F2;
    case Qt::Key_F3:

        return Natron::Key_F3;
    case Qt::Key_F4:

        return Natron::Key_F4;
    case Qt::Key_F5:

        return Natron::Key_F5;
    case Qt::Key_F6:

        return Natron::Key_F6;
    case Qt::Key_F7:

        return Natron::Key_F7;
    case Qt::Key_F8:

        return Natron::Key_F8;
    case Qt::Key_F9:

        return Natron::Key_F9;
    case Qt::Key_F10:

        return Natron::Key_F10;
    case Qt::Key_F11:

        return Natron::Key_F11;
    case Qt::Key_F12:

        return Natron::Key_F12;
    case Qt::Key_F13:

        return Natron::Key_F13;
    case Qt::Key_F14:

        return Natron::Key_F14;
    case Qt::Key_F15:

        return Natron::Key_F15;
    case Qt::Key_F16:

        return Natron::Key_F16;
    case Qt::Key_F17:

        return Natron::Key_F17;
    case Qt::Key_F18:

        return Natron::Key_F18;
    case Qt::Key_F19:

        return Natron::Key_F19;
    case Qt::Key_F20:

        return Natron::Key_F20;
    case Qt::Key_F21:

        return Natron::Key_F21;
    case Qt::Key_F22:

        return Natron::Key_F22;
    case Qt::Key_F23:

        return Natron::Key_F23;
    case Qt::Key_F24:

        return Natron::Key_F24;
    case Qt::Key_F25:

        return Natron::Key_F25;
    case Qt::Key_F26:

        return Natron::Key_F26;
    case Qt::Key_F27:

        return Natron::Key_F27;
    case Qt::Key_F28:

        return Natron::Key_F28;
    case Qt::Key_F29:

        return Natron::Key_F29;
    case Qt::Key_F30:

        return Natron::Key_F30;
    case Qt::Key_F31:

        return Natron::Key_F31;
    case Qt::Key_F32:

        return Natron::Key_F32;
    case Qt::Key_F33:

        return Natron::Key_F33;
    case Qt::Key_F34:

        return Natron::Key_F34;
    case Qt::Key_F35:

        return Natron::Key_F35;


    case Qt::Key_Shift:

        return Natron::Key_Shift_L;
    case Qt::Key_Control:

        return Natron::Key_Control_L;
    case Qt::Key_CapsLock:

        return Natron::Key_Caps_Lock;

    case Qt::Key_Meta:

        return Natron::Key_Meta_L;
    case Qt::Key_Alt:

        return Natron::Key_Alt_L;
    case Qt::Key_Super_L:

        return Natron::Key_Super_L;
    case Qt::Key_Super_R:

        return Natron::Key_Super_R;
    case Qt::Key_Hyper_L:

        return Natron::Key_Hyper_L;
    case Qt::Key_Hyper_R:

        return Natron::Key_Hyper_R;

    case Qt::Key_Space:

        return Natron::Key_space;
    case Qt::Key_Exclam:

        return Natron::Key_exclam;
    case Qt::Key_QuoteDbl:

        return Natron::Key_quotedbl;
    case Qt::Key_NumberSign:

        return Natron::Key_numbersign;
    case Qt::Key_Dollar:

        return Natron::Key_dollar;
    case Qt::Key_Percent:

        return Natron::Key_percent;
    case Qt::Key_Ampersand:

        return Natron::Key_ampersand;
    case Qt::Key_Apostrophe:

        return Natron::Key_apostrophe;
    case Qt::Key_ParenLeft:

        return Natron::Key_parenleft;
    case Qt::Key_ParenRight:

        return Natron::Key_parenright;
    case Qt::Key_Asterisk:

        return Natron::Key_asterisk;
    case Qt::Key_Plus:

        return Natron::Key_plus;
    case Qt::Key_Comma:

        return Natron::Key_comma;
    case Qt::Key_Minus:

        return Natron::Key_minus;
    case Qt::Key_Period:

        return Natron::Key_period;
    case Qt::Key_Slash:

        return Natron::Key_slash;
    case Qt::Key_0:

        return Natron::Key_0;
    case Qt::Key_1:

        return Natron::Key_1;
    case Qt::Key_2:

        return Natron::Key_2;
    case Qt::Key_3:

        return Natron::Key_3;
    case Qt::Key_4:

        return Natron::Key_4;
    case Qt::Key_5:

        return Natron::Key_5;
    case Qt::Key_6:

        return Natron::Key_6;
    case Qt::Key_7:

        return Natron::Key_7;
    case Qt::Key_8:

        return Natron::Key_8;
    case Qt::Key_9:

        return Natron::Key_9;
    case Qt::Key_Colon:

        return Natron::Key_colon;
    case Qt::Key_Semicolon:

        return Natron::Key_semicolon;
    case Qt::Key_Less:

        return Natron::Key_less;
    case Qt::Key_Equal:

        return Natron::Key_equal;
    case Qt::Key_Greater:

        return Natron::Key_greater;
    case Qt::Key_Question:

        return Natron::Key_question;
    case Qt::Key_At:

        return Natron::Key_at;
    case Qt::Key_A:

        return Natron::Key_A;
    case Qt::Key_B:

        return Natron::Key_B;
    case Qt::Key_C:

        return Natron::Key_C;
    case Qt::Key_D:

        return Natron::Key_D;
    case Qt::Key_E:

        return Natron::Key_E;
    case Qt::Key_F:

        return Natron::Key_F;
    case Qt::Key_G:

        return Natron::Key_G;
    case Qt::Key_H:

        return Natron::Key_H;
    case Qt::Key_I:

        return Natron::Key_I;
    case Qt::Key_J:

        return Natron::Key_J;
    case Qt::Key_K:

        return Natron::Key_K;
    case Qt::Key_L:

        return Natron::Key_L;
    case Qt::Key_M:

        return Natron::Key_M;
    case Qt::Key_N:

        return Natron::Key_N;
    case Qt::Key_O:

        return Natron::Key_O;
    case Qt::Key_P:

        return Natron::Key_P;
    case Qt::Key_Q:

        return Natron::Key_Q;
    case Qt::Key_R:

        return Natron::Key_R;
    case Qt::Key_S:

        return Natron::Key_S;
    case Qt::Key_T:

        return Natron::Key_T;
    case Qt::Key_U:

        return Natron::Key_U;
    case Qt::Key_V:

        return Natron::Key_V;
    case Qt::Key_W:

        return Natron::Key_W;
    case Qt::Key_X:

        return Natron::Key_X;
    case Qt::Key_Y:

        return Natron::Key_Y;
    case Qt::Key_Z:

        return Natron::Key_Z;
    case Qt::Key_BracketLeft:

        return Natron::Key_bracketleft;
    case Qt::Key_Backslash:

        return Natron::Key_backslash;
    case Qt::Key_BracketRight:

        return Natron::Key_bracketright;
    case Qt::Key_AsciiCircum:

        return Natron::Key_asciicircum;
    case Qt::Key_Underscore:

        return Natron::Key_underscore;
    case Qt::Key_QuoteLeft:

        return Natron::Key_quoteleft;
    case Qt::Key_BraceLeft:

        return Natron::Key_braceleft;
    case Qt::Key_Bar:

        return Natron::Key_bar;
    case Qt::Key_BraceRight:

        return Natron::Key_braceright;
    case Qt::Key_AsciiTilde:

        return Natron::Key_asciitilde;

    case Qt::Key_nobreakspace:

        return Natron::Key_nobreakspace;
    case Qt::Key_exclamdown:

        return Natron::Key_exclamdown;
    case Qt::Key_cent:

        return Natron::Key_cent;
    case Qt::Key_sterling:

        return Natron::Key_sterling;
    case Qt::Key_currency:

        return Natron::Key_currency;
    case Qt::Key_yen:

        return Natron::Key_yen;
    case Qt::Key_brokenbar:

        return Natron::Key_brokenbar;
    case Qt::Key_section:

        return Natron::Key_section;
    case Qt::Key_diaeresis:

        return Natron::Key_diaeresis;
    case Qt::Key_copyright:

        return Natron::Key_copyright;
    case Qt::Key_ordfeminine:

        return Natron::Key_ordfeminine;
    case Qt::Key_guillemotleft:

        return Natron::Key_guillemotleft;
    case Qt::Key_notsign:

        return Natron::Key_notsign;
    case Qt::Key_hyphen:

        return Natron::Key_hyphen;
    case Qt::Key_registered:

        return Natron::Key_registered;
    case Qt::Key_macron:

        return Natron::Key_macron;
    case Qt::Key_degree:

        return Natron::Key_degree;
    case Qt::Key_plusminus:

        return Natron::Key_plusminus;
    case Qt::Key_twosuperior:

        return Natron::Key_twosuperior;
    case Qt::Key_threesuperior:

        return Natron::Key_threesuperior;
    case Qt::Key_acute:

        return Natron::Key_acute;
    case Qt::Key_mu:

        return Natron::Key_mu;
    case Qt::Key_paragraph:

        return Natron::Key_paragraph;
    case Qt::Key_periodcentered:

        return Natron::Key_periodcentered;
    case Qt::Key_cedilla:

        return Natron::Key_cedilla;
    case Qt::Key_onesuperior:

        return Natron::Key_onesuperior;
    case Qt::Key_masculine:

        return Natron::Key_masculine;
    case Qt::Key_guillemotright:

        return Natron::Key_guillemotright;
    case Qt::Key_onequarter:

        return Natron::Key_onequarter;
    case Qt::Key_onehalf:

        return Natron::Key_onehalf;
    case Qt::Key_threequarters:

        return Natron::Key_threequarters;
    case Qt::Key_questiondown:

        return Natron::Key_questiondown;
    case Qt::Key_Agrave:

        return Natron::Key_Agrave;
    case Qt::Key_Aacute:

        return Natron::Key_Aacute;
    case Qt::Key_Acircumflex:

        return Natron::Key_Acircumflex;
    case Qt::Key_Atilde:

        return Natron::Key_Atilde;
    case Qt::Key_Adiaeresis:

        return Natron::Key_Adiaeresis;
    case Qt::Key_Aring:

        return Natron::Key_Aring;
    case Qt::Key_AE:

        return Natron::Key_AE;
    case Qt::Key_Ccedilla:

        return Natron::Key_Ccedilla;
    case Qt::Key_Egrave:

        return Natron::Key_Egrave;
    case Qt::Key_Eacute:

        return Natron::Key_Eacute;
    case Qt::Key_Ecircumflex:

        return Natron::Key_Ecircumflex;
    case Qt::Key_Ediaeresis:

        return Natron::Key_Ediaeresis;
    case Qt::Key_Igrave:

        return Natron::Key_Igrave;
    case Qt::Key_Iacute:

        return Natron::Key_Iacute;
    case Qt::Key_Icircumflex:

        return Natron::Key_Icircumflex;
    case Qt::Key_Idiaeresis:

        return Natron::Key_Idiaeresis;
    case Qt::Key_ETH:

        return Natron::Key_ETH;
    case Qt::Key_Ntilde:

        return Natron::Key_Ntilde;
    case Qt::Key_Ograve:

        return Natron::Key_Ograve;
    case Qt::Key_Oacute:

        return Natron::Key_Oacute;
    case Qt::Key_Ocircumflex:

        return Natron::Key_Ocircumflex;
    case Qt::Key_Otilde:

        return Natron::Key_Otilde;
    case Qt::Key_Odiaeresis:

        return Natron::Key_Odiaeresis;
    case Qt::Key_multiply:

        return Natron::Key_multiply;
    case Qt::Key_Ooblique:

        return Natron::Key_Ooblique;
    case Qt::Key_Ugrave:

        return Natron::Key_Ugrave;
    case Qt::Key_Uacute:

        return Natron::Key_Uacute;
    case Qt::Key_Ucircumflex:

        return Natron::Key_Ucircumflex;
    case Qt::Key_Udiaeresis:

        return Natron::Key_Udiaeresis;
    case Qt::Key_Yacute:

        return Natron::Key_Yacute;
    case Qt::Key_ssharp:

        return Natron::Key_ssharp;
    case Qt::Key_THORN:

        return Natron::Key_thorn;
    case Qt::Key_ydiaeresis:

        return Natron::Key_ydiaeresis;
    default:

        return Natron::Key_Unknown;
    } // switch
} // fromQtKey

Natron::KeyboardModifier
QtEnumConvert::fromQtModifier(Qt::KeyboardModifier m)
{
    switch (m) {
    case Qt::NoModifier:

        return Natron::NoModifier;
        break;
    case Qt::ShiftModifier:

        return Natron::ShiftModifier;
        break;
    case Qt::ControlModifier:

        return Natron::ControlModifier;
        break;
    case Qt::AltModifier:

        return Natron::AltModifier;
        break;
    case Qt::MetaModifier:

        return Natron::MetaModifier;
        break;
    case Qt::KeypadModifier:

        return Natron::KeypadModifier;
        break;
    case Qt::GroupSwitchModifier:

        return Natron::GroupSwitchModifier;
        break;
    case Qt::KeyboardModifierMask:

        return Natron::KeyboardModifierMask;
        break;
    default:

        return Natron::NoModifier;
        break;
    }
}

Natron::KeyboardModifiers
QtEnumConvert::fromQtModifiers(Qt::KeyboardModifiers m)
{
    Natron::KeyboardModifiers ret;

    if ( m.testFlag(Qt::NoModifier) ) {
        ret |= fromQtModifier(Qt::NoModifier);
    }
    if ( m.testFlag(Qt::ShiftModifier) ) {
        ret |= fromQtModifier(Qt::ShiftModifier);
    }
    if ( m.testFlag(Qt::ControlModifier) ) {
        ret |= fromQtModifier(Qt::ControlModifier);
    }
    if ( m.testFlag(Qt::AltModifier) ) {
        ret |= fromQtModifier(Qt::AltModifier);
    }
    if ( m.testFlag(Qt::MetaModifier) ) {
        ret |= fromQtModifier(Qt::MetaModifier);
    }
    if ( m.testFlag(Qt::KeypadModifier) ) {
        ret |= fromQtModifier(Qt::KeypadModifier);
    }
    if ( m.testFlag(Qt::GroupSwitchModifier) ) {
        ret |= fromQtModifier(Qt::GroupSwitchModifier);
    }
    if ( m.testFlag(Qt::KeyboardModifierMask) ) {
        ret |= fromQtModifier(Qt::KeyboardModifierMask);
    }

    return ret;
}

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

