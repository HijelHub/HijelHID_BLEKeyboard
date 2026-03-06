# <sub><img width="40" height="40" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/keyboard.svg"></sub> HijelHID_BLEKeyboard

A complete BLE HID keyboard library for ESP32, built on [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) 2.x.

Turn your ESP32 into a BLE HID Keyboard device.

Supports all keys on a standard 104/105-key keyboard with numpad, consumer/media keys, and international/language keys. Keyboard and media keys share a single unified API — `press()`, `release()`, `releaseAll()`, and `tap()` handle both automatically.

Works with iOS, Android, macOS, Windows, and Linux.

 **LIMITED** testing completed on:

 <sub><img width="20" height="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/gray/apple.svg"></sub> iOS 26.3

 <sub><img width="20" height="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/green/android2.svg"></sub> Android 11

 <sub><img width="20" height="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/gray_dark/apple.svg"></sub> macOS Ventura 13.7.8
 
 <sub><img width="20" height="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/windows.svg"></sub> Windows 11 Pro 25H2

<sub> <img width="20" height="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/orange/ubuntu.svg"></sub> Ubuntu 22.04.5 LTS

<sub>**Limited testing includes:** Making a secure connection in Just Works (no passkey) security mode and successfully sending a limited number of keys and media keys to the device.</sub>

---

## Requirements


| Requirement | Version | Tested On |
|---|---|---|
| [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) | 3.x.x | 3.3.7 |
| [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) | >= 2.3.8 [Minimum Required] | 2.3.8 |
| Arduino IDE | NA | 2.3.8 |

Install **[NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)** via Arduino IDE: `Tools → Manage Libraries → search "NimBLE-Arduino"`


