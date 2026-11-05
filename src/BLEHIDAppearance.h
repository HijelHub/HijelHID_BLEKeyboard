#pragma once
/**
 * BLEHIDAppearance.h
 * GAP Appearance values for setAppearance().
 *
 * The Appearance tells the host what kind of device this is — hosts may use
 * it to pick the icon shown in Bluetooth settings and device lists. It does
 * not change how keys are sent. Icons are chosen by each OS; some hosts show
 * a generic icon or none for some values.
 *
 * Listed here: devices that work like a keyboard underneath but may not
 * look like one. Any other 16-bit value can be passed to setAppearance() as
 * a hex code — see extras/BLEHIDAppearanceFull.h for every value defined by
 * the Bluetooth SIG.
 *
 * Reference: Bluetooth Assigned Numbers, Section 2.6 (Appearance Values)
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

#include <stdint.h>

#define APPEARANCE_HID_KEYBOARD              ((uint16_t)0x03C1)  // Keyboard (default)
#define APPEARANCE_HID                       ((uint16_t)0x03C0)  // Generic HID — macro pad, foot pedal, button box
#define APPEARANCE_HID_PRESENTATION_REMOTE   ((uint16_t)0x03CA)  // Presentation clicker
#define APPEARANCE_HID_BARCODE_SCANNER       ((uint16_t)0x03C8)  // Barcode scanner (types the scanned code)
#define APPEARANCE_HID_CARD_READER           ((uint16_t)0x03C6)  // Card reader — RFID / magnetic stripe (types the card number)
