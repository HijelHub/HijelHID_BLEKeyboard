# <sub><img width="40" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/keyboard.svg"></sub> HijelHID_BLEKeyboard

A complete Bluetooth Low Energy (BLE) HID keyboard library for ESP32, built on [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino).

Turn your ESP32 into a BLE HID Keyboard. Great for creating a physical device, or just emulating one.

Supports all keys on a standard 104/105-key keyboard with numpad, consumer/media keys, and international/language keys. Keyboard and media keys share a single unified API — `press()`, `release()`, `releaseAll()`, and `tap()` handle both automatically.

Works with iOS, Android, macOS, Windows, and Linux.

 **Testing completed on:**

 <sub><img width="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/gray/apple.svg"></sub> iOS 26.3 - Fully Tested

 <sub><img width="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/green/android2.svg"></sub> Android 16 - Fully Tested

 <sub><img width="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/gray_dark/apple.svg"></sub> macOS Ventura 13.7.8 - Fully Tested
 
 <sub><img width="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/windows.svg"></sub> Windows 11 Pro 25H2 - Fully Tested

<sub> <img width="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/orange/ubuntu.svg"></sub> Ubuntu 22.04.5 LTS - Fully Tested


---

## Requirements


| Requirement | Version | Tested On |
|---|---|---|
| [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) | 3.x.x | 3.3.12 |
| [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) | >= 2.3.8 [Minimum Required] | 2.5.2 |
| Arduino IDE | NA | 2.3.10 |

Install **[NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)** via Arduino IDE: `Tools → Manage Libraries → search "NimBLE-Arduino"`


