/**
 * HijelHID_BLEKeyboard.cpp
 *
 * Implementation of the HijelHID_BLEKeyboard library.
 * Built on NimBLE-Arduino 2.3.8+
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

#include "HijelHID_BLEKeyboard.h"
#include <stdarg.h>

// NimBLE GAP service header — provides ble_svc_gap_device_appearance_set()
// for the GATT Appearance characteristic. Same conditional include as the
// nimble_port header in HijelHID_BLEKeyboard.h.
#if defined(CONFIG_NIMBLE_CPP_IDF)
#include "services/gap/ble_svc_gap.h"
#else
#include "nimble/nimble/host/services/gap/include/services/gap/ble_svc_gap.h"
#endif

// ─── Idle-exit wake pause ─────────────────────────────────────────────────
// When the first key arrives while the radio is Idle, the Active request is
// sent, the current report is sent to wake the link, and the library pauses
// this long before the key itself goes out.
// Hardware testing on Windows 11: with HID_IDLE_LATENCY 30, keys typed at full
// speed straight out of idle intermittently lost a character mid-word
// (100ms pause still lost characters, 200ms+ clean). With HID_IDLE_LATENCY 4
// no characters were lost even with no pause. 100ms is kept as a safety margin
// until other hosts are tested. Costs this delay once, on the first key after
// HID_IDLE_THRESHOLD_MS of inactivity — normal typing is unaffected.
#define HID_IDLE_WAKE_PAUSE_MS  100

// ─── HID Report Descriptor ────────────────────────────────────────────────
//
// Two top-level collections:
//   Report ID 0x01 — Standard keyboard (8-byte input + 1-byte LED output)
//   Report ID 0x02 — Consumer Control  (2-byte 16-bit usage ID input)
//
// Keyboard report layout (8 bytes):
//   Byte 0: Modifier bitmask  (Ctrl / Shift / Alt / GUI × Left / Right)
//   Byte 1: Reserved (always 0x00)
//   Bytes 2–7: Up to 6 simultaneous keycodes (6KRO)
//
// Consumer report layout (2 bytes):
//   16-bit usage ID, little-endian. 0x0000 = all released.

static const uint8_t _hidReportDescriptor[] = {

    // ── Keyboard (Report ID 0x01) ──────────────────────────────────────
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)

    // Modifier byte — 8 bits, one per modifier key (Left/Right Ctrl/Shift/Alt/GUI)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (Left Control  = 0xE0)
    0x29, 0xE7,        //   Usage Maximum (Right GUI     = 0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1 bit)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)

    // Reserved byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8 bits)
    0x81, 0x01,        //   Input (Constant)

    // LED output report — 5 lock-key LEDs + 3 padding bits
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1 bit)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data, Variable, Absolute)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3 bits)
    0x91, 0x01,        //   Output (Constant) — padding to complete the byte

    // 6-key rollover keycode array
    // Logical Maximum 0xE7 covers all standard keys including international
    // and language keys up to KEY_RGUI.
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8 bits)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xE7, 0x00,  //   Logical Maximum (0xE7)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xE7, 0x00,  //   Usage Maximum (0xE7)
    0x81, 0x00,        //   Input (Data, Array, Absolute)

    0xC0,              // End Collection (Keyboard)

    // ── Consumer Control (Report ID 0x02) ─────────────────────────────
    // Logical Maximum 0x3FF covers all MEDIA_* values defined in
    // BLEHIDMediaKeys.h without any descriptor change.
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023 / 0x3FF)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (0x3FF)
    0x75, 0x10,        //   Report Size (16 bits)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data, Array, Absolute)
    0xC0,              // End Collection (Consumer Control)
};

// ─── ASCII → HID Lookup ──────────────────────────────────────────────────
// Character → key translation for print() / println() / write() lives in the
// keyboard layout tables (HIDKeyboardLayouts.h, src/layouts/). The default,
// HIDLayout_US, is identical to the table previously hard-coded here.

// ─── Internal Callback Implementations ──────────────────────────────────────

void HijelHID_Internal::KBServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    // getIdAddress() is the peer's identity address. For bonded hosts using a
    // rotating private address, NimBLE has already resolved it to the bonded
    // identity before this callback fires — used by the Manual pairing check.
    _parent->_onConnect(connInfo.getConnHandle(), connInfo.getIdAddress());
}

void HijelHID_Internal::KBServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    (void)reason;
    _parent->_onDisconnect(connInfo.getConnHandle());
}

void HijelHID_Internal::KBServerCallbacks::onAuthenticationComplete(NimBLEConnInfo& connInfo) {
    _parent->_onAuthComplete(connInfo.getConnHandle(), connInfo.isEncrypted());
}

void HijelHID_Internal::KBServerCallbacks::onConfirmPassKey(NimBLEConnInfo& connInfo, uint32_t pass_key) {
    // Numeric Comparison (DisplayYesNo + LESC). Auto-accept after printing the
    // number to Serial / firing the onPassKey callback. The user confirms on
    // the host side; rejecting here would silently drop the connection on macOS.
    NimBLEDevice::injectConfirmPasskey(connInfo, true);
    _parent->_onConfirmPassKey(pass_key);
}

// ─── LED Callback Implementation ─────────────────────────────────────────────

void HijelHID_Internal::KBLEDCallbacks::onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) {
    std::string val = pChar->getValue();
    if (val.length() >= 1) {
        _parent->_onLEDWrite((uint8_t)val[0]);
    }
}

// ─── Idle Callout Callback ────────────────────────────────────────────────
//
// _idleCallout is a NimBLE callout: when it expires, NimBLE runs this callback
// on its own host task. NimBLE APIs are safe to call here on both single-core
// and dual-core ESP32 variants, so the idle request is sent directly.
//
// Idle parameters follow Apple's Accessory Design Guidelines (strictest common
// host) so the request is valid everywhere. All values in BLE units:
//   interval: 1.25ms units — Active interval rounded up to a multiple of 12 (15ms), min 12
//   latency:  up to HID_IDLE_LATENCY, so interval × (latency+1) ≤ 6s (4800 units)
//   timeout:  10ms units — max(HID_IDLE_TIMEOUT_MIN, > 3 × interval × (latency+1)),
//             capped at HID_IDLE_TIMEOUT_MAX

void HijelHID_BLEKeyboard::_idleCalloutCb(ble_npl_event* ev) {
    HijelHID_BLEKeyboard* self =
        static_cast<HijelHID_BLEKeyboard*>(ble_npl_event_get_arg(ev));
    if (self == nullptr) return;

    // Only go idle from Active with an authenticated host.
    if (self->_connHandle == BLE_HS_CONN_HANDLE_NONE ||
        self->_connState  != _ConnState::Active) {
        return;
    }

    // A keypress may have landed just as the timer expired. If a report was
    // sent within the threshold, re-arm for the remaining time instead.
    uint32_t sinceLast = millis() - self->_lastReportMs;
    if (sinceLast < HID_IDLE_THRESHOLD_MS) {
        uint32_t remaining = HID_IDLE_THRESHOLD_MS - sinceLast;
        ble_npl_callout_reset(&self->_idleCallout, ble_npl_time_ms_to_ticks32(remaining));
        return;
    }

    // Interval: round up to a multiple of 15ms (12 units), minimum 15ms.
    // Based on the Active interval in use (the user's value, or the fallback
    // the host accepted — see _checkActiveApplied()).
    uint32_t itvl = ((uint32_t)self->_activeItvl + 11) / 12 * 12;
    if (itvl < 12) itvl = 12;

    // Latency: keep interval × (latency + 1) ≤ 6s (4800 units).
    uint32_t latency = (4800 / itvl) - 1;
    if (latency > HID_IDLE_LATENCY) latency = HID_IDLE_LATENCY;

    // Timeout: > 3 × interval × (latency + 1).  ms = units × 1.25, so
    // 3 × itvl × (latency+1) × 1.25ms = itvl × (latency+1) × 3.75ms → /10 for 10ms units.
    uint32_t timeout = (itvl * (latency + 1) * 375) / 1000 + 1;
    if (timeout < HID_IDLE_TIMEOUT_MIN) timeout = HID_IDLE_TIMEOUT_MIN;
    if (timeout > HID_IDLE_TIMEOUT_MAX) timeout = HID_IDLE_TIMEOUT_MAX;

    if (!self->_updateConnParams((uint16_t)itvl, (uint16_t)itvl,
                                 (uint16_t)latency, (uint16_t)timeout)) {
        return;  // request not sent — stay Active (warning already logged)
    }

    self->_connState = _ConnState::Idle;
    // When the connection goes idle, Windows may suspend the HID device — the
    // first notify() after resume is dropped as part of the wakeup handshake.
    // Setting this flag ensures the next report is preceded by a zero-report.
    self->_reportPrimingNeeded = true;

    self->_logNf("Radio idle: interval %lu.%02lums, latency %lu (wake ~%lums), timeout %lums.",
                 (unsigned long)(itvl * 125 / 100), (unsigned long)(itvl * 125 % 100),
                 (unsigned long)latency,
                 (unsigned long)(itvl * (latency + 1) * 125 / 100),
                 (unsigned long)(timeout * 10));
}

// ─── Constructor ──────────────────────────────────────────────────────────
//
// deviceName   — stored as std::string. Truncated to HID_MAX_DEVICE_NAME_LEN
//                if longer (hard BLE advertising packet limit). Null/empty
//                falls back to "HijelHID KB".
//
// manufacturer — stored as std::string. Truncated to HID_MAX_MANUFACTURER_LEN
//                (512 bytes) if longer (Bluetooth Core Spec GATT attribute
//                maximum). Null/empty falls back to "Hijel".
//
// batteryLevel — clamped to 1–100. 0 is not a valid battery percentage.

HijelHID_BLEKeyboard::HijelHID_BLEKeyboard(const char* deviceName,
                                            const char* manufacturer,
                                            uint8_t     batteryLevel)
    : _batteryLevel(100),
      _secMode(BLEKeyboardSecurity::JustWorks),
      _tapDelay(HID_DEFAULT_TAP_DELAY_MS),
      _keyGap(HID_DEFAULT_KEY_GAP_MS),
      _logLevel(HIDLogLevel::Off),
      _useRandomAddress(false),
      _appearance(APPEARANCE_HID_KEYBOARD),
      _state(_BLEState::Stopped),
      _connected(false),
      _authenticated(false),
      _ledState(0),
      _consumerActive(false),
      _reportPrimingNeeded(true),
      _lastReportMs(0),
      _txPowerLevel(8),
      _afterWakeTimeoutMs(15000),
      _afterWakeSettleMs(HID_AFTER_WAKE_SETTLE_MS),
      _connInterval(HID_CONN_INTERVAL),
      _activeReqMs(0),
      _activeRetries(0),
      _activeVerified(true),
      _activeItvl(HID_CONN_INTERVAL),
      _lastGoodItvl(0),
      _firstKeyLinkCheck(false),
      _pairingMode(HIDPairingMode::Auto),
      _pairingOpen(false),
      _pairingStartMs(0),
      _pairingWindowMs(HID_DEFAULT_PAIRING_WINDOW_MS),
      _layout(&HIDLayout_US),
      _utf8Need(0),
      _utf8Cp(0),
      _nameTruncated(false),
      _mfrTruncated(false),
      _batClamped(false),
      _connState(_ConnState::Disconnected),
      _connHandle(BLE_HS_CONN_HANDLE_NONE),
      _idleCallout{},
      _idleCalloutReady(false),
      _cbPassKey(nullptr),
      _cbPairingComplete(nullptr),
      _cbLEDChange(nullptr),
      _pServer(nullptr),
      _pHID(nullptr),
      _pKeyboardInput(nullptr),
      _pKeyboardOutput(nullptr),
      _pConsumerInput(nullptr),
      _pServerCb(nullptr),
      _pLEDCb(nullptr)
{
    memset(_keyReport, 0, sizeof(_keyReport));

    // Device name — store in std::string, truncate if it exceeds the BLE
    // advertising packet limit of 29 bytes.
    _deviceName = (deviceName != nullptr && deviceName[0] != '\0')
                  ? deviceName : "HijelHID KB";
    if (_deviceName.length() > HID_MAX_DEVICE_NAME_LEN) {
        _deviceName.resize(HID_MAX_DEVICE_NAME_LEN);
        _nameTruncated = true;
    }

    // Manufacturer — store in std::string, truncate if it exceeds the Bluetooth
    // Core Spec GATT attribute maximum of HID_MAX_MANUFACTURER_LEN (512 bytes).
    _manufacturer = (manufacturer != nullptr && manufacturer[0] != '\0')
                    ? manufacturer : "Hijel";
    if (_manufacturer.length() > HID_MAX_MANUFACTURER_LEN) {
        _manufacturer.resize(HID_MAX_MANUFACTURER_LEN);
        _mfrTruncated = true;
    }

    // Battery level — clamp to valid range 1–100.
    if (batteryLevel == 0) {
        _batteryLevel = 1;
        _batClamped   = true;
    } else if (batteryLevel > 100) {
        _batteryLevel = 100;
        _batClamped   = true;
    } else {
        _batteryLevel = batteryLevel;
    }
}

// ─── Debug Logging ────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setDebugLevel(HIDLogLevel level) {
    _logLevel = level;
}

void HijelHID_BLEKeyboard::_logN(const char* msg) {
    if (_logLevel >= HIDLogLevel::Normal) {
        Serial.print("[HijelHID] ");
        Serial.println(msg);
    }
}

void HijelHID_BLEKeyboard::_logNf(const char* fmt, ...) {
    if (_logLevel >= HIDLogLevel::Normal) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        Serial.print("[HijelHID] ");
        Serial.println(buf);
    }
}

void HijelHID_BLEKeyboard::_logV(const char* msg) {
    if (_logLevel >= HIDLogLevel::Verbose) {
        Serial.print("[HijelHID VERBOSE] ");
        Serial.println(msg);
    }
}

void HijelHID_BLEKeyboard::_logVf(const char* fmt, ...) {
    if (_logLevel >= HIDLogLevel::Verbose) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        Serial.print("[HijelHID VERBOSE] ");
        Serial.println(buf);
    }
}

// ─── begin() ──────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::begin() {

    // ── Guard: already running or permanently killed ──────────────────
    if (_state == _BLEState::Running) {
        _logN("begin() called while already running — ignored.");
        return;
    }
    if (_state == _BLEState::Killed) {
        _logN("ERROR: begin() called after kill() — BLE stack is permanently shut down.");
        return;
    }

    // Print any deferred constructor warnings now that Serial is running
    if (_nameTruncated) {
        Serial.printf("[HijelHID] WARNING: Device name too long, truncated to %d chars: \"%s\"\n",
                      HID_MAX_DEVICE_NAME_LEN, _deviceName.c_str());
    }
    if (_mfrTruncated) {
        Serial.printf("[HijelHID] WARNING: Manufacturer string too long, truncated to %d chars.\n",
                      HID_MAX_MANUFACTURER_LEN);
    }
    if (_batClamped) {
        Serial.printf("[HijelHID] WARNING: Battery level out of range (1-100), clamped to %d%%.\n",
                      _batteryLevel);
    }

    // ── Restart path: stack is already initialised (after end()) ──────
    if (NimBLEDevice::isInitialized()) {
        _logN("Restarting advertising (BLE stack already initialised)...");
        // Re-apply random address on every restart. setOwnAddrType() must be
        // called before startAdvertising() each time — it is not persisted by
        // the NimBLE stack across end()/begin() cycles.
        if (_useRandomAddress) {
            NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
            _logN("Using random static BLE address.");
        }
        // Re-apply TX power — not persisted across end()/begin() cycles.
        setTxPower(_txPowerLevel);
        NimBLEDevice::startAdvertising();
        _state = _BLEState::Running;
        _logNf("Advertising as \"%s\". Tap delay: %dms, key gap: %dms.",
               _deviceName.c_str(), _tapDelay, _keyGap);
        return;
    }

    // ── First-time path: full BLE initialisation ──────────────────────
    _logN("Initialising NimBLE stack...");
    NimBLEDevice::init(_deviceName.c_str());

    // Block until the NimBLE host task is fully ready.
    // Required on ESP32 Arduino Core 3.3.7+ with NimBLE-Arduino 2.3.8+.
    _logN("Waiting for NimBLE host sync...");
    uint32_t t = millis();
    while (!NimBLEDevice::isInitialized()) {
        delay(10);
        if (millis() - t > 5000) {
            _logN("ERROR: NimBLE failed to initialise after 5s.");
            return;
        }
    }
    _logN("NimBLE stack ready.");

    // Apply random address if requested — must be set after init() and
    // before any advertising or security configuration.
    if (_useRandomAddress) {
        NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
        _logN("Using random static BLE address.");
    }

    // Apply TX power — must be set after init().
    setTxPower(_txPowerLevel);

    // Configure security
    if (_secMode == BLEKeyboardSecurity::Passkey) {
        _logN("Security: Passkey");
        NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND |
                                      BLE_SM_PAIR_AUTHREQ_MITM |
                                      BLE_SM_PAIR_AUTHREQ_SC);
        // DisplayYesNo triggers Numeric Comparison under LESC (BT Core Spec
        // Vol 3 Part H Table 2.8). macOS negotiates this path regardless of
        // whether we advertise DisplayOnly, so DisplayYesNo ensures both sides
        // agree. The host shows a 6-digit number; we print the same number to
        // Serial / fire onPassKey and auto-accept via onConfirmPassKey.
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO);
        // Distribute LTK + IRK in Phase 3 — required by macOS; lenient on
        // Windows and iOS but correct behaviour on all hosts.
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    } else {
        _logN("Security: Just Works");
        NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND |
                                      BLE_SM_PAIR_AUTHREQ_SC);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        // Distribute LTK + IRK in Phase 3 — required by macOS, correct on all hosts.
        NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
        NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    }

    // Create GATT server
    _logN("Creating GATT server...");
    _pServer = NimBLEDevice::createServer();
    if (_pServerCb == nullptr) _pServerCb = new HijelHID_Internal::KBServerCallbacks(this);
    _pServer->setCallbacks(_pServerCb);

    // Create HID device and configure its metadata
    _logN("Creating HID device...");
    _pHID = new NimBLEHIDDevice(_pServer);
    _pHID->setManufacturer(_manufacturer.c_str());
    // PnP ID: source=Bluetooth SIG (0x01), VID=Espressif (0x02E5), PID=0x0001, version=0x0100
    _pHID->setPnp(0x01, 0x02E5, 0x0001, 0x0100);
    // HID info: country=0 (not localised), flags=0x01 (normally connectable)
    _pHID->setHidInfo(0x00, 0x01);
    _pHID->setReportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));

    // Retrieve report characteristics by Report ID
    _logN("Getting report characteristics...");
    _pKeyboardInput  = _pHID->getInputReport(HID_REPORT_ID_KEYBOARD);
    _pKeyboardOutput = _pHID->getOutputReport(HID_REPORT_ID_KEYBOARD);
    _pConsumerInput  = _pHID->getInputReport(HID_REPORT_ID_CONSUMER);

    // Register LED output callback if the characteristic was created
    if (_pKeyboardOutput != nullptr) {
        if (_pLEDCb == nullptr) _pLEDCb = new HijelHID_Internal::KBLEDCallbacks(this);
        _pKeyboardOutput->setCallbacks(_pLEDCb);
    }

    _pHID->setBatteryLevel(_batteryLevel);

    _logN("Starting HID services...");
    _pHID->startServices();

    // Create the idle power management timer — a NimBLE callout on NimBLE's
    // default event queue, so its callback runs on the NimBLE host task.
    // Must be created after NimBLEDevice::init() (the queue exists from then).
    // 'this' is passed as the event argument so the static callback can
    // dispatch back to this instance without a global variable.
    if (ble_npl_callout_init(&_idleCallout, nimble_port_get_dflt_eventq(),
                             _idleCalloutCb, this) == 0) {
        _idleCalloutReady = true;
    } else {
        _logN("WARNING: Failed to create idle timer — idle power saving disabled.");
    }

    // Configure and start advertising
    _logN("Configuring advertising...");
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    // Appearance is set once, here on first-time init only: NimBLE appends a
    // new Appearance field on every setAppearance() call, so the restart path
    // must not repeat it. The GATT Appearance characteristic is set to the
    // same value (NimBLE's default is 0x0000, "Unknown").
    pAdv->setAppearance(_appearance);
    ble_svc_gap_device_appearance_set(_appearance);
    pAdv->addServiceUUID(_pHID->getHidService()->getUUID());
    pAdv->addServiceUUID(_pHID->getBatteryService()->getUUID());
    pAdv->setPreferredParams(0x10, 0x20);  // 20ms min, 40ms max (Android 11 compatible)
    pAdv->enableScanResponse(true);
    pAdv->setMinInterval(0x20);
    pAdv->setMaxInterval(0x40);

    // Include device name in scan response so it appears correctly in
    // Bluetooth settings on all hosts (particularly Android).
    NimBLEAdvertisementData scanResponse;
    scanResponse.setName(_deviceName.c_str());
    pAdv->setScanResponseData(scanResponse);

    NimBLEDevice::startAdvertising();
    _state = _BLEState::Running;
    _logNf("Advertising as \"%s\". Tap delay: %dms, key gap: %dms.",
           _deviceName.c_str(), _tapDelay, _keyGap);
}

// ─── end() ────────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::end() {
    if (_state != _BLEState::Running) return;
    _logN("Stopping...");

    // Stop the idle timer before disconnecting so it cannot fire against a
    // connection that is being torn down.
    _stopIdleTimer();
    _connState  = _ConnState::Disconnected;
    _connHandle = BLE_HS_CONN_HANDLE_NONE;

    // Set state BEFORE disconnecting so the onDisconnect callback
    // (which checks _state) won't restart advertising.
    _state = _BLEState::Stopped;

    // Disconnect the current client if connected
    if (_connected && _pServer != nullptr) {
        // Disconnect all connected peers
        auto peers = _pServer->getPeerDevices();
        for (auto& handle : peers) {
            _pServer->disconnect(handle);
        }
        // Give NimBLE time to complete the disconnect and fire callbacks
        delay(150);
    }

    // Stop advertising so no new connections are accepted
    NimBLEDevice::stopAdvertising();

    // Reset internal state
    _connected = false;
    _authenticated = false;
    memset(_keyReport, 0, sizeof(_keyReport));
    _ledState = 0;
    _consumerActive = false;
    _reportPrimingNeeded = true;

    _logN("Stopped. Call begin() to restart.");
}

// ─── kill() ──────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::kill() {
    if (_state == _BLEState::Killed) return;

    // If still running, stop cleanly first (disconnect + stop advertising)
    if (_state == _BLEState::Running) {
        end();
    }

    // Release the idle callout before the NimBLE stack is torn down — it
    // belongs to NimBLE's event queue, which deinit() destroys.
    // end() already stopped it; deinit releases it permanently.
    if (_idleCalloutReady) {
        ble_npl_callout_stop(&_idleCallout);
        ble_npl_callout_deinit(&_idleCallout);
        _idleCalloutReady = false;
    }

    _logN("Killing BLE (permanent shutdown)...");

    // Tear down the NimBLE stack and free all NimBLE-managed objects
    // (server, services, characteristics, descriptors).
    // end() has already disconnected and stopped advertising, so
    // deinit will not fire onDisconnect callbacks.
    NimBLEDevice::deinit(true);

    // deinit(true) frees all BLE objects including memory regions that
    // overlap with objects we allocated (_pHID, _pServerCb, _pLEDCb).
    // Calling delete on these after deinit causes a double-free crash.
    // Just null the pointers — the memory is already reclaimed.
    _pHID       = nullptr;
    _pServerCb  = nullptr;
    _pLEDCb     = nullptr;

    _pServer         = nullptr;
    _pKeyboardInput  = nullptr;
    _pKeyboardOutput = nullptr;
    _pConsumerInput  = nullptr;

    _state = _BLEState::Killed;
    _logN("BLE killed. begin() will be refused from this point.");

    // NOTE: A small one-time memory leak remains after kill().
    // This includes a known leak in the ESP-IDF NimBLE port's init/deinit
    // path (Espressif issue #8136), plus our wrapper objects (_pHID,
    // _pServerCb, _pLEDCb) which cannot be safely freed because deinit(true)
    // corrupts the heap metadata around them.
    // Since kill() sets _state to Killed and begin() is refused afterward,
    // this leak is bounded and will not compound.
}

// ─── Connection State ─────────────────────────────────────────────────────

bool HijelHID_BLEKeyboard::isConnected() const { return _connected; }
bool HijelHID_BLEKeyboard::isPaired()    const { return _authenticated; }

uint32_t HijelHID_BLEKeyboard::getIdleTime() const {
    return millis() - _lastReportMs;
}

bool HijelHID_BLEKeyboard::isBonded() const {
    if (_state == _BLEState::Killed) return false;
    return (NimBLEDevice::getNumBonds() > 0);
}

void HijelHID_BLEKeyboard::clearBonds() {
    if (_state == _BLEState::Killed) {
        _logN("WARNING: clearBonds() called after kill() — ignored.");
        return;
    }
    // Fix: iterate in reverse through all bonded peers and delete each one
    // individually via deleteBond(), which routes through ble_gap_unpair()
    // rather than ble_store_clear(). ble_gap_unpair() goes through the GAP
    // layer and flushes the SM's in-memory state for each peer — which is
    // what NimBLEDevice::deleteAllBonds() fails to do (it calls ble_store_clear()
    // which wipes NVS bond records but leaves stale SM context, causing
    // re-pairing to fail within the same boot cycle on iOS and other hosts).
    // Iterating in reverse avoids index-shift issues that would occur if
    // deleting forward (removing index 0 shifts all remaining entries down).
    //
    // ble_gap_unpair() refuses to delete a bond that includes the peer's IRK
    // (Windows, Apple and Android all distribute one) while advertising is
    // active — it returns BLE_HS_EBUSY and deletes nothing. Advertising runs
    // whenever no host is connected, so advertising is stopped before each
    // delete, and restarted afterwards only if this function stopped it.
    int numBonds = NimBLEDevice::getNumBonds();
    _logNf("Clearing %d bond(s) individually via deleteBond().", numBonds);
    bool stoppedAdv = false;
    for (int i = numBonds - 1; i >= 0; i--) {
        NimBLEAddress addr = NimBLEDevice::getBondedAddress(i);
        if (NimBLEDevice::isInitialized() && NimBLEDevice::getAdvertising()->isAdvertising()) {
            NimBLEDevice::stopAdvertising();
            stoppedAdv = true;
        }
        _logNf("  Deleting bond %d: %s", i, addr.toString().c_str());
        if (!NimBLEDevice::deleteBond(addr)) {
            _logNf("WARNING: could not delete bond %d: %s", i, addr.toString().c_str());
        }
    }
    if (stoppedAdv && _state == _BLEState::Running && !_connected) {
        NimBLEDevice::startAdvertising();
    }
}

// ─── Security ─────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setSecurityMode(BLEKeyboardSecurity mode) {
    if (_state == _BLEState::Running) {
        _logN("WARNING: setSecurityMode() called after begin() — will take effect on next begin() cycle.");
    }
    _secMode = mode;
}

void HijelHID_BLEKeyboard::setRandomAddress(bool enable) {
    if (_state == _BLEState::Running) {
        _logN("WARNING: setRandomAddress() called after begin() — will take effect on next begin() cycle.");
    }
    _useRandomAddress = enable;
}

// ─── Pairing Mode ─────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setPairingMode(HIDPairingMode mode) {
    _pairingMode = mode;
    if (mode == HIDPairingMode::Auto) {
        _pairingOpen = false;  // no pairing window in Auto mode
    }
    _logN(mode == HIDPairingMode::Manual ? "Pairing mode: Manual."
                                         : "Pairing mode: Auto.");
}

void HijelHID_BLEKeyboard::enterPairingMode(uint32_t windowMs) {
    if (_pairingMode != HIDPairingMode::Manual) {
        _logN("enterPairingMode() ignored — pairing mode is Auto (any device can pair).");
        return;
    }
    if (windowMs == 0) windowMs = HID_DEFAULT_PAIRING_WINDOW_MS;
    if (_connected) {
        // Advertising stops while a device is connected, so no new device
        // can connect until it disconnects.
        _logN("NOTE: a device is connected — new devices can pair after it disconnects.");
    }
    _pairingStartMs  = millis();
    _pairingWindowMs = windowMs;
    _pairingOpen     = true;   // set last — timing fields are valid before it opens
    _logNf("Pairing mode open for %lums.", (unsigned long)windowMs);
}

void HijelHID_BLEKeyboard::exitPairingMode() {
    if (!_pairingOpen) return;
    _pairingOpen = false;
    _logN("Pairing mode closed.");
}

bool HijelHID_BLEKeyboard::isPairing() const {
    return _pairingMode == HIDPairingMode::Manual && _pairingWindowOpen();
}

bool HijelHID_BLEKeyboard::_pairingWindowOpen() const {
    // Unsigned subtraction is millis()-rollover safe.
    return _pairingOpen && (millis() - _pairingStartMs) < _pairingWindowMs;
}

void HijelHID_BLEKeyboard::onPassKey(void (*cb)(uint32_t passkey)) {
    _cbPassKey = cb;
}

void HijelHID_BLEKeyboard::onPairingComplete(void (*cb)(bool success)) {
    _cbPairingComplete = cb;
}

// ─── Timing ───────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setTapDelay(uint16_t ms) { _tapDelay = ms; }
void HijelHID_BLEKeyboard::setKeyGap(uint16_t ms)   { _keyGap   = ms; }

// ─── Battery ──────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setBatteryLevel(uint8_t level) {
    if (_state == _BLEState::Killed) {
        _logN("WARNING: setBatteryLevel() called after kill() — ignored.");
        return;
    }
    if (level == 0) {
        _logN("WARNING: Battery level 0 is invalid, clamping to 1.");
        level = 1;
    } else if (level > 100) {
        _logNf("WARNING: Battery level %d out of range, clamping to 100.", level);
        level = 100;
    }
    _batteryLevel = level;
    if (_pHID != nullptr) {
        _pHID->setBatteryLevel(level, true);
        // Give the host a full connection interval to ACK the battery notification
        // before any key reports follow. Without this gap, a key report fired
        // immediately after the battery notify can lose the ACL buffer race.
        if (_connected) delay(30);
        _logNf("Battery level set to %d%%.", level);
    }
}

// ─── TX Power ─────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setTxPower(uint8_t level) {
    if (_state == _BLEState::Killed) {
        _logN("WARNING: setTxPower() called after kill() — ignored.");
        return;
    }
    if (level < 1) {
        _logN("WARNING: TX power level 0 is invalid (use end() to stop the radio), clamping to 1.");
        level = 1;
    } else if (level > 8) {
        _logNf("WARNING: TX power level %d out of range (1-8), clamping to 8.", level);
        level = 8;
    }

    // Map 1–8 onto the 8 discrete ESP32 TX power steps.
    // Each step is 3 dBm apart, from -12 dBm (level 1) to +9 dBm (level 8).
    static const esp_power_level_t pwrMap[8] = {
        ESP_PWR_LVL_N12,  // 1 → -12 dBm
        ESP_PWR_LVL_N9,   // 2 →  -9 dBm
        ESP_PWR_LVL_N6,   // 3 →  -6 dBm
        ESP_PWR_LVL_N3,   // 4 →  -3 dBm
        ESP_PWR_LVL_N0,   // 5 →   0 dBm
        ESP_PWR_LVL_P3,   // 6 →  +3 dBm
        ESP_PWR_LVL_P6,   // 7 →  +6 dBm
        ESP_PWR_LVL_P9,   // 8 →  +9 dBm (maximum)
    };

    esp_power_level_t pwrLevel = pwrMap[level - 1];
    _txPowerLevel = level;  // always save — begin() re-applies after init if not yet running
    if (NimBLEDevice::isInitialized()) {
        NimBLEDevice::setPower(pwrLevel);
        _logNf("TX power set to level %d (%d dBm).", level, ((int)pwrLevel * 3) - 12);
    } else {
        _logNf("TX power level %d stored — will be applied in begin().", level);
    }
}

// ─── Connection Interval ──────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setConnIntervalMs(uint16_t ms) {
    if (_state == _BLEState::Running) {
        _logN("WARNING: setConnIntervalMs() called after begin() — will take effect on next connection.");
    }

    // Convert ms → 1.25ms units, rounded to nearest: units = ms / 1.25 = ms × 4 / 5.
    uint32_t units = ((uint32_t)ms * 4 + 2) / 5;

    if (units < HID_CONN_INTERVAL_MIN) {
        _logNf("WARNING: Connection interval %dms below minimum, clamping to 7.5ms.", ms);
        units = HID_CONN_INTERVAL_MIN;
    } else if (units > HID_CONN_INTERVAL_MAX) {
        _logNf("WARNING: Connection interval %dms above maximum, clamping to 1000ms.", ms);
        units = HID_CONN_INTERVAL_MAX;
    }

    _connInterval = (uint16_t)units;
    _activeItvl     = _connInterval;  // a new value replaces any fallback in use
    _activeRetries  = 0;      // a new value gets its own set of re-asks
    _activeVerified = false;  // if the host is not at full rate, the next key asks for the new value
    _logNf("Connection interval set to %lu × 1.25ms (%lu.%02lums).",
           (unsigned long)units, (unsigned long)(units * 125 / 100),
           (unsigned long)(units * 125 % 100));
}

// ─── Appearance ───────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setAppearance(uint16_t appearance) {
    // _pServer is created on the first begin(), where the appearance is
    // applied. It cannot be changed afterwards (see begin()).
    if (_pServer != nullptr) {
        _logN("WARNING: setAppearance() must be called before begin() — ignored.");
        return;
    }
    _appearance = appearance;
    _logNf("Appearance set to 0x%04X.", appearance);
}

// ─── Idle Power Management — Internal Helpers ─────────────────────────────

// Returns true if the request was handed to NimBLE (the host may still decline it).
bool HijelHID_BLEKeyboard::_updateConnParams(uint16_t minInterval, uint16_t maxInterval,
                                              uint16_t latency, uint16_t timeout) {
    // No-op if not connected — handle could be stale from a prior session.
    if (_connHandle == BLE_HS_CONN_HANDLE_NONE) return false;

    // ble_gap_update_params() posts asynchronously to the NimBLE host task
    // queue and returns immediately. On dual-core boards the NimBLE task
    // processes it on Core 0 while Arduino continues on Core 1. On single-core
    // boards, the NimBLE task gets CPU time at the next yield point (e.g. the
    // delay() calls inside tap()). The host is not required to accept the
    // request — acceptance is best-effort and host-controlled.
    ble_gap_upd_params params;
    params.itvl_min            = minInterval;
    params.itvl_max            = maxInterval;
    params.latency             = latency;
    params.supervision_timeout = timeout;
    params.min_ce_len          = BLE_GAP_INITIAL_CONN_MIN_CE_LEN;
    params.max_ce_len          = BLE_GAP_INITIAL_CONN_MAX_CE_LEN;

    int rc = ble_gap_update_params(_connHandle, &params);
    if (rc != 0) {
        _logNf("WARNING: ble_gap_update_params() failed (rc=%d) — host may not accept.", rc);
        return false;
    }
    return true;
}

void HijelHID_BLEKeyboard::_transitionToActive() {
    // If NimBLE refuses the request (e.g. the idle update is still in progress),
    // stay Idle so the next key event retries. Keys are still delivered while
    // Idle — peripheral latency only lets the keyboard skip events, it can
    // always transmit at the next one.
    if (!_updateConnParams(_activeItvl, _activeItvl, 0, HID_CONN_TIMEOUT)) {
        return;
    }
    _connState = _ConnState::Active;
    _markActiveRequested();
    _logN("Radio awake (full rate restored).");
    _startIdleTimer();

    // Wake the link with the current (unchanged) report, then pause before the
    // caller's key goes out — see HID_IDLE_WAKE_PAUSE_MS. Only on a successful
    // Idle -> Active transition, so a refused request (retried on the next key)
    // never adds the pause twice. The current report is resent as-is, so keys
    // already held are not released.
    _sendKeyReport();
    delay(HID_IDLE_WAKE_PAUSE_MS);
}

// An Active request was just sent: the host gets HID_ACTIVE_CHECK_MS to apply it
// before _checkActiveApplied() looks at the real connection parameters.
void HijelHID_BLEKeyboard::_markActiveRequested() {
    _activeReqMs    = millis();
    _activeVerified = false;
}

// Called on key events while Active. The host decides whether to apply a
// parameter request (macOS ignores some, iOS refuses intervals below 15ms), so
// check the connection's actual latency instead of assuming. Latency 0 = applied.
// Otherwise ask again, at most HID_ACTIVE_MAX_RETRIES times. If the host still
// has not applied it, fall back (once each, for the rest of this connection):
//   1. 15ms, if the refused interval was below 15ms (the Apple minimum);
//   2. the last interval this host was seen using at latency 0.
// After that keep what the host chose (typing works either way).
// Reads NimBLE's connection record only — no radio traffic unless re-asking.
void HijelHID_BLEKeyboard::_checkActiveApplied() {
    if (_connHandle == BLE_HS_CONN_HANDLE_NONE) return;

    // First key after a (re)connect: the library starts in Active, but some
    // hosts reconnect straight onto idle-style parameters (iOS after light sleep
    // came back at latency 4). Keys sent then without a wake step can be lost —
    // same as straight out of idle. If the link is at latency > 0, do the same
    // wake step as _transitionToActive(): resend the current report, then pause.
    if (_firstKeyLinkCheck) {
        _firstKeyLinkCheck = false;
        ble_gap_conn_desc d;
        if (ble_gap_conn_find(_connHandle, &d) == 0 && d.conn_latency > 0) {
            _logNf("Link at latency %u after connect — waking it before the first key.",
                   (unsigned)d.conn_latency);
            _sendKeyReport();
            delay(HID_IDLE_WAKE_PAUSE_MS);
        }
    }

    if (_activeVerified) return;
    if (millis() - _activeReqMs < HID_ACTIVE_CHECK_MS) return;

    ble_gap_conn_desc desc;
    if (ble_gap_conn_find(_connHandle, &desc) != 0) return;

    if (desc.conn_latency == 0) {           // host applied the Active parameters
        _activeVerified = true;
        _activeRetries  = 0;
        _lastGoodItvl   = desc.conn_itvl;   // known accepted by this host
        // Below 15ms a slower interval at latency 0 usually means the host
        // refused the request and kept its own (iOS keeps 30ms). Try 15ms once.
        if (_activeItvl < 12 && desc.conn_itvl > 12) {
            _logNf("Host kept %u.%02ums (asked %u.%02ums) — trying 15.00ms for this connection.",
                   (unsigned)(desc.conn_itvl * 125 / 100), (unsigned)(desc.conn_itvl * 125 % 100),
                   (unsigned)(_activeItvl * 125 / 100), (unsigned)(_activeItvl * 125 % 100));
            _activeItvl     = 12;
            _activeVerified = false;
            _activeRetries  = HID_ACTIVE_MAX_RETRIES;   // one check, then fall back
            _updateConnParams(_activeItvl, _activeItvl, 0, HID_CONN_TIMEOUT);
            _activeReqMs = millis();
            return;
        }
        if (desc.conn_itvl != _activeItvl) {
            _logNf("Host chose a %u.%02ums interval (asked %u.%02ums).",
                   (unsigned)(desc.conn_itvl * 125 / 100), (unsigned)(desc.conn_itvl * 125 % 100),
                   (unsigned)(_activeItvl * 125 / 100), (unsigned)(_activeItvl * 125 % 100));
            _activeItvl = desc.conn_itvl;   // ask for what this host accepts from now on
        }
        return;
    }

    if (_activeRetries < HID_ACTIVE_MAX_RETRIES) {
        _activeRetries++;
        _logNf("Host still at latency %u — asking for full rate again (%u/%u).",
               (unsigned)desc.conn_latency, (unsigned)_activeRetries, (unsigned)HID_ACTIVE_MAX_RETRIES);
        _updateConnParams(_activeItvl, _activeItvl, 0, HID_CONN_TIMEOUT);
        _activeReqMs = millis();            // check again after another HID_ACTIVE_CHECK_MS
        return;
    }

    // Re-asks used up: pick a fallback interval the host is more likely to accept.
    uint16_t next = 0;
    if (_activeItvl < 12) {
        next = 12;                          // 15ms — the Apple minimum
    } else if (_lastGoodItvl != 0 && _lastGoodItvl != _activeItvl) {
        next = _lastGoodItvl;               // last interval this host accepted
    }

    if (next == 0) {
        _activeVerified = true;             // nothing left to try — stop checking
        _logNf("Host did not apply the requested connection parameters — using %u.%02ums, latency %u.",
               (unsigned)(desc.conn_itvl * 125 / 100), (unsigned)(desc.conn_itvl * 125 % 100),
               (unsigned)desc.conn_latency);
        return;
    }

    _logNf("Host refused %u.%02ums — falling back to %u.%02ums%s for this connection.",
           (unsigned)(_activeItvl * 125 / 100), (unsigned)(_activeItvl * 125 % 100),
           (unsigned)(next * 125 / 100), (unsigned)(next * 125 % 100),
           (next == _lastGoodItvl) ? " (last accepted)" : "");
    _activeItvl = next;
    // A fallback gets one check, no re-asks: if it is refused too, the next
    // check moves on to the next fallback (or stops).
    _activeRetries = HID_ACTIVE_MAX_RETRIES;
    _updateConnParams(_activeItvl, _activeItvl, 0, HID_CONN_TIMEOUT);
    _activeReqMs = millis();
}

void HijelHID_BLEKeyboard::_startIdleTimer() {
    if (!_idleCalloutReady) return;
    // ble_npl_callout_reset starts the callout if stopped, or restarts its
    // period if running. Safe to call from the Arduino loop task.
    ble_npl_callout_reset(&_idleCallout, ble_npl_time_ms_to_ticks32(HID_IDLE_THRESHOLD_MS));
}

void HijelHID_BLEKeyboard::_stopIdleTimer() {
    if (!_idleCalloutReady) return;
    ble_npl_callout_stop(&_idleCallout);
}

// ─── Keyboard Press / Release ────────────────────────────────────────────
//
// KEY_* constants are defined as (uint8_t) in BLEHIDKeys.h, so the compiler
// always resolves keyboard key calls to the uint8_t overload unambiguously.

void HijelHID_BLEKeyboard::press(uint8_t keycode, uint8_t modifiers) {
    if (!_connected) return;

    // Idle → Active transition: apply full-rate params on first keypress.
    // _connState is set to Idle by the idle callout (NimBLE task).
    if (_connState == _ConnState::Idle) {
        _transitionToActive();
    } else if (_connState == _ConnState::Active) {
        _checkActiveApplied();
        _startIdleTimer();
    }

    if (_isModifier(keycode)) {
        _keyReport[0] |= _keycodeToModBit(keycode);
    } else {
        _addKeycode(keycode);
    }
    _keyReport[0] |= modifiers;
    _logVf("press(0x%02X, mod=0x%02X)", keycode, _keyReport[0]);
    _sendKeyReport();
}

void HijelHID_BLEKeyboard::release(uint8_t keycode) {
    if (!_connected) return;
    if (keycode == KEY_NONE) { releaseAll(); return; }

    if (_connState == _ConnState::Idle) {
        _transitionToActive();
    } else if (_connState == _ConnState::Active) {
        _checkActiveApplied();
        _startIdleTimer();
    }

    if (_isModifier(keycode)) {
        _keyReport[0] &= ~_keycodeToModBit(keycode);
    } else {
        _removeKeycode(keycode);
    }
    _logVf("release(0x%02X)", keycode);
    _sendKeyReport();
}

// ─── Consumer / Media Press / Release ────────────────────────────────────
//
// MEDIA_* constants are plain uint16_t values in BLEHIDMediaKeys.h, so the
// compiler always resolves media key calls to the uint16_t overload.

void HijelHID_BLEKeyboard::press(uint16_t usageId) {
    if (!_connected) return;

    if (_connState == _ConnState::Idle) {
        _transitionToActive();
    } else if (_connState == _ConnState::Active) {
        _checkActiveApplied();
        _startIdleTimer();
    }

    _logVf("press(consumer 0x%04X)", usageId);
    _consumerActive = true;
    _sendConsumerReport(usageId);
}

void HijelHID_BLEKeyboard::release(uint16_t usageId) {
    (void)usageId;  // parameter kept for API symmetry; consumer release always sends 0x0000
    if (!_connected) return;

    if (_connState == _ConnState::Idle) {
        _transitionToActive();
    } else if (_connState == _ConnState::Active) {
        _checkActiveApplied();
        _startIdleTimer();
    }

    _logV("release(consumer)");
    _consumerActive = false;
    _sendConsumerReport(0x0000);
}

// ─── Release All ──────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::releaseAll() {
    if (!_connected) return;

    if (_connState == _ConnState::Idle) {
        _transitionToActive();
    } else if (_connState == _ConnState::Active) {
        _checkActiveApplied();
        _startIdleTimer();
    }

    memset(_keyReport, 0, sizeof(_keyReport));
    _logV("releaseAll()");
    _sendKeyReport();
    // Only send the consumer zero-report if a consumer key is actually held.
    // Sending an unnecessary consumer notify after every keyboard tap adds a
    // spurious notify() call that can interfere with the next keyboard report.
    if (_consumerActive) {
        _consumerActive = false;
        _sendConsumerReport(0x0000);
    }
}

// ─── Tap ──────────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::tap(uint8_t keycode, uint8_t modifiers,
                                uint16_t delayMs, uint16_t keyGap) {
    if (!_connected) return;
    if (delayMs == 0) delayMs = _tapDelay;
    if (keyGap  == 0) keyGap  = _keyGap;

    // Save the modifier byte before press() merges in the tap's own modifiers.
    // After releasing the key we restore this baseline so that externally held
    // modifiers (e.g. a prior press(KEY_LSHIFT)) are preserved, while any
    // modifiers added by this tap are cleaned up.
    uint8_t savedMods = _keyReport[0];

    press(keycode, modifiers);
    delay(delayMs);

    // Release only the keycode this tap pressed (not everything held)
    if (_isModifier(keycode)) {
        _keyReport[0] &= ~_keycodeToModBit(keycode);
    } else {
        _removeKeycode(keycode);
    }

    // Restore the modifier baseline — undoes the tap's `modifiers` parameter
    // without affecting modifiers the caller is holding via press().
    _keyReport[0] = savedMods;
    _sendKeyReport();

    delay(keyGap);
}

void HijelHID_BLEKeyboard::tap(uint16_t usageId,
                                uint16_t delayMs, uint16_t keyGap) {
    if (!_connected) return;
    if (delayMs == 0) delayMs = _tapDelay;
    if (keyGap  == 0) keyGap  = _keyGap;
    press(usageId);
    delay(delayMs);
    release(usageId);
    delay(keyGap);
}

// ─── Keyboard Layout ──────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setKeyboardLayout(const HIDKeyboardLayout& layout) {
    _layout   = &layout;
    _utf8Need = 0;   // discard any half-decoded character from the old layout
    _utf8Cp   = 0;
    _logNf("Keyboard layout: %s.", layout.name);
}

const HIDKeyboardLayout& HijelHID_BLEKeyboard::getKeyboardLayout() const {
    return *_layout;
}

// Key that carries character c on the active layout — the key only, without
// the Shift / AltGr the character needs. Same tables as print(), so the two
// can never disagree. 0 if no single key carries it (e.g. dead key + letter).
uint8_t HijelHID_BLEKeyboard::keycodeFor(char32_t c) const {
    // Control characters — same keys write() sends.
    if (c == '\n' || c == '\r') return KEY_RETURN;
    if (c == '\t')              return KEY_TAB;
    if (c == 0x08)              return KEY_BACKSPACE;
    if (c == 0x1B)              return KEY_ESCAPE;

    // Printable ASCII — the layout's ASCII table (dead keys included).
    if (c >= 0x20 && c <= 0x7E) {
        return _layout->ascii[c - 0x20].kc;
    }

    // Non-ASCII — characters typed with a single key (ç, ñ, ü, €…).
    for (uint8_t i = 0; i < _layout->directCount; i++) {
        if (_layout->direct[i].cp == c) return _layout->direct[i].kc;
    }
    return KEY_NONE;
}

uint8_t HijelHID_BLEKeyboard::keycodeFor(const char* utf8) const {
    if (utf8 == nullptr || utf8[0] == '\0') return KEY_NONE;

    // Decode the first UTF-8 character.
    const uint8_t* p = (const uint8_t*)utf8;
    uint32_t cp;
    uint8_t  need;
    if      (p[0] < 0x80)           { return keycodeFor((char32_t)p[0]); }
    else if ((p[0] & 0xE0) == 0xC0) { cp = p[0] & 0x1F; need = 1; }
    else if ((p[0] & 0xF0) == 0xE0) { cp = p[0] & 0x0F; need = 2; }
    else if ((p[0] & 0xF8) == 0xF0) { cp = p[0] & 0x07; need = 3; }
    else                            { return KEY_NONE; }   // invalid lead byte
    for (uint8_t i = 1; i <= need; i++) {
        if ((p[i] & 0xC0) != 0x80) return KEY_NONE;        // truncated / invalid
        cp = (cp << 6) | (p[i] & 0x3F);
    }
    return keycodeFor((char32_t)cp);
}

// Tap one layout entry: apply Shift / AltGr as flagged, and if it is a dead
// key, follow it with Space so the character prints on its own.
void HijelHID_BLEKeyboard::_tapLayoutKey(uint8_t kc, uint8_t flags) {
    uint8_t mods = 0;
    if (flags & HID_LAYOUT_SHIFT) mods |= KEY_MOD_LSHIFT;
    if (flags & HID_LAYOUT_ALTGR) mods |= _layout->altMod;
    tap(kc, mods);
    if (flags & HID_LAYOUT_DEAD) tap(KEY_SPACE);
}

// Type a non-ASCII character (Unicode code point) if the active layout has it:
// first as a single key, otherwise as dead key + base letter.
// Returns true if keys were sent.
bool HijelHID_BLEKeyboard::_writeCodepoint(uint32_t cp) {
    for (uint8_t i = 0; i < _layout->directCount; i++) {
        const HIDLayoutChar& d = _layout->direct[i];
        if (d.cp == cp) {
            _logVf("write(U+%04lX) -> keycode=0x%02X flags=0x%02X",
                   (unsigned long)cp, d.kc, d.flags);
            _tapLayoutKey(d.kc, d.flags);
            return true;
        }
    }
    for (uint8_t i = 0; i < _layout->composeCount; i++) {
        const HIDLayoutCompose& c = _layout->compose[i];
        if (c.cp == cp) {
            const HIDLayoutKey& base = _layout->ascii[(uint8_t)c.base - 0x20];
            _logVf("write(U+%04lX) -> dead key 0x%02X + '%c'",
                   (unsigned long)cp, c.deadKc, c.base);
            // Dead key without the trailing Space, then the base letter.
            _tapLayoutKey(c.deadKc, c.deadFlags & ~HID_LAYOUT_DEAD);
            _tapLayoutKey(base.kc, base.flags);
            return true;
        }
    }
    return false;
}

// ─── Print::write() — text → HID ─────────────────────────────────────────
//
// Return value counts characters actually typed: bytes that only start a
// UTF-8 sequence, and characters the layout cannot type, return 0 and send
// nothing (same as previous versions did for any non-ASCII byte).

size_t HijelHID_BLEKeyboard::write(uint8_t c) {
    if (!_connected) return 0;

    // ── UTF-8 decoding (bytes 0x80–0xFF) ──────────────────────────────────
    if (c >= 0x80) {
        if ((c & 0xC0) == 0x80) {                       // continuation byte
            if (_utf8Need == 0) return 0;                // stray — ignore
            _utf8Cp = (_utf8Cp << 6) | (c & 0x3F);
            if (--_utf8Need > 0) return 0;               // more bytes to come
            return _writeCodepoint(_utf8Cp) ? 1 : 0;
        }
        if      ((c & 0xE0) == 0xC0) { _utf8Need = 1; _utf8Cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { _utf8Need = 2; _utf8Cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { _utf8Need = 3; _utf8Cp = c & 0x07; }
        else                         { _utf8Need = 0; }  // invalid lead byte
        return 0;
    }
    _utf8Need = 0;  // an ASCII byte ends any unfinished UTF-8 sequence

    // Handle ASCII control characters explicitly
    if (c == '\n' || c == '\r') { tap(KEY_RETURN);    return 1; }
    if (c == '\t')               { tap(KEY_TAB);       return 1; }
    if (c == 0x08)               { tap(KEY_BACKSPACE); return 1; }
    if (c == 0x1B)               { tap(KEY_ESCAPE);    return 1; }

    // Printable ASCII 0x20–0x7E — look up key and modifiers in the active layout
    if (c >= 0x20 && c <= 0x7E) {
        const HIDLayoutKey& k = _layout->ascii[c - 0x20];
        if (k.kc != 0) {
            _logVf("write('%c') -> keycode=0x%02X flags=0x%02X", c, k.kc, k.flags);
            _tapLayoutKey(k.kc, k.flags);
            return 1;
        }
    }
    return 0;
}

size_t HijelHID_BLEKeyboard::write(const uint8_t* buffer, size_t size) {
    if (!_connected || size == 0) return 0;
    size_t written = 0;
    for (size_t i = 0; i < size; i++) {
        written += write(buffer[i]);
    }
    return written;
}

// ─── LED State ────────────────────────────────────────────────────────────

bool HijelHID_BLEKeyboard::isNumLockOn()    const { return (_ledState & HID_LED_NUM_LOCK)    != 0; }
bool HijelHID_BLEKeyboard::isCapsLockOn()   const { return (_ledState & HID_LED_CAPS_LOCK)   != 0; }
bool HijelHID_BLEKeyboard::isScrollLockOn() const { return (_ledState & HID_LED_SCROLL_LOCK) != 0; }

void HijelHID_BLEKeyboard::onLEDChange(void (*cb)(uint8_t leds)) {
    _cbLEDChange = cb;
}

// ─── Sleep Hooks ──────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::beforeSleep() {
    // Release all keys so the host does not see keys stuck down across
    // the sleep boundary. releaseAll() is a no-op if not connected, so
    // this is safe to call regardless of connection state.
    // (If the radio was Idle, releaseAll() restores Active params and restarts
    // the idle timer — which is why the timer is stopped after this call.)
    releaseAll();

    // Stop the idle timer before sleeping.
    // millis() does not increment during light sleep — leaving the timer running
    // would cause it to fire immediately on wakeup with a stale _connHandle,
    // which could attempt a param update into an invalid connection state.
    _stopIdleTimer();

    // Ensure the first report after wakeup sends a priming zero-report.
    // This is necessary because millis() does not increment during light
    // sleep — without this flag the priming logic would never trigger
    // after resume, causing the first real report to be dropped by Windows.
    _reportPrimingNeeded = true;
    _lastReportMs = 0;

    _logN("beforeSleep() — idle timer stopped, keys released, priming flag set.");
}

void HijelHID_BLEKeyboard::afterWake() {
    _logN("afterWake() — waiting for host...");

    if (_afterWakeTimeoutMs == 0) {
        _logN("afterWake() — timeout is 0, returning immediately.");
        return;
    }

    // Use a single shared deadline across all wait steps so the total time
    // spent in afterWake() never exceeds _afterWakeTimeoutMs regardless of
    // how long each step takes.
    uint32_t deadline = millis() + _afterWakeTimeoutMs;

    // Step 1: Wait for _authenticated to go false.
    // After light sleep, a fast bonded reconnect can fire onConnect() and
    // onAuthenticationComplete() before the old connection's supervision
    // timeout has been processed by NimBLE's Core 0 task. If we proceed
    // while _authenticated is still true from the previous session we risk
    // sending reports into a connection that is about to be torn down.
    // We wait here until _authenticated goes false, confirming the old
    // connection has been cleanly disconnected before we look for a new one.
    if (_authenticated) {
        _logN("afterWake() — waiting for disconnect...");
        while (_authenticated && millis() < deadline) {
            delay(10);
        }
        if (_authenticated) {
            _logN("afterWake() — timed out waiting for disconnect.");
            return;
        }
    }

    // Step 2: Wait for _authenticated to go true.
    // _onDisconnect() has cleared _authenticated and restarted advertising.
    // We now wait for the host to reconnect and complete the LTK
    // re-encryption handshake, which sets _authenticated = true via
    // _onAuthComplete(). This is a more reliable "ready" signal than
    // _connected alone, which fires at the raw GAP layer before the host
    // HID stack has finished negotiation.
    _logN("afterWake() — waiting for host to reconnect and authenticate...");
    while (!_authenticated && millis() < deadline) {
        delay(100);
    }

    if (!_authenticated) {
        _logN("afterWake() — timed out waiting for authentication.");
        return;
    }

    // Step 3: Authenticated — settle to allow the host to finish HID
    // descriptor negotiation before the first report is sent.
    // Default HID_AFTER_WAKE_SETTLE_MS; set with setAfterWakeSettle().
    _logNf("afterWake() — authenticated, settling %lums...", (unsigned long)_afterWakeSettleMs);
    delay(_afterWakeSettleMs);

    // Step 4: Restart the idle timer now that the connection is ready.
    _startIdleTimer();

    // Treat reconnection as an activity event so getIdleTime() returns a
    // meaningful value immediately after afterWake() returns, rather than
    // reflecting the large elapsed time since the last pre-sleep report.
    _lastReportMs = millis();

    _logN("afterWake() — ready.");
}

void HijelHID_BLEKeyboard::_onConnect(uint16_t connHandle, const NimBLEAddress& idAddr) {
    // Manual pairing mode: outside the pairing window, only already-bonded
    // devices may connect. Reject unknown devices before any HID state is
    // touched. The resulting disconnect is handled by _onDisconnect() as usual
    // (filtered as a secondary client if a host is connected, otherwise it
    // resets state and restarts advertising).
    if (_pairingMode == HIDPairingMode::Manual && !_pairingWindowOpen() &&
        !NimBLEDevice::isBonded(idAddr)) {
        _logNf("Pairing mode closed — rejected unknown device %s.", idAddr.toString().c_str());
        if (_pServer != nullptr) _pServer->disconnect(connHandle);
        return;
    }

    // A client has connected at the GAP layer. We cannot yet identify whether
    // this is the HID host or a secondary BLE client — that distinction is only
    // resolved in _onAuthComplete(), which is the correct place to set _connHandle.
    // For now, mark as connected and move to Connecting state. If _connHandle is
    // already set (we have an authenticated HID host), this is a secondary client
    // connecting alongside it — log it and leave all HID state untouched.
    // A connect event carrying the host's own handle is the host itself: on a
    // bonded reconnect the authentication event can arrive before the connect
    // event, so _connHandle may already be this connection's handle. It is
    // already authenticated and Active — keep that state, just mark connected.
    if (_connHandle != BLE_HS_CONN_HANDLE_NONE && _connHandle == connHandle) {
        _connected = true;
        _logNf("Host connect event after authentication (handle=0x%04X) — already active.", connHandle);
        return;
    }
    if (_connHandle != BLE_HS_CONN_HANDLE_NONE) {
        _logNf("Secondary client connected (handle=0x%04X) — HID host state unchanged.", connHandle);
        return;
    }
    _connected = true;
    _connState = _ConnState::Connecting;
    _logNf("Client connected (handle=0x%04X) — waiting for authentication.", connHandle);
}

void HijelHID_BLEKeyboard::_onDisconnect(uint16_t connHandle) {
    // If we have an authenticated HID host and this disconnect is NOT from that
    // host, it is a secondary client disconnecting. Ignore it — the HID host
    // connection and all associated state must be left intact.
    // We guard on _connHandle (the authenticated host's handle), not _authenticated,
    // to avoid the stale-authentication false positive that can occur during a fast
    // bonded reconnect where _authenticated is still true from the prior session
    // while the supervision timeout is still being processed by NimBLE.
    if (_connHandle != BLE_HS_CONN_HANDLE_NONE && _connHandle != connHandle) {
        _logNf("Secondary client disconnected (handle=0x%04X) — HID host state unchanged.", connHandle);
        return;
    }

    _stopIdleTimer();
    _connState  = _ConnState::Disconnected;
    _connHandle = BLE_HS_CONN_HANDLE_NONE;

    _connected = false;
    _authenticated = false;
    memset(_keyReport, 0, sizeof(_keyReport));
    _ledState = 0;
    _consumerActive = false;
    _reportPrimingNeeded = true;
    _lastReportMs = 0;
    _utf8Need = 0;  // drop any half-decoded UTF-8 character from a print() cut off mid-string
    _activeRetries  = 0;
    _activeVerified = true;
    _activeItvl     = _connInterval;  // fallback is per connection
    _lastGoodItvl   = 0;
    _firstKeyLinkCheck = false;

    if (_state == _BLEState::Running) {
        _logN("Host disconnected. Restarting advertising...");
        NimBLEDevice::startAdvertising();
    } else {
        _logN("Host disconnected.");
    }
}

void HijelHID_BLEKeyboard::_onAuthComplete(uint16_t connHandle, bool success) {
    if (success) {
        _authenticated = true;
        // Set _connHandle here — authentication is the definitive moment at which
        // we can identify this connection as the HID host. Setting it in _onConnect()
        // would be premature: at that point we cannot distinguish the HID host from
        // a secondary BLE client. _connHandle remaining BLE_HS_CONN_HANDLE_NONE
        // until authentication also means _onConnect()'s secondary-client guard
        // and _onDisconnect()'s secondary-client filter both work correctly from
        // the first connection onwards.
        _connHandle    = connHandle;
        // An authenticated link is a connected link. Set here as well as in
        // _onConnect() so the result does not depend on which event arrives
        // first (bonded reconnects can deliver authentication before connect).
        _connected     = true;
        _connState     = _ConnState::Active;
        _logN("Pairing complete (encrypted). Requesting full-rate connection params...");
        // Request Active-state connection params: full rate, no latency.
        // No param update was sent in _onConnect() to avoid a race condition
        // where the host hasn't finished its own connection setup yet.
        // The advertising preferred params (0x10–0x20) will have been applied
        // by the host at connection time; we now explicitly request the
        // Active rate to ensure consistency across all hosts.
        // New connection: start from the user's interval; no fallback yet.
        // The host's own connection-time parameters count as accepted if it
        // chose latency 0 (iOS usually starts at 30ms) — a last-resort fallback.
        _activeItvl   = _connInterval;
        _lastGoodItvl = 0;
        {
            ble_gap_conn_desc desc;
            if (ble_gap_conn_find(connHandle, &desc) == 0 && desc.conn_latency == 0) {
                _lastGoodItvl = desc.conn_itvl;
            }
        }
        _updateConnParams(_activeItvl, _activeItvl, 0, HID_CONN_TIMEOUT);
        _activeRetries = 0;
        _markActiveRequested();
        _firstKeyLinkCheck = true;   // first key checks the link's real latency
        _startIdleTimer();
        // A device has connected and secured the link — pairing mode ends.
        // Advertising has stopped, so no other device could connect anyway.
        if (_pairingOpen) {
            _pairingOpen = false;
            _logN("Pairing mode closed (device connected).");
        }
    } else {
        _authenticated = false;
        _connState     = _ConnState::Connecting;  // remains connecting — did not reach Active
        _logN("Pairing failed.");
    }
    if (_cbPairingComplete) _cbPairingComplete(success);
}

void HijelHID_BLEKeyboard::_onConfirmPassKey(uint32_t passkey) {
    // Always fire the user callback if registered — Serial is their business.
    // If no callback is registered, print via the normal log path so the
    // number only appears when log level is Normal or Verbose, not Off.
    if (_cbPassKey) {
        _cbPassKey(passkey);
    } else {
        _logNf("Passkey: %06lu", (unsigned long)passkey);
    }
}

void HijelHID_BLEKeyboard::_onLEDWrite(uint8_t ledByte) {
    _ledState = ledByte;
    _logVf("LED state: NumLock=%d CapsLock=%d ScrollLock=%d",
           (ledByte & HID_LED_NUM_LOCK)    ? 1 : 0,
           (ledByte & HID_LED_CAPS_LOCK)   ? 1 : 0,
           (ledByte & HID_LED_SCROLL_LOCK) ? 1 : 0);
    if (_cbLEDChange) _cbLEDChange(ledByte);
}

// ─── Internal Helpers ─────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::_sendKeyReport() {
    if (_pKeyboardInput == nullptr || !_connected) return;
    _logVf("keyReport: mod=0x%02X [0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]",
           _keyReport[0],
           _keyReport[2], _keyReport[3], _keyReport[4],
           _keyReport[5], _keyReport[6], _keyReport[7]);

    // Windows selectively suspends idle BLE HID devices after ~800ms of
    // inactivity at the application layer. When suspended, the first notify()
    // triggers a wakeup — but Windows drops that first packet as part of the
    // resume handshake. We guard against this in two ways:
    //
    // 1. _reportPrimingNeeded — set on construction, disconnect, and beforeSleep().
    //    Catches the first report after connect or wake.
    //
    // 2. _lastReportMs — millis() timestamp of the last sent report. If no report
    //    has been sent for HID_WINDOWS_PRIME_MS, prime regardless of the flag.
    //    Catches mid-session suspend cycles (e.g. after a 2s delay between strings).
    //
    // In both cases: send a silent zero-report first to absorb the dropped packet,
    // then send the real report. Skip if the real report is already all zeros.
    bool needsPrime = _reportPrimingNeeded ||
                      ((millis() - _lastReportMs) >= HID_WINDOWS_PRIME_MS);
    if (needsPrime) {
        bool reportEmpty = true;
        for (int i = 0; i < HID_KEYBOARD_REPORT_SIZE; i++) {
            if (_keyReport[i] != 0) { reportEmpty = false; break; }
        }
        if (!reportEmpty) {
            uint8_t empty[HID_KEYBOARD_REPORT_SIZE] = {};
            _pKeyboardInput->setValue(empty, HID_KEYBOARD_REPORT_SIZE);
            _pKeyboardInput->notify();
            delay(50);
        }
        _reportPrimingNeeded = false;
    }

    _pKeyboardInput->setValue(_keyReport, HID_KEYBOARD_REPORT_SIZE);
    // Same retry as before (up to 21 attempts, ~20ms), but a report that still
    // could not be sent is now logged instead of being dropped silently.
    bool sent = _pKeyboardInput->notify();
    int retries = 0;
    while (!sent && _connected && retries < 20) {
        vTaskDelay(1);
        retries++;
        sent = _pKeyboardInput->notify();
    }
    if (!sent && _connected) {
        _logNf("WARNING: key report dropped — notify() failed %d times (mod=0x%02X key=0x%02X).",
               retries + 1, _keyReport[0], _keyReport[2]);
    }
    _lastReportMs = millis();
}

void HijelHID_BLEKeyboard::_sendConsumerReport(uint16_t usageId) {
    if (_pConsumerInput == nullptr || !_connected) return;
    uint8_t report[HID_CONSUMER_REPORT_SIZE];
    report[0] = (uint8_t)(usageId & 0xFF);
    report[1] = (uint8_t)(usageId >> 8);
    _logVf("consumerReport: 0x%04X", usageId);

    // Same Windows resume prime as _sendKeyReport(). Skip if this is already
    // a zero release report — no point sending two identical empty reports.
    bool needsPrime = _reportPrimingNeeded ||
                      ((millis() - _lastReportMs) >= HID_WINDOWS_PRIME_MS);
    if (needsPrime && usageId != 0x0000) {
        uint8_t empty[HID_CONSUMER_REPORT_SIZE] = {};
        _pConsumerInput->setValue(empty, HID_CONSUMER_REPORT_SIZE);
        _pConsumerInput->notify();
        delay(50);
        _reportPrimingNeeded = false;
    }

    _pConsumerInput->setValue(report, HID_CONSUMER_REPORT_SIZE);
    bool sent = _pConsumerInput->notify();
    int retries = 0;
    while (!sent && _connected && retries < 20) {
        vTaskDelay(1);
        retries++;
        sent = _pConsumerInput->notify();
    }
    if (!sent && _connected) {
        _logNf("WARNING: consumer report dropped — notify() failed %d times (usage=0x%04X).",
               retries + 1, usageId);
    }
    _lastReportMs = millis();
}

bool HijelHID_BLEKeyboard::_addKeycode(uint8_t keycode) {
    // If already held, do nothing (prevents duplicate entries)
    for (int i = 2; i < 8; i++) {
        if (_keyReport[i] == keycode) return true;
    }
    // Find a free slot in the 6-key rollover array
    for (int i = 2; i < 8; i++) {
        if (_keyReport[i] == 0) {
            _keyReport[i] = keycode;
            return true;
        }
    }
    _logV("WARNING: 6KRO limit reached, keycode dropped.");
    return false;
}

bool HijelHID_BLEKeyboard::_removeKeycode(uint8_t keycode) {
    for (int i = 2; i < 8; i++) {
        if (_keyReport[i] == keycode) {
            _keyReport[i] = 0;
            return true;
        }
    }
    return false;
}

bool HijelHID_BLEKeyboard::_isModifier(uint8_t keycode) {
    return (keycode >= KEY_LCTRL && keycode <= KEY_RGUI);
}

uint8_t HijelHID_BLEKeyboard::_keycodeToModBit(uint8_t keycode) {
    return (1 << (keycode - KEY_LCTRL));
}
