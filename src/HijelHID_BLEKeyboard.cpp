/**
 * HijelHID_BLEKeyboard.cpp
 *
 * Implementation of the HijelHID_BLEKeyboard library.
 * Built on NimBLE-Arduino 2.x.
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

// ─── ASCII → HID Lookup Tables ───────────────────────────────────────────
// Index into these with (ascii_value - 0x20) for printable ASCII 0x20–0x7E.
// Control characters (\n, \r, \t, BS, ESC) are handled explicitly in write()
// before these tables are consulted.
//
// _kcTable  — the HID keycode to send
// _modTable — the modifier byte to send alongside it (0 or KEY_MOD_LSHIFT)

static const uint8_t _kcTable[95] = {
    // 0x20..0x2F  (space ! " # $ % & ' ( ) * + , - . /)
    KEY_SPACE, KEY_1, KEY_APOSTROPHE, KEY_3, KEY_4, KEY_5, KEY_7,
    KEY_APOSTROPHE, KEY_9, KEY_0, KEY_8, KEY_EQUAL, KEY_COMMA,
    KEY_MINUS, KEY_DOT, KEY_SLASH,
    // 0x30..0x39  (0–9)
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    // 0x3A..0x3F  (: ; < = > ?)
    KEY_SEMICOLON, KEY_SEMICOLON, KEY_COMMA, KEY_EQUAL, KEY_DOT, KEY_SLASH,
    // 0x40  (@)
    KEY_2,
    // 0x41..0x5A  (A–Z)
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    // 0x5B..0x60  ([ \ ] ^ _ `)
    KEY_LEFTBRACE, KEY_BACKSLASH, KEY_RIGHTBRACE, KEY_6, KEY_MINUS, KEY_GRAVE,
    // 0x61..0x7A  (a–z)
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    // 0x7B..0x7E  ({ | } ~)
    KEY_LEFTBRACE, KEY_BACKSLASH, KEY_RIGHTBRACE, KEY_GRAVE,
};

static const uint8_t _modTable[95] = {
    // 0x20..0x2F
    0,               // space
    KEY_MOD_LSHIFT,  // !
    KEY_MOD_LSHIFT,  // "
    KEY_MOD_LSHIFT,  // #
    KEY_MOD_LSHIFT,  // $
    KEY_MOD_LSHIFT,  // %
    KEY_MOD_LSHIFT,  // &
    0,               // '
    KEY_MOD_LSHIFT,  // (
    KEY_MOD_LSHIFT,  // )
    KEY_MOD_LSHIFT,  // *
    KEY_MOD_LSHIFT,  // +
    0,               // ,
    0,               // -
    0,               // .
    0,               // /
    // 0x30..0x39  (0–9, no shift)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x3A..0x3F
    KEY_MOD_LSHIFT,  // :
    0,               // ;
    KEY_MOD_LSHIFT,  // <
    0,               // =
    KEY_MOD_LSHIFT,  // >
    KEY_MOD_LSHIFT,  // ?
    // 0x40  (@)
    KEY_MOD_LSHIFT,
    // 0x41..0x5A  (A–Z, all shifted)
    KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
    KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
    KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
    KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
    KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
    KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
    KEY_MOD_LSHIFT, KEY_MOD_LSHIFT,
    // 0x5B..0x60
    0,               // [
    0,               // backslash
    0,               // ]
    KEY_MOD_LSHIFT,  // ^
    KEY_MOD_LSHIFT,  // _
    0,               // `
    // 0x61..0x7A  (a–z, no shift)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x7B..0x7E
    KEY_MOD_LSHIFT,  // {
    KEY_MOD_LSHIFT,  // |
    KEY_MOD_LSHIFT,  // }
    KEY_MOD_LSHIFT,  // ~
};

// ─── Server Callbacks ─────────────────────────────────────────────────────

void _HijelKBServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    _parent->_onConnect();
}

void _HijelKBServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    (void)reason;
    _parent->_onDisconnect();
    NimBLEDevice::startAdvertising();
}

void _HijelKBServerCallbacks::onAuthenticationComplete(NimBLEConnInfo& connInfo) {
    _parent->_onAuthComplete(connInfo.isEncrypted());
}

uint32_t _HijelKBServerCallbacks::onPassKeyDisplay() {
    return _parent->_onPassKeyDisplay();
}

void _HijelKBServerCallbacks::onConfirmPassKey(NimBLEConnInfo& connInfo, uint32_t pass_key) {
    NimBLEDevice::injectConfirmPasskey(connInfo, true);
    _parent->_onConfirmPassKey(pass_key);
}

// ─── LED Output Report Callback ───────────────────────────────────────────

void _HijelKBLEDCallbacks::onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) {
    std::string val = pChar->getValue();
    if (val.length() >= 1) {
        _parent->_onLEDWrite((uint8_t)val[0]);
    }
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
    : _secMode(BLEKeyboardSecurity::JustWorks),
      _tapDelay(HID_DEFAULT_TAP_DELAY_MS),
      _keyGap(HID_DEFAULT_KEY_GAP_MS),
      _logLevel(HIDLogLevel::Off),
      _connected(false),
      _ledState(0),
      _consumerActive(false),
      _lastReportMs(0),
      _nameTruncated(false),
      _mfrTruncated(false),
      _batClamped(false),
      _cbPassKey(nullptr),
      _cbPairingComplete(nullptr),
      _cbLEDChange(nullptr),
      _pServer(nullptr),
      _pHID(nullptr),
      _pKeyboardInput(nullptr),
      _pKeyboardOutput(nullptr),
      _pConsumerInput(nullptr)
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
        // Two-pass: measure exact size needed, then allocate precisely that
        // on the stack with alloca (a pointer adjustment — no heap involvement).
        va_list args, args2;
        va_start(args, fmt);
        va_copy(args2, args);
        int needed = vsnprintf(nullptr, 0, fmt, args);
        va_end(args);
        if (needed > 0) {
            char* buf = static_cast<char*>(alloca(needed + 1));
            vsnprintf(buf, needed + 1, fmt, args2);
            Serial.print("[HijelHID] ");
            Serial.println(buf);
        }
        va_end(args2);
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
        va_list args, args2;
        va_start(args, fmt);
        va_copy(args2, args);
        int needed = vsnprintf(nullptr, 0, fmt, args);
        va_end(args);
        if (needed > 0) {
            char* buf = static_cast<char*>(alloca(needed + 1));
            vsnprintf(buf, needed + 1, fmt, args2);
            Serial.print("[HijelHID VERBOSE] ");
            Serial.println(buf);
        }
        va_end(args2);
    }
}

// ─── begin() ──────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::begin() {

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

    // Configure security
    if (_secMode == BLEKeyboardSecurity::Passkey) {
        _logN("Security: Passkey");
        NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND |
                                      BLE_SM_PAIR_AUTHREQ_MITM |
                                      BLE_SM_PAIR_AUTHREQ_SC);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    } else {
        _logN("Security: Just Works");
        NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND |
                                      BLE_SM_PAIR_AUTHREQ_SC);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    }

    // Create GATT server
    _logN("Creating GATT server...");
    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(new _HijelKBServerCallbacks(this));

    // Create HID device and configure its metadata
    _logN("Creating HID device...");
    _pHID = new NimBLEHIDDevice(_pServer);
    _pHID->setManufacturer(_manufacturer.c_str());
    // PnP ID: source=USB-IF (0x02), VID=Espressif (0xE502), PID=0x0001, version=0x0100
    _pHID->setPnp(0x02, 0xE502, 0x0001, 0x0100);
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
        _pKeyboardOutput->setCallbacks(new _HijelKBLEDCallbacks(this));
    }

    _pHID->setBatteryLevel(_batteryLevel);

    _logN("Starting HID services...");
    _pHID->startServices();

    // Seed the Arduino PRNG with hardware entropy so passkeys generated by
    // _onPassKeyDisplay() are not deterministic across reboots.
    randomSeed(esp_random());

    // Configure and start advertising
    _logN("Configuring advertising...");
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->setAppearance(0x03C1);  // HID Keyboard appearance
    pAdv->addServiceUUID(_pHID->getHidService()->getUUID());
    pAdv->addServiceUUID(_pHID->getBatteryService()->getUUID());
    pAdv->setPreferredParams(0x06, 0x12);
    pAdv->enableScanResponse(true);
    pAdv->setMinInterval(0x20);
    pAdv->setMaxInterval(0x40);

    // Include device name in scan response so it appears correctly in
    // Bluetooth settings on all hosts (particularly Android).
    NimBLEAdvertisementData scanResponse;
    scanResponse.setName(_deviceName.c_str());
    pAdv->setScanResponseData(scanResponse);

    NimBLEDevice::startAdvertising();
    _logNf("Advertising as \"%s\". Tap delay: %dms, key gap: %dms.",
           _deviceName.c_str(), _tapDelay, _keyGap);
}

// ─── end() ────────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::end() {
    _logN("Deinitialising...");
    NimBLEDevice::deinit(true);
    _connected = false;
}

// ─── Connection State ─────────────────────────────────────────────────────

bool HijelHID_BLEKeyboard::isConnected() const { return _connected; }

bool HijelHID_BLEKeyboard::isBonded() const {
    return (NimBLEDevice::getNumBonds() > 0);
}

void HijelHID_BLEKeyboard::clearBonds() {
    _logN("Clearing all bonds.");
    NimBLEDevice::deleteAllBonds();
}

// ─── Security ─────────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::setSecurityMode(BLEKeyboardSecurity mode) {
    _secMode = mode;
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
    if (level == 0) {
        _logN("WARNING: Battery level 0 is invalid, clamping to 1.");
        level = 1;
        _batClamped = true;
    } else if (level > 100) {
        _logNf("WARNING: Battery level %d out of range, clamping to 100.", level);
        level = 100;
        _batClamped = true;
    } else {
        _batClamped = false;
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

// ─── Keyboard Press / Release ────────────────────────────────────────────
//
// KEY_* constants are defined as (uint8_t) in BLEHIDKeys.h, so the compiler
// always resolves keyboard key calls to the uint8_t overload unambiguously.

void HijelHID_BLEKeyboard::press(uint8_t keycode, uint8_t modifiers) {
    if (!_connected) return;
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
    _logVf("press(consumer 0x%04X)", usageId);
    _consumerActive = true;
    _sendConsumerReport(usageId);
}

void HijelHID_BLEKeyboard::release(uint16_t usageId) {
    (void)usageId;  // parameter kept for API symmetry; consumer release always sends 0x0000
    if (!_connected) return;
    _logV("release(consumer)");
    _consumerActive = false;
    _sendConsumerReport(0x0000);
}

// ─── Release All ──────────────────────────────────────────────────────────

void HijelHID_BLEKeyboard::releaseAll() {
    if (!_connected) return;
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
    if (delayMs == 0) delayMs = _tapDelay;
    if (keyGap  == 0) keyGap  = _keyGap;
    press(keycode, modifiers);
    delay(delayMs);
    // Use releaseAll() rather than release() to guarantee the modifier byte is
    // fully zeroed after every tap. release() only removes the keycode slot —
    // it does not clear modifiers — so without this, a shifted tap would leave
    // Shift set in _keyReport[0], causing every subsequent key to arrive shifted.
    releaseAll();
    delay(keyGap);
}

void HijelHID_BLEKeyboard::tap(uint16_t usageId,
                                uint16_t delayMs, uint16_t keyGap) {
    if (delayMs == 0) delayMs = _tapDelay;
    if (keyGap  == 0) keyGap  = _keyGap;
    press(usageId);
    delay(delayMs);
    release(usageId);
    delay(keyGap);
}

// ─── Print::write() — ASCII → HID ────────────────────────────────────────

size_t HijelHID_BLEKeyboard::write(uint8_t c) {
    if (!_connected) return 0;

    // Handle ASCII control characters explicitly
    if (c == '\n' || c == '\r') { tap(KEY_RETURN);    return 1; }
    if (c == '\t')               { tap(KEY_TAB);       return 1; }
    if (c == 0x08)               { tap(KEY_BACKSPACE); return 1; }
    if (c == 0x1B)               { tap(KEY_ESCAPE);    return 1; }

    // Printable ASCII 0x20–0x7E — look up keycode and modifier from tables
    if (c >= 0x20 && c <= 0x7E) {
        uint8_t idx      = c - 0x20;
        uint8_t keycode  = _kcTable[idx];
        uint8_t modifier = _modTable[idx];
        if (keycode != 0) {
            _logVf("write('%c') -> keycode=0x%02X mod=0x%02X", c, keycode, modifier);
            tap(keycode, modifier);
            return 1;
        }
    }
    return 0;
}

// ─── LED State ────────────────────────────────────────────────────────────

bool HijelHID_BLEKeyboard::isNumLockOn()    const { return (_ledState & HID_LED_NUM_LOCK);    }
bool HijelHID_BLEKeyboard::isCapsLockOn()   const { return (_ledState & HID_LED_CAPS_LOCK);   }
bool HijelHID_BLEKeyboard::isScrollLockOn() const { return (_ledState & HID_LED_SCROLL_LOCK); }

void HijelHID_BLEKeyboard::onLEDChange(void (*cb)(uint8_t leds)) {
    _cbLEDChange = cb;
}

// ─── Internal Callback Handlers ───────────────────────────────────────────

void HijelHID_BLEKeyboard::_onConnect() {
    _connected = true;
    _logN("Host connected.");
}

void HijelHID_BLEKeyboard::_onDisconnect() {
    _connected = false;
    memset(_keyReport, 0, sizeof(_keyReport));
    _ledState = 0;
    _consumerActive = false;
    _lastReportMs = 0;  // force wakeup prime on next report after reconnect
    _logN("Host disconnected. Restarting advertising...");
}

void HijelHID_BLEKeyboard::_onAuthComplete(bool success) {
    if (success) {
        _logN("Pairing complete (encrypted).");
    } else {
        _logN("Pairing failed.");
    }
    if (_cbPairingComplete) _cbPairingComplete(success);
}

uint32_t HijelHID_BLEKeyboard::_onPassKeyDisplay() {
    uint32_t passkey = random(100000, 1000000);
    _logNf("Passkey: %06lu", (unsigned long)passkey);
    if (_cbPassKey) _cbPassKey(passkey);
    return passkey;
}

void HijelHID_BLEKeyboard::_onConfirmPassKey(uint32_t passkey) {
    _logNf("Confirm passkey: %06lu", (unsigned long)passkey);
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

    // Windows selectively suspends idle BLE HID devices after ~2 seconds of
    // inactivity. When suspended, the first notify() triggers a USB remote
    // wakeup — but Windows drops that first packet as part of the resume
    // handshake. The workaround: if we have been idle long enough that the
    // host may have suspended the device, send one silent all-zeros report
    // first. That report absorbs the drop, and the real report follows
    // immediately once the host is awake and listening.
    if ((millis() - _lastReportMs) > 800) {
        uint8_t empty[HID_KEYBOARD_REPORT_SIZE] = {};
        _pKeyboardInput->setValue(empty, HID_KEYBOARD_REPORT_SIZE);
        _pKeyboardInput->notify();
        delay(50);  // give the host time to fully resume before the real report
    }

    _pKeyboardInput->setValue(_keyReport, HID_KEYBOARD_REPORT_SIZE);
    while (!_pKeyboardInput->notify() && _connected) {
        vTaskDelay(1);
    }
    _lastReportMs = millis();
}

void HijelHID_BLEKeyboard::_sendConsumerReport(uint16_t usageId) {
    if (_pConsumerInput == nullptr || !_connected) return;
    uint8_t report[HID_CONSUMER_REPORT_SIZE];
    report[0] = (uint8_t)(usageId & 0xFF);
    report[1] = (uint8_t)(usageId >> 8);
    _logVf("consumerReport: 0x%04X", usageId);

    // Windows selectively suspends idle BLE HID devices after ~2 seconds.
    // Same wakeup-prime workaround as _sendKeyReport() — see comments there.
    if ((millis() - _lastReportMs) > 800) {
        uint8_t empty[HID_CONSUMER_REPORT_SIZE] = {};
        _pConsumerInput->setValue(empty, HID_CONSUMER_REPORT_SIZE);
        _pConsumerInput->notify();
        delay(50);  // give the host time to fully resume before the real report
    }

    _pConsumerInput->setValue(report, HID_CONSUMER_REPORT_SIZE);
    while (!_pConsumerInput->notify() && _connected) {
        vTaskDelay(1);
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
