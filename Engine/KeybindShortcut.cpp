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

#include "KeybindShortcut.h"

NATRON_NAMESPACE_ENTER

struct KeyStringPair
{
    Key key;
    std::string string;
};

NATRON_NAMESPACE_ANONYMOUS_ENTER

static KeyStringPair keyStringMapping[] = {
    { Key_Unknown, "Unknown" },
    { Key_BackSpace, "BackSpace" },
    { Key_Tab, "Tab" },
    { Key_Linefeed, "Linefeed" },
    { Key_Clear, "Clear" },
    { Key_Return, "Return" },
    { Key_Pause, "Pause" },
    { Key_Scroll_Lock, "Scroll_Lock" },
    { Key_Sys_Req, "Sys_Req" },
    { Key_Escape, "Escape" },
    { Key_Delete, "Delete" },
    { Key_Multi_key, "Multi_key" },
    { Key_SingleCandidate, "SingleCandidate" },
    { Key_MultipleCandidate, "MultipleCandidate" },
    { Key_PreviousCandidate, "PreviousCandidate" },
    { Key_Kanji, "Kanji" },
    { Key_Muhenkan, "Muhenkan" },
    { Key_Henkan_Mode, "Henkan_Mode" },
    { Key_Henkan, "Henkan" },
    { Key_Romaji, "Romaji" },
    { Key_Hiragana, "Hiragana" },
    { Key_Katakana, "Katakana" },
    { Key_Hiragana_Katakana, "Hiragana_Katakana" },
    { Key_Zenkaku, "Zenkaku" },
    { Key_Hankaku, "Hankaku" },
    { Key_Zenkaku_Hankaku, "Zenkaku_Hankaku" },
    { Key_Touroku, "Touroku" },
    { Key_Massyo, "Massyo" },
    { Key_Kana_Lock, "Kana_Lock" },
    { Key_Kana_Shift, "Kana_Shift" },
    { Key_Eisu_Shift, "Eisu_Shift" },
    { Key_Eisu_toggle, "Eisu_toggle" },
    { Key_Zen_Koho, "Zen_Koho" },
    { Key_Mae_Koho, "Mae_Koho" },
    { Key_Home, "Home" },
    { Key_Left, "Left" },
    { Key_Up, "Up" },
    { Key_Right, "Right" },
    { Key_Down, "Down" },
    { Key_Prior, "Prior" },
    { Key_Page_Up, "Page_Up" },
    { Key_Next, "Next" },
    { Key_Page_Down, "Page_Down" },
    { Key_End, "End" },
    { Key_Begin, "Begin" },
    { Key_Select, "Select" },
    { Key_Print, "Print" },
    { Key_Execute, "Execute" },
    { Key_Insert, "Insert" },
    { Key_Undo, "Undo" },
    { Key_Redo, "Redo" },
    { Key_Menu, "Menu" },
    { Key_Find, "Find" },
    { Key_Cancel, "Cancel" },
    { Key_Help, "Help" },
    { Key_Break, "Break" },
    { Key_Mode_switch, "Mode_switch" },
    { Key_script_switch, "script_switch" },
    { Key_Num_Lock, "Num_Lock" },
    { Key_KP_Space, "KP_Space" },
    { Key_KP_Tab, "KP_Tab" },
    { Key_KP_Enter, "KP_Enter" },
    { Key_KP_F1, "KP_F1" },
    { Key_KP_F2, "KP_F2" },
    { Key_KP_F3, "KP_F3" },
    { Key_KP_F4, "KP_F4" },
    { Key_KP_Home, "KP_Home" },
    { Key_KP_Left, "KP_Left" },
    { Key_KP_Up, "KP_Up" },
    { Key_KP_Right, "KP_Right" },
    { Key_KP_Down, "KP_Down" },
    { Key_KP_Prior, "KP_Prior" },
    { Key_KP_Page_Up, "KP_Page_Up" },
    { Key_KP_Next, "KP_Next" },
    { Key_KP_Page_Down, "KP_Page_Down" },
    { Key_KP_End, "KP_End" },
    { Key_KP_Begin, "KP_Begin" },
    { Key_KP_Insert, "KP_Insert" },
    { Key_KP_Delete, "KP_Delete" },
    { Key_KP_Equal, "KP_Equal" },
    { Key_KP_Multiply, "KP_Multiply" },
    { Key_KP_Add, "KP_Add" },
    { Key_KP_Separator, "KP_Separator" },
    { Key_KP_Subtract, "KP_Subtract" },
    { Key_KP_Decimal, "KP_Decimal" },
    { Key_KP_Divide, "KP_Divide" },
    { Key_KP_0, "KP_0" },
    { Key_KP_1, "KP_1" },
    { Key_KP_2, "KP_2" },
    { Key_KP_3, "KP_3" },
    { Key_KP_4, "KP_4" },
    { Key_KP_5, "KP_5" },
    { Key_KP_6, "KP_6" },
    { Key_KP_7, "KP_7" },
    { Key_KP_8, "KP_8" },
    { Key_KP_9, "KP_9" },
    { Key_F1, "F1" },
    { Key_F2, "F2" },
    { Key_F3, "F3" },
    { Key_F4, "F4" },
    { Key_F5, "F5" },
    { Key_F6, "F6" },
    { Key_F7, "F7" },
    { Key_F8, "F8" },
    { Key_F9, "F9" },
    { Key_F10, "F10" },
    { Key_F11, "F11" },
    { Key_L1, "L1" },
    { Key_F12, "F12" },
    { Key_L2, "L2" },
    { Key_F13, "F13" },
    { Key_L3, "L3" },
    { Key_F14, "F14" },
    { Key_L4, "L4" },
    { Key_F15, "F15" },
    { Key_L5, "L5" },
    { Key_F16, "F16" },
    { Key_L6, "L6" },
    { Key_F17, "F17" },
    { Key_L7, "L7" },
    { Key_F18, "F18" },
    { Key_L8, "L8" },
    { Key_F19, "F19" },
    { Key_L9, "L9" },
    { Key_F20, "F20" },
    { Key_L10, "L10" },
    { Key_F21, "F21" },
    { Key_R1, "R1" },
    { Key_F22, "F22" },
    { Key_R2, "R2" },
    { Key_F23, "F23" },
    { Key_R3, "R3" },
    { Key_F24, "F24" },
    { Key_R4, "R4" },
    { Key_F25, "F25" },
    { Key_R5, "R5" },
    { Key_F26, "F26" },
    { Key_R6, "R6" },
    { Key_F27, "F27" },
    { Key_R7, "R7" },
    { Key_F28, "F28" },
    { Key_R8, "R8" },
    { Key_F29, "F29" },
    { Key_R9, "R9" },
    { Key_F30, "F30" },
    { Key_R10, "R10" },
    { Key_F31, "F31" },
    { Key_R11, "R11" },
    { Key_F32, "F32" },
    { Key_R12, "R12" },
    { Key_F33, "F33" },
    { Key_R13, "R13" },
    { Key_F34, "F34" },
    { Key_R14, "R14" },
    { Key_F35, "F35" },
    { Key_R15, "R15" },
    { Key_Shift_L, "Shift_L" },
    { Key_Shift_R, "Shift_R" },
    { Key_Control_L, "Control_L" },
    { Key_Control_R, "Control_R" },
    { Key_Caps_Lock, "Caps_Lock" },
    { Key_Shift_Lock, "Shift_Lock" },
    { Key_Meta_L, "Meta_L" },
    { Key_Meta_R, "Meta_R" },
    { Key_Alt_L, "Alt_L" },
    { Key_Alt_R, "Alt_R" },
    { Key_Super_L, "Super_L" },
    { Key_Super_R, "Super_R" },
    { Key_Hyper_L, "Hyper_L" },
    { Key_Hyper_R, "Hyper_R" },
    { Key_space, "space" },
    { Key_exclam, "exclam" },
    { Key_quotedbl, "quotedbl" },
    { Key_numbersign, "numbersign" },
    { Key_dollar, "dollar" },
    { Key_percent, "percent" },
    { Key_ampersand, "ampersand" },
    { Key_apostrophe, "apostrophe" },
    { Key_quoteright, "quoteright" },
    { Key_parenleft, "parenleft" },
    { Key_parenright, "parenright" },
    { Key_asterisk, "asterisk" },
    { Key_plus, "plus" },
    { Key_comma, "comma" },
    { Key_minus, "minus" },
    { Key_period, "period" },
    { Key_slash, "slash" },
    { Key_0, "0" },
    { Key_1, "1" },
    { Key_2, "2" },
    { Key_3, "3" },
    { Key_4, "4" },
    { Key_5, "5" },
    { Key_6, "6" },
    { Key_7, "7" },
    { Key_8, "8" },
    { Key_9, "9" },
    { Key_colon, "colon" },
    { Key_semicolon, "semicolon" },
    { Key_less, "less" },
    { Key_equal, "equal" },
    { Key_greater, "greater" },
    { Key_question, "question" },
    { Key_at, "at" },
    { Key_A, "A" },
    { Key_B, "B" },
    { Key_C, "C" },
    { Key_D, "D" },
    { Key_E, "E" },
    { Key_F, "F" },
    { Key_G, "G" },
    { Key_H, "H" },
    { Key_I, "I" },
    { Key_J, "J" },
    { Key_K, "K" },
    { Key_L, "L" },
    { Key_M, "M" },
    { Key_N, "N" },
    { Key_O, "O" },
    { Key_P, "P" },
    { Key_Q, "Q" },
    { Key_R, "R" },
    { Key_S, "S" },
    { Key_T, "T" },
    { Key_U, "U" },
    { Key_V, "V" },
    { Key_W, "W" },
    { Key_X, "X" },
    { Key_Y, "Y" },
    { Key_Z, "Z" },
    { Key_bracketleft, "bracketleft" },
    { Key_backslash, "backslash" },
    { Key_bracketright, "bracketright" },
    { Key_asciicircum, "asciicircum" },
    { Key_underscore, "underscore" },
    { Key_grave, "grave" },
    { Key_quoteleft, "quoteleft" },
    { Key_a, "a" },
    { Key_b, "b" },
    { Key_c, "c" },
    { Key_d, "d" },
    { Key_e, "e" },
    { Key_f, "f" },
    { Key_g, "g" },
    { Key_h, "h" },
    { Key_i, "i" },
    { Key_j, "j" },
    { Key_k, "k" },
    { Key_l, "l" },
    { Key_m, "m" },
    { Key_n, "n" },
    { Key_o, "o" },
    { Key_p, "p" },
    { Key_q, "q" },
    { Key_r, "r" },
    { Key_s, "s" },
    { Key_t, "t" },
    { Key_u, "u" },
    { Key_v, "v" },
    { Key_w, "w" },
    { Key_x, "x" },
    { Key_y, "y" },
    { Key_z, "z" },
    { Key_braceleft, "braceleft" },
    { Key_bar, "bar" },
    { Key_braceright, "braceright" },
    { Key_asciitilde, "asciitilde" },
    { Key_nobreakspace, "nobreakspace" },
    { Key_exclamdown, "exclamdown" },
    { Key_cent, "cent" },
    { Key_sterling, "sterling" },
    { Key_currency, "currency" },
    { Key_yen, "yen" },
    { Key_brokenbar, "brokenbar" },
    { Key_section, "section" },
    { Key_diaeresis, "diaeresis" },
    { Key_copyright, "copyright" },
    { Key_ordfeminine, "ordfeminine" },
    { Key_guillemotleft, "guillemotleft" },
    { Key_notsign, "notsign" },
    { Key_hyphen, "hyphen" },
    { Key_registered, "registered" },
    { Key_macron, "macron" },
    { Key_degree, "degree" },
    { Key_plusminus, "plusminus" },
    { Key_twosuperior, "twosuperior" },
    { Key_threesuperior, "threesuperior" },
    { Key_acute, "acute" },
    { Key_mu, "mu" },
    { Key_paragraph, "paragraph" },
    { Key_periodcentered, "periodcentered" },
    { Key_cedilla, "cedilla" },
    { Key_onesuperior, "onesuperior" },
    { Key_masculine, "masculine" },
    { Key_guillemotright, "guillemotright" },
    { Key_onequarter, "onequarter" },
    { Key_onehalf, "onehalf" },
    { Key_threequarters, "threequarters" },
    { Key_questiondown, "questiondown" },
    { Key_Agrave, "Agrave" },
    { Key_Aacute, "Aacute" },
    { Key_Acircumflex, "Acircumflex" },
    { Key_Atilde, "Atilde" },
    { Key_Adiaeresis, "Adiaeresis" },
    { Key_Aring, "Aring" },
    { Key_AE, "AE" },
    { Key_Ccedilla, "Ccedilla" },
    { Key_Egrave, "Egrave" },
    { Key_Eacute, "Eacute" },
    { Key_Ecircumflex, "Ecircumflex" },
    { Key_Ediaeresis, "Ediaeresis" },
    { Key_Igrave, "Igrave" },
    { Key_Iacute, "Iacute" },
    { Key_Icircumflex, "Icircumflex" },
    { Key_Idiaeresis, "Idiaeresis" },
    { Key_ETH, "ETH" },
    { Key_Eth, "Eth" },
    { Key_Ntilde, "Ntilde" },
    { Key_Ograve, "Ograve" },
    { Key_Oacute, "Oacute" },
    { Key_Ocircumflex, "Ocircumflex" },
    { Key_Otilde, "Otilde" },
    { Key_Odiaeresis, "Odiaeresis" },
    { Key_multiply, "multiply" },
    { Key_Ooblique, "Ooblique" },
    { Key_Ugrave, "Ugrave" },
    { Key_Uacute, "Uacute" },
    { Key_Ucircumflex, "Ucircumflex" },
    { Key_Udiaeresis, "Udiaeresis" },
    { Key_Yacute, "Yacute" },
    { Key_THORN, "THORN" },
    { Key_ssharp, "ssharp" },
    { Key_agrave, "agrave" },
    { Key_aacute, "aacute" },
    { Key_acircumflex, "acircumflex" },
    { Key_atilde, "atilde" },
    { Key_adiaeresis, "adiaeresis" },
    { Key_aring, "aring" },
    { Key_ae, "ae" },
    { Key_ccedilla, "ccedilla" },
    { Key_egrave, "egrave" },
    { Key_eacute, "eacute" },
    { Key_ecircumflex, "ecircumflex" },
    { Key_ediaeresis, "ediaeresis" },
    { Key_igrave, "igrave" },
    { Key_iacute, "iacute" },
    { Key_icircumflex, "icircumflex" },
    { Key_idiaeresis, "idiaeresis" },
    { Key_eth, "eth" },
    { Key_ntilde, "ntilde" },
    { Key_ograve, "ograve" },
    { Key_oacute, "oacute" },
    { Key_ocircumflex, "ocircumflex" },
    { Key_otilde, "otilde" },
    { Key_odiaeresis, "odiaeresis" },
    { Key_division, "division" },
    { Key_oslash, "oslash" },
    { Key_ugrave, "ugrave" },
    { Key_uacute, "uacute" },
    { Key_ucircumflex, "ucircumflex" },
    { Key_udiaeresis, "udiaeresis" },
    { Key_yacute, "yacute" },
    { Key_thorn, "thorn" },
    { Key_ydiaeresis, "ydiaeresis" },
    { (Key)0, ""}
};

