/**
 * LayoutKeyboard.ino
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
 *
 * ─────────────────────────────────────────────────────────────────────────
 * Demonstrates keyboard layout support for non-US keyboards.
 *
 *   - setKeyboardLayout() so print() types the right characters,
 *     including accented ones (é, ñ, ü, ç...)
 *   - keycodeFor() so shortcuts and held keys follow the labels on
 *     your keyboard instead of US key positions
 *
 * 1. Set KEYBOARD_LAYOUT below to match the keyboard layout selected
 *    on your host computer (e.g. German).
 * 2. Upload to your ESP32.
 * 3. Open Bluetooth settings on your host device and pair with "HijelHID KB".
 * 4. Open a text editor on the host.
 * 5. You can use the boot button GPIO0 as BUTTON_PIN if you have one,
 *    or use any other digital input to trigger the key pressing sequence
 *
 * NOTE: US is fully tested. The other layouts are best effort and not yet
 * tested on real hardware — please report how yours works in the Discussions.
 */

#include <HijelHID_BLEKeyboard.h>

// ─────────────────────────────────────────────────────────────────────────
// Layout Selection
// ─────────────────────────────────────────────────────────────────────────
//
// Must match the keyboard layout selected on the HOST computer.
//
//   Windows / Linux                  macOS
//   HIDLayout_US (default)           HIDLayout_US
//   HIDLayout_US_International       —
//   HIDLayout_UK                     HIDLayout_UK_Mac
//   HIDLayout_German                 HIDLayout_German_Mac
//   HIDLayout_French                 HIDLayout_French_Mac
//   HIDLayout_Spanish                HIDLayout_Spanish_Mac
//   HIDLayout_Spanish_LatAm          —
//   HIDLayout_Italian                HIDLayout_Italian_Mac
//   HIDLayout_Portuguese             HIDLayout_Portuguese_Mac
//   HIDLayout_Portuguese_Brazil      HIDLayout_Portuguese_Brazil_Mac

#define KEYBOARD_LAYOUT   HIDLayout_German

// Optional Modifier used for shortcuts (undo, etc.).
// Windows / Linux: KEY_MOD_LCTRL   macOS: KEY_MOD_LGUI (Command)
#define SHORTCUT_MOD      KEY_MOD_LCTRL

HijelHID_BLEKeyboard keyboard;

const int BUTTON_PIN = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("HijelHID BLE Keyboard — Layout Example");

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    keyboard.setLogLevel(HIDLogLevel::Normal);

    // Tell the library which layout the host is using.
    // Can be called any time — before or after begin().
    keyboard.setKeyboardLayout(KEYBOARD_LAYOUT);

    keyboard.begin();

    Serial.print("Keyboard layout: ");
    Serial.println(keyboard.getKeyboardLayout().name);
    Serial.println("Ready. Pair via Bluetooth settings, then press BOOT to type.");
}

void loop() {
    if (!keyboard.isPaired()) {
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 3000) {
            Serial.println("Waiting for connection...");
            lastPrint = millis();
        }
        return;
    }

    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50); // debounce
        if (digitalRead(BUTTON_PIN) == LOW) {
            Serial.println("Button pressed — sending keystrokes");

            // Don't forget to open a text editor on the Host.

            // ── 1. Typing text with print() ─────────────────────────────
            // print() looks up every character in the selected layout, so
            // text comes out correctly on the host. Accented characters work
            // on layouts that have them. Characters the layout cannot type
            // are skipped — print() returns how many were actually typed.
            keyboard.println("Hello from ESP32!");
            delay(1000);

            size_t typed = keyboard.println("Grüße, café, señor, ação, l'été");
            Serial.printf("Typed %u characters\n", (unsigned)typed);
            delay(2000);

            // ── 2. tap(KEY_*) sends US key POSITIONS ────────────────────
            // KEY_Z is the key in the US "Z" position. On a German keyboard
            // that key is labelled Y — so this types "y", not "z".
            keyboard.print("tap(KEY_Z) types: ");
            keyboard.tap(KEY_Z);
            keyboard.tap(KEY_RETURN);
            delay(1000);

            // ── 3. keycodeFor() finds the key by its LABEL ─────────────
            // keycodeFor('z') returns the key labelled Z on the selected
            // layout. Pass it to tap(), press() or release().
            keyboard.print("tap(keycodeFor('z')) types: ");
            keyboard.tap(keyboard.keycodeFor('z'));
            keyboard.tap(KEY_RETURN);
            delay(2000);

            // ── 4. Shortcuts that follow your keyboard ──────────────────
            // Shortcuts on Windows, macOS and Linux follow the key LABEL.
            // Type a word, then undo it with Ctrl+Z (Cmd+Z on macOS).
            // With tap(KEY_Z, ...) a German host would get Ctrl+Y (redo).
            keyboard.print("This line will be undone");
            delay(1500);
            keyboard.tap(keyboard.keycodeFor('z'), SHORTCUT_MOD);
            delay(2000);

            // ── 5. Holding a key by its label ───────────────────────────
            // keycodeFor() also works with press() / release(). Accented
            // characters use the string form "é" (or U'é') — 'é' is not a
            // single char in C++.
            //
            // keycodeFor() returns 0 (KEY_NONE) when no single key on the
            // layout carries the character — always check before pressing,
            // because release(0) releases ALL keys.
            uint8_t kc = keyboard.keycodeFor("é");
            if (kc != KEY_NONE) {
                Serial.println("Holding the é key");
                keyboard.press(kc);
                delay(25);
                keyboard.release(kc);
                delay(25);
            } else {
                // e.g. German has no é key (it is typed as ´ then e),
                // but print("é") can still type it.
                Serial.println("No é key on this layout — using print() instead");
                keyboard.print("é");
            }
            keyboard.tap(KEY_RETURN);
            delay(2000);

            // ── 6. Keys without a character are the same everywhere ────
            // Enter, Tab, Escape, arrows, F-keys, Delete and media keys need
            // no layout handling — use KEY_* / MEDIA_* as usual.
            keyboard.tap(KEY_TAB);
            keyboard.print("Done!");
            keyboard.tap(KEY_RETURN);

            // end any sending of keys with a releaseAll just to be sure.
            keyboard.releaseAll();

            while (digitalRead(BUTTON_PIN) == LOW) { delay(10); }
        }
    }

    delay(10);
}
