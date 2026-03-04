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
// Manufacturer: stored in the GATT Device Information service, not in the
//   advertisement. No hard BLE limit, but beyond 64 chars wastes RAM.
#define HID_MAX_DEVICE_NAME_LEN  29
#define HID_MAX_MANUFACTURER_LEN 64

// ─── Security Modes ────────────────────────────────────────────────────────
enum BLEKeyboardSecurity {
    BLE_KB_SEC_JUST_WORKS = 0,  // Auto-pair with no passcode (default)
    BLE_KB_SEC_PASSKEY,         // Require a 6-digit passkey printed to Serial
};

// ─── Debug Log Levels ──────────────────────────────────────────────────────
// Pass one of these to setDebugLevel() before calling begin().
//
// HID_LOG_OFF     — no Serial output from the library (default)
// HID_LOG_NORMAL  — connection, pairing, and advertising events
// HID_LOG_VERBOSE — all of the above plus every HID report sent
enum HIDLogLevel {
    HID_LOG_OFF     = 0,
    HID_LOG_NORMAL  = 1,
    HID_LOG_VERBOSE = 2,
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
     * @param deviceName   BLE name shown to the host during pairing.
     *                     Max 29 chars — longer names are truncated with a
     *                     warning printed in begin(). Defaults to "HijelHID KB".
     * @param manufacturer Manufacturer string in the GATT Device Info service.
     *                     Max 64 chars. Defaults to "Hijel".
     * @param batteryLevel Initial battery percentage (1–100).
     *                     Values of 0 or >100 are clamped with a warning.
     */
    HijelHID_BLEKeyboard(const char* deviceName   = "HijelHID KB",
                         const char* manufacturer = "Hijel",
                         uint8_t     batteryLevel = 100);

    // ─── Debug ───────────────────────────────────────────────────────────
    /**
     * Set the serial debug verbosity. Call before begin().
     *   HID_LOG_OFF     — silent (default)
     *   HID_LOG_NORMAL  — connection, pairing, and advertising events
     *   HID_LOG_VERBOSE — all of the above plus every HID report sent
     */
    void setDebugLevel(HIDLogLevel level);

    // ─── Lifecycle ───────────────────────────────────────────────────────
    /**
     * Initialise BLE, create GATT services, and start advertising.
     * Call once in setup(). Blocks until the NimBLE host task is ready.
     */
    void begin();

    /**
     * Stop advertising and deinitialise the BLE stack.
     */
    void end();

    // ─── Connection State ────────────────────────────────────────────────
    /** Returns true if a host is currently connected. */
    bool isConnected() const;

    /** Returns true if at least one bond is stored in NVS. */
    bool isBonded() const;

    /** Erase all stored bonds. Forces re-pairing on the next connection. */
    void clearBonds();

    // ─── Security ────────────────────────────────────────────────────────
    /**
     * Set the pairing security mode. Must be called before begin().
     *   BLE_KB_SEC_JUST_WORKS — auto-pair, no passcode (default)
     *   BLE_KB_SEC_PASSKEY    — 6-digit passkey printed to Serial
     */
    void setSecurityMode(BLEKeyboardSecurity mode);

    /**
     * Optional callback fired when a passkey is generated (passkey mode only).
     * @param cb  Function receiving the 6-digit passkey as uint32_t.
     */
    void onPassKey(void (*cb)(uint32_t passkey));

    /**
     * Optional callback fired when pairing completes.
     * @param cb  Function receiving true on success, false on failure.
     */
    void onPairingComplete(void (*cb)(bool success));

    // ─── Timing ──────────────────────────────────────────────────────────
    /**
     * Set the global key hold time (how long a key is held before release).
     * Default: HID_DEFAULT_TAP_DELAY_MS (25ms).
     * Increase if the host misses keypresses.
     */
    void setTapDelay(uint16_t ms);

    /**
     * Set the global inter-key gap (delay after release before the next press).
     * Default: HID_DEFAULT_KEY_GAP_MS (25ms).
     * Increase if repeated or back-to-back keys are dropped by the host.
     */
    void setKeyGap(uint16_t ms);

    // ─── Battery ─────────────────────────────────────────────────────────
    /**
     * Update the battery level reported to the host (1–100).
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
     * You are responsible for calling release() or releaseAll() afterwards,
     * and for adding appropriate delays between press/release calls.
     *
     * @param keycode   HID keycode from BLEHIDKeys.h (KEY_*)
     * @param modifiers Optional modifier bitmask (KEY_MOD_* values OR'd together)
     */
    void press(uint8_t keycode, uint8_t modifiers = 0);

    /**
     * Hold a consumer/media key down (sends a consumer report immediately).
     * You are responsible for calling release() afterwards, and for adding
     * appropriate delays between press/release calls.
     *
     * @param usageId  16-bit Consumer Page usage ID from BLEHIDMediaKeys.h (MEDIA_*)
     */
    void press(uint16_t usageId);

