#pragma once
/**
 * HijelHID_BLEKeyboard.h
 *
 * BLE HID keyboard library for ESP32, built on NimBLE-Arduino 2.x.
 *
 * Supports all keys on a standard 104/105-key keyboard with numpad,
 * consumer/media keys, international and language keys.
 *
 * Both keyboard keys (KEY_*) and consumer/media keys (MEDIA_*) are sent
 * through the same press() / release() / releaseAll() / tap() API.
 * The correct HID report is selected automatically based on the value passed:
 *   - KEY_*   values are uint8_t  — sent via the keyboard report (Report ID 1)
 *   - MEDIA_* values are uint16_t — sent via the consumer report (Report ID 2)
 *
 * Dependencies:
 *   - NimBLE-Arduino >= 2.3.8  (install via Arduino Library Manager)
 *   - ESP32 Arduino Core >= 3.3.7
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

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEHIDDevice.h>
#include <NimBLECharacteristic.h>
#include <string>
#include "BLEHIDKeys.h"
#include "BLEHIDMediaKeys.h"

// ─── Report IDs ────────────────────────────────────────────────────────────
#define HID_REPORT_ID_KEYBOARD  0x01
#define HID_REPORT_ID_CONSUMER  0x02

// ─── Report Sizes ──────────────────────────────────────────────────────────
// Keyboard input:  8 bytes — [modifiers][reserved][key0..key5]
// LED output:      1 byte  — bitmask sent from host to keyboard
// Consumer input:  2 bytes — single 16-bit usage ID
#define HID_KEYBOARD_REPORT_SIZE  8
#define HID_LED_REPORT_SIZE       1
#define HID_CONSUMER_REPORT_SIZE  2

// ─── LED Bitmask Constants (host → keyboard) ──────────────────────────────
// Received via the LED output report. Use with isNumLockOn() etc. or the
// raw value passed to the onLEDChange() callback.
#define HID_LED_NUM_LOCK    0x01
#define HID_LED_CAPS_LOCK   0x02
#define HID_LED_SCROLL_LOCK 0x04
#define HID_LED_COMPOSE     0x08
#define HID_LED_KANA        0x10

// ─── Timing Defaults ───────────────────────────────────────────────────────
// TAP_DELAY_MS: how long a key is held down before release (milliseconds).
// KEY_GAP_MS:   gap between the release of one tap and the next press (ms).
//
// Both values default to 25ms. iOS/iPadOS require at least ~15ms for each
// to reliably register every keypress, including repeated identical keys.
// Adjust globally with setTapDelay() / setKeyGap(), or override per-call
// using the delayMs and keyGap parameters on tap().
#define HID_DEFAULT_TAP_DELAY_MS  25
#define HID_DEFAULT_KEY_GAP_MS    25

// ─── String Length Limits ──────────────────────────────────────────────────
// Device name: BLE scan response packet is 31 bytes; 2 bytes are consumed by
//   AD type+length overhead, leaving 29 bytes for the name string.
//   Names longer than 29 chars are truncated; a warning prints in begin().
//
// Manufacturer: stored in the GATT Device Information Service characteristic
//   (UUID 0x2A29), read only after connection — not in the advertising packet.
//   The Bluetooth Core Spec defines 512 bytes as the maximum GATT attribute
//   length. Strings longer than 512 chars are truncated; a warning prints in begin().
#define HID_MAX_DEVICE_NAME_LEN    29
#define HID_MAX_MANUFACTURER_LEN  512

// ─── Security Modes ────────────────────────────────────────────────────────
enum class BLEKeyboardSecurity : uint8_t {
    JustWorks = 0,  // Auto-pair with no passcode (default)
    Passkey,        // Require a 6-digit passkey printed to Serial
};

// ─── Debug Log Levels ──────────────────────────────────────────────────────
// Pass one of these to setDebugLevel() before calling begin().
//
// HIDLogLevel::Off     — no Serial output from the library (default)
// HIDLogLevel::Normal  — connection, pairing, and advertising events
// HIDLogLevel::Verbose — all of the above plus every HID report sent
enum class HIDLogLevel : uint8_t {
    Off     = 0,
    Normal  = 1,
    Verbose = 2,
};

// ─── BLE Lifecycle State ──────────────────────────────────────────────────
// Used internally to track the BLE stack state across begin()/end()/kill().
enum class _BLEState : uint8_t {
    Stopped = 0,  // Not running — begin() will start or restart BLE
    Running = 1,  // Actively advertising or connected — begin() is a no-op
    Killed  = 2,  // Permanently shut down via kill() — begin() is refused
};

// ─── Forward Declaration ───────────────────────────────────────────────────
class HijelHID_BLEKeyboard;

// ─── Internal: BLE Server + Security Callbacks ────────────────────────────
// Not part of the public API. Used internally by begin().
class _HijelKBServerCallbacks : public NimBLEServerCallbacks {
public:
    _HijelKBServerCallbacks(HijelHID_BLEKeyboard* parent) : _parent(parent) {}
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override;
    uint32_t onPassKeyDisplay() override;
    void onConfirmPassKey(NimBLEConnInfo& connInfo, uint32_t pass_key) override;
private:
    HijelHID_BLEKeyboard* _parent;
};

// ─── Internal: LED Output Report Callback ─────────────────────────────────
// Not part of the public API. Receives LED state changes from the host.
class _HijelKBLEDCallbacks : public NimBLECharacteristicCallbacks {
public:
    _HijelKBLEDCallbacks(HijelHID_BLEKeyboard* parent) : _parent(parent) {}
    void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override;
private:
    HijelHID_BLEKeyboard* _parent;
};

// ─── HijelHID_BLEKeyboard ─────────────────────────────────────────────────
class HijelHID_BLEKeyboard : public Print {
public:

    // ─── Constructor ─────────────────────────────────────────────────────

    /**
     * Create a BLE HID keyboard.
     *
     * `deviceName` is the BLE name shown to the host during pairing (max 29 chars,
     * longer names are truncated with a warning printed in `begin()`).
     * Defaults to `"HijelHID KB"`.
     *
     * `manufacturer` is the manufacturer string in the GATT Device Info service
     * (max 512 chars, the Bluetooth Core Spec GATT attribute limit).
     * Defaults to `"Hijel"`.
     *
     * `batteryLevel` is the initial battery percentage (1–100).
     * Values of 0 or >100 are clamped with a warning.
     */
    HijelHID_BLEKeyboard(const char* deviceName   = "HijelHID KB",
                         const char* manufacturer = "Hijel",
                         uint8_t     batteryLevel = 100);

    // ─── Debug ───────────────────────────────────────────────────────────

    /**
     * Set the Serial debug verbosity. Call before `begin()`.
     *
     * `HIDLogLevel::Off` — silent (default).
     * `HIDLogLevel::Normal` — connection, pairing, and advertising events.
     * `HIDLogLevel::Verbose` — all of the above, plus every HID report sent.
     */
    void setDebugLevel(HIDLogLevel level);

    // ─── Lifecycle ───────────────────────────────────────────────────────

    /**
     * Initialise BLE, register GATT services, and start advertising.
     * Call once in `setup()`. Blocks until the NimBLE host task is ready.
     *
     * After `end()`, calling `begin()` again restarts advertising without
     * reinitialising the BLE stack — all GATT objects are reused.
     *
     * After `kill()`, calling `begin()` is refused with a warning.
     */
    void begin();

    /**
     * Stop advertising and disconnect any connected host.
     * The BLE stack and GATT objects remain in memory so `begin()` can
     * restart quickly without reinitialisation.
     *
     * For permanent shutdown with full memory cleanup, use `kill()`.
     */
    void end();

    /**
     * Permanently shut down and deinitialise the BLE stack.
     * Disconnects, stops advertising, tears down the NimBLE stack, and
     * frees all BLE memory. `begin()` cannot be called after `kill()`.
     *
     * A small per-cycle leak (~48 bytes) exists in the ESP-IDF NimBLE
     * port and cannot be avoided. For pause/resume use `end()`/`begin()`.
     */
    void kill();

    // ─── Connection State ────────────────────────────────────────────────

    /** Returns `true` if a host is currently connected. */
    bool isConnected() const;

    /** Returns `true` if at least one bond is stored in NVS. */
    bool isBonded() const;

    /** Erase all stored bonds. Forces re-pairing on the next connection. */
    void clearBonds();

    // ─── Security ────────────────────────────────────────────────────────

    /**
     * Set the pairing security mode. Must be called before `begin()`.
     *
     * `BLEKeyboardSecurity::JustWorks` — auto-pair, no passcode (default).
     * `BLEKeyboardSecurity::Passkey` — require a 6-digit passkey printed to Serial.
     */
    void setSecurityMode(BLEKeyboardSecurity mode);

    /**
     * Optional callback fired when a passkey is generated (passkey mode only).
     * `cb` receives the 6-digit passkey as `uint32_t`.
     * Use this to display the passkey on your own hardware instead of Serial.
     */
    void onPassKey(void (*cb)(uint32_t passkey));

    /**
     * Optional callback fired when pairing completes or fails.
     * `cb` receives `true` on success, `false` on failure.
     */
    void onPairingComplete(void (*cb)(bool success));

    // ─── Timing ──────────────────────────────────────────────────────────

    /**
     * Set the global key hold time in milliseconds (how long a key is held before release).
     * Default: `HID_DEFAULT_TAP_DELAY_MS` (25 ms).
     * Increase if the host misses keypresses.
     */
    void setTapDelay(uint16_t ms);

    /**
     * Set the global inter-key gap in milliseconds (delay after release before the next press).
     * Default: `HID_DEFAULT_KEY_GAP_MS` (25 ms).
     * Increase if repeated or back-to-back keys are dropped by the host.
     */
    void setKeyGap(uint16_t ms);

    // ─── Battery ─────────────────────────────────────────────────────────

    /**
     * Update the battery percentage reported to the host.
     * Valid range is 1–100; values outside this range are clamped with a warning.
     */
    void setBatteryLevel(uint8_t level);

    // ─── Key Input (keyboard and consumer/media) ─────────────────────────
    //
    // press() / release() / releaseAll() / tap() handle both keyboard keys
    // (KEY_*) and consumer/media keys (MEDIA_*) through overloaded signatures.
    //
    // KEY_*   constants are cast to uint8_t in BLEHIDKeys.h   → keyboard report
    // MEDIA_* constants are uint16_t in BLEHIDMediaKeys.h     → consumer report
    //
    // The compiler selects the correct overload automatically at compile time —
    // no runtime value checks, no ambiguity, no casting required in user code.

    /**
     * Hold a keyboard key down (sends a key-down report immediately).
     * Supports up to 6 simultaneous non-modifier keys (6KRO).
     * You must call `release()` or `releaseAll()` afterwards, with appropriate delays.
     * Modifiers added here remain held until explicitly cleared via `releaseAll()`
     * or by releasing the corresponding modifier keycode.
     *
     * `keycode` is a `KEY_*` constant from `BLEHIDKeys.h`.
     * `modifiers` is an optional bitmask of `KEY_MOD_*` values OR'd together.
     * For media keys use `press(uint16_t usageId)` with a `MEDIA_*` constant from `BLEHIDMediaKeys.h`.
     */
    void press(uint8_t keycode, uint8_t modifiers = 0);

    /**
     * Hold a consumer/media key down (sends a consumer report immediately).
     * You must call `release()` or `releaseAll()` afterwards, with appropriate delays.
     *
     * `usageId` is a `MEDIA_*` constant from `BLEHIDMediaKeys.h`.
     * For keyboard keys use `press(uint8_t keycode)` with a `KEY_*` constant from `BLEHIDKeys.h`.
     */
    void press(uint16_t usageId);

    /**
     * Release a previously pressed keyboard key.
     * Passing `KEY_NONE` (0x00) calls `releaseAll()`.
     *
     * `keycode` is the same `KEY_*` value passed to `press()`.
     */
    void release(uint8_t keycode);

    /**
     * Release a previously pressed consumer/media key (sends usage ID 0x0000).
     *
     * `usageId` is the same `MEDIA_*` value passed to `press()`.
     */
    void release(uint16_t usageId);

    /**
     * Release all held keyboard keys, modifiers, and any active consumer key.
     * Sends both a zeroed keyboard report and a zeroed consumer report.
     * Safe to call at any time to clear stuck keys.
     */
    void releaseAll();

    /**
     * Press and release a keyboard key in one call.
     *
     * `keycode` is a `KEY_*` constant from `BLEHIDKeys.h`.
     * `modifiers` is an optional bitmask of `KEY_MOD_*` values OR'd together.
     * `delayMs` overrides the global hold time for this tap only (0 = use `setTapDelay()` value).
     * `keyGap` overrides the global post-release gap for this tap only (0 = use `setKeyGap()` value).
     * For media keys use `tap(uint16_t usageId)` with a `MEDIA_*` constant from `BLEHIDMediaKeys.h`.
     */
    void tap(uint8_t keycode, uint8_t modifiers = 0,
             uint16_t delayMs = 0, uint16_t keyGap = 0);

    /**
     * Press and release a consumer/media key in one call.
     *
     * `usageId` is a `MEDIA_*` constant from `BLEHIDMediaKeys.h`.
     * `delayMs` overrides the global hold time for this tap only (0 = use `setTapDelay()` value).
     * `keyGap` overrides the global post-release gap for this tap only (0 = use `setKeyGap()` value).
     * For keyboard keys use `tap(uint8_t keycode)` with a `KEY_*` constant from `BLEHIDKeys.h`.
     */
    void tap(uint16_t usageId,
             uint16_t delayMs = 0, uint16_t keyGap = 0);

    // ─── String Output ───────────────────────────────────────────────────

    /**
     * Type one ASCII character. Implements `Arduino Print::write()`.
     * Translates the character to the correct `KEY_*` keycode and modifier automatically.
     * Supports printable ASCII (0x20–0x7E) and control chars `\n`, `\r`, `\t`, BS, ESC.
     * Use `print()` / `println()` to send full strings — they call `write()` per character.
     */
    size_t write(uint8_t c) override;

    /**
     * Type a string of ASCII characters. Overrides `Print::write(buf, size)`.
     * Each character is sent via the single-byte `write(uint8_t)` overload,
     * which calls `tap()` internally with the current global timing.
     */
    size_t write(const uint8_t* buffer, size_t size) override;

    // ─── LED State (from host) ────────────────────────────────────────────

    /** Returns `true` if the Num Lock LED is active on the host. */
    bool isNumLockOn()    const;

    /** Returns `true` if the Caps Lock LED is active on the host. */
    bool isCapsLockOn()   const;

    /** Returns `true` if the Scroll Lock LED is active on the host. */
    bool isScrollLockOn() const;

    /**
     * Optional callback fired when the host changes the keyboard LED state.
     * `cb` receives the raw LED bitmask byte (`HID_LED_*` values).
     */
    void onLEDChange(void (*cb)(uint8_t leds));

    // ─── Internal Callbacks (do not call directly) ────────────────────────
    void     _onConnect();
    void     _onDisconnect();
    void     _onAuthComplete(bool success);
    uint32_t _onPassKeyDisplay();
    void     _onConfirmPassKey(uint32_t passkey);
    void     _onLEDWrite(uint8_t ledByte);