Install **[Espressif arduino-esp32](https://github.com/espressif/arduino-esp32)** core via Arduino IDE: `Tools → Boards → Boards Manager → search "arduino-esp32"`

An ESP32 board/module with BLE [ *All except ESP32-S2 and ESP32-P4* ]

---

## Installation

[![Latest Release](https://img.shields.io/github/release/HijelHub/HijelHID_BLEKeyboard.svg?style=plastic&label=Latest%20Release&color=blue)](https://github.com/HijelHub/HijelHID_BLEKeyboard/releases/latest)
[![Release Date](https://img.shields.io/github/release-date/HijelHub/HijelHID_BLEKeyboard.svg?style=plastic&label=Release%20Date&color=brightgreen)](https://github.com/HijelHub/HijelHID_BLEKeyboard/releases/latest)
<!-- [![Downloads](https://img.shields.io/github/downloads/HijelHub/HijelHID_BLEKeyboard/latest/total.svg?style=plastic&label=Downloads&color=green)](https://github.com/HijelHub/HijelHID_BLEKeyboard/releases/latest) -->


Arduino Library Manager: `Sketch → Include Library → Manage Libraries` Search for "HijelHID"

--- OR ---

Manual Zip Install:
1. Download the [Latest ZIP](https://github.com/HijelHub/HijelHID_BLEKeyboard/releases/latest/download/HijelHID_BLEKeyboard.zip) [Direct Download Link]
2. In Arduino IDE: `Sketch → Include Library → Add .ZIP Library`
3. Select the downloaded zip

---

## Quick Start

```.ino
#include <HijelHID_BLEKeyboard.h>

HijelHID_BLEKeyboard keyboard;

void setup() {
    keyboard.begin();
}

void loop() {
    delay(5000);
    // Open a text editor on your host device
    if (keyboard.isConnected()) {
        // Print "Hello, World!"
        keyboard.print("Hello, ");
        keyboard.println("World!");
        // Press and Tap "ESP32!"
        keyboard.press(KEY_LSHIFT);
        delay(25);
        keyboard.tap(KEY_E);
        keyboard.tap(KEY_S);
        keyboard.tap(KEY_P);
        keyboard.release(KEY_LSHIFT);
        keyboard.tap(KEY_3);
        keyboard.tap(KEY_2);
        // tap an exclamation point "!"
        keyboard.tap(KEY_1, KEY_MOD_LSHIFT);
        keyboard.tap(KEY_RETURN);
        
        keyboard.releaseAll();
    }
}
```

---

## <a name="api-reference"></a><sub><img width="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/code.svg"></sub> API Reference

<details>
<summary> CLICK FOR API INDEX </summary>

* [Constructor](#constructor)
* [Lifecycle](#lifecycle)
* [Connection](#connection)
* [Typing Text](#typing-text)
  * [Keyboard Layouts](#keyboard-layouts)
* [Tapping Keys](#tapping-keys)
  * [Shortcuts on Non-US Layouts](#shortcuts-non-us)
* [Pressing Keys](#pressing-keys)
* [Media / Consumer Keys](#media--consumer-keys)
* [Timing](#timing)
* [Battery Level](#battery-level)
* [Appearance](#appearance)
* [Security / Pairing](#security--pairing)
  * [Manual Pairing Mode](#manual-pairing-mode)
* [Power Saving](#power-saving)
  * [TX Power](#tx-power)
  * [Connection Interval](#connection-interval)
  * [Idle Radio Power Saving](#idle-radio-power-saving)
  * [Light Sleep](#light-sleep)
  * [Deep Sleep](#deep-sleep)
* [LED State](#led-state)
* [Debug Logging](#debug-logging)

</details>

---

<br>

## <a name="constructor"></a>Constructor

Create a keyboard object with an optional custom name, manufacturer, and battery level.

```.ino
// Default — shows as "HijelHID KB" when pairing
HijelHID_BLEKeyboard keyboard;

// Custom name and manufacturer
HijelHID_BLEKeyboard keyboard("My Keyboard", "My Company", 100);
```

| Parameter | Description | Default |
|---|---|---|
| `deviceName` | Name shown to the host when pairing. Max 29 chars. | `"HijelHID KB"` |
| `manufacturer` | Manufacturer name. Max 512 chars. | `"Hijel"` |
| `batteryLevel` | Starting battery level (1–100). | `100` |

[[Top]](#api-reference)

---

<br>

## <a name="lifecycle"></a>Lifecycle

Call `begin()` once in `setup()` to start BLE advertising. The device will be discoverable and ready to pair.

```.ino
void setup() {
    keyboard.begin();
}
```

Call `end()` to disconnect and stop advertising. The BLE stack stays in memory so `begin()` can restart quickly without reinitialisation.

```.ino
keyboard.end();         // pause — BLE stack stays alive
keyboard.begin();       // restart — fast, no full reinit
```

Call `kill()` to permanently shut down and deinitialise the BLE stack, freeing all BLE memory. `begin()` cannot be called after `kill()`.

```.ino
keyboard.kill();        // permanent shutdown — frees ~38KB of heap
// keyboard.begin();    // ← refused after kill(), logs a warning
```

| Method | Effect | `begin()` after? |
|---|---|---|
| `begin()` | Initialise BLE and start advertising | N/A |
| `end()` | Disconnect and stop advertising, BLE stack stays in memory | Yes — fast restart |
| `kill()` | Full teardown, frees all BLE memory | No — permanently shut down |

> [!NOTE]
> A small bounded memory leak (~308 bytes) remains after `kill()` due to an apperant upstream issue with NimBLE. Since `begin()` is refused after `kill()`, this leak cannot compound. For pause/resume scenarios, use `end()` and `begin()` instead.


[[Top]](#api-reference)

---

<br>

## <a name="connection"></a>Connection

`isConnected()`

Check whether a host is connected before sending keys.

```.ino
void loop() {
    if (keyboard.isConnected()) {
        keyboard.tap(KEY_A);
        delay(2000);
    }
}
```

<br>

`isPaired()`

A more reliable ready-to-send signal than `isConnected()`. It returns `true` only after the host has fully authenticated — `isConnected()` becomes true briefly before the encryption handshake completes on reconnect, which can cause the first report to be dropped.

```.ino
// Wait until fully ready before sending
while (!keyboard.isPaired()) {
    delay(10);
}
keyboard.println("Ready!");
```

<br>

`getIdleTime()`

Returns the number of milliseconds since the last HID report was sent. Use it in your sketch to decide when to enter light or deep sleep.

```.ino
if (keyboard.isPaired() && keyboard.getIdleTime() > 30000) {
    keyboard.beforeSleep();
    esp_light_sleep_start();
    keyboard.afterWake();
}
```
[[Top]](#api-reference)

---

<br>

## <a name="typing-text"></a>Typing Text

`print()` and `println()` type a string of characters. Upper case, punctuation, and spaces are handled automatically. `println()` adds a newline at the end.

```.ino
keyboard.print("Hello, World!");
keyboard.println("This line ends with Enter");
```

You can also send characters one at a time using `write()`.

```.ino
const char* str = "Writing Hello from ESP32!";
for (int i = 0; str[i] != '\0'; i++) {
    keyboard.write((uint8_t)str[i]);
}
keyboard.tap(KEY_RETURN);
```

### <a name="keyboard-layouts"></a>Keyboard Layouts ![New in 0.6.0](https://img.shields.io/badge/New_in-0.6.0-blue?style=plastic)

`print()` sends *key positions*, and the host computer turns them into characters using its own keyboard layout setting. If your computer is set to a layout other than US, tell the library which one so text comes out correctly. Characters with accents (`é`, `ñ`, `ç`, `ü`…) can be typed on layouts that have them.

```.ino
keyboard.setKeyboardLayout(HIDLayout_German);   // can be changed any time
keyboard.print("Grüße, café");
```

The layout only affects `print()`, `println()` and `write()`. `tap()`, `press()` and media keys send physical keys and are unaffected — for shortcuts that follow your layout's key labels, see [Shortcuts on Non-US Layouts](#shortcuts-non-us).

| Windows / Linux | macOS | Host layout |
|---|---|---|
| `HIDLayout_US` (default) | `HIDLayout_US` | US |
| `HIDLayout_US_International` | — | US-International |
| `HIDLayout_UK` | `HIDLayout_UK_Mac` | United Kingdom / British |
| `HIDLayout_German` | `HIDLayout_German_Mac` | German |
| `HIDLayout_French` | `HIDLayout_French_Mac` | French (AZERTY) |
| `HIDLayout_Spanish` | `HIDLayout_Spanish_Mac` | Spanish (Spain) / Spanish – ISO |
| `HIDLayout_Spanish_LatAm` | — | Latin American |
| `HIDLayout_Italian` | `HIDLayout_Italian_Mac` | Italian / Italian – Pro |
| `HIDLayout_Portuguese` | `HIDLayout_Portuguese_Mac` | Portuguese |
| `HIDLayout_Portuguese_Brazil` | `HIDLayout_Portuguese_Brazil_Mac` | Portuguese (Brazil ABNT2) / Brazilian |

Only the layouts your sketch uses take up flash.

> [!NOTE]
> **US is fully tested. The other layouts are best effort and not yet tested on real hardware.** They are generated from [Unicode CLDR](https://cldr.unicode.org/) keyboard data and cross-checked against the Arduino Keyboard library's layouts. If you use one, please report how it works in the Discussions — corrections welcome.
>
> - Characters a layout cannot type are skipped (e.g. `` ` `` and `~` do not exist on the Windows Italian layout).
> - Accent keys that are "dead keys" on your layout (e.g. `^` on German) are sent followed by a space so the character prints on its own.
> - If Caps Lock is on at the host, letters come out in the opposite case.
> - macOS: if macOS identifies the keyboard as ISO, it may swap the key left of `1` with the key left of `Z`, swapping two characters (e.g. `<` and `^` on German).
>
> Layouts are generated by `extras/layout_generator/` — see its README to add or regenerate one.

[[Top]](#api-reference)

---

<br>

## <a name="tapping-keys"></a>Tapping Keys

`tap()` is the simplest way to press and release a single key. Use `KEY_*` constants from `src/BLEHIDKeys.h`.

```.ino
// Tap a single key
keyboard.tap(KEY_RETURN);
keyboard.tap(KEY_SPACE);
keyboard.tap(KEY_ESCAPE);

// Tap with a modifier (Shift, Ctrl, Alt, etc.)
keyboard.tap(KEY_A, KEY_MOD_LSHIFT);    // Types uppercase 'A'
keyboard.tap(KEY_C, KEY_MOD_LCTRL);    // Ctrl+C (copy)
keyboard.tap(KEY_Z, KEY_MOD_LCTRL);    // Ctrl+Z (undo)

// Multiple modifiers — combine with |
keyboard.tap(KEY_DELETE, KEY_MOD_LCTRL | KEY_MOD_LALT);  // Ctrl+Alt+Del
```

### <a name="shortcuts-non-us"></a>Shortcuts on Non-US Layouts ![New in 0.6.0](https://img.shields.io/badge/New_in-0.6.0-blue?style=plastic)

`KEY_*` names are **US key positions**. Keys without a character — Enter, Tab, Escape, Delete, arrows, F-keys, modifiers, numpad, media — are the same on every layout, so they work everywhere. Letter and symbol keys are not: on a German computer `KEY_Z` is the key labelled **Y** (so `tap(KEY_Z, KEY_MOD_LCTRL)` is Ctrl+Y), and on French AZERTY `KEY_A` is the key labelled **Q**.

`keycodeFor()` returns the key that carries a character on the active layout, so shortcuts and held keys follow the labels on your keyboard. Pass the result to `tap()`, `press()` or `release()`.

```.ino
keyboard.setKeyboardLayout(HIDLayout_French);

keyboard.tap(keyboard.keycodeFor('a'), KEY_MOD_LCTRL);   // Ctrl+A (Select All) on AZERTY
keyboard.tap(keyboard.keycodeFor('z'), KEY_MOD_LCTRL);   // Ctrl+Z (Undo) on any layout

keyboard.press(keyboard.keycodeFor("é"));                // hold the é key
delay(500);
keyboard.release(keyboard.keycodeFor("é"));
```

- **Text → `print()`. Shortcuts and held keys → `keycodeFor()`.** On US, `keycodeFor('z')` returns `KEY_Z`, so both give the same result.
- **It returns the key only.** Shift or AltGr the character needs are not added — on German, `keycodeFor('@')` is the Q key, not AltGr+Q. That is what you want for shortcuts; to type `@`, use `print("@")`.
- **Accented characters:** use the string form `keycodeFor("é")` or `keycodeFor(U'é')` — `'é'` is not a single `char` in C++.
- **Returns 0** when no single key carries the character, e.g. `ê` on French (dead key + `e`). `print()` can still type it. Note that `release(0)` releases all keys.
- `keycodeFor()` sends nothing and works before `begin()`.

[[Top]](#api-reference)

---

<br>

## <a name="pressing-keys"></a>Pressing Keys

Use `press()` and `release()` when you need to hold a key down. You must add `delay()` calls yourself between each step.

```.ino
// Hold Shift while pressing a key, then release
keyboard.press(KEY_H, KEY_MOD_LSHIFT);
delay(25);
keyboard.releaseAll();
delay(25);

keyboard.press(KEY_I);
delay(25);
keyboard.release(KEY_I);
delay(25);
keyboard.press(KEY_I);
delay(25);
keyboard.releaseAll();
```

> **Tip:** For most use cases, `tap()` is simpler and handles all timing automatically. Use `press()`/`release()` only when you need precise control over hold timing.

`releaseAll()` releases every held key at once — useful as a safety call to clear any stuck keys.

```.ino
keyboard.releaseAll();
```

[[Top]](#api-reference)

---

<br>

## <a name="media--consumer-keys"></a>Media / Consumer Keys

Media keys work with both `tap()` and `press()`. Use `MEDIA_*` constants from `src/BLEHIDMediaKeys.h`.

```.ino
// Tap a media key (press and release automatically)
keyboard.tap(MEDIA_PLAY_PAUSE);
keyboard.tap(MEDIA_VOLUME_UP);
keyboard.tap(MEDIA_VOLUME_DOWN);
keyboard.tap(MEDIA_MUTE);
keyboard.tap(MEDIA_NEXT_TRACK);
keyboard.tap(MEDIA_PREV_TRACK);

// Hold a media key down, then release
keyboard.press(MEDIA_VOLUME_UP);
delay(500);
keyboard.releaseAll();
```

[[Top]](#api-reference)

---

<br>

## <a name="timing"></a>Timing

By default, `tap()` holds each key for **25ms** and waits **25ms** after release before the next key. You can adjust these globally, or override them for a single `tap()` call.

```.ino
// Change timing globally (affects all tap() and print/println calls)
keyboard.setTapDelay(40);  // hold each key for 40ms
keyboard.setKeyGap(40);    // wait 40ms after each release

// Override timing for a single tap
keyboard.tap(KEY_A);                    // uses global timing
keyboard.tap(KEY_A, 0, 60, 40);        // hold 60ms, gap 40ms
keyboard.tap(KEY_A, KEY_MOD_LSHIFT, 60, 40);  // with modifier + custom timing
keyboard.tap(MEDIA_VOLUME_UP, 60, 40); // media key with custom timing
```

[[Top]](#api-reference)

---

<br>

## <a name="battery-level"></a>Battery Level

Update the battery percentage shown to the host at any time.

```.ino
keyboard.setBatteryLevel(85);  // Report 85% battery
```

[[Top]](#api-reference)

---

<br>

## <a name="appearance"></a>Appearance ![New in 0.6.0](https://img.shields.io/badge/New_in-0.6.0-blue?style=plastic)

The Appearance tells the host what kind of device this is. Hosts may use it to pick the icon shown in Bluetooth settings and device lists. Default is a keyboard. Call `setAppearance()` before `begin()`.

```.ino
keyboard.setAppearance(APPEARANCE_HID_PRESENTATION_REMOTE);  // a presentation clicker
keyboard.begin();
```

| Value | Device |
|---|---|
| `APPEARANCE_HID_KEYBOARD` | Keyboard (default) |
| `APPEARANCE_HID` | Generic HID device — macro pad, foot pedal, button box |
| `APPEARANCE_HID_PRESENTATION_REMOTE` | Presentation clicker |
| `APPEARANCE_HID_BARCODE_SCANNER` | Barcode scanner |
| `APPEARANCE_HID_CARD_READER` | Card reader (RFID / magnetic stripe) |

Any other value can be passed as a hex code, e.g. `keyboard.setAppearance(0x04CD);` (Control Device: Dial). Every value defined by the Bluetooth SIG is listed in `extras/BLEHIDAppearanceFull.h` — values outside the list above are untested.

> [!NOTE]
> The Appearance only changes how the device is described — keys are sent the same way. Icons are chosen by each OS, and some show a generic icon or none (e.g. Linux shows none for a presentation remote). Hosts remember the value from pairing, so remove the device on the host and re-pair after changing it.

[[Top]](#api-reference)

---

<br>

## <a name="security--pairing"></a>Security / Pairing

By default the keyboard pairs automatically with no passkey. To require a passkey challenge, call `setSecurityMode()` before `begin()`.

```.ino
void setup() {
    Serial.begin(115200);
    keyboard.setSecurityMode(HIDSecurity::Passkey);  // Must be before begin()
    keyboard.begin();
}
```

| Mode | Behaviour |
|---|---|
| `HIDSecurity::JustWorks` | Auto-pair with no passcode (default) |
| `HIDSecurity::Passkey` | Require a numerical comparison passkey printed to Serial |

When passkey mode is active, the passkey is printed to Serial automatically. You can also register callbacks to handle the passkey and pairing result in your own code.

```.ino
#include <HijelHID_BLEKeyboard.h>

HijelHID_BLEKeyboard keyboard;

// Called when a passkey needs to be displayed to the user.
// Show it however makes sense for your project — Serial, display, LEDs, etc.
void onPassKey(uint32_t passkey) {
    Serial.print("Does this passkey match on your device? ");
    Serial.println(passkey);
}

// Called when pairing completes or fails.
void onPairingComplete(bool success) {
    if (success) {
        Serial.println("Pairing successful — keyboard is ready.");
    } else {
        Serial.println("Pairing failed. Try removing and re-pairing.");
    }
}

void setup() {
    Serial.begin(115200);
    keyboard.setSecurityMode(HIDSecurity::Passkey);
    keyboard.setPasskeyCallback(onPassKey);
    keyboard.onPairingComplete(onPairingComplete);
    keyboard.begin();
}
```

> [!NOTE]
> If `HIDLogLevel::Off` is set and no passkey callback is registered, the passkey code will not be displayed anywhere. Always register a passkey callback when using `HIDLogLevel::Off` in Passkey mode.

To forget all previously paired devices and force re-pairing:

```.ino
keyboard.clearBonds();
```

To check if a bond is already stored:

```.ino
if (keyboard.isBonded()) {
    Serial.println("A device is already bonded.");
}
```

The keyboard remembers up to 3 bonded devices. Pairing a 4th removes the oldest.

### Manual Pairing Mode ![New in 0.6.0](https://img.shields.io/badge/New_in-0.6.0-blue?style=plastic)

By default any nearby device can pair whenever the keyboard isn't connected. In Manual mode, only devices that are already bonded can connect — a new device can only pair after you open pairing mode, e.g. with a button, like most Bluetooth devices.

Pairing mode ends when a device connects, when `exitPairingMode()` is called, or after the timeout (default 30 seconds).

```.ino
#define PAIR_BUTTON 0
#define PAIR_LED    2

void setup() {
    pinMode(PAIR_BUTTON, INPUT_PULLUP);
    pinMode(PAIR_LED, OUTPUT);
    keyboard.setPairingMode(HIDPairingMode::Manual);
    keyboard.begin();
}

void loop() {
    if (digitalRead(PAIR_BUTTON) == LOW) {
        keyboard.enterPairingMode(30000);  // accept a new device for 30 seconds
    }
    digitalWrite(PAIR_LED, keyboard.isPairing());  // LED on while pairing
}
```

| Function | Description |
|---|---|
| `setPairingMode(mode)` | `HIDPairingMode::Auto` (default) or `HIDPairingMode::Manual` |
| `enterPairingMode(ms)` | Open pairing mode (Manual only) |
| `exitPairingMode()` | Close pairing mode early |
| `isPairing()` | `true` while pairing mode is open |

> [!NOTE]
> Manual mode blocks unknown devices from connecting, closing the window where anyone could pair after `begin()`. It does not change how the underlying BLE stack handles security — pairing security beyond this depends on NimBLE and the host.

[[Top]](#api-reference)

---

<br>

## <a name="power-saving"></a>Power Saving

### TX Power

The BLE radio transmit power can be reduced to save energy when the device is operating at close range. Valid levels are 1–8, 1 being the lowest TX power. Default is set to 8.

```.ino
keyboard.setTxPower(1);  // -12 dBm, lowest range/setting 
keyboard.setTxPower(8);  // +9 dBm, maximum range/setting (default)
```

### Connection Interval ![New in 0.6.0](https://img.shields.io/badge/New_in-0.6.0-blue?style=plastic)

How often the keyboard and host exchange data. Default is 7.5ms. A longer interval uses less radio time, which can fix audio stuttering when the host also streams to Bluetooth speakers or headphones. The trade-off is that keys reach the host more slowly — fine for media controllers, may be noticeable for fast typing. Call before `begin()`. Range 7.5–1000ms.

```.ino
keyboard.setConnIntervalMs(50);  // 50ms interval
keyboard.begin();
```

The host decides whether to accept the requested interval, and some hosts only accept certain values. The library checks what the host actually applied. If the interval is rejected, it asks again, then falls back gracefully to an interval the host accepts — first 15ms (if you asked for less), then the last interval the host was seen using. Typing works either way, the fallback lasts until the host disconnects, and with logging on (`HIDLogLevel::Normal`) the interval actually used is shown.

### Idle Radio Power Saving

The library automatically reduces the BLE radio duty cycle after 5 seconds of inactivity. The radio skips connection events during idle, reducing wake-ups from ~133/sec to ~2/sec. Full rate is restored immediately on the next keypress. No user code changes are required. The host decides whether to accept the reduced rate, so savings vary by host.

Use `getIdleTime()` in your sketch to check how long the keyboard has been idle, for example to decide when to enter light or deep sleep.

```.ino
if (keyboard.isPaired() && keyboard.getIdleTime() > 30000) {
    // No key sent for 30 seconds — enter light sleep
    keyboard.beforeSleep();
    esp_light_sleep_start();
    keyboard.afterWake();
}
```

### Light Sleep

Call `beforeSleep()` immediately before entering light sleep and `afterWake()` immediately after. `afterWake()` blocks until the host has fully reconnected and the HID stack has settled — or until the default timout (15000ms) expires.<br>
If needed, you can change the timeout value by setting `setAfterWakeTimeout()` in your `setup()` function.

```.ino
keyboard.beforeSleep();
esp_light_sleep_start();
keyboard.afterWake();  // blocks until host is ready

keyboard.println("Woke from light sleep!");
```


![New in 0.6.0](https://img.shields.io/badge/New_in-0.6.0-blue?style=plastic)

After the host reconnects, `afterWake()` waits a short settle time (default 250ms) before returning, so the host is ready for the first keys. Some hosts take longer to get ready after reconnecting; if the first characters typed after waking are missing, increase it with `setAfterWakeSettle()` in `setup()`. iPhone/iPad may need around 1500ms.

```.ino
keyboard.setAfterWakeSettle(1500);  // wait 1500ms after reconnecting before afterWake() returns
```

### Deep Sleep

No special library calls are needed for deep sleep. However, you should call `beforeSleep()` before sleeping to release any held keys cleanly, then call begin() as normal in setup() on wakeup. The stored bond survives deep sleep and the host will reconnect automatically.

```.ino
// Before sleeping:
keyboard.beforeSleep();
esp_deep_sleep_start();

// On wakeup, setup() runs as normal:
void setup() {
    keyboard.begin();  // reconnects via stored bond automatically
}
```

[[Top]](#api-reference)

---

<br>

## <a name="led-state"></a>LED State

The host sends LED state back to the keyboard (Num Lock, Caps Lock, Scroll Lock). You can read the current state or set a callback to react to changes.

```.ino
// Read current state
if (keyboard.isCapsLockOn()) {
    Serial.println("Caps Lock is ON");
}

// React to changes with a callback
keyboard.onLEDChange([](uint8_t leds) {
    if (leds & HID_LED_CAPS_LOCK) {
        Serial.println("Caps Lock ON");
    } else {
        Serial.println("Caps Lock OFF");
    }
});
```

| Function | Returns |
|---|---|
| `isCapsLockOn()` | `true` if Caps Lock is active |
| `isNumLockOn()` | `true` if Num Lock is active |
| `isScrollLockOn()` | `true` if Scroll Lock is active |

[[Top]](#api-reference)

---

<br>

## <a name="debug-logging"></a>Debug Logging

Enable Serial logging to help with troubleshooting. Call before `begin()`.

```.ino
void setup() {
    Serial.begin(115200);
    keyboard.setLogLevel(HIDLogLevel::Normal);  // Connection and pairing events
    keyboard.begin();
}
```

| Level | Output |
|---|---|
| `HIDLogLevel::Off` | No output (default) |
| `HIDLogLevel::Normal` | Connection, pairing, and advertising events |
| `HIDLogLevel::Verbose` | All of the above, plus every HID report sent |

[[Top]](#api-reference)

---

---

## <sub><img width="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/card-list.svg"></sub> Platform Notes

| Platform | Pairing | Notes |
|---|---|---|
| **iOS** | Auto or passkey | When attempting to change the deviceName, new names will only appear:<br> &emsp; 1) AFTER you re-pair the device <br> &emsp; 2) IF the new deviceName is SHORTER than the cached name<br> &emsp; 3) OR the new deviceName is LESS than 20 Characters<br>iOS hides the on-screen keyboard while a keyboard is connected. Sending `keyboard.tap(MEDIA_EJECT);` toggles it back on. |
| **Android** | Auto | Vendor quirks vary; Just Works works on most devices |
| **macOS** | Auto or passkey | May be asked to "Setup Keyboard", but canceling out of this setup appears to be harmless and tests still passed<br>With `HIDSecurity::Passkey`, use a keyboard Appearance (the default). With a Mouse Appearance (`0x03C2`) macOS showed no passkey and pairing stalled. |
| **Windows 10/11** | Auto or passkey | Caches HID descriptor — fully unpair before flashing a new descriptor during development |
| **Linux (BlueZ)** | Auto or passkey | Initial testing revealed odd modifier key behaviour, the culprit was found to be ibus intercepting modifier keys. You can uninstall ibus with `sudo apt purge ibus` if you don't need it. |

---

## <sub><img width="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/emoji-angry-fill.svg"></sub> Troubleshooting

**ESP32 stuck in reboot loop**
- Ensure you have at least Nimble-Arduino 2.3.8 installed.

**Device not appearing in Bluetooth scan**
- Check that no previous bond is stored on both sides — call `keyboard.clearBonds()` and remove from host Bluetooth settings

**Keys not registering / wrong characters**
- Confirm the library layout matches the host's keyboard layout — `setKeyboardLayout()` (default US)
- Make sure if you are using `press()` that you are correctly setting timing delays and releasing keys

**Windows shows ghost device after reflashing**
- Completely remove the device in Windows Bluetooth settings before flashing
- Call `keyboard.clearBonds()` in setup temporarily, then remove and re-add

**Media keys not working on some apps**
- Not all applications respond to consumer HID keys — test with the OS-level media player first

---

## <sub><img width="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/trophy.svg"></sub> Acknowledgements

- The hundreds of contributors and maintainers of the [Espressif arduino-esp32](https://github.com/espressif/arduino-esp32) library
- My fellow Canadian Ryan Powell AKA [h2zero](https://github.com/h2zero) for his continued work on [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) and all the contributors to the project.
- [Unicode CLDR](https://cldr.unicode.org/) for the keyboard layout data (© Unicode, Inc., Unicode License v3).
- [MaximilianoMeyer](https://github.com/MaximilianoMeyer) for the Brazilian ABNT2 layout work that led to keyboard layout support.
- My good friend Claude over at [Anthropic](https://www.anthropic.com/) for working tirelessly and for always telling me how smart, and right, and great I am. Even when I'm being an absolute moron.

---

## <sub><img width="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/piggy-bank.svg"></sub> Support This Project

If you found this library useful, your support would mean a lot!

<sub><img width="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/purple/stripe.svg"></sub>  [Securely Donate with Stripe](https://buy.stripe.com/fZu8wQdt01Oi5wWdKycMM01)

If you are intending to use this library in a commercial product, your support is **expected**.

<br>

### My other projects you might like:

[HijelHub/HijelHID_BLEMouse](https://github.com/HijelHub/HijelHID_BLEMouse)

A Bluetooth Low Energy HID Mouse library for ESP32

[HijelHub/HijelHub_Dashboard](https://github.com/HijelHub/HijelHub_Dashboard)

A Github Analytics dashboard that automatically Gets, Stores, and Displays traffic data from multiple repos on a single page. Built completely on Githubs Free Tier.

##

Feel free to post your known working hardware/OS versions and combos in the Discussions section.

Please take the time to **properly** report any bugs you come across.

---