struct ModifierStringPair
{
    KeyboardModifierEnum modifier;
    std::string name;
};

static ModifierStringPair modifierStringMapping[] = {
    { eKeyboardModifierNone, "NoModifier" },
    { eKeyboardModifierShift, "Shift" },
    { eKeyboardModifierControl, "Control" },
    { eKeyboardModifierAlt, "Alt" },
    { eKeyboardModifierMeta, "Meta" },
    { eKeyboardModifierKeypad, "Keypad" },
    { eKeyboardModifierGroupSwitch, "GroupSwitch" },
    { (KeyboardModifierEnum)0, "" }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

KeybindShortcut::KeybindShortcut()
: grouping()
, actionID()
, actionLabel()
, description()
, modifiers(eKeyboardModifierNone)
, defaultModifiers(eKeyboardModifierNone)
, ignoreMask(eKeyboardModifierNone)
, currentShortcut()
, defaultShortcut()
, actions()
, editable(true)
{
}

KeybindShortcut::~KeybindShortcut()
{
    
}

void
KeybindShortcut::updateActionsShortcut()
{
    for (std::list<KeybindListenerI*>::iterator it = actions.begin(); it != actions.end(); ++it) {
        (*it)->onShortcutChanged(actionID, currentShortcut, modifiers);
    }
}

std::string
KeybindShortcut::keySymbolToString(Key key)
{
    KeyStringPair* it = &keyStringMapping[0];
    while (!it->string.empty()) {
        if (it->key == key) {
            return it->string;
        }
        ++it;
    }
    return std::string();
}

Key
KeybindShortcut::keySymbolFromString(const std::string& key)
{
    KeyStringPair* it = &keyStringMapping[0];
    while (!it->string.empty()) {
        if (it->string == key) {
            return it->key;
        }
        ++it;
    }
    return (Key)0;
}


std::string
KeybindShortcut::modifierToString(const KeyboardModifierEnum& key)
{
    ModifierStringPair* it = &modifierStringMapping[0];
    while (!it->name.empty()) {
        if (it->modifier == key) {
            return it->name;
        }
        ++it;
    }
    return std::string();
}

KeyboardModifierEnum
KeybindShortcut::modifierFromString(const std::string& key)
{
    ModifierStringPair* it = &modifierStringMapping[0];
    while (!it->name.empty()) {
        if (it->name == key) {
            return it->modifier;
        }
        ++it;
    }
    return eKeyboardModifierNone;
}

std::list<std::string>
KeybindShortcut::modifiersToStringList(const KeyboardModifiers& mods)
{
    std::list<std::string> ret;
    if (mods & eKeyboardModifierAlt) {
        ret.push_back(modifierToString(eKeyboardModifierAlt));
    }
    if (mods & eKeyboardModifierShift) {
        ret.push_back(modifierToString(eKeyboardModifierShift));
    }
    if (mods & eKeyboardModifierControl) {
        ret.push_back(modifierToString(eKeyboardModifierControl));
    }
    if (mods & eKeyboardModifierMeta) {
        ret.push_back(modifierToString(eKeyboardModifierMeta));
    }
    if (mods & eKeyboardModifierKeypad) {
        ret.push_back(modifierToString(eKeyboardModifierKeypad));
    }
    if (mods & eKeyboardModifierGroupSwitch) {
        ret.push_back(modifierToString(eKeyboardModifierGroupSwitch));
    }
    return ret;
}

KeyboardModifiers
KeybindShortcut::modifiersFromStringList(const std::list<std::string>& mods)
{
    KeyboardModifiers ret;
    for (std::list<std::string>::const_iterator it = mods.begin(); it != mods.end(); ++it) {
        ret |= modifierFromString(*it);
    }
    return ret;
}

NATRON_NAMESPACE_EXIT
