/**
 * BasicKeyboard.ino
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
 * Demonstrates basic text typing and key presses with HijelHID_BLEKeyboard.
 *
 * 1. Upload to your ESP32.
 * 2. Open Bluetooth settings on your host device and pair with "HijelHID KB".
 * 3. Open a text editor on the host.
 * 4. You can use the boot button GPIO0 as BUTTON_PIN if you have one,
 *    or use any other digital input to trigger the key pressing sequence
 */

#include <HijelHID_BLEKeyboard.h>

HijelHID_BLEKeyboard keyboard;

const int BUTTON_PIN = 38;

void setup() {
    Serial.begin(115200);
    Serial.println("HijelHID BLE Keyboard — Basic Example");

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    keyboard.setDebugLevel(HIDLogLevel::Normal);

    keyboard.begin();

    Serial.println("Ready. Pair via Bluetooth settings, then press BOOT to type.");
}

void loop() {
    if (!keyboard.isConnected()) {
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

            // set battery level report to 75%
            keyboard.setBatteryLevel(75);

            // Send Text Using Arduino Print and Write Commands
            keyboard.print("Printing Hello from ESP32!");
            keyboard.tap(KEY_RETURN);
            delay(2000);

            keyboard.println("Printing Line The quick brown fox");
            delay(2000);

            // keyboard.write only accepts a single uint8_t at a time
            // if you want to use it for whatever reason....
            const char* str = "Writing Hello from ESP32!";
            for (int i = 0; str[i] != '\0'; i++) {
                keyboard.write((uint8_t)str[i]);
            }
            keyboard.tap(KEY_RETURN);
            delay(2000);

            // Using the tap feature takes care of pressing and releasing keys and maintaining 
            // Press and Gap Timing (How long the key is pressed, and how long between key presses)
            keyboard.tap(KEY_H, KEY_MOD_LSHIFT);
            keyboard.tap(KEY_E);
            keyboard.tap(KEY_L);
            keyboard.tap(KEY_L);
            keyboard.tap(KEY_O);
            keyboard.tap(KEY_RETURN);
            delay(2000);

            // Manual key presses require you to manage both how long the key as pressed AND the 
            // time between key presses using a delay(ms)
            keyboard.press(KEY_E, KEY_MOD_RSHIFT);
            delay(25);
            keyboard.releaseAll();
            delay(25);
            keyboard.press(KEY_S, KEY_MOD_RSHIFT); 
            delay(25);
            keyboard.releaseAll();
            delay(25);
            keyboard.press(KEY_P, KEY_MOD_RSHIFT);
            delay(25); 
            keyboard.releaseAll();
            delay(25);
            keyboard.press(KEY_3);
            delay(25); 
            keyboard.releaseAll();
            delay(25);
            keyboard.press(KEY_2); 
            delay(25);
            keyboard.releaseAll();
            delay(25);
            keyboard.press(KEY_RETURN); 
            delay(25);
            keyboard.releaseAll();
            delay(2000);

            keyboard.tap(MEDIA_VOLUME_UP);
            delay(4000);
            keyboard.press(MEDIA_VOLUME_DOWN);
            delay(25);
            keyboard.releaseAll();
            delay(4000);


            keyboard.tap(MEDIA_CALCULATOR);
            delay(25);
            keyboard.releaseAll();
            delay(25);
            // end any sending of keys with a releaseAll just to be sure.
             keyboard.releaseAll();

            while (digitalRead(BUTTON_PIN) == LOW) { delay(10); }
        }
    }

    delay(10);
}
