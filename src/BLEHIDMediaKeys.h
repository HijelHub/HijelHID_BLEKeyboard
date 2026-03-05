#pragma once
/**
 * BLEHIDMediaKeys.h
 * USB HID Consumer Page (0x0C) usage values.
 * Used with press(uint16_t) / release(uint16_t) / tap(uint16_t).
 * The overloaded API automatically selects the consumer report
 * when a uint16_t MEDIA_* constant is passed.
 *
 * These are 16-bit usage IDs sent via Report ID 0x02.
 * The HID descriptor declares Logical Maximum 0x3FF so all values
 * up to 0x3FF are valid without any descriptor change.
 *
 * Reference: USB HID Usage Tables v1.22, Section 15 (Consumer Page)
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

// ─── Playback Controls ─────────────────────────────────────────────────────
#define MEDIA_PLAY                  ((uint16_t)0x00B0)  // Play
#define MEDIA_PAUSE                 ((uint16_t)0x00B1)  // Pause
#define MEDIA_RECORD                ((uint16_t)0x00B2)  // Record
#define MEDIA_FAST_FORWARD          ((uint16_t)0x00B3)  // Fast Forward
#define MEDIA_REWIND                ((uint16_t)0x00B4)  // Rewind
#define MEDIA_NEXT_TRACK            ((uint16_t)0x00B5)  // Scan Next Track
#define MEDIA_PREV_TRACK            ((uint16_t)0x00B6)  // Scan Previous Track
#define MEDIA_STOP                  ((uint16_t)0x00B7)  // Stop
#define MEDIA_EJECT                 ((uint16_t)0x00B8)  // Eject
#define MEDIA_RANDOM_PLAY           ((uint16_t)0x00B9)  // Random Play (shuffle)
#define MEDIA_PLAY_PAUSE            ((uint16_t)0x00CD)  // Play/Pause toggle (most common)
#define MEDIA_PLAY_SKIP             ((uint16_t)0x00CE)  // Play Skip

// ─── Volume Controls ───────────────────────────────────────────────────────
#define MEDIA_MUTE                  ((uint16_t)0x00E2)  // Mute
#define MEDIA_BASS_BOOST            ((uint16_t)0x00E5)  // Bass Boost
#define MEDIA_VOLUME_UP             ((uint16_t)0x00E9)  // Volume Increment
#define MEDIA_VOLUME_DOWN           ((uint16_t)0x00EA)  // Volume Decrement
#define MEDIA_BASS_UP               ((uint16_t)0x0152)  // Bass Increment
#define MEDIA_BASS_DOWN             ((uint16_t)0x0153)  // Bass Decrement
#define MEDIA_TREBLE_UP             ((uint16_t)0x0154)  // Treble Increment
#define MEDIA_TREBLE_DOWN           ((uint16_t)0x0155)  // Treble Decrement

// ─── Display / Brightness ──────────────────────────────────────────────────
#define MEDIA_BRIGHTNESS_UP         ((uint16_t)0x006F)  // Display Brightness Increment
#define MEDIA_BRIGHTNESS_DOWN       ((uint16_t)0x0070)  // Display Brightness Decrement
#define MEDIA_DISPLAY_BACKLIGHT     ((uint16_t)0x007A)  // Display Backlight toggle

// ─── Keyboard Backlight ────────────────────────────────────────────────────
#define MEDIA_KBD_BACKLIGHT_DOWN    ((uint16_t)0x0078)  // Keyboard Backlight Decrement
#define MEDIA_KBD_BACKLIGHT_UP      ((uint16_t)0x0079)  // Keyboard Backlight Increment
#define MEDIA_KBD_BACKLIGHT_TOGGLE  ((uint16_t)0x007C)  // Keyboard Backlight OOC toggle

// ─── Application Launch ────────────────────────────────────────────────────
#define MEDIA_MEDIA_SELECT          ((uint16_t)0x0183)  // Launch Media Select / Player
#define MEDIA_MAIL                  ((uint16_t)0x018A)  // Launch Mail client
#define MEDIA_CALCULATOR            ((uint16_t)0x0192)  // Launch Calculator
#define MEDIA_FILE_EXPLORER         ((uint16_t)0x0194)  // Launch File Browser / Explorer
#define MEDIA_SCREENSAVER           ((uint16_t)0x019E)  // Launch Screen Saver
#define MEDIA_TASK_MANAGER          ((uint16_t)0x01A6)  // Launch Task Manager

// ─── Browser / Web Navigation ─────────────────────────────────────────────
#define MEDIA_BROWSER_SEARCH        ((uint16_t)0x0221)  // AC Search
#define MEDIA_BROWSER_HOME          ((uint16_t)0x0223)  // AC Home
#define MEDIA_BROWSER_BACK          ((uint16_t)0x0224)  // AC Back
#define MEDIA_BROWSER_FORWARD       ((uint16_t)0x0225)  // AC Forward
#define MEDIA_BROWSER_STOP          ((uint16_t)0x0226)  // AC Stop
#define MEDIA_BROWSER_REFRESH       ((uint16_t)0x0227)  // AC Refresh
#define MEDIA_BROWSER_BOOKMARKS     ((uint16_t)0x022A)  // AC Bookmarks / Favorites

// ─── Power / System ────────────────────────────────────────────────────────
#define MEDIA_SLEEP                 ((uint16_t)0x0032)  // Sleep
#define MEDIA_LOCK_SCREEN           ((uint16_t)0x019E)  // Lock screen (OS-dependent behaviour)