Install **[Espressif arduino-esp32](https://github.com/espressif/arduino-esp32)** core via Arduino IDE: `Tools → Boards → Boards Manager → search "arduino-esp32"`

An ESP32 board/module with BLE [ *All except ESP32-S2 and ESP32-P4* ]

---

## Installation

[![Latest Release](https://img.shields.io/github/v/release/HijelHub/HijelHID_BLEKeyboard?label=Latest%20Release&color=blue&style=plastic)](https://github.com/HijelHub/HijelHID_BLEKeyboard/releases/latest)

1. Download the [Latest Release](https://github.com/HijelHub/HijelHID_BLEKeyboard/releases/latest)
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

## <a name="api-reference"></a><sub><img width="30" height="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/code.svg"></sub> API Reference

<details>
<summary>API INDEX</summary>

* [Constructor](#constructor)
* [Setup](#setup)
* [Connection](#connection)
* [Typing Text](#typing-text)
* [Tapping Keys](#tapping-keys)
* [Pressing Keys](#pressing-keys)
* [Media / Consumer Keys](#media--consumer-keys)
* [Timing](#timing)
* [Battery Level](#battery-level)
* [Security / Pairing](#security--pairing)
* [LED State](#led-state)
* [Debug Logging](#debug-logging)

</details>

---

### <a name="constructor"></a>Constructor

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

### <a name="setup"></a>Setup 

Call `begin()` once in `setup()` to start BLE advertising. The device will be discoverable and ready to pair.

```.ino
void setup() {
    keyboard.begin();
}
```

Call `end()` to stop BLE and disconnect.

```.ino
keyboard.end();
```

[[Top]](#api-reference)

---

### <a name="connection"></a>Connection

Check whether a host is connected before sending keys.

```.ino
void loop() {
    if (keyboard.isConnected()) {
        keyboard.tap(KEY_A);
        delay(2000);
    }
}
```
[[Top]](#api-reference)

---

### <a name="typing-text"></a>Typing Text

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

[[Top]](#api-reference)

---

### <a name="tapping-keys"></a>Tapping Keys

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

[[Top]](#api-reference)

---

### <a name="pressing-keys"></a>Pressing Keys

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

### <a name="media--consumer-keys"></a>Media / Consumer Keys

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

### <a name="timing"></a>Timing

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

> **Note:** iOS requires at least ~15ms for both values to reliably register every keypress. The 25ms defaults cover this.

[[Top]](#api-reference)

---

### <a name="battery-level"></a>Battery Level

Update the battery percentage shown to the host at any time.

```.ino
keyboard.setBatteryLevel(85);  // Report 85% battery
```

[[Top]](#api-reference)

---

### <a name="security--pairing"></a>Security / Pairing

By default the keyboard pairs automatically with no passkey. To require a 6-digit passkey, call `setSecurityMode()` before `begin()`.

```.ino
void setup() {
    Serial.begin(115200);
    keyboard.setSecurityMode(BLEKeyboardSecurity::Passkey);  // Must be before begin()
    keyboard.begin();
}
```

| Mode | Behaviour |
|---|---|
| `BLEKeyboardSecurity::JustWorks` | Auto-pair with no passcode (default) |
| `BLEKeyboardSecurity::Passkey` | Require a 6-digit passkey printed to Serial |

When passkey mode is active, the passkey is printed to Serial automatically. You can also register callbacks to handle the passkey and pairing result in your own code.

```.ino
#include <HijelHID_BLEKeyboard.h>

HijelHID_BLEKeyboard keyboard;

// Called when a passkey needs to be displayed to the user.
// Show it however makes sense for your project — Serial, display, LEDs, etc.
void onPassKey(uint32_t passkey) {
    Serial.print("Enter this passkey on your device: ");
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
    keyboard.setSecurityMode(BLEKeyboardSecurity::Passkey);
    keyboard.onPassKey(onPassKey);
    keyboard.onPairingComplete(onPairingComplete);
    keyboard.begin();
}
```

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

[[Top]](#api-reference)

---

### <a name="led-state"></a>LED State

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

### <a name="debug-logging"></a>Debug Logging

Enable Serial logging to help with troubleshooting. Call before `begin()`.

```.ino
void setup() {
    Serial.begin(115200);
    keyboard.setDebugLevel(HIDLogLevel::Normal);  // Connection and pairing events
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

## <sub><img width="30" height="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/card-list.svg"></sub> Platform Notes

| Platform | Pairing | Notes |
|---|---|---|
| **iOS** | Auto or passkey | PnP ID required; LE Secure Connections enabled by default |
| **Android** | Auto | Vendor quirks vary; Just Works works on most devices |
| **macOS** | Auto or passkey | May be asked to "Setup Keyboard", but canceling out of this setup appears to be harmless and tests still passed |
| **Windows 10/11** | Auto or passkey | Caches HID descriptor — fully unpair before flashing a new descriptor during development |
| **Linux (BlueZ)** | Auto | May need `bluetoothctl trust <MAC>` after first pairing |

---

## <sub><img width="30" height="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/emoji-angry-fill.svg"></sub> Troubleshooting

**ESP32 stuck in reboot loop**
- Ensure you have at least Nimble-Arduino 2.3.8 installed.

**Device not appearing in Bluetooth scan**
- Check that no previous bond is stored on both sides — call `keyboard.clearBonds()` and remove from host Bluetooth settings

**Keys not registering / wrong characters**
- Confirm the host keyboard layout is set to US QWERTY
- The library uses US HID keycodes; non-US layouts will produce different characters for punctuation
- Make sure if you are using `press()` that you are correctly setting timing delays and releasing keys

**Windows shows ghost device after reflashing**
- Completely remove the device in Windows Bluetooth settings before flashing
- Call `keyboard.clearBonds()` in setup temporarily, then remove and re-add

**Media keys not working on some apps**
- Not all applications respond to consumer HID keys — test with the OS-level media player first

---

## <sub><img width="30" height="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/trophy.svg"></sub> Acknowledgements

- The hundreds of contributors and maintainers of the [Espressif arduino-esp32](https://github.com/espressif/arduino-esp32) library
- My fellow Canadian Ryan Powell AKA [h2zero](https://github.com/h2zero) for his continued work on [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) and all the contributors to the project.
- My good friend Claude over at [Anthropic](https://www.anthropic.com/) for working tirelessly and for always telling me how smart, and right, and great I am. Even when I'm being an absolute moron.

---

## <sub><img width="30" height="30" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/blue/piggy-bank.svg"></sub> Support This Project

If you found this library useful, your support would mean a lot!

<sub><img width="20" height="20" src="https://raw.githubusercontent.com/HijelHub/GitStrap_SVG_Icons/b674246b8f46d8bc2c75f3cf5cf395a370b86ae2/icons/purple/stripe.svg"></sub>  [Securely Donate with Stripe](https://buy.stripe.com/fZu8wQdt01Oi5wWdKycMM01)

If you are intending to use this library in a commercial product, your support is **expected**.

##

Feel free to post your known working hardware/OS versions and combos in the Discussions section.

Please take the time to **properly** report any bugs you come across.

---



