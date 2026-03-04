#pragma once
/**
 * BLEHIDKeys.h
 * USB HID Keyboard/Keypad Page (0x07) usage values.
 * All key codes for a standard 104/105-key keyboard with numpad.
 *
 * Reference: USB HID Usage Tables v1.22, Section 10 (Keyboard/Keypad Page)
 *
 * Copyright (c) 2026 Hijel. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, this software
 * is provided "AS IS", WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. The author(s) accept no liability for any damages,
 * loss, or consequences arising from the use or misuse of this software.
 * See the License for the full terms governing permissions and limitations.
 */

// ─── Modifier Keys (used in the modifier byte, NOT as keycodes) ───────────────
// These are bit positions in the modifier byte of the keyboard report.
// Use KEY_MOD_* with press(keycode, modifiers) calls.
#define KEY_MOD_LCTRL   ((uint8_t)0x01)  // Left Control
#define KEY_MOD_LSHIFT  ((uint8_t)0x02)  // Left Shift
#define KEY_MOD_LALT    ((uint8_t)0x04)  // Left Alt
#define KEY_MOD_LGUI    ((uint8_t)0x08)  // Left GUI (Win/Cmd/Super)
#define KEY_MOD_RCTRL   ((uint8_t)0x10)  // Right Control
#define KEY_MOD_RSHIFT  ((uint8_t)0x20)  // Right Shift
#define KEY_MOD_RALT    ((uint8_t)0x40)  // Right Alt (AltGr)
#define KEY_MOD_RGUI    ((uint8_t)0x80)  // Right GUI

// ─── Special / Control Keys ────────────────────────────────────────────────
#define KEY_NONE        ((uint8_t)0x00)  // No key / key release
#define KEY_ERR_OVF     ((uint8_t)0x01)  // Keyboard Error Roll Over
#define KEY_ERR_POST    ((uint8_t)0x02)  // Keyboard POST Fail
#define KEY_ERR_UND     ((uint8_t)0x03)  // Keyboard Error Undefined

// ─── Letters (a-z) ─────────────────────────────────────────────────────────
#define KEY_A           ((uint8_t)0x04)
#define KEY_B           ((uint8_t)0x05)
#define KEY_C           ((uint8_t)0x06)
#define KEY_D           ((uint8_t)0x07)
#define KEY_E           ((uint8_t)0x08)
#define KEY_F           ((uint8_t)0x09)
#define KEY_G           ((uint8_t)0x0A)
#define KEY_H           ((uint8_t)0x0B)
#define KEY_I           ((uint8_t)0x0C)
#define KEY_J           ((uint8_t)0x0D)
#define KEY_K           ((uint8_t)0x0E)
#define KEY_L           ((uint8_t)0x0F)
#define KEY_M           ((uint8_t)0x10)
#define KEY_N           ((uint8_t)0x11)
#define KEY_O           ((uint8_t)0x12)
#define KEY_P           ((uint8_t)0x13)
#define KEY_Q           ((uint8_t)0x14)
#define KEY_R           ((uint8_t)0x15)
#define KEY_S           ((uint8_t)0x16)
#define KEY_T           ((uint8_t)0x17)
#define KEY_U           ((uint8_t)0x18)
#define KEY_V           ((uint8_t)0x19)
#define KEY_W           ((uint8_t)0x1A)
#define KEY_X           ((uint8_t)0x1B)
#define KEY_Y           ((uint8_t)0x1C)
#define KEY_Z           ((uint8_t)0x1D)

// ─── Number Row ────────────────────────────────────────────────────────────
#define KEY_1           ((uint8_t)0x1E)  // 1 and !
#define KEY_2           ((uint8_t)0x1F)  // 2 and @
#define KEY_3           ((uint8_t)0x20)  // 3 and #
#define KEY_4           ((uint8_t)0x21)  // 4 and $
#define KEY_5           ((uint8_t)0x22)  // 5 and %
#define KEY_6           ((uint8_t)0x23)  // 6 and ^
#define KEY_7           ((uint8_t)0x24)  // 7 and &
#define KEY_8           ((uint8_t)0x25)  // 8 and *
#define KEY_9           ((uint8_t)0x26)  // 9 and (
#define KEY_0           ((uint8_t)0x27)  // 0 and )

