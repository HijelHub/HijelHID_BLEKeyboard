#!/usr/bin/env python3
"""
gen_layouts.py — generate HijelHID_BLEKeyboard keyboard layout tables.

Source data: Unicode CLDR keyboard data, release 43 (the last release that
includes per-platform Windows and macOS layouts in LDML keyboard format).
    git clone --depth 1 --branch release-43 https://github.com/unicode-org/cldr.git
Pass the path to the CLDR checkout's `keyboards` folder.

CLDR data is © Unicode, Inc., used under the Unicode License v3
(https://www.unicode.org/license.txt).

Usage:
    python3 gen_layouts.py <cldr>/keyboards  <library src folder>  [report.txt]

Writes <src>/HIDKeyboardLayouts.h and one <src>/layouts/HIDLayout_<Name>.cpp per layout.

Each layout entry produces:
  - ASCII table (0x20–0x7E): HID usage + flags (Shift / AltGr / dead key)
  - Direct characters: non-ASCII characters typed by one key (e.g. ç, ñ, €)
  - Compositions: accented characters typed as dead key + base letter (e.g. ´ + a → á)

Key positions use CLDR's ISO names (E00 = key left of 1, B00 = key left of Z,
etc.), mapped to USB HID Keyboard/Keypad usages below.
"""

import re
import sys
import unicodedata
import xml.etree.ElementTree as ET

# ── Layouts: (enum name, CLDR file, display name, platform) ──────────────────
LAYOUTS = [
    ("US",                    "windows/en-t-k0-windows.xml",          "US",                         "windows"),
    ("US_International",      "windows/en-t-k0-windows-extended.xml", "US-International",           "windows"),
    ("UK",                    "windows/en-GB-t-k0-windows.xml",       "United Kingdom",             "windows"),
    ("German",                "windows/de-t-k0-windows.xml",          "German",                     "windows"),
    ("French",                "windows/fr-t-k0-windows.xml",          "French (AZERTY)",            "windows"),
    ("Spanish",               "windows/es-t-k0-windows.xml",          "Spanish (Spain)",            "windows"),
    ("Spanish_LatAm",         "windows/es-419-t-k0-windows.xml",      "Spanish (Latin America)",    "windows"),
    ("Italian",               "windows/it-t-k0-windows.xml",          "Italian",                    "windows"),
    ("Portuguese",            "windows/pt-PT-t-k0-windows.xml",       "Portuguese (Portugal)",      "windows"),
    ("Portuguese_Brazil",     "windows/pt-t-k0-windows-var.xml",      "Portuguese (Brazil ABNT2)",  "windows"),
    ("UK_Mac",                "osx/en-GB-t-k0-osx.xml",               "British (macOS)",            "osx"),
    ("German_Mac",            "osx/de-t-k0-osx.xml",                  "German (macOS)",             "osx"),
    ("French_Mac",            "osx/fr-t-k0-osx.xml",                  "French (macOS)",             "osx"),
    ("Spanish_Mac",           "osx/es-t-k0-osx-extended.xml",         "Spanish - ISO (macOS)",      "osx"),
    ("Italian_Mac",           "osx/it-t-k0-osx-extended.xml",         "Italian - Pro (macOS)",      "osx"),
    ("Portuguese_Mac",        "osx/pt-PT-t-k0-osx.xml",               "Portuguese (macOS)",         "osx"),
    ("Portuguese_Brazil_Mac", "osx/pt-t-k0-osx.xml",                  "Brazilian (macOS)",          "osx"),
]

# ── ISO key position → USB HID usage ──────────────────────────────────────────
ISO_TO_HID = {
    "E00": 0x35,  # key left of 1 (grave / tilde on US)
    "E01": 0x1E, "E02": 0x1F, "E03": 0x20, "E04": 0x21, "E05": 0x22,
    "E06": 0x23, "E07": 0x24, "E08": 0x25, "E09": 0x26, "E10": 0x27,
    "E11": 0x2D, "E12": 0x2E,
    "D01": 0x14, "D02": 0x1A, "D03": 0x08, "D04": 0x15, "D05": 0x17,
    "D06": 0x1C, "D07": 0x18, "D08": 0x0C, "D09": 0x12, "D10": 0x13,
    "D11": 0x2F, "D12": 0x30,
    "D13": 0x31,  # ANSI key above Enter (backslash on US)
    "C01": 0x04, "C02": 0x16, "C03": 0x07, "C04": 0x09, "C05": 0x0A,
    "C06": 0x0B, "C07": 0x0D, "C08": 0x0E, "C09": 0x0F,
    "C10": 0x33, "C11": 0x34,
    # Key beside Enter: CLDR calls it C12 on every layout (including US backslash).
    # HID 0x31 (Backslash) — what the library has always sent for US \ and |.
    # Windows and Linux treat HID 0x31 and 0x32 (Non-US #) as the same key.
    "C12": 0x31,
    "B00": 0x64,  # ISO key left of Z (Non-US \ and |)
    "B01": 0x1D, "B02": 0x1B, "B03": 0x06, "B04": 0x19, "B05": 0x05,
    "B06": 0x11, "B07": 0x10, "B08": 0x36, "B09": 0x37, "B10": 0x38,
    "B11": 0x87,  # extra key right of / (ABNT2 "/?", HID International1)
    "A03": 0x2C,  # space
}