private:
    // ── Configuration ─────────────────────────────────────────────────────
    // std::string owns its storage — no fixed buffers, no aliasing pointers.
    // Device name is enforced to HID_MAX_DEVICE_NAME_LEN in the constructor
    // because the BLE advertising packet has a hard 29-byte limit.
    // Manufacturer is enforced to HID_MAX_MANUFACTURER_LEN (512 bytes),
    // the maximum GATT attribute length per the Bluetooth Core Spec.
    std::string         _deviceName;
    std::string         _manufacturer;
    uint8_t             _batteryLevel;
    BLEKeyboardSecurity _secMode;
    uint16_t            _tapDelay;
    uint16_t            _keyGap;
    HIDLogLevel         _logLevel;

    // ── Constructor Validation Flags ──────────────────────────────────────
    // Set in the constructor when an argument is out of range. Warnings are
    // deferred to begin() so they print after Serial.begin() has been called.
    bool _nameTruncated;  // device name exceeded HID_MAX_DEVICE_NAME_LEN
    bool _mfrTruncated;   // manufacturer exceeded HID_MAX_MANUFACTURER_LEN
    bool _batClamped;     // battery level was 0 or >100

    // ── Runtime State ─────────────────────────────────────────────────────
    _BLEState _state;             // lifecycle: Stopped → Running → Stopped (or Killed)
    volatile bool _connected;     // true while a host is connected (set from NimBLE task)
    volatile uint8_t _ledState;   // LED bitmask from host (written from NimBLE task)
    uint8_t  _keyReport[HID_KEYBOARD_REPORT_SIZE];  // [mod][0x00][k0..k5]
    bool     _consumerActive;  // true while a consumer/media key is held down
    uint32_t _lastReportMs;   // millis() of last successful notify — used to detect host-side
                              // selective suspend (Windows drops first packet after ~2s idle)

    // ── User Callbacks ────────────────────────────────────────────────────
    void (*_cbPassKey)(uint32_t);
    void (*_cbPairingComplete)(bool);
    void (*_cbLEDChange)(uint8_t);

    // ── BLE Objects ───────────────────────────────────────────────────────
    NimBLEServer*              _pServer;
    NimBLEHIDDevice*           _pHID;
    NimBLECharacteristic*      _pKeyboardInput;   // Report ID 0x01 Input  (keys → host)
    NimBLECharacteristic*      _pKeyboardOutput;  // Report ID 0x01 Output (LEDs ← host)
    NimBLECharacteristic*      _pConsumerInput;   // Report ID 0x02 Input  (media → host)
    _HijelKBServerCallbacks*   _pServerCb;        // Owned by us, passed to NimBLE
    _HijelKBLEDCallbacks*      _pLEDCb;           // Owned by us, passed to NimBLE

    // ── Internal Helpers ──────────────────────────────────────────────────
    void    _sendKeyReport();
    void    _sendConsumerReport(uint16_t usageId);
    bool    _addKeycode(uint8_t keycode);
    bool    _removeKeycode(uint8_t keycode);
    bool    _isModifier(uint8_t keycode);
    uint8_t _keycodeToModBit(uint8_t keycode);

    // Logging helpers — produce no code when log level is HIDLogLevel::Off
    void _logN(const char* msg);
    void _logNf(const char* fmt, ...);
    void _logV(const char* msg);
    void _logVf(const char* fmt, ...);
};