// ─── Control / Editing ─────────────────────────────────────────────────────
#define KEY_RETURN      ((uint8_t)0x28)  // Enter (main keyboard)
#define KEY_ESCAPE      ((uint8_t)0x29)
#define KEY_BACKSPACE   ((uint8_t)0x2A)
#define KEY_TAB         ((uint8_t)0x2B)
#define KEY_SPACE       ((uint8_t)0x2C)

// ─── Punctuation & Symbols ─────────────────────────────────────────────────
#define KEY_MINUS       ((uint8_t)0x2D)  // - and _
#define KEY_EQUAL       ((uint8_t)0x2E)  // = and +
#define KEY_LEFTBRACE   ((uint8_t)0x2F)  // [ and {
#define KEY_RIGHTBRACE  ((uint8_t)0x30)  // ] and }
#define KEY_BACKSLASH   ((uint8_t)0x31)  // \ and |
#define KEY_HASH_TILDE  ((uint8_t)0x32)  // Non-US # and ~
#define KEY_SEMICOLON   ((uint8_t)0x33)  // ; and :
#define KEY_APOSTROPHE  ((uint8_t)0x34)  // ' and "
#define KEY_GRAVE       ((uint8_t)0x35)  // ` and ~
#define KEY_COMMA       ((uint8_t)0x36)  // , and <
#define KEY_DOT         ((uint8_t)0x37)  // . and >
#define KEY_SLASH       ((uint8_t)0x38)  // / and ?

// ─── Lock Keys ─────────────────────────────────────────────────────────────
#define KEY_CAPS_LOCK   ((uint8_t)0x39)
#define KEY_SCROLL_LOCK ((uint8_t)0x47)
#define KEY_NUM_LOCK    ((uint8_t)0x53)

// ─── Function Keys ─────────────────────────────────────────────────────────
#define KEY_F1          ((uint8_t)0x3A)
#define KEY_F2          ((uint8_t)0x3B)
#define KEY_F3          ((uint8_t)0x3C)
#define KEY_F4          ((uint8_t)0x3D)
#define KEY_F5          ((uint8_t)0x3E)
#define KEY_F6          ((uint8_t)0x3F)
#define KEY_F7          ((uint8_t)0x40)
#define KEY_F8          ((uint8_t)0x41)
#define KEY_F9          ((uint8_t)0x42)
#define KEY_F10         ((uint8_t)0x43)
#define KEY_F11         ((uint8_t)0x44)
#define KEY_F12         ((uint8_t)0x45)
#define KEY_F13         ((uint8_t)0x68)
#define KEY_F14         ((uint8_t)0x69)
#define KEY_F15         ((uint8_t)0x6A)
#define KEY_F16         ((uint8_t)0x6B)
#define KEY_F17         ((uint8_t)0x6C)
#define KEY_F18         ((uint8_t)0x6D)
#define KEY_F19         ((uint8_t)0x6E)
#define KEY_F20         ((uint8_t)0x6F)
#define KEY_F21         ((uint8_t)0x70)
#define KEY_F22         ((uint8_t)0x71)
#define KEY_F23         ((uint8_t)0x72)
#define KEY_F24         ((uint8_t)0x73)

// ─── System / Media Keys (on keyboard) ────────────────────────────────────
#define KEY_PRINT_SCREEN  ((uint8_t)0x46)
#define KEY_PAUSE         ((uint8_t)0x48)
#define KEY_INSERT        ((uint8_t)0x49)
#define KEY_HOME          ((uint8_t)0x4A)
#define KEY_PAGE_UP       ((uint8_t)0x4B)
#define KEY_DELETE        ((uint8_t)0x4C)
#define KEY_END           ((uint8_t)0x4D)
#define KEY_PAGE_DOWN     ((uint8_t)0x4E)

// ─── Navigation ────────────────────────────────────────────────────────────
#define KEY_RIGHT         ((uint8_t)0x4F)
#define KEY_LEFT          ((uint8_t)0x50)
#define KEY_DOWN          ((uint8_t)0x51)
#define KEY_UP            ((uint8_t)0x52)