FLAG_SHIFT = 0x01
FLAG_ALTGR = 0x02
FLAG_DEAD  = 0x04

# Modifier states we can send, in preference order (fewest modifiers first).
# Windows/Linux: AltGr is sent as Right Alt.
# macOS: Option is sent as LEFT Option — some macOS layouts (e.g. German) map
# Right Option + Left Shift to the plain Shift level, so Right Option is unreliable.
STATES = {
    "windows": [(0, set()), (FLAG_SHIFT, {"shiftL"}),
                (FLAG_ALTGR, {"altR"}), (FLAG_SHIFT | FLAG_ALTGR, {"altR", "shiftL"})],
    "osx":     [(0, set()), (FLAG_SHIFT, {"shiftL"}),
                (FLAG_ALTGR, {"optL"}), (FLAG_SHIFT | FLAG_ALTGR, {"optL", "shiftL"})],
}
ALT_MOD = {"windows": "KEY_MOD_RALT", "osx": "KEY_MOD_LALT"}

# Non-ASCII characters we keep: Latin-1 Supplement, Latin Extended-A, and €.
def wanted(cp):
    return (0xA0 <= cp <= 0x17F) or cp == 0x20AC


def unescape(s):
    """CLDR values may contain \\u{XXXX} escapes."""
    return re.sub(r"\\u\{([0-9A-Fa-f]+)\}", lambda m: chr(int(m.group(1), 16)), s)


def token_ok(tok, pressed):
    """Does a single required token match something in the pressed set?"""
    for p in pressed:
        if p == tok:
            return True
        if tok in ("shift", "alt", "opt", "ctrl", "cmd") and p.startswith(tok):
            return True
    return False


def covers(tok, p):
    tok = tok.rstrip("?")
    return p == tok or (tok in ("shift", "alt", "opt", "ctrl", "cmd") and p.startswith(tok))


def modifiers_match(modattr, pressed):
    """True if a keyMap's modifiers attribute is satisfied by exactly `pressed`."""
    if modattr is None or modattr.strip() == "":
        return len(pressed) == 0
    for alt in modattr.split():
        toks = alt.split("+")
        required = [t for t in toks if not t.endswith("?")]
        if not all(token_ok(t, pressed) for t in required):
            continue
        if all(any(covers(t, p) for t in toks) for p in pressed):
            return True
    return False


def parse_layout(path, platform):
    root = ET.parse(path).getroot()
    keymaps = root.findall("keyMap")

    # Per state: iso -> char
    state_maps = []
    for flags, pressed in STATES[platform]:
        km = next((k for k in keymaps if modifiers_match(k.get("modifiers"), pressed)), None)
        m = {}
        if km is not None:
            for e in km.findall("map"):
                to = unescape(e.get("to", ""))
                if len(to) == 1:
                    # transform="no" marks a key that types the character directly,
                    # even when the same character is also a dead key elsewhere.
                    m[e.get("iso")] = (to, e.get("transform") == "no")
        state_maps.append((flags, m))

    transforms = {}
    for t in root.iter("transform"):
        frm, to = unescape(t.get("from", "")), unescape(t.get("to", ""))
        if len(frm) == 2 and len(to) == 1:
            transforms[frm] = to
    dead_chars = {f[0] for f in transforms}

    # best:      char -> (hid, flags) used to TYPE the character. Prefer a key
    #            that types it directly (non-dead), then fewest modifiers.
    # dead_keys: char -> (hid, flags) for the DEAD version of the character,
    #            used as the first key of dead key + letter compositions.
    best = {}
    dead_keys = {}
    unknown_iso = set()
    for flags, m in state_maps:
        for iso, (ch, no_transform) in m.items():
            if iso not in ISO_TO_HID:
                unknown_iso.add(iso)
                continue
            is_dead = ch in dead_chars and not no_transform
            f = flags | (FLAG_DEAD if is_dead else 0)
            cand = (ISO_TO_HID[iso], f)
            if is_dead and ch not in dead_keys:
                dead_keys[ch] = cand
            cur = best.get(ch)
            if cur is None:
                best[ch] = cand
            else:
                cur_dead, new_dead = bool(cur[1] & FLAG_DEAD), bool(f & FLAG_DEAD)
                if cur_dead and not new_dead:
                    best[ch] = cand
    return best, dead_keys, transforms, dead_chars, unknown_iso


