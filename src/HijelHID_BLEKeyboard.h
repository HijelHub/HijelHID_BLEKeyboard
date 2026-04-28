#pragma once
/**
 * HijelHID_BLEKeyboard.h
 *
 * BLE HID keyboard library for ESP32, built on NimBLE-Arduino 2.3.8+
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
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
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
#define HID_DEFAULT_TAP_DELAY_MS    25
#define HID_DEFAULT_KEY_GAP_MS      25
#define HID_AFTER_WAKE_SETTLE_MS   250
#define HID_WINDOWS_PRIME_MS       800  // zero-report prime threshold (Windows BLE HID resume)

// ─── Peripheral Latency / Idle Power Saving ────────────────────────────────
// When no key has been pressed for HID_IDLE_THRESHOLD_MS milliseconds, the
// library requests a connection parameter update with slave latency applied.
// This allows the BLE radio to skip up to HID_IDLE_LATENCY consecutive
// connection events, reducing the effective radio duty cycle from ~133/sec
// to ~1.6/sec during idle — a significant power saving with no impact on
// typing responsiveness (the first keypress immediately restores full rate).
//
// The host is not required to accept connection parameter update requests.
// iOS, macOS, Windows, and Linux will generally honour them, but the timing
// and acceptance are host-controlled. Power saving is best-effort.
//
// On single-core ESP32 variants (C3, C6, C2, H2), the NimBLE host task and
// the Arduino loop task share one CPU. The param update is posted to the
// NimBLE task queue from the Arduino loop task — it takes effect once the
// NimBLE task gets CPU time, which happens naturally at the next delay()
// call inside tap(). This means the transition has slightly more latency on
// single-core boards than on dual-core (ESP32, S3), but is functionally correct.
#define HID_IDLE_THRESHOLD_MS   5000   // inactivity before requesting idle params
#define HID_IDLE_LATENCY          80   // slave latency events to skip when idle
#define HID_CONN_INTERVAL          6   // shared connection interval: 6 × 1.25ms = 7.5ms (Active and Idle states both use this interval; slave latency is what differs)
#define HID_CONN_TIMEOUT         300   // supervision timeout: 300 × 10ms = 3000ms

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
    Passkey,        // Numeric Comparison — 6-digit code shown on host and Serial
};

// Alias — use HIDSecurity for consistency with BLEMouse library.
// BLEKeyboardSecurity remains valid for existing sketches.
using HIDSecurity = BLEKeyboardSecurity;

// ─── Debug Log Levels ──────────────────────────────────────────────────────
// Pass one of these to setLogLevel() / setDebugLevel() before calling begin().
//
// HIDLogLevel::Off     — no Serial output from the library (default)
// HIDLogLevel::Normal  — connection, pairing, and advertising events
// HIDLogLevel::Verbose — all of the above plus every HID report sent
enum class HIDLogLevel : uint8_t {
    Off     = 0,
    Normal  = 1,
    Verbose = 2,
};

// ─── Forward Declaration ───────────────────────────────────────────────────
class HijelHID_BLEKeyboard;

// ─── Internal Callbacks ───────────────────────────────────────────────────
// Not part of the public API. Scoped inside HijelHID_Internal to avoid
// polluting the global namespace. Used internally by begin().
namespace HijelHID_Internal {

class KBServerCallbacks : public NimBLEServerCallbacks {
public:
    KBServerCallbacks(HijelHID_BLEKeyboard* parent) : _parent(parent) {}
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override;
    void onConfirmPassKey(NimBLEConnInfo& connInfo, uint32_t pass_key) override;
private:
    HijelHID_BLEKeyboard* _parent;
};

class KBLEDCallbacks : public NimBLECharacteristicCallbacks {
public:
    KBLEDCallbacks(HijelHID_BLEKeyboard* parent) : _parent(parent) {}
    void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override;
private:
    HijelHID_BLEKeyboard* _parent;
};

} // namespace HijelHID_Internal

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
     *
     * Legacy name — prefer `setLogLevel()` for consistency with the BLEMouse library.
     */
    void setDebugLevel(HIDLogLevel level);

    /**
     * Set the Serial debug verbosity. Call before `begin()`.
     *
     * `HIDLogLevel::Off` — silent (default).
     * `HIDLogLevel::Normal` — connection, pairing, and advertising events.
     * `HIDLogLevel::Verbose` — all of the above, plus every HID report sent.
     */
    void setLogLevel(HIDLogLevel level) { setDebugLevel(level); }

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
     * A small leak exists in the ESP-IDF NimBLE port and cannot be avoided.
     * For pause/resume use `end()`/`begin()`.
     */
    void kill();

    // ─── Connection State ────────────────────────────────────────────────

    /** Returns `true` if a host is currently connected at the GAP layer.
     *  Note: a GAP connection exists briefly before authentication completes —
     *  use `isPaired()` as a more reliable ready-to-send signal, or use
     *  `afterWake()` after light sleep rather than polling directly. */
    bool isConnected() const;

    /**
     * Returns `true` if the host is connected and fully authenticated.
     * More reliable than `isConnected()` as a ready-to-send signal —
     * `isConnected()` becomes true briefly before the LTK re-encryption
     * handshake completes on reconnect, while `isPaired()` waits for the
     * full authentication to finish.
     */
    bool isPaired() const;

    /**
     * Returns the number of milliseconds since the last HID report was sent.
     * Useful for deciding when to enter light or deep sleep from your sketch.
     * Reset to zero on the first keypress after wake, and stamped inside
     * `afterWake()` on reconnection so the value is meaningful immediately
     * after the wake sequence completes.
     *
     * Check `isPaired()` before acting on this value — it returns a large
     * number before the first report has been sent after boot or deep sleep.
     */
    uint32_t getIdleTime() const;

    /** Returns `true` if at least one bond is stored in NVS. */
    bool isBonded() const;

    /** Erase all stored bonds. Forces re-pairing on the next connection. */
    void clearBonds();

    // ─── Security ────────────────────────────────────────────────────────

    /**
     * Set the pairing security mode. Must be called before `begin()`.
     *
     * `HIDSecurity::JustWorks` — auto-pair, no passcode (default).
     * `HIDSecurity::Passkey` — Numeric Comparison pairing. A 6-digit code is
     *   printed to Serial (and passed to `setPasskeyCallback()` if registered);
     *   the user confirms it matches on the host side.
     *
     * NOTE: If `HIDLogLevel::Off` is set and no passkey callback is registered,
     * the passkey code will not be displayed anywhere. Pairing will still complete,
     * but MITM protection is effectively equivalent to JustWorks since the user
     * cannot verify the code. Always register a passkey callback when using
     * `HIDLogLevel::Off` in Passkey mode.
     */
    void setSecurityMode(BLEKeyboardSecurity mode);

    /**
     * Register a callback for Passkey pairing (optional).
     * Called with the 6-digit code when a host initiates Passkey pairing.
     *
     * If no callback is registered and `HIDLogLevel` is `Off`, the passkey
     * code will not be displayed — the user will be unable to verify the
     * matching code on the host side, and MITM protection is not effective.
     */
    void setPasskeyCallback(void (*cb)(uint32_t passkey)) { onPassKey(cb); }

    /**
     * Register a callback for Passkey pairing (optional).
     * Called with the 6-digit Numeric Comparison code (passkey mode only).
     * `cb` receives the code as `uint32_t`. If not registered, the code is
     * printed to Serial only when log level is `Normal` or `Verbose`.
     *
     * Legacy name — prefer `setPasskeyCallback()` for consistency with the
     * BLEMouse library.
     */
    void onPassKey(void (*cb)(uint32_t passkey));

    /**
     * Optional callback fired when pairing completes or fails.
     * `cb` receives `true` on success, `false` on failure.
     */
    void onPairingComplete(void (*cb)(bool success));

    /**
     * Use a random static BLE address instead of the ESP32's fixed public MAC address.
     * Must be called before `begin()`.
     *
     * When `true`, the device advertises with a randomly generated static address,
     * causing hosts to treat it as a previously-unseen device. This bypasses stale
     * bond cache on hosts (particularly Android) that have cached a failed or
     * incomplete pairing attempt and silently refuse to pair again.
     *
     * The address is stable for the lifetime of the BLE stack — it does not rotate
     * mid-session. It changes after each power cycle, so hosts that bonded
     * successfully will need to re-pair after a reboot when this mode is enabled.
     *
     * Default: `false` (use the ESP32's fixed public MAC address).
     */
    void setRandomAddress(bool enable);

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

    /**
     * Set the BLE radio transmit power.
     * Valid range is 1–8, mapping onto the ESP32's 8 discrete power steps:
     *
     *   1 = -12 dBm  (lowest — centimetre range, maximum power saving)
     *   2 =  -9 dBm
     *   3 =  -6 dBm
     *   4 =  -3 dBm
     *   5 =   0 dBm
     *   6 =  +3 dBm
     *   7 =  +6 dBm
     *   8 =  +9 dBm  (highest — maximum range, default)
     *
     * Values outside 1–8 are clamped with a warning. To stop the radio
     * entirely use `end()` rather than setting power to 0.
     *
     * Can be called before or after `begin()`. The value is re-applied
     * automatically on every `begin()` / `end()` + `begin()` cycle.
     */
    void setTxPower(uint8_t level);

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

    // ─── Sleep Hooks ─────────────────────────────────────────────────────

    /**
     * Call immediately before entering light sleep (`esp_light_sleep_start()`).
     *
     * Releases all held keys so the host does not see keys stuck down across
     * the sleep boundary, sets the internal priming flag so the first report
     * after wakeup correctly primes the Windows HID resume handshake, stops
     * the idle timer, and clears any pending idle transition flag.
     *
     * Safe to call even if not connected — `releaseAll()` is a no-op when
     * there is no active connection.
     *
     * Not required before deep sleep — deep sleep is a full chip reset and
     * `begin()` in `setup()` handles all necessary initialisation on wakeup.
     */
    void beforeSleep();

    /**
     * Call immediately after returning from light sleep.
     *
     * Handles the full post-wakeup reconnection sequence within a single
     * shared timeout budget (`setAfterWakeTimeout()`):
     *
     * 1. Waits for any stale authenticated connection to drop — a fast bonded
     *    reconnect can fire before the old supervision timeout is processed,
     *    causing reports to be sent into a connection that is about to be torn down.
     * 2. Waits for the host to reconnect and complete LTK re-encryption.
     * 3. Settles briefly (`HID_AFTER_WAKE_SETTLE_MS`) to allow the host to
     *    finish HID descriptor negotiation before the first report is sent.
     * 4. Restarts the idle timer so the connection transitions correctly to
     *    Idle if no key is pressed within `HID_IDLE_THRESHOLD_MS`.
     *
     * The total time spent in `afterWake()` never exceeds the value set by
     * `setAfterWakeTimeout()` (default 15000ms).
     * If any step times out, `afterWake()` returns without sending any reports.
     * Check `isPaired()` after `afterWake()` returns to confirm success.
     *
     * Not required after deep sleep — `begin()` in `setup()` handles everything.
     */
    void afterWake();

    /**
     * Set the total time budget for `afterWake()` in milliseconds (default 15000ms).
     * This is shared across all internal wait steps — the total time spent in
     * `afterWake()` will never exceed this value. Must be greater than 0.
     */
    void setAfterWakeTimeout(uint32_t ms) { _afterWakeTimeoutMs = ms; }

    // ─── Internal Callbacks (do not call directly) ────────────────────────
    void        _onConnect(uint16_t connHandle);
    void        _onDisconnect(uint16_t connHandle);
    void        _onAuthComplete(uint16_t connHandle, bool success);
    void        _onConfirmPassKey(uint32_t passkey);
    void        _onLEDWrite(uint8_t ledByte);
    static void _idleTimerCallback(TimerHandle_t xTimer);

