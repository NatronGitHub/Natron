/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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


Key
QtEnumConvert::fromQtKey(Qt::Key k)
{
    switch (k) {
        case Qt::Key_Escape:

            return Key_Escape;
        case Qt::Key_Backspace:

            return Key_BackSpace;
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

Qt::Key
QtEnumConvert::toQtKey(Key k)
{
    switch (k) {
        case Key_Escape:

            return Qt::Key_Escape;
        case Key_BackSpace:

            return Qt::Key_Backspace;
        case Key_Tab:

            return Qt::Key_Tab;
        case Key_Clear:

            return Qt::Key_Clear;
        case Key_Return:

            return Qt::Key_Return;
        case Key_Pause:

            return Qt::Key_Pause;
        case Key_Delete:

            return Qt::Key_Delete;

        case Key_Multi_key:

            return Qt::Key_Multi_key;
        case Key_SingleCandidate:

            return Qt::Key_SingleCandidate;
        case Key_MultipleCandidate:

            return Qt::Key_MultipleCandidate;
        case Key_PreviousCandidate:

            return Qt::Key_PreviousCandidate;

        case Key_Kanji:

            return Qt::Key_Kanji;
        case Key_Muhenkan:

            return Qt::Key_Muhenkan;
        case Key_Henkan:

            return Qt::Key_Henkan;
        case Key_Romaji:

            return Qt::Key_Romaji;
        case Key_Hiragana:

            return Qt::Key_Hiragana;
        case Key_Katakana:

            return Qt::Key_Katakana;
        case Key_Hiragana_Katakana:

            return Qt::Key_Hiragana_Katakana;
        case Key_Zenkaku:

            return Qt::Key_Zenkaku;
        case Key_Hankaku:

            return Qt::Key_Hankaku;
        case Key_Zenkaku_Hankaku:

            return Qt::Key_Zenkaku_Hankaku;
        case Key_Touroku:

            return Qt::Key_Touroku;
        case Key_Massyo:

            return Qt::Key_Massyo;
        case Key_Kana_Lock:

            return Qt::Key_Kana_Lock;
        case Key_Kana_Shift:

            return Qt::Key_Kana_Shift;
        case Key_Eisu_Shift:

            return Qt::Key_Eisu_Shift;
        case Key_Eisu_toggle:

            return Qt::Key_Eisu_toggle;

        case Key_Home:

            return Qt::Key_Home;
        case Key_Left:

            return Qt::Key_Left;
        case Key_Up:

            return Qt::Key_Up;
        case Key_Right:

            return Qt::Key_Right;
        case Key_Down:

            return Qt::Key_Down;
        case Key_End:

            return Qt::Key_End;

        case Key_Select:

            return Qt::Key_Select;
        case Key_Print:

            return Qt::Key_Print;
        case Key_Execute:

            return Qt::Key_Execute;
        case Key_Insert:

            return Qt::Key_Insert;
        case Key_Menu:

            return Qt::Key_Menu;
        case Key_Cancel:

            return Qt::Key_Cancel;
        case Key_Help:

            return Qt::Key_Help;
        case Key_Mode_switch:

            return Qt::Key_Mode_switch;
        case Key_F1:

            return Qt::Key_F1;
        case Key_F2:

            return Qt::Key_F2;
        case Key_F3:

            return Qt::Key_F3;
        case Key_F4:

            return Qt::Key_F4;
        case Key_F5:

            return Qt::Key_F5;
        case Key_F6:

            return Qt::Key_F6;
        case Key_F7:

            return Qt::Key_F7;
        case Key_F8:

            return Qt::Key_F8;
        case Key_F9:

            return Qt::Key_F9;
        case Key_F10:

            return Qt::Key_F10;
        case Key_F11:

            return Qt::Key_F11;
        case Key_F12:

            return Qt::Key_F12;
        case Key_F13:

            return Qt::Key_F13;
        case Key_F14:

            return Qt::Key_F14;
        case Key_F15:

            return Qt::Key_F15;
        case Key_F16:

            return Qt::Key_F16;
        case Key_F17:

            return Qt::Key_F17;
        case Key_F18:

            return Qt::Key_F18;
        case Key_F19:

            return Qt::Key_F19;
        case Key_F20:

            return Qt::Key_F20;
        case Key_F21:

            return Qt::Key_F21;
        case Key_F22:

            return Qt::Key_F22;
        case Key_F23:

            return Qt::Key_F23;
        case Key_F24:

            return Qt::Key_F24;
        case Key_F25:

            return Qt::Key_F25;
        case Key_F26:

            return Qt::Key_F26;
        case Key_F27:

            return Qt::Key_F27;
        case Key_F28:

            return Qt::Key_F28;
        case Key_F29:

            return Qt::Key_F29;
        case Key_F30:

            return Qt::Key_F30;
        case Key_F31:

            return Qt::Key_F31;
        case Key_F32:

            return Qt::Key_F32;
        case Key_F33:

            return Qt::Key_F33;
        case Key_F34:

            return Qt::Key_F34;
        case Key_F35:

            return Qt::Key_F35;


        case Key_Shift_L:
        case Key_Shift_R:

            return Qt::Key_Shift;
        case Key_Control_L:
        case Key_Control_R:

            return Qt::Key_Control;
        case Key_Caps_Lock:

            return Qt::Key_CapsLock;

        case Key_Meta_L:
        case Key_Meta_R:

            return Qt::Key_Meta;
        case Key_Alt_L:
        case Key_Alt_R:

            return Qt::Key_Alt;
        case Key_Super_L:

            return Qt::Key_Super_L;
        case Key_Super_R:

            return Qt::Key_Super_R;
        case Key_Hyper_L:

            return Qt::Key_Hyper_L;
        case Key_Hyper_R:

            return Qt::Key_Hyper_R;

        case Key_space:

            return Qt::Key_Space;
        case Key_exclam:

            return Qt::Key_Exclam;
        case Key_quotedbl:

            return Qt::Key_QuoteDbl;
        case Key_numbersign:

            return Qt::Key_NumberSign;
        case Key_dollar:

            return Qt::Key_Dollar;
        case Key_percent:

            return Qt::Key_Percent;
        case Key_ampersand:

            return Qt::Key_Ampersand;
        case Key_apostrophe:

            return Qt::Key_Apostrophe;
        case Key_parenleft:

            return Qt::Key_ParenLeft;
        case Key_parenright:

            return Qt::Key_ParenRight;
        case Key_asterisk:

            return Qt::Key_Asterisk;
        case Key_plus:
            
            return Qt::Key_Plus;
        case Key_comma:
            
            return Qt::Key_Comma;
        case Key_minus:
            
            return Qt::Key_Minus;
        case Key_period:
            
            return Qt::Key_Period;
        case Key_slash:
            
            return Qt::Key_Slash;
        case Key_0:
            
            return Qt::Key_0;
        case Key_1:
            
            return Qt::Key_1;
        case Key_2:
            
            return Qt::Key_2;
        case Key_3:
            
            return Qt::Key_3;
        case Key_4:
            
            return Qt::Key_4;
        case Key_5:
            
            return Qt::Key_5;
        case Key_6:
            
            return Qt::Key_6;
        case Key_7:
            
            return Qt::Key_7;
        case Key_8:
            
            return Qt::Key_8;
        case Key_9:
            
            return Qt::Key_9;
        case Key_colon:
            
            return Qt::Key_Colon;
        case Key_semicolon:
            
            return Qt::Key_Semicolon;
        case Key_less:
            
            return Qt::Key_Less;
        case Key_equal:
            
            return Qt::Key_Equal;
        case Key_greater:
            
            return Qt::Key_Greater;
        case Key_question:
            
            return Qt::Key_Question;
        case Key_at:
            
            return Qt::Key_At;
        case Key_A:
            
            return Qt::Key_A;
        case Key_B:
            
            return Qt::Key_B;
        case Key_C:
            
            return Qt::Key_C;
        case Key_D:
            
            return Qt::Key_D;
        case Key_E:
            
            return Qt::Key_E;
        case Key_F:
            
            return Qt::Key_F;
        case Key_G:
            
            return Qt::Key_G;
        case Key_H:
            
            return Qt::Key_H;
        case Key_I:
            
            return Qt::Key_I;
        case Key_J:
            
            return Qt::Key_J;
        case Key_K:
            
            return Qt::Key_K;
        case Key_L:
            
            return Qt::Key_L;
        case Key_M:
            
            return Qt::Key_M;
        case Key_N:
            
            return Qt::Key_N;
        case Key_O:
            
            return Qt::Key_O;
        case Key_P:
            
            return Qt::Key_P;
        case Key_Q:
            
            return Qt::Key_Q;
        case Key_R:
            
            return Qt::Key_R;
        case Key_S:
            
            return Qt::Key_S;
        case Key_T:
            
            return Qt::Key_T;
        case Key_U:
            
            return Qt::Key_U;
        case Key_V:
            
            return Qt::Key_V;
        case Key_W:
            
            return Qt::Key_W;
        case Key_X:
            
            return Qt::Key_X;
        case Key_Y:
            
            return Qt::Key_Y;
        case Key_Z:
            
            return Qt::Key_Z;
        case Key_bracketleft:
            
            return Qt::Key_BracketLeft;
        case Key_backslash:
            
            return Qt::Key_Backslash;
        case Key_bracketright:
            
            return Qt::Key_BracketRight;
        case Key_asciicircum:
            
            return Qt::Key_AsciiCircum;
        case Key_underscore:
            
            return Qt::Key_Underscore;
        case Key_quoteleft:
            
            return Qt::Key_QuoteLeft;
        case Key_braceleft:
            
            return Qt::Key_BraceLeft;
        case Key_bar:
            
            return Qt::Key_Bar;
        case Key_braceright:
            
            return Qt::Key_BraceRight;
        case Key_asciitilde:
            
            return Qt::Key_AsciiTilde;
            
        case Key_nobreakspace:
            
            return Qt::Key_nobreakspace;
        case Key_exclamdown:
            
            return Qt::Key_exclamdown;
        case Key_cent:
            
            return Qt::Key_cent;
        case Key_sterling:
            
            return Qt::Key_sterling;
        case Key_currency:
            
            return Qt::Key_currency;
        case Key_yen:
            
            return Qt::Key_yen;
        case Key_brokenbar:
            
            return Qt::Key_brokenbar;
        case Key_section:
            
            return Qt::Key_section;
        case Key_diaeresis:
            
            return Qt::Key_diaeresis;
        case Key_copyright:
            
            return Qt::Key_copyright;
        case Key_ordfeminine:
            
            return Qt::Key_ordfeminine;
        case Key_guillemotleft:
            
            return Qt::Key_guillemotleft;
        case Key_notsign:
            
            return Qt::Key_notsign;
        case Key_hyphen:
            
            return Qt::Key_hyphen;
        case Key_registered:
            
            return Qt::Key_registered;
        case Key_macron:
            
            return Qt::Key_macron;
        case Key_degree:
            
            return Qt::Key_degree;
        case Key_plusminus:
            
            return Qt::Key_plusminus;
        case Key_twosuperior:
            
            return Qt::Key_twosuperior;
        case Key_threesuperior:
            
            return Qt::Key_threesuperior;
        case Key_acute:
            
            return Qt::Key_acute;
        case Key_mu:
            
            return Qt::Key_mu;
        case Key_paragraph:
            
            return Qt::Key_paragraph;
        case Key_periodcentered:
            
            return Qt::Key_periodcentered;
        case Key_cedilla:
            
            return Qt::Key_cedilla;
        case Key_onesuperior:
            
            return Qt::Key_onesuperior;
        case Key_masculine:
            
            return Qt::Key_masculine;
        case Key_guillemotright:
            
            return Qt::Key_guillemotright;
        case Key_onequarter:
            
            return Qt::Key_onequarter;
        case Key_onehalf:
            
            return Qt::Key_onehalf;
        case Key_threequarters:
            
            return Qt::Key_threequarters;
        case Key_questiondown:
            
            return Qt::Key_questiondown;
        case Key_Agrave:
            
            return Qt::Key_Agrave;
        case Key_Aacute:
            
            return Qt::Key_Aacute;
        case Key_Acircumflex:
            
            return Qt::Key_Acircumflex;
        case Key_Atilde:
            
            return Qt::Key_Atilde;
        case Key_Adiaeresis:
            
            return Qt::Key_Adiaeresis;
        case Key_Aring:
            
            return Qt::Key_Aring;
        case Key_AE:
            
            return Qt::Key_AE;
        case Key_Ccedilla:
            
            return Qt::Key_Ccedilla;
        case Key_Egrave:
            
            return Qt::Key_Egrave;
        case Key_Eacute:
            
            return Qt::Key_Eacute;
        case Key_Ecircumflex:
            
            return Qt::Key_Ecircumflex;
        case Key_Ediaeresis:
            
            return Qt::Key_Ediaeresis;
        case Key_Igrave:
            
            return Qt::Key_Igrave;
        case Key_Iacute:
            
            return Qt::Key_Iacute;
        case Key_Icircumflex:
            
            return Qt::Key_Icircumflex;
        case Key_Idiaeresis:
            
            return Qt::Key_Idiaeresis;
        case Key_ETH:
            
            return Qt::Key_ETH;
        case Key_Ntilde:
            
            return Qt::Key_Ntilde;
        case Key_Ograve:
            
            return Qt::Key_Ograve;
        case Key_Oacute:
            
            return Qt::Key_Oacute;
        case Key_Ocircumflex:
            
            return Qt::Key_Ocircumflex;
        case Key_Otilde:
            
            return Qt::Key_Otilde;
        case Key_Odiaeresis:
            
            return Qt::Key_Odiaeresis;
        case Key_multiply:
            
            return Qt::Key_multiply;
        case Key_Ooblique:
            
            return Qt::Key_Ooblique;
        case Key_Ugrave:
            
            return Qt::Key_Ugrave;
        case Key_Uacute:
            
            return Qt::Key_Uacute;
        case Key_Ucircumflex:
            
            return Qt::Key_Ucircumflex;
        case Key_Udiaeresis:
            
            return Qt::Key_Udiaeresis;
        case Key_Yacute:
            
            return Qt::Key_Yacute;
        case Key_ssharp:
            
            return Qt::Key_ssharp;
        case Key_thorn:
            
            return Qt::Key_THORN;
        case Key_ydiaeresis:
            
            return Qt::Key_ydiaeresis;
        default:
            
            return (Qt::Key)0;
    } // switch
} // toQtKey


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

Qt::KeyboardModifier
QtEnumConvert::toQtModifier(KeyboardModifierEnum m)
{
    switch (m) {
        case eKeyboardModifierNone:

            return Qt::NoModifier;
            break;
        case eKeyboardModifierShift:

            return Qt::ShiftModifier;
            break;
        case eKeyboardModifierControl:

            return Qt::ControlModifier;
            break;
        case eKeyboardModifierAlt:

            return Qt::AltModifier;
            break;
        case eKeyboardModifierMeta:

            return Qt::MetaModifier;
            break;
        case eKeyboardModifierKeypad:

            return Qt::KeypadModifier;
            break;
        case eKeyboardModifierGroupSwitch:

            return Qt::GroupSwitchModifier;
            break;
        case eKeyboardModifierMask:

            return Qt::KeyboardModifierMask;
            break;
        default:

            return Qt::NoModifier;
            break;
    }
}

Qt::KeyboardModifiers
QtEnumConvert::toQtModifiers(const KeyboardModifiers& m)
{
    Qt::KeyboardModifiers ret;

    if ( m.testFlag(eKeyboardModifierNone) ) {
        ret |= toQtModifier(eKeyboardModifierNone);
    }
    if ( m.testFlag(eKeyboardModifierShift) ) {
        ret |= toQtModifier(eKeyboardModifierShift);
    }
    if ( m.testFlag(eKeyboardModifierControl) ) {
        ret |= toQtModifier(eKeyboardModifierControl);
    }
    if ( m.testFlag(eKeyboardModifierAlt) ) {
        ret |= toQtModifier(eKeyboardModifierAlt);
    }
    if ( m.testFlag(eKeyboardModifierMeta) ) {
        ret |= toQtModifier(eKeyboardModifierMeta);
    }
    if ( m.testFlag(eKeyboardModifierKeypad) ) {
        ret |= toQtModifier(eKeyboardModifierKeypad);
    }
    if ( m.testFlag(eKeyboardModifierGroupSwitch) ) {
        ret |= toQtModifier(eKeyboardModifierGroupSwitch);
    }
    if ( m.testFlag(eKeyboardModifierMask) ) {
        ret |= toQtModifier(eKeyboardModifierMask);
    }

    return ret;
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

bool
QtEnumConvert::fromOfxtoQtModifier(int mod,
                                   Qt::KeyboardModifier* qtModifier)
{
    if ( (mod == kOfxKey_Control_L) || (mod == kOfxKey_Control_R) ) {
        *qtModifier =  Qt::ControlModifier;

        return true;
    } else if ( (mod == kOfxKey_Shift_L) || (mod == kOfxKey_Shift_R) ) {
        *qtModifier = Qt::ShiftModifier;

        return true;
    } else if ( (mod == kOfxKey_Alt_L) || (mod == kOfxKey_Alt_R) ) {
        *qtModifier = Qt::AltModifier;

        return true;
    } else if ( (mod == kOfxKey_Meta_L) || (mod == kOfxKey_Meta_R) ) {
        *qtModifier = Qt::MetaModifier;

        return true;
    }

    return false;
}

Qt::KeyboardModifiers
QtEnumConvert::fromOfxtoQtModifiers(const std::list<int>& modifiers)
{
    Qt::KeyboardModifiers ret = Qt::NoModifier;
    
    for (std::list<int>::const_iterator it = modifiers.begin(); it != modifiers.end(); ++it) {
        Qt::KeyboardModifier m;
        bool ok =  fromOfxtoQtModifier(*it, &m);
        if (!ok) {
            qDebug() << "Unrecognized keyboard modifier " << *it;
            
            return ret;
        }
        ret |= m;
    }
    
    return ret;
}


bool
QtEnumConvert::toQtCursor(CursorEnum c,
                          Qt::CursorShape* ret)
{
    bool b = true;

    switch (c) {
        case eCursorArrow:
            *ret = Qt::ArrowCursor;
            break;

        case eCursorBDiag:
            *ret = Qt::SizeBDiagCursor;
            break;

        case eCursorFDiag:
            *ret = Qt::SizeFDiagCursor;
            break;

        case eCursorBlank:
            *ret = Qt::BlankCursor;
            break;

        case eCursorBusy:
            *ret = Qt::BusyCursor;
            break;

        case eCursorClosedHand:
            *ret = Qt::ClosedHandCursor;
            break;

        case eCursorCross:
            *ret = Qt::CrossCursor;
            break;

        case eCursorForbidden:
            *ret = Qt::ForbiddenCursor;
            break;

        case eCursorIBeam:
            *ret = Qt::IBeamCursor;
            break;

        case eCursorOpenHand:
            *ret = Qt::OpenHandCursor;
            break;

        case eCursorPointingHand:
            *ret = Qt::PointingHandCursor;
            break;

        case eCursorSizeAll:
            *ret = Qt::SizeAllCursor;
            break;

        case eCursorSizeHor:
            *ret = Qt::SizeHorCursor;
            break;

        case eCursorSizeVer:
            *ret = Qt::SizeVerCursor;
            break;

        case eCursorSplitH:
            *ret = Qt::SplitHCursor;
            break;

        case eCursorSplitV:
            *ret = Qt::SplitVCursor;
            break;

        case eCursorUpArrow:
            *ret = Qt::UpArrowCursor;
            break;

        case eCursorWait:
            *ret = Qt::WaitCursor;
            break;

        case eCursorWhatsThis:
            *ret = Qt::WhatsThisCursor;
            break;

        case eCursorDefault:
        default:
            b = false;
            break;
    }
    
    return b;
} // toQtCursor

NATRON_NAMESPACE_EXIT
