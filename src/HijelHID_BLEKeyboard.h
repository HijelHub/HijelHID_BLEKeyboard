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
#include <string>
#include "BLEHIDKeys.h"
#include "BLEHIDMediaKeys.h"
#include "HIDKeyboardLayouts.h"
#include "BLEHIDAppearance.h"

// NimBLE port header — provides ble_npl_callout (NimBLE's own timer) and the
// default event queue. Same conditional include NimBLE-Arduino uses internally
// (valid for NimBLE-Arduino 2.3.8 through 2.5.x).
#if defined(CONFIG_NIMBLE_CPP_IDF)
#include "nimble/nimble_port.h"
#else
#include "nimble/porting/nimble/include/nimble/nimble_port.h"
#endif

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
#define HID_AFTER_WAKE_SETTLE_MS   250  // default for setAfterWakeSettle()
#define HID_WINDOWS_PRIME_MS       800  // zero-report prime threshold (Windows BLE HID resume)

// ─── Peripheral Latency / Idle Power Saving ────────────────────────────────
// When no report has been sent for HID_IDLE_THRESHOLD_MS milliseconds, the
// library requests idle connection parameters with peripheral latency applied,
// letting the radio skip connection events. At the default interval the radio
// wakes ~13×/sec instead of ~133×/sec. The first keypress restores the Active
// parameters — keys still go out at the next connection event.
//
// The idle request follows Apple's Accessory Design Guidelines (the strictest
// common host), so it is also valid on every other host:
//   interval  — the Active interval rounded up to a multiple of 15ms (min 15ms)
//   latency   — up to HID_IDLE_LATENCY (4), reduced so interval × (latency+1) ≤ 6s
//   timeout   — max(6s, 3 × interval × (latency+1)), capped at 18s
// The Active request (7.5ms / latency 0 / 3s) is unchanged and tested on all hosts.
//
// HID_IDLE_LATENCY is kept low on purpose: hosts can take several seconds to
// apply the Active request after the first keypress, and until then reports
// only go out at the radio's idle wake-ups. Latency 4 keeps that gap to
// ~75ms at the default interval (latency 30 gave visibly choppy typing for
// ~3s after a pause on Windows, ~465ms chunks). Apple allows up to 30.
//
// The host is not required to accept connection parameter update requests.
// Acceptance is host-controlled. Power saving is best-effort.
//
// The idle timer is a NimBLE callout: its callback runs on the NimBLE host task,
// so the idle request is made from the correct task on single- and dual-core
// ESP32 variants alike.
//
// These are library defaults. They are not overridable by #define in a sketch —
// the library .cpp is compiled separately and never sees sketch-level defines.
// The connection interval can be changed at runtime with setConnIntervalMs().
//
// HID_CONN_TIMEOUT: the BLE spec requires timeout > (1 + latency) × interval × 2.
// The Active request uses latency 0, so 3000ms is valid for every interval
// setConnIntervalMs() accepts (max 1000ms).
#define HID_IDLE_THRESHOLD_MS   5000   // inactivity before requesting idle params
#define HID_IDLE_LATENCY           4   // max peripheral latency when idle (Apple allows up to 30)
#define HID_IDLE_TIMEOUT_MIN     600   // idle supervision timeout floor: 6s   (Apple min)
#define HID_IDLE_TIMEOUT_MAX    1800   // idle supervision timeout cap:   18s  (Apple max)
#define HID_CONN_INTERVAL          6   // default connection interval: 6 × 1.25ms = 7.5ms
#define HID_CONN_TIMEOUT         300   // supervision timeout: 300 × 10ms = 3000ms