    /**
     * Release a previously pressed keyboard key.
     * Passing KEY_NONE (0x00) calls releaseAll().
     *
     * @param keycode  HID keycode to release
     */
    void release(uint8_t keycode);

    /**
     * Release a previously pressed consumer/media key (sends usage ID 0x0000).
     *
     * @param usageId  The same MEDIA_* value passed to press()
     */
    void release(uint16_t usageId);

    /**
     * Release all held keyboard keys, modifiers, and any active consumer key.
     * Sends both a zeroed keyboard report and a zeroed consumer report.
     */
    void releaseAll();

    /**
     * Press and release a keyboard key in one call.
     *
     * Uses the global tap delay and key gap by default (set via setTapDelay()
     * and setKeyGap()). Override per-call with delayMs and keyGap when a
     * specific key needs different timing from the global defaults.
     *
     * @param keycode   HID keycode from BLEHIDKeys.h (KEY_*)
     * @param modifiers Optional modifier bitmask (KEY_MOD_* values OR'd together)
     * @param delayMs   Hold time in ms (default: HID_DEFAULT_TAP_DELAY_MS)
     * @param keyGap    Post-release gap in ms (default: HID_DEFAULT_KEY_GAP_MS)
     */
    void tap(uint8_t keycode, uint8_t modifiers = 0,
             uint16_t delayMs = HID_DEFAULT_TAP_DELAY_MS,
             uint16_t keyGap  = HID_DEFAULT_KEY_GAP_MS);

    /**
     * Press and release a consumer/media key in one call.
     *
     * @param usageId  16-bit Consumer Page usage ID from BLEHIDMediaKeys.h (MEDIA_*)
     * @param delayMs  Hold time in ms (default: HID_DEFAULT_TAP_DELAY_MS)
     * @param keyGap   Post-release gap in ms (default: HID_DEFAULT_KEY_GAP_MS)
     */
    void tap(uint16_t usageId,
             uint16_t delayMs = HID_DEFAULT_TAP_DELAY_MS,
             uint16_t keyGap  = HID_DEFAULT_KEY_GAP_MS);

    // ─── String Output ───────────────────────────────────────────────────
    /**
     * Type one ASCII character. Implements Arduino Print::write().
     * Translates ASCII to the correct HID keycode and modifier automatically.
     * Supports printable ASCII (0x20–0x7E) and control chars \n, \r, \t, BS, ESC.
     *
     * Use print() / println() to send strings — they call write() per character.
     */
    size_t write(uint8_t c) override;

    // ─── LED State (from host) ────────────────────────────────────────────
    bool isNumLockOn()    const;  ///< True if Num Lock LED is active on the host
    bool isCapsLockOn()   const;  ///< True if Caps Lock LED is active on the host
    bool isScrollLockOn() const;  ///< True if Scroll Lock LED is active on the host

    /**
     * Optional callback fired when the host changes the keyboard LED state.
     * @param cb  Function receiving the raw LED bitmask byte (HID_LED_* values).
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
    // Device name and manufacturer are copied into owned buffers in the
    // constructor so the library never holds a dangling pointer to the
    // caller's string (which may be a temporary or go out of scope before
    // begin() is called).
    char                _deviceNameBuf[HID_MAX_DEVICE_NAME_LEN + 1];
    char                _manufacturerBuf[HID_MAX_MANUFACTURER_LEN + 1];
    const char*         _deviceName;    // always points to _deviceNameBuf
    const char*         _manufacturer;  // always points to _manufacturerBuf
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
    bool    _connected;
    uint8_t _ledState;
    uint8_t _keyReport[HID_KEYBOARD_REPORT_SIZE];  // [mod][0x00][k0..k5]

    // ── User Callbacks ────────────────────────────────────────────────────
    void (*_cbPassKey)(uint32_t);
    void (*_cbPairingComplete)(bool);
    void (*_cbLEDChange)(uint8_t);

    // ── BLE Objects ───────────────────────────────────────────────────────
    NimBLEServer*         _pServer;
    NimBLEHIDDevice*      _pHID;
    NimBLECharacteristic* _pKeyboardInput;   // Report ID 0x01 Input  (keys → host)
    NimBLECharacteristic* _pKeyboardOutput;  // Report ID 0x01 Output (LEDs ← host)
    NimBLECharacteristic* _pConsumerInput;   // Report ID 0x02 Input  (media → host)

    // ── Internal Helpers ──────────────────────────────────────────────────
    void    _sendKeyReport();
    void    _sendConsumerReport(uint16_t usageId);
    bool    _addKeycode(uint8_t keycode);
    bool    _removeKeycode(uint8_t keycode);
    bool    _isModifier(uint8_t keycode);
    uint8_t _keycodeToModBit(uint8_t keycode);

    // Logging helpers — produce no code when log level is HID_LOG_OFF
    void _logN(const char* msg);
    void _logNf(const char* fmt, ...);
    void _logV(const char* msg);
    void _logVf(const char* fmt, ...);
};
