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

#include <stdexcept>

NATRON_NAMESPACE_ENTER;

///what a painful mapping!
Key
QtEnumConvert::fromQtKey(Qt::Key k)
{
    switch (k) {
    case Qt::Key_Escape:

        return Key_Escape;
    case Qt::Key_Tab:

        return Key_Tab;
    case Qt::Key_Clear:

        return Key_Clear;
    case Qt::Key_Return:

        return Key_Return;
    case Qt::Key_Pause:

        return Key_Pause;
    case Qt::Key_Delete:

        return Key_Delete;

    case Qt::Key_Multi_key:

        return Key_Multi_key;
    case Qt::Key_SingleCandidate:

        return Key_SingleCandidate;
    case Qt::Key_MultipleCandidate:

        return Key_MultipleCandidate;
    case Qt::Key_PreviousCandidate:

        return Key_PreviousCandidate;

    case Qt::Key_Kanji:

        return Key_Kanji;
    case Qt::Key_Muhenkan:

        return Key_Muhenkan;
    case Qt::Key_Henkan:

        return Key_Henkan;
    case Qt::Key_Romaji:

        return Key_Romaji;
    case Qt::Key_Hiragana:

        return Key_Hiragana;
    case Qt::Key_Katakana:

        return Key_Katakana;
    case Qt::Key_Hiragana_Katakana:

        return Key_Hiragana_Katakana;
    case Qt::Key_Zenkaku:

        return Key_Zenkaku;
    case Qt::Key_Hankaku:

        return Key_Hankaku;
    case Qt::Key_Zenkaku_Hankaku:

        return Key_Zenkaku_Hankaku;
    case Qt::Key_Touroku:

        return Key_Touroku;
    case Qt::Key_Massyo:

        return Key_Massyo;
    case Qt::Key_Kana_Lock:

        return Key_Kana_Lock;
    case Qt::Key_Kana_Shift:

        return Key_Kana_Shift;
    case Qt::Key_Eisu_Shift:

        return Key_Eisu_Shift;
    case Qt::Key_Eisu_toggle:

        return Key_Eisu_toggle;

    case Qt::Key_Home:

        return Key_Home;
    case Qt::Key_Left:

        return Key_Left;
    case Qt::Key_Up:

        return Key_Up;
    case Qt::Key_Right:

        return Key_Right;
    case Qt::Key_Down:

        return Key_Down;
    case Qt::Key_End:

        return Key_End;

    case Qt::Key_Select:

        return Key_Select;
    case Qt::Key_Print:

        return Key_Print;
    case Qt::Key_Execute:

        return Key_Execute;
    case Qt::Key_Insert:

        return Key_Insert;
    case Qt::Key_Menu:

        return Key_Menu;
    case Qt::Key_Cancel:

        return Key_Cancel;
    case Qt::Key_Help:

        return Key_Help;
    case Qt::Key_Mode_switch:

        return Key_Mode_switch;
    case Qt::Key_F1:

        return Key_F1;
    case Qt::Key_F2:

        return Key_F2;
    case Qt::Key_F3:

        return Key_F3;
    case Qt::Key_F4:

        return Key_F4;
    case Qt::Key_F5:

        return Key_F5;
    case Qt::Key_F6:

        return Key_F6;
    case Qt::Key_F7:

        return Key_F7;
    case Qt::Key_F8:

        return Key_F8;
    case Qt::Key_F9:

        return Key_F9;
    case Qt::Key_F10:

        return Key_F10;
    case Qt::Key_F11:

        return Key_F11;
    case Qt::Key_F12:

        return Key_F12;
    case Qt::Key_F13:

        return Key_F13;
    case Qt::Key_F14:

        return Key_F14;
    case Qt::Key_F15:

        return Key_F15;
    case Qt::Key_F16:

        return Key_F16;
    case Qt::Key_F17:

        return Key_F17;
    case Qt::Key_F18:

        return Key_F18;
    case Qt::Key_F19:

        return Key_F19;
    case Qt::Key_F20:

        return Key_F20;
    case Qt::Key_F21:

        return Key_F21;
    case Qt::Key_F22:

        return Key_F22;
    case Qt::Key_F23:

        return Key_F23;
    case Qt::Key_F24:

        return Key_F24;
    case Qt::Key_F25:

        return Key_F25;
    case Qt::Key_F26:

        return Key_F26;
    case Qt::Key_F27:

        return Key_F27;
    case Qt::Key_F28:

        return Key_F28;
    case Qt::Key_F29:

        return Key_F29;
    case Qt::Key_F30:

        return Key_F30;
    case Qt::Key_F31:

        return Key_F31;
    case Qt::Key_F32:

        return Key_F32;
    case Qt::Key_F33:

        return Key_F33;
    case Qt::Key_F34:

        return Key_F34;
    case Qt::Key_F35:

        return Key_F35;


    case Qt::Key_Shift:

        return Key_Shift_L;
    case Qt::Key_Control:

        return Key_Control_L;
    case Qt::Key_CapsLock:

        return Key_Caps_Lock;

    case Qt::Key_Meta:

        return Key_Meta_L;
    case Qt::Key_Alt:

        return Key_Alt_L;
    case Qt::Key_Super_L:

        return Key_Super_L;
    case Qt::Key_Super_R:

        return Key_Super_R;
    case Qt::Key_Hyper_L:

        return Key_Hyper_L;
    case Qt::Key_Hyper_R:

        return Key_Hyper_R;

    case Qt::Key_Space:

        return Key_space;
    case Qt::Key_Exclam:

        return Key_exclam;
    case Qt::Key_QuoteDbl:

        return Key_quotedbl;
    case Qt::Key_NumberSign:

        return Key_numbersign;
    case Qt::Key_Dollar:

        return Key_dollar;
    case Qt::Key_Percent:

        return Key_percent;
    case Qt::Key_Ampersand:

        return Key_ampersand;
    case Qt::Key_Apostrophe:

        return Key_apostrophe;
    case Qt::Key_ParenLeft:

        return Key_parenleft;
    case Qt::Key_ParenRight:

        return Key_parenright;
    case Qt::Key_Asterisk:

        return Key_asterisk;
    case Qt::Key_Plus:

        return Key_plus;
    case Qt::Key_Comma:

        return Key_comma;
    case Qt::Key_Minus:

        return Key_minus;
    case Qt::Key_Period:

        return Key_period;
    case Qt::Key_Slash:

        return Key_slash;
    case Qt::Key_0:

        return Key_0;
    case Qt::Key_1:

        return Key_1;
    case Qt::Key_2:

        return Key_2;
    case Qt::Key_3:

        return Key_3;
    case Qt::Key_4:

        return Key_4;
    case Qt::Key_5:

        return Key_5;
    case Qt::Key_6:

        return Key_6;
    case Qt::Key_7:

        return Key_7;
    case Qt::Key_8:

        return Key_8;
    case Qt::Key_9:

        return Key_9;
    case Qt::Key_Colon:

        return Key_colon;
    case Qt::Key_Semicolon:

        return Key_semicolon;
    case Qt::Key_Less:

        return Key_less;
    case Qt::Key_Equal:

        return Key_equal;
    case Qt::Key_Greater:

        return Key_greater;
    case Qt::Key_Question:

        return Key_question;
    case Qt::Key_At:

        return Key_at;
    case Qt::Key_A:

        return Key_A;
    case Qt::Key_B:

        return Key_B;
    case Qt::Key_C:

        return Key_C;
    case Qt::Key_D:

        return Key_D;
    case Qt::Key_E:

        return Key_E;
    case Qt::Key_F:

        return Key_F;
    case Qt::Key_G:

        return Key_G;
    case Qt::Key_H:

        return Key_H;
    case Qt::Key_I:

        return Key_I;
    case Qt::Key_J:

        return Key_J;
    case Qt::Key_K:

        return Key_K;
    case Qt::Key_L:

        return Key_L;
    case Qt::Key_M:

        return Key_M;
    case Qt::Key_N:

        return Key_N;
    case Qt::Key_O:

        return Key_O;
    case Qt::Key_P:

        return Key_P;
    case Qt::Key_Q:

        return Key_Q;
    case Qt::Key_R:

        return Key_R;
    case Qt::Key_S:

        return Key_S;
    case Qt::Key_T:

        return Key_T;
    case Qt::Key_U:

        return Key_U;
    case Qt::Key_V:

        return Key_V;
    case Qt::Key_W:

        return Key_W;
    case Qt::Key_X:

        return Key_X;
    case Qt::Key_Y:

        return Key_Y;
    case Qt::Key_Z:

        return Key_Z;
    case Qt::Key_BracketLeft:

        return Key_bracketleft;
    case Qt::Key_Backslash:

        return Key_backslash;
    case Qt::Key_BracketRight:

        return Key_bracketright;
    case Qt::Key_AsciiCircum:

        return Key_asciicircum;
    case Qt::Key_Underscore:

        return Key_underscore;
    case Qt::Key_QuoteLeft:

        return Key_quoteleft;
    case Qt::Key_BraceLeft:

        return Key_braceleft;
    case Qt::Key_Bar:

        return Key_bar;
    case Qt::Key_BraceRight:

        return Key_braceright;
    case Qt::Key_AsciiTilde:

        return Key_asciitilde;

    case Qt::Key_nobreakspace:

        return Key_nobreakspace;
    case Qt::Key_exclamdown:

        return Key_exclamdown;
    case Qt::Key_cent:

        return Key_cent;
    case Qt::Key_sterling:

        return Key_sterling;
    case Qt::Key_currency:

        return Key_currency;
    case Qt::Key_yen:

        return Key_yen;
    case Qt::Key_brokenbar:

        return Key_brokenbar;
    case Qt::Key_section:

        return Key_section;
    case Qt::Key_diaeresis:

        return Key_diaeresis;
    case Qt::Key_copyright:

        return Key_copyright;
    case Qt::Key_ordfeminine:

        return Key_ordfeminine;
    case Qt::Key_guillemotleft:

        return Key_guillemotleft;
    case Qt::Key_notsign:

        return Key_notsign;
    case Qt::Key_hyphen:

        return Key_hyphen;
    case Qt::Key_registered:

        return Key_registered;
    case Qt::Key_macron:

        return Key_macron;
    case Qt::Key_degree:

        return Key_degree;
    case Qt::Key_plusminus:

        return Key_plusminus;
    case Qt::Key_twosuperior:

        return Key_twosuperior;
    case Qt::Key_threesuperior:

        return Key_threesuperior;
    case Qt::Key_acute:

        return Key_acute;
    case Qt::Key_mu:

        return Key_mu;
    case Qt::Key_paragraph:

        return Key_paragraph;
    case Qt::Key_periodcentered:

        return Key_periodcentered;
    case Qt::Key_cedilla:

        return Key_cedilla;
    case Qt::Key_onesuperior:

        return Key_onesuperior;
    case Qt::Key_masculine:

        return Key_masculine;
    case Qt::Key_guillemotright:

        return Key_guillemotright;
    case Qt::Key_onequarter:

        return Key_onequarter;
    case Qt::Key_onehalf:

        return Key_onehalf;
    case Qt::Key_threequarters:

        return Key_threequarters;
    case Qt::Key_questiondown:

        return Key_questiondown;
    case Qt::Key_Agrave:

        return Key_Agrave;
    case Qt::Key_Aacute:

        return Key_Aacute;
    case Qt::Key_Acircumflex:

        return Key_Acircumflex;
    case Qt::Key_Atilde:

        return Key_Atilde;
    case Qt::Key_Adiaeresis:

        return Key_Adiaeresis;
    case Qt::Key_Aring:

        return Key_Aring;
    case Qt::Key_AE:

        return Key_AE;
    case Qt::Key_Ccedilla:

        return Key_Ccedilla;
    case Qt::Key_Egrave:

        return Key_Egrave;
    case Qt::Key_Eacute:

        return Key_Eacute;
    case Qt::Key_Ecircumflex:

        return Key_Ecircumflex;
    case Qt::Key_Ediaeresis:

        return Key_Ediaeresis;
    case Qt::Key_Igrave:

        return Key_Igrave;
    case Qt::Key_Iacute:

        return Key_Iacute;
    case Qt::Key_Icircumflex:

        return Key_Icircumflex;
    case Qt::Key_Idiaeresis:

        return Key_Idiaeresis;
    case Qt::Key_ETH:

        return Key_ETH;
    case Qt::Key_Ntilde:

        return Key_Ntilde;
    case Qt::Key_Ograve:

        return Key_Ograve;
    case Qt::Key_Oacute:

        return Key_Oacute;
    case Qt::Key_Ocircumflex:

        return Key_Ocircumflex;
    case Qt::Key_Otilde:

        return Key_Otilde;
    case Qt::Key_Odiaeresis:

        return Key_Odiaeresis;
    case Qt::Key_multiply:

        return Key_multiply;
    case Qt::Key_Ooblique:

        return Key_Ooblique;
    case Qt::Key_Ugrave:

        return Key_Ugrave;
    case Qt::Key_Uacute:

        return Key_Uacute;
    case Qt::Key_Ucircumflex:

        return Key_Ucircumflex;
    case Qt::Key_Udiaeresis:

        return Key_Udiaeresis;
    case Qt::Key_Yacute:

        return Key_Yacute;
    case Qt::Key_ssharp:

        return Key_ssharp;
    case Qt::Key_THORN:

        return Key_thorn;
    case Qt::Key_ydiaeresis:

        return Key_ydiaeresis;
    default:

        return Key_Unknown;
    } // switch
} // fromQtKey

KeyboardModifierEnum
QtEnumConvert::fromQtModifier(Qt::KeyboardModifier m)
{
    switch (m) {
    case Qt::NoModifier:

        return eKeyboardModifierNone;
        break;
    case Qt::ShiftModifier:

        return eKeyboardModifierShift;
        break;
    case Qt::ControlModifier:

        return eKeyboardModifierControl;
        break;
    case Qt::AltModifier:

        return eKeyboardModifierAlt;
        break;
    case Qt::MetaModifier:

        return eKeyboardModifierMeta;
        break;
    case Qt::KeypadModifier:

        return eKeyboardModifierKeypad;
        break;
    case Qt::GroupSwitchModifier:

        return eKeyboardModifierGroupSwitch;
        break;
    case Qt::KeyboardModifierMask:

        return eKeyboardModifierMask;
        break;
    default:

        return eKeyboardModifierNone;
        break;
    }
}

KeyboardModifiers
QtEnumConvert::fromQtModifiers(Qt::KeyboardModifiers m)
{
    KeyboardModifiers ret;

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

NATRON_NAMESPACE_EXIT;

