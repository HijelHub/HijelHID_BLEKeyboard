/**
 * BasicKeyboard.ino
 *
 * Demonstrates basic text typing and key presses with HijelHID_BLEKeyboard.
 *
 * 1. Upload to your ESP32.
 * 2. Open Bluetooth settings on your host device and pair with "HijelHID KB".
 * 3. Open a text editor on the host.
 * 4. Press the BOOT button (GPIO0) to send keystrokes.
 */

#include <HijelHID_BLEKeyboard.h>

// Instantiate HID Keyboard and Set Device Name [Default: HijelHID KB], Manufacturer [Default: Hijel], Battery Level [Default: 100]
HijelHID_BLEKeyboard keyboard("HijelHID KB", "Hijel", 100);

const int BUTTON_PIN = 0;

void setup() {

    Serial.begin(115200);
    Serial.println("HijelHID BLE Keyboard — Basic Example");

    pinMode(BUTTON_PIN, INPUT_PULLUP);

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

           // keyboard.print("Hello from ESP32!");
            //keyboard.tap(KEY_RETURN);
            //delay(2000);

            //keyboard.println("The quick brown fox");
            //delay(2000);

            // Ctrl+A (select all)
           // keyboard.tap(KEY_A, KEY_MOD_LCTRL);
            //delay(200);

            //keyboard.tap(KEY_F5);
            //delay(200);

            //keyboard.tap(KEY_HOME);
            //delay(100);
            //keyboard.tap(KEY_END);
            keyboard.mediaPress(MEDIA_FAST_FORWARD);
            delay(2000);
            keyboard.mediaRelease();
            while (digitalRead(BUTTON_PIN) == LOW) { delay(10); }
        }
    }

    delay(10);
}
