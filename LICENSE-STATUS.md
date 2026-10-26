# License status

The **original project code** in this repository (the game host, ROM
decryption tool, packaging scripts and documentation) is licensed under the
**PolyForm Noncommercial License 1.0.0**. The project owner confirmed this
variant on 2026-10-26. The full license text is in `LICENSE`.

Required Notice: Copyright (c) 2026 the Metal Slug 3 Recompilation authors.

What this license does **not** cover
------------------------------------
* `neogeorecomp/` is the upstream toolkit by sp00nznet, MIT licensed. Its
  `LICENSE`, `NOTICE` and `LICENSES/` directory are preserved. Do not
  relicense those files under PolyForm.
* Musashi (MIT), superzazu/z80 (MIT) and ymfm (BSD-3-Clause) are vendored
  under `neogeorecomp/ext/`; their notices are preserved.
* `tools/prot_cmc.cpp`, `tools/prot_sma.cpp` and the tables generated from
  them are derived from MAME and are BSD-3-Clause (see the file headers).
* SDL2 is zlib licensed; the Windows DLL and the AppImage bundle it under
  that license.
* The game ROMs, the Neo Geo BIOS, and anything generated from them
  (recompiled C, decrypted ROMs, entry lists) are **not** part of this
  project's licensed source and are not distributed. Users supply their own
  legally obtained dumps.
* Game screenshots and captures contain SNK's copyrighted imagery and are
  **not** distributed in this repository or its release assets. The local
  test run generated them under `docs/screenshots/`, which is git-ignored.

PolyForm Noncommercial is not a free/open-source license and does not permit
commercial redistribution. Do not assume otherwise. If you need commercial
terms, contact the project owner.

See `THIRD_PARTY_NOTICES.md` for the complete attribution list.