// Active-request check. The host decides whether to apply a connection parameter
// request (macOS ignores some). After asking for the Active parameters, the
// library checks the connection's actual latency on later key events (no sooner
// than HID_ACTIVE_CHECK_MS after the request). If the host has not applied them,
// it asks again — at most HID_ACTIVE_MAX_RETRIES times. If the host still refuses,
// the library falls back for the rest of that connection: first to 15ms (if the
// requested interval was below 15ms, the Apple minimum), then to the last interval
// the host was seen using at latency 0. If neither works it keeps the parameters
// the host chose (typing works either way). Disconnect or setConnIntervalMs()
// resets the fallback.
#define HID_ACTIVE_CHECK_MS     2000   // time the host gets to apply a request
#define HID_ACTIVE_MAX_RETRIES     3   // re-asks per request before giving up

// Valid range for setConnIntervalMs(), in 1.25ms units.
#define HID_CONN_INTERVAL_MIN      6   // 7.5ms  (BLE spec minimum)
#define HID_CONN_INTERVAL_MAX    800   // 1000ms

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

// ─── Pairing Modes ─────────────────────────────────────────────────────────
// Auto   — any device can pair whenever the keyboard is advertising (default).
// Manual — only already-bonded devices can connect. A new device can pair
//          only while pairing mode is open (enterPairingMode()).
enum class HIDPairingMode : uint8_t {
    Auto = 0,
    Manual,
};