private:
    // ── Internal State Enums ──────────────────────────────────────────────
    // Private nested types — not part of the public API and not visible in
    // the global namespace.
    //
    // _BLEState — tracks the BLE stack lifecycle across begin()/end()/kill().
    //   Stopped → Running → Stopped (or Killed permanently).
    //
    // _ConnState — tracks the active connection's activity state, independent
    //   of the BLE stack lifecycle. Drives idle power-saving conn param updates.
    //   Disconnected — no host connected.
    //   Connecting   — GAP connection established, waiting for authentication.
    //   Active       — authenticated, full-rate connection params (latency = 0).
    //   Idle         — no input for HID_IDLE_THRESHOLD_MS, reduced-rate params applied.
    enum class _BLEState : uint8_t {
        Stopped = 0,  // Not running — begin() will start or restart BLE
        Running = 1,  // Actively advertising or connected — begin() is a no-op
        Killed  = 2,  // Permanently shut down via kill() — begin() is refused
    };
    enum class _ConnState : uint8_t {
        Disconnected = 0,
        Connecting,
        Active,
        Idle,
    };

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
    bool                _useRandomAddress;  // if true, use random static address in begin()

    // ── Constructor Validation Flags ──────────────────────────────────────
    // Set in the constructor when an argument is out of range. Warnings are
    // deferred to begin() so they print after Serial.begin() has been called.
    bool _nameTruncated;  // device name exceeded HID_MAX_DEVICE_NAME_LEN
    bool _mfrTruncated;   // manufacturer exceeded HID_MAX_MANUFACTURER_LEN
    bool _batClamped;     // battery level was 0 or >100

    // ── Runtime State ─────────────────────────────────────────────────────
    _BLEState _state;             // lifecycle: Stopped → Running → Stopped (or Killed)
    volatile bool _connected;     // true while a host is connected (set from NimBLE task)
    volatile bool _authenticated; // true after onAuthenticationComplete(success=true)
                                  // cleared on disconnect. Used by afterWake() as a
                                  // reliable "host is fully ready" signal.
    volatile uint8_t _ledState;   // LED bitmask from host (written from NimBLE task)
    uint8_t  _keyReport[HID_KEYBOARD_REPORT_SIZE];  // [mod][0x00][k0..k5]
    bool     _consumerActive;      // true while a consumer/media key is held down
    volatile bool _reportPrimingNeeded; // true when the next report must be preceded by a
                                        // zero-report to wake the host (Windows HID resume
                                        // handshake). Set on construction, disconnect, end(),
                                        // and beforeSleep(). Also set by the idle timer callback.
    uint32_t _lastReportMs;             // millis() when the last HID report was sent.
                                        // Used to detect Windows BLE HID selective suspend —
                                        // if no report has been sent for HID_WINDOWS_PRIME_MS,
                                        // a zero-report is sent first to absorb the dropped
                                        // packet from the Windows resume handshake.
    uint8_t  _txPowerLevel;        // current TX power level (1–8). Stored so begin() can
                                   // re-apply it on every end()/begin() restart cycle.
    uint32_t _afterWakeTimeoutMs;  // total time budget for afterWake() across all wait steps

    // ── Idle Power Management ─────────────────────────────────────────────
    // _connState    — current activity state of the BLE connection, independent
    //                 of the BLE stack lifecycle (_BLEState).
    // _connHandle   — NimBLE connection handle, stored in _onConnect() and used
    //                 by _updateConnParams(). BLE_HS_CONN_HANDLE_NONE when not
    //                 connected. volatile because it is written from the NimBLE
    //                 callback task and read from the Arduino loop task.
    // _idleTimer    — one-shot FreeRTOS software timer. Created in begin(), started
    //                 in _onAuthComplete(), reset on every key input event,
    //                 stopped in _onDisconnect() / end() / beforeSleep().
    // _pendingIdleTransition — set true by the timer callback (FreeRTOS timer
    //                 daemon task). Checked and cleared at the top of press() /
    //                 release() / releaseAll() on the Arduino loop task, which
    //                 then calls _updateConnParams() safely from that context.
    //                 This flag-based approach avoids calling NimBLE APIs from
    //                 the timer daemon task, keeping single-core boards safe.
    volatile _ConnState _connState;
    volatile uint16_t   _connHandle;
    TimerHandle_t     _idleTimer;
    volatile bool     _pendingIdleTransition;

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
    HijelHID_Internal::KBServerCallbacks*   _pServerCb;  // Owned by us, passed to NimBLE
    HijelHID_Internal::KBLEDCallbacks*      _pLEDCb;      // Owned by us, passed to NimBLE

    // ── Internal Helpers ──────────────────────────────────────────────────
    void    _sendKeyReport();
    void    _sendConsumerReport(uint16_t usageId);
    void    _updateConnParams(uint16_t minInterval, uint16_t maxInterval,
                              uint16_t latency, uint16_t timeout);
    void    _transitionToActive();
    void    _startIdleTimer();
    void    _stopIdleTimer();
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
