# Combos

Every keymap in this userspace overlay (`crkbd/mockingb1rdblue`, `reviung/reviung34/mockingb1rdblue`, `keebio/nyquist/mockingb1rdblue`) uses the [gboards combo system](https://combos.gboards.ca) to define combos in a compact `combos.def` DSL instead of hand-writing the QMK `combo_t key_combos[]` array.

## How it actually works

There is **no Python (or Go) code generation step**. The gboards system is **pure C preprocessor**: a static dispatcher header named `keymap_combo.h` `#include`s the keymap's `combos.def` four times under different macro definitions to emit:

1. an `enum combos { ... }` (combo IDs),
2. `const uint16_t PROGMEM cmb_*[]` data arrays (key sequences),
3. the `combo_t key_combos[]` table (QMK runtime registration), and
4. the `process_combo_event(...)` switch (for `SUBS` / `TOGG` actions).

The dispatcher header is **identical across keymaps** — it does not depend on `combos.def` content. It's third-party content vendored from upstream `qmk-combos/combos` (see `../custom/tooling/README.md` in the parent monorepo for the pinned SHA and license disposition).

The previous version of this doc claimed a `gen_keymap_combo.py` script existed and produced `keymap_combo.h` from `combos.def`. That was wrong — no such script exists in either `qmk-combos/combos` or `germ/qmk_firmware`. The `.h` file is a static template, not a build product.

## Repo layout decision

- `combos.def` (per keymap) — **tracked source**. Edit this to add/remove combos.
- `keymap_combo.h` (per keymap) — **untracked** (see root `.gitignore`). It's a vendored upstream file, identical across keymaps. The canonical copy lives in the monorepo at `custom/tooling/gboards/g/keymap_combo.h`.
- On a fresh clone, QMK builds will fail at `#include "keymap_combo.h"` until the file is materialized in each keymap directory — run the round-trip below.

This split keeps the userspace lean (no committed third-party content) while ensuring per-keymap `combos.def` changes still get reviewed in this repo.

## Round-trip: materialize `keymap_combo.h` for every keymap

From the **monorepo root** (`mockingb1rdblue-keyboards/`):

```bash
SRC="custom/tooling/gboards/g/keymap_combo.h"
for kmap in \
    qmk_userspace/keyboards/crkbd/keymaps/mockingb1rdblue \
    qmk_userspace/keyboards/reviung/reviung34/keymaps/mockingb1rdblue \
    qmk_userspace/keyboards/keebio/nyquist/keymaps/mockingb1rdblue; do
    cp "$SRC" "$kmap/keymap_combo.h"
done
```

Or for a single keymap, from inside its directory:

```bash
# example: crkbd
cp ../../../../../../custom/tooling/gboards/g/keymap_combo.h ./keymap_combo.h
```

After this, `qmk compile -kb crkbd/rev1 -km mockingb1rdblue` (etc.) will work.

## When CI runs

`qmk_userspace` CI checks out this repo only — it does not have access to the parent monorepo. CI works today because each keymap directory ships its own `keymap_combo.h` *on disk* (currently untracked) once it has been materialized. If the on-disk copy is ever deleted, CI will fail until it's re-materialized.

A pre-build CI step that fetches `qmk-combos/combos:g/keymap_combo.h` and drops it into each keymap directory would let us remove the on-disk copies entirely — that's a future cleanup, not done here.

## When upstream `qmk-combos/combos` changes

The dispatcher header has evolved over time (`COMBO_LENGTH`, `TOGG` support, `uint16_t COMBO_LEN`, etc.). If a future upstream change breaks our `combos.def` syntax or adds features we want:

1. Refresh the vendored copy: see `custom/tooling/README.md` § "Refresh procedure" in the parent monorepo.
2. Re-run the round-trip block above to push the new dispatcher into every keymap.
3. Verify each keymap still compiles: `qmk compile -kb <kb> -km mockingb1rdblue`.
4. Commit the monorepo change (`custom/tooling/...`) and the on-disk refresh in `qmk_userspace` if any keymap-side adjustment was needed (it shouldn't be — `combos.def` is the only keymap-side input).

## Format reference

`combos.def` macros (defined by `keymap_combo.h`):

- `COMB(name, keycode, key1, key2, ...)` — combo of N keys sends `keycode`.
- `SUBS(name, "string", key1, key2, ...)` — combo sends a string via `SEND_STRING`.
- `TOGG(name, layer, key1, key2, ...)` — combo toggles `layer`.

See `keyboards/crkbd/keymaps/mockingb1rdblue/combos.def` for working examples.