#define HID_DEFAULT_PAIRING_WINDOW_MS  30000  // enterPairingMode() default window

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

    /**
     * Set how new devices are allowed to pair.
     *
     * `HIDPairingMode::Auto` — any device can pair while the keyboard is
     *   advertising (default — same behaviour as previous versions).
     * `HIDPairingMode::Manual` — only already-bonded devices can connect.
     *   A new device can pair only while pairing mode is open, started with
     *   `enterPairingMode()` (e.g. from a button press). Unknown devices are
     *   disconnected immediately.
     *
     * Can be called before or after `begin()`; applies to the next connection.
     * Note: Manual mode also rejects unbonded secondary BLE clients.
     */
    void setPairingMode(HIDPairingMode mode);

    /**
     * Open pairing mode so a new device can pair (Manual mode only).
     *
     * Pairing mode ends when any device connects and secures the link,
     * when `exitPairingMode()` is called, or after `windowMs` milliseconds.
     * `windowMs` = 0 uses the default (30000 ms).
     * Has no effect in Auto mode.
     */
    void enterPairingMode(uint32_t windowMs = HID_DEFAULT_PAIRING_WINDOW_MS);

    /** End pairing mode early (Manual mode only). */
    void exitPairingMode();

    /**
     * Returns `true` while pairing mode is open (Manual mode only) —
     * e.g. to drive a "pairing" LED. Always `false` in Auto mode.
     * This means new devices may pair right now, not that a pairing
     * handshake is in progress.
     */
    bool isPairing() const;

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

    /**
     * Set the BLE connection interval in milliseconds. Call before `begin()`.
     * Default: 7.5 ms. Valid range 7.5–1000 ms; values are rounded to the
     * nearest 1.25 ms (the BLE unit) and clamped with a warning.
     *
     * A longer interval reduces BLE air-time, which can stop audio stuttering
     * on hosts that share one radio between this keyboard and Bluetooth audio.
     * Trade-off: key reports reach the host more slowly. Fine for media
     * controllers; may be noticeable when typing quickly.
     *
     * The host decides whether to accept the requested interval.
     */
    void setConnIntervalMs(uint16_t ms);

    /**
     * Set the GAP Appearance — the device type hosts may use to pick the icon
     * shown in Bluetooth settings. Call before `begin()`; calls after the
     * first `begin()` are ignored with a warning.
     *
     * Pass an `APPEARANCE_*` value from `BLEHIDAppearance.h`, or any 16-bit
     * hex code (see `extras/BLEHIDAppearanceFull.h` for all SIG values).
     * Default: `APPEARANCE_HID_KEYBOARD` (0x03C1).
     *
     * Only changes how the device is described — keys are sent the same way.
     * Icons are chosen by each OS. Hosts remember the value from pairing:
     * re-pair after changing it.
     */
    void setAppearance(uint16_t appearance);

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
     * Set the keyboard layout used by `print()`, `println()` and `write()`.
     * Must match the keyboard layout selected on the host computer.
     * Default: `HIDLayout_US`. Can be changed at any time.
     *
     * e.g. `keyboard.setKeyboardLayout(HIDLayout_German);`
     * See `HIDKeyboardLayouts.h` for all layouts. Only layouts your sketch
     * uses take up flash.
     *
     * Only affects text output — `tap()`, `press()`, `release()` and media
     * keys send physical keys and are unaffected.
     */
    void setKeyboardLayout(const HIDKeyboardLayout& layout);

    /** Returns the active keyboard layout. `.name` gives its display name. */
    const HIDKeyboardLayout& getKeyboardLayout() const;

    /**
     * Returns the keycode of the key that carries character `c` on the active
     * keyboard layout (`setKeyboardLayout()`), for use with `tap()`, `press()`
     * and `release()`. Lets shortcuts and held keys follow the key labels on
     * non-US layouts — `KEY_*` names are US key positions.
     *
     *   keyboard.tap(keyboard.keycodeFor('z'), KEY_MOD_LCTRL);  // Ctrl+Z on any layout
     *   keyboard.press(keyboard.keycodeFor(U'ü'));              // hold the ü key
     *   keyboard.press(keyboard.keycodeFor("ü"));               // same, UTF-8 string form
     *
     * Returns the key only — the Shift / AltGr the character needs are not
     * included (`'Z'` and `'z'` return the same key). Dead keys return their key.
     * `\n`, `\t`, backspace and escape return KEY_RETURN, KEY_TAB, KEY_BACKSPACE
     * and KEY_ESCAPE.
     *
     * Returns 0 (`KEY_NONE`) if no single key on the layout carries the
     * character — e.g. characters typed as dead key + letter (`ê` on French).
     * Use `print()` to type those. Note: `release(0)` releases all keys.
     *
     * A pure lookup: sends nothing, works before `begin()`.
     * On `HIDLayout_US` it returns the matching `KEY_*` value.
     */
    uint8_t keycodeFor(char32_t c) const;

    /** Same as `keycodeFor(char32_t)`, for the first character of a UTF-8 string (e.g. `"é"`). */
    uint8_t keycodeFor(const char* utf8) const;

    /**
     * Type one character. Implements `Arduino Print::write()`.
     * Translates the character to the correct key and modifiers for the
     * active keyboard layout (`setKeyboardLayout()`).
     * Supports printable ASCII, control chars `\n`, `\r`, `\t`, BS, ESC, and —
     * on layouts that have them — accented / non-ASCII characters sent as UTF-8
     * (e.g. `print("café")`). Characters the layout cannot type are skipped.
     * Use `print()` / `println()` to send full strings — they call `write()` per byte.
     */
    size_t write(uint8_t c) override;

    /**
     * Type a string. Overrides `Print::write(buf, size)`.
     * Each byte is sent via the single-byte `write(uint8_t)` overload,
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
     * after wakeup correctly primes the Windows HID resume handshake, and
     * stops the idle timer.
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
     * 3. Settles (`setAfterWakeSettle()`, default 250ms) to allow the host to
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

    /**
     * Set how long `afterWake()` waits after the host has reconnected, before it
     * returns (default 250ms, `HID_AFTER_WAKE_SETTLE_MS`). Not part of the
     * `setAfterWakeTimeout()` budget.
     *
     * 250ms is enough for Windows, macOS and Linux. iOS can take up to ~1s after
     * reconnecting before it accepts keys — keys typed straight after
     * `afterWake()` are lost until then. For iPhone/iPad hosts use 1500.
     *
     *   keyboard.setAfterWakeSettle(1500);   // iPhone / iPad
     */
    void setAfterWakeSettle(uint32_t ms) { _afterWakeSettleMs = ms; }

    // ─── Internal Callbacks (do not call directly) ────────────────────────
    void        _onConnect(uint16_t connHandle, const NimBLEAddress& idAddr);
    void        _onDisconnect(uint16_t connHandle);
    void        _onAuthComplete(uint16_t connHandle, bool success);
    void        _onConfirmPassKey(uint32_t passkey);
    void        _onLEDWrite(uint8_t ledByte);
    static void _idleCalloutCb(ble_npl_event* ev);

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
    uint16_t            _appearance;        // GAP Appearance, applied once in begin(). Default 0x03C1.

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
    uint32_t _afterWakeSettleMs;   // afterWake() settle after reconnect (setAfterWakeSettle)
    uint16_t _connInterval;        // requested connection interval in 1.25ms units.
                                   // Default HID_CONN_INTERVAL; set via setConnIntervalMs().
    uint32_t _activeReqMs;         // millis() of the last Active request (see HID_ACTIVE_CHECK_MS)
    uint8_t  _activeRetries;       // re-asks made for the current Active request
    bool     _activeVerified;      // true once the host is seen at latency 0 (or retries used up)
    uint16_t _activeItvl;          // Active interval requested now: _connInterval, or a fallback
                                   // the host accepts (see _checkActiveApplied). Per connection.
    uint16_t _lastGoodItvl;        // last interval seen at latency 0 on this connection (0 = none)
    bool     _firstKeyLinkCheck;   // set on authentication: the first key checks the link's latency
                                   // and wakes it first if > 0 (see _checkActiveApplied)

    // ── Keyboard Layout ───────────────────────────────────────────────────
    const HIDKeyboardLayout* _layout;  // active layout for write(); default &HIDLayout_US
    uint8_t                  _utf8Need; // UTF-8 continuation bytes still expected
    uint32_t                 _utf8Cp;   // code point being decoded

    // ── Pairing Mode ──────────────────────────────────────────────────────
    // _pairingOpen is set by enterPairingMode() (loop task) and cleared by
    // _onAuthComplete() (NimBLE task) or exitPairingMode(). Timeout is checked
    // lazily against millis() — no timer needed.
    HIDPairingMode _pairingMode;
    volatile bool  _pairingOpen;
    uint32_t       _pairingStartMs;
    uint32_t       _pairingWindowMs;

    // ── Idle Power Management ─────────────────────────────────────────────
    // _connState    — current activity state of the BLE connection, independent
    //                 of the BLE stack lifecycle (_BLEState).
    // _connHandle   — NimBLE connection handle of the authenticated HID host,
    //                 stored in _onAuthComplete() and used by _updateConnParams().
    //                 BLE_HS_CONN_HANDLE_NONE when not connected. volatile because
    //                 it is written from the NimBLE task and read from the loop task.
    // _idleCallout  — NimBLE callout (one-shot timer). Created in begin(), started
    //                 in _onAuthComplete(), reset on every key input event,
    //                 stopped in _onDisconnect() / end() / beforeSleep().
    //                 Its callback runs on the NimBLE host task, so it can send
    //                 the idle connection-parameter request directly — safe on
    //                 single-core boards (replaces the old FreeRTOS timer + flag).
    // _idleCalloutReady — true once _idleCallout has been initialised.
    volatile _ConnState _connState;
    volatile uint16_t   _connHandle;
    ble_npl_callout     _idleCallout;
    bool                _idleCalloutReady;

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
    bool    _updateConnParams(uint16_t minInterval, uint16_t maxInterval,
                              uint16_t latency, uint16_t timeout);
    void    _transitionToActive();
    void    _markActiveRequested();
    void    _checkActiveApplied();
    void    _startIdleTimer();
    void    _stopIdleTimer();
    bool    _pairingWindowOpen() const;
    void    _tapLayoutKey(uint8_t kc, uint8_t flags);
    bool    _writeCodepoint(uint32_t cp);
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