def build(cldr_dir):
    results = []
    for enum, rel, name, platform in LAYOUTS:
        best, dead_keys, transforms, dead_chars, unknown = parse_layout(f"{cldr_dir}/{rel}", platform)

        ascii_tbl = []
        for c in range(0x20, 0x7F):
            ascii_tbl.append(best.get(chr(c), (0, 0)))

        direct = []
        for ch, (hid, f) in sorted(best.items(), key=lambda kv: ord(kv[0])):
            cp = ord(ch)
            if wanted(cp):
                direct.append((cp, hid, f))
        direct_cps = {d[0] for d in direct}

        compose = []
        for frm, to in sorted(transforms.items(), key=lambda kv: ord(kv[1])):
            cp = ord(to)
            dead, base = frm[0], frm[1]
            if not wanted(cp) or cp in direct_cps or base == " ":
                continue
            if dead not in dead_keys:
                continue
            if not (0x21 <= ord(base) <= 0x7E):
                continue
            bhid, bflags = ascii_tbl[ord(base) - 0x20]
            if bhid == 0 or (bflags & FLAG_DEAD):
                continue
            dhid, dflags = dead_keys[dead]
            compose.append((cp, dhid, dflags & ~FLAG_DEAD, base))
            direct_cps.add(cp)

        results.append(dict(enum=enum, name=name, platform=platform, file=rel,
                            ascii=ascii_tbl, direct=direct, compose=compose,
                            unknown=unknown, best=best))
    return results


def c_char_comment(c):
    if c == "\\":
        return "backslash"
    return c


def flagstr(f):
    parts = []
    if f & FLAG_SHIFT: parts.append("S")
    if f & FLAG_ALTGR: parts.append("G")
    if f & FLAG_DEAD:  parts.append("D")
    return "|".join(parts) if parts else "0"


LICENSE_LINES = [
    "// Generated by extras/layout_generator/gen_layouts.py — do not edit by hand.",
    "// Source: Unicode CLDR keyboard data, release 43.",
    "// CLDR data © Unicode, Inc., used under the Unicode License v3",
    "// (https://www.unicode.org/license.txt).",
]


def emit_header(results, out):
    L = ["#pragma once", "/**", " * HIDKeyboardLayouts.h", " *",
         " * Keyboard layouts for print() / println() / write().",
         " * Select one with keyboard.setKeyboardLayout(HIDLayout_German) etc.",
         " * The host computer must be set to the same keyboard layout.",
         " *",
         " * Each layout lives in its own file in src/layouts/. Layouts that a sketch",
         " * never references are removed by the linker, so only the layouts you",
         " * use take up flash.",
         " */", ""]
    L += LICENSE_LINES + [""]
    L += ["#include <stdint.h>", '#include "BLEHIDKeys.h"', "",
          "// Layout entry flags",
          "#define HID_LAYOUT_SHIFT  0x01  // hold Shift",
          "#define HID_LAYOUT_ALTGR  0x02  // hold AltGr (Windows/Linux: Right Alt; macOS: Option)",
          "#define HID_LAYOUT_DEAD   0x04  // dead key — Space is sent after it to print it alone",
          "",
          "// One printable ASCII character (0x20–0x7E). kc = 0 means the layout cannot type it.",
          "struct HIDLayoutKey     { uint8_t kc; uint8_t flags; };",
          "// A non-ASCII character typed with a single key (e.g. ç, ñ, €).",
          "struct HIDLayoutChar    { uint16_t cp; uint8_t kc; uint8_t flags; };",
          "// An accented character typed as dead key + base letter (e.g. ´ then a = á).",
          "struct HIDLayoutCompose { uint16_t cp; uint8_t deadKc; uint8_t deadFlags; char base; };",
          "",
          "struct HIDKeyboardLayout {",
          "    const char*             name;          // display name, e.g. \"German\"",
          "    uint8_t                 altMod;        // modifier bit sent for HID_LAYOUT_ALTGR",
          "    const HIDLayoutKey*     ascii;         // 95 entries, index = char - 0x20",
          "    const HIDLayoutChar*    direct;        // may be nullptr",
          "    uint8_t                 directCount;",
          "    const HIDLayoutCompose* compose;       // may be nullptr",
          "    uint8_t                 composeCount;",
          "};", "",
          "// ── Available layouts ────────────────────────────────────────────────────",
          "// Windows / Linux layouts:"]
    mac_heading_done = False
    for r in results:
        if r["platform"] == "osx" and not mac_heading_done:
            L.append("// macOS layouts (Option is sent as Left Option):")
            mac_heading_done = True
        L.append(f"extern const HIDKeyboardLayout HIDLayout_{r['enum']};"
                 f"{' ' * max(1, 24 - len(r['enum']))}// {r['name']}")
    open(out, "w", encoding="utf-8", newline="\n").write("\n".join(L) + "\n")


