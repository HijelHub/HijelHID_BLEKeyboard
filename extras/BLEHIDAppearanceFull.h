#pragma once
/**
 * BLEHIDAppearanceFull.h — REFERENCE ONLY (not compiled with the library)
 *
 * Every GAP Appearance value defined by the Bluetooth SIG. Use it to look up
 * a hex code for setAppearance() when none of the values in
 * src/BLEHIDAppearance.h fit your device:
 *
 *     keyboard.setAppearance(0x04CD);   // Control Device: Dial
 *
 * Or copy the #define you need into your sketch. The library still sends
 * keyboard / media reports whatever Appearance is set — values outside the
 * HID category (0x03C0–0x03FF) are untested and hosts may show a generic
 * icon or none, or treat the device unexpectedly.
 *
 * Appearance is a 16-bit value, composed as:
 *
 *     bits 15–6 : category    (10 bits)
 *     bits  5–0 : subcategory ( 6 bits)
 *
 * Categories without a defined subcategory use subcategory 0x00, so their
 * appearance value is simply (category << 6).
 *
 * Reference: Bluetooth Assigned Numbers, Section 2.6 (Appearance Values).
 * Table contributed by ptrrr1 (PR #17).
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

// ─── 0x000 — Unknown ──────────────────────────────────────────────────────
#define APPEARANCE_UNKNOWN                                ((uint16_t)0x0000)

// ─── 0x001 — Phone ────────────────────────────────────────────────────────
#define APPEARANCE_PHONE                                  ((uint16_t)0x0040)

// ─── 0x002 — Computer ─────────────────────────────────────────────────────
#define APPEARANCE_COMPUTER                               ((uint16_t)0x0080)
#define APPEARANCE_COMPUTER_DESKTOP_WORKSTATION           ((uint16_t)0x0081)
#define APPEARANCE_COMPUTER_SERVER_CLASS                  ((uint16_t)0x0082)
#define APPEARANCE_COMPUTER_LAPTOP                        ((uint16_t)0x0083)
#define APPEARANCE_COMPUTER_HANDHELD_PC_PDA_CLAMSHELL     ((uint16_t)0x0084)
#define APPEARANCE_COMPUTER_PALM_SIZE_PC_PDA              ((uint16_t)0x0085)
#define APPEARANCE_COMPUTER_WEARABLE_WATCH_SIZE           ((uint16_t)0x0086)
#define APPEARANCE_COMPUTER_TABLET                        ((uint16_t)0x0087)
#define APPEARANCE_COMPUTER_DOCKING_STATION               ((uint16_t)0x0088)
#define APPEARANCE_COMPUTER_ALL_IN_ONE                    ((uint16_t)0x0089)
#define APPEARANCE_COMPUTER_BLADE_SERVER                  ((uint16_t)0x008A)
#define APPEARANCE_COMPUTER_CONVERTIBLE                   ((uint16_t)0x008B)
#define APPEARANCE_COMPUTER_DETACHABLE                    ((uint16_t)0x008C)
#define APPEARANCE_COMPUTER_IOT_GATEWAY                   ((uint16_t)0x008D)
#define APPEARANCE_COMPUTER_MINI_PC                       ((uint16_t)0x008E)
#define APPEARANCE_COMPUTER_STICK_PC                      ((uint16_t)0x008F)

// ─── 0x003 — Watch ────────────────────────────────────────────────────────
#define APPEARANCE_WATCH                                  ((uint16_t)0x00C0)
#define APPEARANCE_WATCH_SPORTS                           ((uint16_t)0x00C1)
#define APPEARANCE_WATCH_SMARTWATCH                       ((uint16_t)0x00C2)

// ─── 0x004 — Clock ────────────────────────────────────────────────────────
#define APPEARANCE_CLOCK                                  ((uint16_t)0x0100)

// ─── 0x005 — Display ──────────────────────────────────────────────────────
#define APPEARANCE_DISPLAY                                ((uint16_t)0x0140)

// ─── 0x006 — Remote Control ───────────────────────────────────────────────
#define APPEARANCE_REMOTE_CONTROL                         ((uint16_t)0x0180)

// ─── 0x007 — Eye-glasses ──────────────────────────────────────────────────
#define APPEARANCE_EYE_GLASSES                            ((uint16_t)0x01C0)

// ─── 0x008 — Tag ──────────────────────────────────────────────────────────
#define APPEARANCE_TAG                                    ((uint16_t)0x0200)

// ─── 0x009 — Keyring ──────────────────────────────────────────────────────
#define APPEARANCE_KEYRING                                ((uint16_t)0x0240)

// ─── 0x00A — Media Player ─────────────────────────────────────────────────
#define APPEARANCE_MEDIA_PLAYER                           ((uint16_t)0x0280)

// ─── 0x00B — Barcode Scanner ──────────────────────────────────────────────
#define APPEARANCE_BARCODE_SCANNER                        ((uint16_t)0x02C0)

// ─── 0x00C — Thermometer ──────────────────────────────────────────────────
#define APPEARANCE_THERMOMETER                            ((uint16_t)0x0300)
#define APPEARANCE_THERMOMETER_EAR                        ((uint16_t)0x0301)

// ─── 0x00D — Heart Rate Sensor ────────────────────────────────────────────
#define APPEARANCE_HEART_RATE_SENSOR                      ((uint16_t)0x0340)
#define APPEARANCE_HEART_RATE_SENSOR_BELT                 ((uint16_t)0x0341)

// ─── 0x00E — Blood Pressure ───────────────────────────────────────────────
#define APPEARANCE_BLOOD_PRESSURE                         ((uint16_t)0x0380)
#define APPEARANCE_BLOOD_PRESSURE_ARM                     ((uint16_t)0x0381)
#define APPEARANCE_BLOOD_PRESSURE_WRIST                   ((uint16_t)0x0382)

// ─── 0x00F — Human Interface Device ───────────────────────────────────────
#define APPEARANCE_HID                                    ((uint16_t)0x03C0)
#define APPEARANCE_HID_KEYBOARD                           ((uint16_t)0x03C1)
#define APPEARANCE_HID_MOUSE                              ((uint16_t)0x03C2)
#define APPEARANCE_HID_JOYSTICK                           ((uint16_t)0x03C3)
#define APPEARANCE_HID_GAMEPAD                            ((uint16_t)0x03C4)
#define APPEARANCE_HID_DIGITIZER_TABLET                   ((uint16_t)0x03C5)
#define APPEARANCE_HID_CARD_READER                        ((uint16_t)0x03C6)
#define APPEARANCE_HID_DIGITAL_PEN                        ((uint16_t)0x03C7)
#define APPEARANCE_HID_BARCODE_SCANNER                    ((uint16_t)0x03C8)
#define APPEARANCE_HID_TOUCHPAD                           ((uint16_t)0x03C9)
#define APPEARANCE_HID_PRESENTATION_REMOTE                ((uint16_t)0x03CA)

// ─── 0x010 — Glucose Meter ────────────────────────────────────────────────
#define APPEARANCE_GLUCOSE_METER                          ((uint16_t)0x0400)

// ─── 0x011 — Running Walking Sensor ───────────────────────────────────────
#define APPEARANCE_RUNNING_WALKING_SENSOR                 ((uint16_t)0x0440)
#define APPEARANCE_RUNNING_WALKING_SENSOR_IN_SHOE         ((uint16_t)0x0441)
#define APPEARANCE_RUNNING_WALKING_SENSOR_ON_SHOE         ((uint16_t)0x0442)
#define APPEARANCE_RUNNING_WALKING_SENSOR_ON_HIP          ((uint16_t)0x0443)

// ─── 0x012 — Cycling ──────────────────────────────────────────────────────
#define APPEARANCE_CYCLING                                ((uint16_t)0x0480)
#define APPEARANCE_CYCLING_COMPUTER                       ((uint16_t)0x0481)
#define APPEARANCE_CYCLING_SPEED_SENSOR                   ((uint16_t)0x0482)
#define APPEARANCE_CYCLING_CADENCE_SENSOR                 ((uint16_t)0x0483)
#define APPEARANCE_CYCLING_POWER_SENSOR                   ((uint16_t)0x0484)
#define APPEARANCE_CYCLING_SPEED_CADENCE_SENSOR           ((uint16_t)0x0485)

// ─── 0x013 — Control Device ───────────────────────────────────────────────
#define APPEARANCE_CONTROL_DEVICE                         ((uint16_t)0x04C0)
#define APPEARANCE_CONTROL_DEVICE_SWITCH                  ((uint16_t)0x04C1)
#define APPEARANCE_CONTROL_DEVICE_MULTI_SWITCH            ((uint16_t)0x04C2)
#define APPEARANCE_CONTROL_DEVICE_BUTTON                  ((uint16_t)0x04C3)
#define APPEARANCE_CONTROL_DEVICE_SLIDER                  ((uint16_t)0x04C4)
#define APPEARANCE_CONTROL_DEVICE_ROTARY_SWITCH           ((uint16_t)0x04C5)
#define APPEARANCE_CONTROL_DEVICE_TOUCH_PANEL             ((uint16_t)0x04C6)
#define APPEARANCE_CONTROL_DEVICE_SINGLE_SWITCH           ((uint16_t)0x04C7)
#define APPEARANCE_CONTROL_DEVICE_DOUBLE_SWITCH           ((uint16_t)0x04C8)
#define APPEARANCE_CONTROL_DEVICE_TRIPLE_SWITCH           ((uint16_t)0x04C9)
#define APPEARANCE_CONTROL_DEVICE_BATTERY_SWITCH          ((uint16_t)0x04CA)
#define APPEARANCE_CONTROL_DEVICE_ENERGY_HARVESTING       ((uint16_t)0x04CB)
#define APPEARANCE_CONTROL_DEVICE_PUSH_BUTTON             ((uint16_t)0x04CC)
#define APPEARANCE_CONTROL_DEVICE_DIAL                    ((uint16_t)0x04CD)

// ─── 0x014 — Network Device ───────────────────────────────────────────────
#define APPEARANCE_NETWORK_DEVICE                         ((uint16_t)0x0500)
#define APPEARANCE_NETWORK_DEVICE_ACCESS_POINT            ((uint16_t)0x0501)
#define APPEARANCE_NETWORK_DEVICE_MESH_DEVICE             ((uint16_t)0x0502)
#define APPEARANCE_NETWORK_DEVICE_MESH_NETWORK_PROXY      ((uint16_t)0x0503)

// ─── 0x015 — Sensor ───────────────────────────────────────────────────────
#define APPEARANCE_SENSOR                                 ((uint16_t)0x0540)
#define APPEARANCE_SENSOR_MOTION                          ((uint16_t)0x0541)
#define APPEARANCE_SENSOR_AIR_QUALITY                     ((uint16_t)0x0542)
#define APPEARANCE_SENSOR_TEMPERATURE                     ((uint16_t)0x0543)
#define APPEARANCE_SENSOR_HUMIDITY                        ((uint16_t)0x0544)
#define APPEARANCE_SENSOR_LEAK                            ((uint16_t)0x0545)
#define APPEARANCE_SENSOR_SMOKE                           ((uint16_t)0x0546)
#define APPEARANCE_SENSOR_OCCUPANCY                       ((uint16_t)0x0547)
#define APPEARANCE_SENSOR_CONTACT                         ((uint16_t)0x0548)
#define APPEARANCE_SENSOR_CARBON_MONOXIDE                 ((uint16_t)0x0549)
#define APPEARANCE_SENSOR_CARBON_DIOXIDE                  ((uint16_t)0x054A)
#define APPEARANCE_SENSOR_AMBIENT_LIGHT                   ((uint16_t)0x054B)
#define APPEARANCE_SENSOR_ENERGY                          ((uint16_t)0x054C)
#define APPEARANCE_SENSOR_COLOR_LIGHT                     ((uint16_t)0x054D)
#define APPEARANCE_SENSOR_RAIN                            ((uint16_t)0x054E)
#define APPEARANCE_SENSOR_FIRE                            ((uint16_t)0x054F)
#define APPEARANCE_SENSOR_WIND                            ((uint16_t)0x0550)
#define APPEARANCE_SENSOR_PROXIMITY                       ((uint16_t)0x0551)
#define APPEARANCE_SENSOR_MULTI                           ((uint16_t)0x0552)
#define APPEARANCE_SENSOR_FLUSH_MOUNTED                   ((uint16_t)0x0553)
#define APPEARANCE_SENSOR_CEILING_MOUNTED                 ((uint16_t)0x0554)
#define APPEARANCE_SENSOR_WALL_MOUNTED                    ((uint16_t)0x0555)
#define APPEARANCE_SENSOR_MULTISENSOR                     ((uint16_t)0x0556)
#define APPEARANCE_SENSOR_ENERGY_METER                    ((uint16_t)0x0557)
#define APPEARANCE_SENSOR_FLAME_DETECTOR                  ((uint16_t)0x0558)
#define APPEARANCE_SENSOR_VEHICLE_TIRE_PRESSURE           ((uint16_t)0x0559)

// ─── 0x016 — Light Fixtures ───────────────────────────────────────────────
#define APPEARANCE_LIGHT_FIXTURES                         ((uint16_t)0x0580)
#define APPEARANCE_LIGHT_FIXTURES_WALL                    ((uint16_t)0x0581)
#define APPEARANCE_LIGHT_FIXTURES_CEILING                 ((uint16_t)0x0582)
#define APPEARANCE_LIGHT_FIXTURES_FLOOR                   ((uint16_t)0x0583)
#define APPEARANCE_LIGHT_FIXTURES_CABINET                 ((uint16_t)0x0584)
#define APPEARANCE_LIGHT_FIXTURES_DESK                    ((uint16_t)0x0585)
#define APPEARANCE_LIGHT_FIXTURES_TROFFER                 ((uint16_t)0x0586)
#define APPEARANCE_LIGHT_FIXTURES_PENDANT                 ((uint16_t)0x0587)
#define APPEARANCE_LIGHT_FIXTURES_IN_GROUND               ((uint16_t)0x0588)
#define APPEARANCE_LIGHT_FIXTURES_FLOOD                   ((uint16_t)0x0589)
#define APPEARANCE_LIGHT_FIXTURES_UNDERWATER              ((uint16_t)0x058A)
#define APPEARANCE_LIGHT_FIXTURES_BOLLARD                 ((uint16_t)0x058B)
#define APPEARANCE_LIGHT_FIXTURES_PATHWAY                 ((uint16_t)0x058C)
#define APPEARANCE_LIGHT_FIXTURES_GARDEN                  ((uint16_t)0x058D)
#define APPEARANCE_LIGHT_FIXTURES_POLE_TOP                ((uint16_t)0x058E)
#define APPEARANCE_LIGHT_FIXTURES_SPOTLIGHT               ((uint16_t)0x058F)
#define APPEARANCE_LIGHT_FIXTURES_LINEAR                  ((uint16_t)0x0590)
#define APPEARANCE_LIGHT_FIXTURES_STREET                  ((uint16_t)0x0591)
#define APPEARANCE_LIGHT_FIXTURES_SHELVES                 ((uint16_t)0x0592)
#define APPEARANCE_LIGHT_FIXTURES_BAY                     ((uint16_t)0x0593)
#define APPEARANCE_LIGHT_FIXTURES_EMERGENCY_EXIT          ((uint16_t)0x0594)
#define APPEARANCE_LIGHT_FIXTURES_CONTROLLER              ((uint16_t)0x0595)
#define APPEARANCE_LIGHT_FIXTURES_DRIVER                  ((uint16_t)0x0596)
#define APPEARANCE_LIGHT_FIXTURES_BULB                    ((uint16_t)0x0597)
#define APPEARANCE_LIGHT_FIXTURES_LOW_BAY                 ((uint16_t)0x0598)
#define APPEARANCE_LIGHT_FIXTURES_HIGH_BAY                ((uint16_t)0x0599)

// ─── 0x017 — Fan ──────────────────────────────────────────────────────────
#define APPEARANCE_FAN                                    ((uint16_t)0x05C0)
#define APPEARANCE_FAN_CEILING                            ((uint16_t)0x05C1)
#define APPEARANCE_FAN_AXIAL                              ((uint16_t)0x05C2)
#define APPEARANCE_FAN_EXHAUST                            ((uint16_t)0x05C3)
#define APPEARANCE_FAN_PEDESTAL                           ((uint16_t)0x05C4)
#define APPEARANCE_FAN_DESK                               ((uint16_t)0x05C5)
#define APPEARANCE_FAN_WALL                               ((uint16_t)0x05C6)

// ─── 0x018 — HVAC ─────────────────────────────────────────────────────────
#define APPEARANCE_HVAC                                   ((uint16_t)0x0600)
#define APPEARANCE_HVAC_THERMOSTAT                        ((uint16_t)0x0601)
#define APPEARANCE_HVAC_HUMIDIFIER                        ((uint16_t)0x0602)
#define APPEARANCE_HVAC_DEHUMIDIFIER                      ((uint16_t)0x0603)
#define APPEARANCE_HVAC_HEATER                            ((uint16_t)0x0604)
#define APPEARANCE_HVAC_RADIATOR                          ((uint16_t)0x0605)
#define APPEARANCE_HVAC_BOILER                            ((uint16_t)0x0606)
#define APPEARANCE_HVAC_HEAT_PUMP                         ((uint16_t)0x0607)
#define APPEARANCE_HVAC_INFRARED_HEATER                   ((uint16_t)0x0608)
#define APPEARANCE_HVAC_RADIANT_PANEL_HEATER              ((uint16_t)0x0609)
#define APPEARANCE_HVAC_FAN_HEATER                        ((uint16_t)0x060A)
#define APPEARANCE_HVAC_AIR_CURTAIN                       ((uint16_t)0x060B)

// ─── 0x019 — Air Conditioning ─────────────────────────────────────────────
#define APPEARANCE_AIR_CONDITIONING                       ((uint16_t)0x0640)

// ─── 0x01A — Humidifier ───────────────────────────────────────────────────
#define APPEARANCE_HUMIDIFIER                             ((uint16_t)0x0680)

// ─── 0x01B — Heating ──────────────────────────────────────────────────────
#define APPEARANCE_HEATING                                ((uint16_t)0x06C0)
#define APPEARANCE_HEATING_RADIATOR                       ((uint16_t)0x06C1)
#define APPEARANCE_HEATING_BOILER                         ((uint16_t)0x06C2)
#define APPEARANCE_HEATING_HEAT_PUMP                      ((uint16_t)0x06C3)
#define APPEARANCE_HEATING_INFRARED_HEATER                ((uint16_t)0x06C4)
#define APPEARANCE_HEATING_RADIANT_PANEL_HEATER           ((uint16_t)0x06C5)
#define APPEARANCE_HEATING_FAN_HEATER                     ((uint16_t)0x06C6)
#define APPEARANCE_HEATING_AIR_CURTAIN                    ((uint16_t)0x06C7)

// ─── 0x01C — Access Control ───────────────────────────────────────────────
#define APPEARANCE_ACCESS_CONTROL                         ((uint16_t)0x0700)
#define APPEARANCE_ACCESS_CONTROL_ACCESS_DOOR             ((uint16_t)0x0701)
#define APPEARANCE_ACCESS_CONTROL_GARAGE_DOOR             ((uint16_t)0x0702)
#define APPEARANCE_ACCESS_CONTROL_EMERGENCY_EXIT_DOOR     ((uint16_t)0x0703)
#define APPEARANCE_ACCESS_CONTROL_ACCESS_LOCK             ((uint16_t)0x0704)
#define APPEARANCE_ACCESS_CONTROL_ELEVATOR                ((uint16_t)0x0705)
#define APPEARANCE_ACCESS_CONTROL_WINDOW                  ((uint16_t)0x0706)
#define APPEARANCE_ACCESS_CONTROL_ENTRANCE_GATE           ((uint16_t)0x0707)
#define APPEARANCE_ACCESS_CONTROL_DOOR_LOCK               ((uint16_t)0x0708)
#define APPEARANCE_ACCESS_CONTROL_LOCKER                  ((uint16_t)0x0709)

// ─── 0x01D — Motorized Device ─────────────────────────────────────────────
#define APPEARANCE_MOTORIZED_DEVICE                       ((uint16_t)0x0740)
#define APPEARANCE_MOTORIZED_DEVICE_GATE                  ((uint16_t)0x0741)
#define APPEARANCE_MOTORIZED_DEVICE_AWNING                ((uint16_t)0x0742)
#define APPEARANCE_MOTORIZED_DEVICE_BLINDS_SHADES         ((uint16_t)0x0743)
#define APPEARANCE_MOTORIZED_DEVICE_CURTAINS              ((uint16_t)0x0744)
#define APPEARANCE_MOTORIZED_DEVICE_SCREEN                ((uint16_t)0x0745)

// ─── 0x01E — Power Device ─────────────────────────────────────────────────
#define APPEARANCE_POWER_DEVICE                           ((uint16_t)0x0780)
#define APPEARANCE_POWER_DEVICE_OUTLET                    ((uint16_t)0x0781)
#define APPEARANCE_POWER_DEVICE_STRIP                     ((uint16_t)0x0782)
#define APPEARANCE_POWER_DEVICE_PLUG                      ((uint16_t)0x0783)
#define APPEARANCE_POWER_DEVICE_SUPPLY                    ((uint16_t)0x0784)
#define APPEARANCE_POWER_DEVICE_LED_DRIVER                ((uint16_t)0x0785)
#define APPEARANCE_POWER_DEVICE_FLUORESCENT_LAMP_GEAR     ((uint16_t)0x0786)
#define APPEARANCE_POWER_DEVICE_HID_LAMP_GEAR             ((uint16_t)0x0787)
#define APPEARANCE_POWER_DEVICE_CHARGE_CASE               ((uint16_t)0x0788)
#define APPEARANCE_POWER_DEVICE_POWER_BANK                ((uint16_t)0x0789)

// ─── 0x01F — Light Source ─────────────────────────────────────────────────
#define APPEARANCE_LIGHT_SOURCE                           ((uint16_t)0x07C0)
#define APPEARANCE_LIGHT_SOURCE_INCANDESCENT_BULB         ((uint16_t)0x07C1)
#define APPEARANCE_LIGHT_SOURCE_LED_LAMP                  ((uint16_t)0x07C2)
#define APPEARANCE_LIGHT_SOURCE_HID_LAMP                  ((uint16_t)0x07C3)
#define APPEARANCE_LIGHT_SOURCE_FLUORESCENT_LAMP          ((uint16_t)0x07C4)
#define APPEARANCE_LIGHT_SOURCE_LED_ARRAY                 ((uint16_t)0x07C5)
#define APPEARANCE_LIGHT_SOURCE_MULTI_COLOR_LED_ARRAY     ((uint16_t)0x07C6)
#define APPEARANCE_LIGHT_SOURCE_LOW_VOLTAGE_HALOGEN       ((uint16_t)0x07C7)
#define APPEARANCE_LIGHT_SOURCE_OLED                      ((uint16_t)0x07C8)

// ─── 0x020 — Window Covering ──────────────────────────────────────────────
#define APPEARANCE_WINDOW_COVERING                        ((uint16_t)0x0800)
#define APPEARANCE_WINDOW_COVERING_SHADES                 ((uint16_t)0x0801)
#define APPEARANCE_WINDOW_COVERING_BLINDS                 ((uint16_t)0x0802)
#define APPEARANCE_WINDOW_COVERING_AWNING                 ((uint16_t)0x0803)
#define APPEARANCE_WINDOW_COVERING_CURTAIN                ((uint16_t)0x0804)
#define APPEARANCE_WINDOW_COVERING_EXTERIOR_SHUTTER       ((uint16_t)0x0805)
#define APPEARANCE_WINDOW_COVERING_EXTERIOR_SCREEN        ((uint16_t)0x0806)

// ─── 0x021 — Audio Sink ───────────────────────────────────────────────────
#define APPEARANCE_AUDIO_SINK                             ((uint16_t)0x0840)
#define APPEARANCE_AUDIO_SINK_STANDALONE_SPEAKER          ((uint16_t)0x0841)
#define APPEARANCE_AUDIO_SINK_SOUNDBAR                    ((uint16_t)0x0842)
#define APPEARANCE_AUDIO_SINK_BOOKSHELF_SPEAKER           ((uint16_t)0x0843)
#define APPEARANCE_AUDIO_SINK_STANDMOUNTED_SPEAKER        ((uint16_t)0x0844)
#define APPEARANCE_AUDIO_SINK_SPEAKERPHONE                ((uint16_t)0x0845)

// ─── 0x022 — Audio Source ─────────────────────────────────────────────────
#define APPEARANCE_AUDIO_SOURCE                           ((uint16_t)0x0880)
#define APPEARANCE_AUDIO_SOURCE_MICROPHONE                ((uint16_t)0x0881)
#define APPEARANCE_AUDIO_SOURCE_ALARM                     ((uint16_t)0x0882)
#define APPEARANCE_AUDIO_SOURCE_BELL                      ((uint16_t)0x0883)
#define APPEARANCE_AUDIO_SOURCE_HORN                      ((uint16_t)0x0884)
#define APPEARANCE_AUDIO_SOURCE_BROADCASTING_DEVICE       ((uint16_t)0x0885)
#define APPEARANCE_AUDIO_SOURCE_SERVICE_DESK              ((uint16_t)0x0886)
#define APPEARANCE_AUDIO_SOURCE_KIOSK                     ((uint16_t)0x0887)
#define APPEARANCE_AUDIO_SOURCE_BROADCASTING_ROOM         ((uint16_t)0x0888)
#define APPEARANCE_AUDIO_SOURCE_AUDITORIUM                ((uint16_t)0x0889)

// ─── 0x023 — Motorized Vehicle ────────────────────────────────────────────
#define APPEARANCE_MOTORIZED_VEHICLE                      ((uint16_t)0x08C0)
#define APPEARANCE_MOTORIZED_VEHICLE_CAR                  ((uint16_t)0x08C1)
#define APPEARANCE_MOTORIZED_VEHICLE_LARGE_GOODS          ((uint16_t)0x08C2)
#define APPEARANCE_MOTORIZED_VEHICLE_TWO_WHEELED          ((uint16_t)0x08C3)
#define APPEARANCE_MOTORIZED_VEHICLE_MOTORBIKE            ((uint16_t)0x08C4)
#define APPEARANCE_MOTORIZED_VEHICLE_SCOOTER              ((uint16_t)0x08C5)
#define APPEARANCE_MOTORIZED_VEHICLE_MOPED                ((uint16_t)0x08C6)
#define APPEARANCE_MOTORIZED_VEHICLE_THREE_WHEELED        ((uint16_t)0x08C7)
#define APPEARANCE_MOTORIZED_VEHICLE_LIGHT                ((uint16_t)0x08C8)
#define APPEARANCE_MOTORIZED_VEHICLE_QUAD_BIKE            ((uint16_t)0x08C9)
#define APPEARANCE_MOTORIZED_VEHICLE_MINIBUS              ((uint16_t)0x08CA)
#define APPEARANCE_MOTORIZED_VEHICLE_BUS                  ((uint16_t)0x08CB)
#define APPEARANCE_MOTORIZED_VEHICLE_TROLLEY              ((uint16_t)0x08CC)
#define APPEARANCE_MOTORIZED_VEHICLE_AGRICULTURAL         ((uint16_t)0x08CD)
#define APPEARANCE_MOTORIZED_VEHICLE_CAMPER_CARAVAN       ((uint16_t)0x08CE)
#define APPEARANCE_MOTORIZED_VEHICLE_RECREATIONAL         ((uint16_t)0x08CF)

// ─── 0x024 — Domestic Appliance ───────────────────────────────────────────
#define APPEARANCE_DOMESTIC_APPLIANCE                     ((uint16_t)0x0900)
#define APPEARANCE_DOMESTIC_APPLIANCE_REFRIGERATOR        ((uint16_t)0x0901)
#define APPEARANCE_DOMESTIC_APPLIANCE_FREEZER             ((uint16_t)0x0902)
#define APPEARANCE_DOMESTIC_APPLIANCE_OVEN                ((uint16_t)0x0903)
#define APPEARANCE_DOMESTIC_APPLIANCE_MICROWAVE           ((uint16_t)0x0904)
#define APPEARANCE_DOMESTIC_APPLIANCE_TOASTER             ((uint16_t)0x0905)
#define APPEARANCE_DOMESTIC_APPLIANCE_WASHING_MACHINE     ((uint16_t)0x0906)
#define APPEARANCE_DOMESTIC_APPLIANCE_DRYER               ((uint16_t)0x0907)
#define APPEARANCE_DOMESTIC_APPLIANCE_COFFEE_MAKER        ((uint16_t)0x0908)
#define APPEARANCE_DOMESTIC_APPLIANCE_CLOTHES_IRON        ((uint16_t)0x0909)
#define APPEARANCE_DOMESTIC_APPLIANCE_CURLING_IRON        ((uint16_t)0x090A)
#define APPEARANCE_DOMESTIC_APPLIANCE_HAIR_DRYER          ((uint16_t)0x090B)
#define APPEARANCE_DOMESTIC_APPLIANCE_VACUUM_CLEANER      ((uint16_t)0x090C)
#define APPEARANCE_DOMESTIC_APPLIANCE_ROBOTIC_VACUUM      ((uint16_t)0x090D)
#define APPEARANCE_DOMESTIC_APPLIANCE_RICE_COOKER         ((uint16_t)0x090E)
#define APPEARANCE_DOMESTIC_APPLIANCE_CLOTHES_STEAMER     ((uint16_t)0x090F)

// ─── 0x025 — Wearable Audio Device ────────────────────────────────────────
#define APPEARANCE_WEARABLE_AUDIO_DEVICE                  ((uint16_t)0x0940)
#define APPEARANCE_WEARABLE_AUDIO_DEVICE_EARBUD           ((uint16_t)0x0941)
#define APPEARANCE_WEARABLE_AUDIO_DEVICE_HEADSET          ((uint16_t)0x0942)
#define APPEARANCE_WEARABLE_AUDIO_DEVICE_HEADPHONES       ((uint16_t)0x0943)
#define APPEARANCE_WEARABLE_AUDIO_DEVICE_NECK_BAND        ((uint16_t)0x0944)
#define APPEARANCE_WEARABLE_AUDIO_DEVICE_LEFT_EARBUD      ((uint16_t)0x0945)
#define APPEARANCE_WEARABLE_AUDIO_DEVICE_RIGHT_EARBUD     ((uint16_t)0x0946)

// ─── 0x026 — Aircraft ─────────────────────────────────────────────────────
#define APPEARANCE_AIRCRAFT                               ((uint16_t)0x0980)
#define APPEARANCE_AIRCRAFT_LIGHT                         ((uint16_t)0x0981)
#define APPEARANCE_AIRCRAFT_MICROLIGHT                    ((uint16_t)0x0982)
#define APPEARANCE_AIRCRAFT_PARAGLIDER                    ((uint16_t)0x0983)
#define APPEARANCE_AIRCRAFT_LARGE_PASSENGER               ((uint16_t)0x0984)

// ─── 0x027 — AV Equipment ─────────────────────────────────────────────────
#define APPEARANCE_AV_EQUIPMENT                           ((uint16_t)0x09C0)
#define APPEARANCE_AV_EQUIPMENT_AMPLIFIER                 ((uint16_t)0x09C1)
#define APPEARANCE_AV_EQUIPMENT_RECEIVER                  ((uint16_t)0x09C2)
#define APPEARANCE_AV_EQUIPMENT_RADIO                     ((uint16_t)0x09C3)
#define APPEARANCE_AV_EQUIPMENT_TUNER                     ((uint16_t)0x09C4)
#define APPEARANCE_AV_EQUIPMENT_TURNTABLE                 ((uint16_t)0x09C5)
#define APPEARANCE_AV_EQUIPMENT_CD_PLAYER                 ((uint16_t)0x09C6)
#define APPEARANCE_AV_EQUIPMENT_DVD_PLAYER                ((uint16_t)0x09C7)
#define APPEARANCE_AV_EQUIPMENT_BLURAY_PLAYER             ((uint16_t)0x09C8)
#define APPEARANCE_AV_EQUIPMENT_OPTICAL_DISC_PLAYER       ((uint16_t)0x09C9)
#define APPEARANCE_AV_EQUIPMENT_SET_TOP_BOX               ((uint16_t)0x09CA)

// ─── 0x028 — Display Equipment ────────────────────────────────────────────
#define APPEARANCE_DISPLAY_EQUIPMENT                      ((uint16_t)0x0A00)
#define APPEARANCE_DISPLAY_EQUIPMENT_TELEVISION           ((uint16_t)0x0A01)
#define APPEARANCE_DISPLAY_EQUIPMENT_MONITOR              ((uint16_t)0x0A02)
#define APPEARANCE_DISPLAY_EQUIPMENT_PROJECTOR            ((uint16_t)0x0A03)

// ─── 0x029 — Hearing Aid ──────────────────────────────────────────────────
#define APPEARANCE_HEARING_AID                            ((uint16_t)0x0A40)
#define APPEARANCE_HEARING_AID_IN_EAR                     ((uint16_t)0x0A41)
#define APPEARANCE_HEARING_AID_BEHIND_EAR                 ((uint16_t)0x0A42)
#define APPEARANCE_HEARING_AID_COCHLEAR_IMPLANT           ((uint16_t)0x0A43)

// ─── 0x02A — Gaming ───────────────────────────────────────────────────────
#define APPEARANCE_GAMING                                 ((uint16_t)0x0A80)
#define APPEARANCE_GAMING_HOME_VIDEO_CONSOLE              ((uint16_t)0x0A81)
#define APPEARANCE_GAMING_PORTABLE_HANDHELD               ((uint16_t)0x0A82)

// ─── 0x02B — Signage ──────────────────────────────────────────────────────
#define APPEARANCE_SIGNAGE                                ((uint16_t)0x0AC0)
#define APPEARANCE_SIGNAGE_DIGITAL                        ((uint16_t)0x0AC1)
#define APPEARANCE_SIGNAGE_ELECTRONIC_LABEL               ((uint16_t)0x0AC2)

// ─── 0x031 — Pulse Oximeter ───────────────────────────────────────────────
#define APPEARANCE_PULSE_OXIMETER                         ((uint16_t)0x0C40)
#define APPEARANCE_PULSE_OXIMETER_FINGERTIP               ((uint16_t)0x0C41)
#define APPEARANCE_PULSE_OXIMETER_WRIST_WORN              ((uint16_t)0x0C42)

// ─── 0x032 — Weight Scale ─────────────────────────────────────────────────
#define APPEARANCE_WEIGHT_SCALE                           ((uint16_t)0x0C80)

// ─── 0x033 — Personal Mobility Device ─────────────────────────────────────
#define APPEARANCE_PERSONAL_MOBILITY_DEVICE               ((uint16_t)0x0CC0)
#define APPEARANCE_PERSONAL_MOBILITY_DEVICE_WHEELCHAIR    ((uint16_t)0x0CC1)
#define APPEARANCE_PERSONAL_MOBILITY_DEVICE_SCOOTER       ((uint16_t)0x0CC2)

// ─── 0x034 — Continuous Glucose Monitor ───────────────────────────────────
#define APPEARANCE_CONTINUOUS_GLUCOSE_MONITOR             ((uint16_t)0x0D00)

// ─── 0x035 — Insulin Pump ─────────────────────────────────────────────────
#define APPEARANCE_INSULIN_PUMP                           ((uint16_t)0x0D40)
#define APPEARANCE_INSULIN_PUMP_DURABLE                   ((uint16_t)0x0D41)
#define APPEARANCE_INSULIN_PUMP_PATCH                     ((uint16_t)0x0D44)
#define APPEARANCE_INSULIN_PEN                            ((uint16_t)0x0D48)

// ─── 0x036 — Medication Delivery ──────────────────────────────────────────
#define APPEARANCE_MEDICATION_DELIVERY                    ((uint16_t)0x0D80)

// ─── 0x037 — Spirometer ───────────────────────────────────────────────────
#define APPEARANCE_SPIROMETER                             ((uint16_t)0x0DC0)
#define APPEARANCE_SPIROMETER_HANDHELD                    ((uint16_t)0x0DC1)

// ─── 0x051 — Outdoor Sports Activity ──────────────────────────────────────
#define APPEARANCE_OUTDOOR_SPORTS_ACTIVITY                ((uint16_t)0x1440)
#define APPEARANCE_OUTDOOR_SPORTS_LOCATION_DISPLAY        ((uint16_t)0x1441)
#define APPEARANCE_OUTDOOR_SPORTS_LOCATION_NAV_DISPLAY    ((uint16_t)0x1442)
#define APPEARANCE_OUTDOOR_SPORTS_LOCATION_POD            ((uint16_t)0x1443)
#define APPEARANCE_OUTDOOR_SPORTS_LOCATION_NAV_POD        ((uint16_t)0x1444)

// ─── 0x052 — Industrial Measurement Device ────────────────────────────────
#define APPEARANCE_INDUSTRIAL_MEASUREMENT_DEVICE          ((uint16_t)0x1480)
#define APPEARANCE_INDUSTRIAL_MEASUREMENT_TORQUE_TEST     ((uint16_t)0x1481)
#define APPEARANCE_INDUSTRIAL_MEASUREMENT_CALIPER         ((uint16_t)0x1482)
#define APPEARANCE_INDUSTRIAL_MEASUREMENT_DIAL_INDICATOR  ((uint16_t)0x1483)
#define APPEARANCE_INDUSTRIAL_MEASUREMENT_MICROMETER      ((uint16_t)0x1484)
#define APPEARANCE_INDUSTRIAL_MEASUREMENT_HEIGHT_GAUGE    ((uint16_t)0x1485)
#define APPEARANCE_INDUSTRIAL_MEASUREMENT_FORCE_GAUGE     ((uint16_t)0x1486)

// ─── 0x053 — Industrial Tools ─────────────────────────────────────────────
#define APPEARANCE_INDUSTRIAL_TOOLS                       ((uint16_t)0x14C0)
#define APPEARANCE_INDUSTRIAL_TOOLS_MACHINE_TOOL_HOLDER   ((uint16_t)0x14C1)
#define APPEARANCE_INDUSTRIAL_TOOLS_GENERIC_CLAMPING      ((uint16_t)0x14C2)
#define APPEARANCE_INDUSTRIAL_TOOLS_CLAMPING_JAWS         ((uint16_t)0x14C3)
#define APPEARANCE_INDUSTRIAL_TOOLS_CLAMPING_COLLET       ((uint16_t)0x14C4)
#define APPEARANCE_INDUSTRIAL_TOOLS_CLAMPING_MANDREL      ((uint16_t)0x14C5)
#define APPEARANCE_INDUSTRIAL_TOOLS_VISE                  ((uint16_t)0x14C6)
#define APPEARANCE_INDUSTRIAL_TOOLS_ZERO_POINT_CLAMPING   ((uint16_t)0x14C7)
#define APPEARANCE_INDUSTRIAL_TOOLS_TORQUE_WRENCH         ((uint16_t)0x14C8)
#define APPEARANCE_INDUSTRIAL_TOOLS_TORQUE_SCREWDRIVER    ((uint16_t)0x14C9)

// ─── 0x054 — Cookware Device ──────────────────────────────────────────────
#define APPEARANCE_COOKWARE_DEVICE                        ((uint16_t)0x1500)
#define APPEARANCE_COOKWARE_POTS_AND_JUGS                 ((uint16_t)0x1501)
#define APPEARANCE_COOKWARE_PRESSURE_COOKER               ((uint16_t)0x1502)
#define APPEARANCE_COOKWARE_SLOW_COOKER                   ((uint16_t)0x1503)
#define APPEARANCE_COOKWARE_STEAM_COOKER                  ((uint16_t)0x1504)
#define APPEARANCE_COOKWARE_SAUCEPAN                      ((uint16_t)0x1505)
#define APPEARANCE_COOKWARE_FRYING_PAN                    ((uint16_t)0x1506)
#define APPEARANCE_COOKWARE_CASSEROLE                     ((uint16_t)0x1507)
#define APPEARANCE_COOKWARE_DUTCH_OVEN                    ((uint16_t)0x1508)
#define APPEARANCE_COOKWARE_GRILL_PAN                     ((uint16_t)0x1509)
#define APPEARANCE_COOKWARE_BRAISING_PAN                  ((uint16_t)0x150A)
#define APPEARANCE_COOKWARE_WOK_PAN                       ((uint16_t)0x150B)
#define APPEARANCE_COOKWARE_PAELLA_PAN                    ((uint16_t)0x150C)
#define APPEARANCE_COOKWARE_CREPE_PAN                     ((uint16_t)0x150D)
#define APPEARANCE_COOKWARE_TAGINE                        ((uint16_t)0x150E)
#define APPEARANCE_COOKWARE_FONDUE                        ((uint16_t)0x150F)
#define APPEARANCE_COOKWARE_LID                           ((uint16_t)0x1510)
#define APPEARANCE_COOKWARE_WIRED_PROBE                   ((uint16_t)0x1511)
#define APPEARANCE_COOKWARE_WIRELESS_PROBE                ((uint16_t)0x1512)
#define APPEARANCE_COOKWARE_BAKING_MOLDS                  ((uint16_t)0x1513)
#define APPEARANCE_COOKWARE_BAKING_TRAY                   ((uint16_t)0x1514)