// ─── Numeric Keypad ────────────────────────────────────────────────────────
#define KEY_KP_SLASH      ((uint8_t)0x54)  // Keypad /
#define KEY_KP_ASTERISK   ((uint8_t)0x55)  // Keypad *
#define KEY_KP_MINUS      ((uint8_t)0x56)  // Keypad -
#define KEY_KP_PLUS       ((uint8_t)0x57)  // Keypad +
#define KEY_KP_ENTER      ((uint8_t)0x58)  // Keypad Enter
#define KEY_KP_1          ((uint8_t)0x59)  // Keypad 1 / End
#define KEY_KP_2          ((uint8_t)0x5A)  // Keypad 2 / Down
#define KEY_KP_3          ((uint8_t)0x5B)  // Keypad 3 / PageDn
#define KEY_KP_4          ((uint8_t)0x5C)  // Keypad 4 / Left
#define KEY_KP_5          ((uint8_t)0x5D)  // Keypad 5
#define KEY_KP_6          ((uint8_t)0x5E)  // Keypad 6 / Right
#define KEY_KP_7          ((uint8_t)0x5F)  // Keypad 7 / Home
#define KEY_KP_8          ((uint8_t)0x60)  // Keypad 8 / Up
#define KEY_KP_9          ((uint8_t)0x61)  // Keypad 9 / PageUp
#define KEY_KP_0          ((uint8_t)0x62)  // Keypad 0 / Insert
#define KEY_KP_DOT        ((uint8_t)0x63)  // Keypad . / Delete
#define KEY_KP_EQUAL      ((uint8_t)0x67)  // Keypad =

// ─── Extra Keys ────────────────────────────────────────────────────────────
#define KEY_NONUS_BACKSLASH ((uint8_t)0x64)  // Non-US \ and | (ISO key left of Z)
#define KEY_APPLICATION   ((uint8_t)0x65)    // Application / Context menu key
#define KEY_POWER         ((uint8_t)0x66)    // Keyboard Power (not System Control)
#define KEY_KP_COMMA      ((uint8_t)0x85)    // Keypad comma (European numeric layouts use , instead of .)

// ─── International Keys ────────────────────────────────────────────────────
// Physical extra keys found on Japanese and Brazilian ABNT keyboards.
// The host OS layout setting determines what character these produce.
// NOTE: requires HID descriptor logical maximum of 0xE7 (set in library).
#define KEY_INTL1         ((uint8_t)0x87)    // Japanese/Brazilian extra key (¥ / _)
#define KEY_INTL2         ((uint8_t)0x88)    // Katakana/Hiragana/Romaji toggle (Japanese)
#define KEY_INTL3         ((uint8_t)0x89)    // Yen sign key (Japanese)
#define KEY_INTL4         ((uint8_t)0x8A)    // Henkan / Convert (Japanese IME)
#define KEY_INTL5         ((uint8_t)0x8B)    // Muhenkan / Non-convert (Japanese IME)
#define KEY_INTL6         ((uint8_t)0x8C)    // PC9800 keypad comma (Japanese legacy)

// ─── Language Keys ─────────────────────────────────────────────────────────
// IME control keys found on Korean and Japanese keyboards.
// NOTE: requires HID descriptor logical maximum of 0xE7 (set in library).
#define KEY_LANG1         ((uint8_t)0x90)    // Hangul/English toggle (Korean)
#define KEY_LANG2         ((uint8_t)0x91)    // Hanja conversion (Korean)
#define KEY_LANG3         ((uint8_t)0x92)    // Katakana (Japanese)
#define KEY_LANG4         ((uint8_t)0x93)    // Hiragana (Japanese)
#define KEY_LANG5         ((uint8_t)0x94)    // Zenkaku/Hankaku fullwidth toggle (Japanese)

// ─── Modifier Keycodes (also sendable as keycodes in the keycode slots) ────
// These are the actual HID keycodes for modifier keys — distinct from the
// modifier byte bitmask values above. Use these with press() if you want
// modifier keys in the keycode array rather than the modifier byte.
#define KEY_LCTRL         ((uint8_t)0xE0)
#define KEY_LSHIFT        ((uint8_t)0xE1)
#define KEY_LALT          ((uint8_t)0xE2)
#define KEY_LGUI          ((uint8_t)0xE3)
#define KEY_RCTRL         ((uint8_t)0xE4)
#define KEY_RSHIFT        ((uint8_t)0xE5)
#define KEY_RALT          ((uint8_t)0xE6)
#define KEY_RGUI          ((uint8_t)0xE7)