def emit_layout_file(r, out):
    e = r["enum"]
    L = [f"// {r['name']} keyboard layout — CLDR {r['file']}"] + LICENSE_LINES
    L += ["//", "// Flags: S = Shift, G = AltGr/Option, D = dead key (Space sent after it).",
          "", '#include "../HIDKeyboardLayouts.h"', "",
          "#define S HID_LAYOUT_SHIFT", "#define G HID_LAYOUT_ALTGR", "#define D HID_LAYOUT_DEAD", ""]
    L.append(f"static const HIDLayoutKey _ascii[95] = {{")
    for i, (hid, f) in enumerate(r["ascii"]):
        ch = chr(0x20 + i)
        label = "space" if ch == " " else ("backslash" if ch == "\\" else ch)
        L.append(f"    {{0x{hid:02X}, {flagstr(f):<5}}},  // {label}")
    L.append("};")
    if r["direct"]:
        L += ["", "static const HIDLayoutChar _direct[] = {"]
        for cp, hid, f in r["direct"]:
            L.append(f"    {{0x{cp:04X}, 0x{hid:02X}, {flagstr(f):<5}}},  // {chr(cp)}")
        L.append("};")
    if r["compose"]:
        L += ["", "static const HIDLayoutCompose _compose[] = {"]
        for cp, dhid, df, base in r["compose"]:
            b = "'\\\\'" if base == "\\" else ("'\\''" if base == "'" else f"'{base}'")
            L.append(f"    {{0x{cp:04X}, 0x{dhid:02X}, {flagstr(df):<3}, {b}}},  // {chr(cp)}")
        L.append("};")
    d  = "_direct"  if r["direct"]  else "nullptr"
    dn = "sizeof(_direct) / sizeof(_direct[0])" if r["direct"] else "0"
    c  = "_compose" if r["compose"] else "nullptr"
    cn = "sizeof(_compose) / sizeof(_compose[0])" if r["compose"] else "0"
    L += ["", f"const HIDKeyboardLayout HIDLayout_{e} = {{",
          f'    "{r["name"]}", {ALT_MOD[r["platform"]]},',
          f"    _ascii, {d}, {dn}, {c}, {cn}", "};", "",
          "#undef S", "#undef G", "#undef D"]
    open(out, "w", encoding="utf-8", newline="\n").write("\n".join(L) + "\n")


def emit_report(results, out):
    L = []
    for r in results:
        missing = [chr(0x20 + i) for i, (h, f) in enumerate(r["ascii"]) if h == 0]
        dead = [chr(0x20 + i) for i, (h, f) in enumerate(r["ascii"]) if f & FLAG_DEAD]
        altgr = [chr(0x20 + i) for i, (h, f) in enumerate(r["ascii"]) if f & FLAG_ALTGR]
        L.append(f"{r['enum']:<22} direct={len(r['direct']):3d} compose={len(r['compose']):3d} "
                 f"missing={''.join(missing) or '-'} dead={''.join(dead) or '-'} "
                 f"altgr={''.join(altgr) or '-'} unknownISO={sorted(r['unknown']) or '-'}")
    open(out, "w", encoding="utf-8").write("\n".join(L) + "\n")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    import os
    cldr, src = sys.argv[1], sys.argv[2]
    res = build(cldr)
    os.makedirs(os.path.join(src, "layouts"), exist_ok=True)
    emit_header(res, os.path.join(src, "HIDKeyboardLayouts.h"))
    for r in res:
        emit_layout_file(r, os.path.join(src, "layouts", f"HIDLayout_{r['enum']}.cpp"))
    if len(sys.argv) > 3:
        emit_report(res, sys.argv[3])
    print(f"Generated {len(res)} layouts -> {src}/HIDKeyboardLayouts.h + {src}/layouts/")
