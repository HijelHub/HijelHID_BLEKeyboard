# Keyboard Layout Generator

Generates the keyboard layout tables used by `print()` / `println()` / `write()`:

- `src/HIDKeyboardLayouts.h` — layout types and the list of available layouts
- `src/layouts/HIDLayout_<Name>.cpp` — one file per layout

These files are generated — don't edit them by hand. Change `gen_layouts.py` and regenerate.

## Source data

[Unicode CLDR](https://cldr.unicode.org/) keyboard data, **release 43** — the last release that includes per-platform Windows and macOS layouts. © Unicode, Inc., used under the [Unicode License v3](https://www.unicode.org/license.txt).

## Regenerate

```
git clone --depth 1 --branch release-43 https://github.com/unicode-org/cldr.git
python3 gen_layouts.py cldr/keyboards ../../src report.txt
```

`report.txt` lists, per layout: characters it cannot type, dead keys, AltGr characters, and counts of accented characters.

## Add a layout

1. Find the CLDR file in `cldr/keyboards/windows/` or `cldr/keyboards/osx/`.
2. Add a line to `LAYOUTS` in `gen_layouts.py`: enum name, file, display name, platform.
3. Regenerate.
4. Test on a host set to that layout before submitting.

## How it works

- CLDR names key positions in ISO style (`E00` = key left of 1, `B00` = key left of Z). `ISO_TO_HID` maps these to USB HID usages.
- For each character, the generator picks the key and modifiers that type it — preferring a key that types it directly over a dead key, then the fewest modifiers (none → Shift → AltGr → Shift+AltGr).
- AltGr is sent as Right Alt on Windows/Linux layouts and Left Option on macOS layouts (some macOS layouts treat Right Option + Shift as plain Shift).
- Dead keys (`^`, `` ` ``, `~`, `´`, `¨` on many layouts) are flagged; the library sends Space after them when the character itself is typed.
- Accented characters (Latin-1, Latin Extended-A, and €) are either typed directly or as dead key + base letter, taken from CLDR's dead-key tables.
