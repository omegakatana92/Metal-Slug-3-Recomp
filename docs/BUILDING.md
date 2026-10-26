# Building

## Prerequisites

| Tool | Windows | Linux |
|---|---|---|
| Compiler | Visual Studio 2022/2026, Desktop C++ | gcc/g++ 11+ |
| Build system | CMake 3.21+ | CMake 3.21+, make |
| SDL2 | SDL2 2.x MSVC development package (2.30.9 used) | `libsdl2-dev` |
| Other | Git (only if you use a submodule checkout) | `strip`, `wget` for the AppImage |

The toolkit source is vendored in `neogeorecomp/`, including Musashi,
superzazu/z80 and ymfm. No network access is needed to build.

## ROM files

The build reads the **encrypted MAME `mslug3` set** plus the MVS `neogeo`
files from `MSLUG3_ROM_DIR` (one flat directory). It writes the decrypted,
runtime-ready set to `build-*/roms`.

Expected files:

```
mslug3 set:  256-pg1.p1 256-pg2.p2 green.neo-sma
             256-c1.c1 .. 256-c8.c8
             256-m1.m1
             256-v1.v1 .. 256-v4.v4
neogeo set:  sp-s2.sp1 sfix.sfix sm1.sm1 000-lo.lo
```

The project never downloads ROMs and never modifies the originals.

## Windows x64 (MSVC)

From the project root:

```powershell
$sdl = "C:\path\to\SDL2-2.30.9\cmake"
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 `
      -DMSLUG3_ROM_DIR="C:\path\to\roms-src" `
      -DSDL2_DIR="$sdl"
cmake --build build-windows --config Release --parallel 8
```

The result is `build-windows\Release\Metal_Slug_3.exe`, with `SDL2.dll`
copied beside it.

Notes:

* The generated dispatch functions are enormous. The project compiles them
  with `/bigobj;/O1`; `/O2` makes MSVC's optimizer take tens of minutes per
  file on the 15 MB generated sources. `/O1` still runs the 68000 code at
  full speed.
* Make sure `SDL2_DIR` points at an **MSVC** SDL2. A MinGW/MSYS SDL2 on
  `PATH` will make MSVC include MinGW headers and fail with `stdio.h`
  syntax errors.
* A headless-only build works without SDL2: omit `-DSDL2_DIR` and do not
  let `find_package` find a MinGW copy. Headless mode still runs the game
  and `--screenshot`/`--wav` work.

## Linux x86_64

```bash
sudo apt install build-essential cmake libsdl2-dev
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
      -DMSLUG3_ROM_DIR="$PWD/roms-src"
cmake --build build-linux --parallel "$(nproc)"
```

Run:

```bash
./build-linux/Metal_Slug_3 --rom-path build-linux/roms
```

The binary links SDL2 from the system; the AppImage bundles it.

## Steam Deck AppImage

```bash
bash packaging/appimage/build-appimage.sh "$PWD/roms-src"
```

This configures/builds `build-linux`, assembles an AppDir, copies the
binary, `mslug3_decrypt`, `libSDL2-2.0.so.0`, the desktop entry and the icon,
then runs `appimagetool` (downloaded on first use, `--appimage-extract-and-run`
so FUSE is not required).

Output:

```
dist/steamdeck/Metal_Slug_3-SteamDeck-x86_64.AppImage
```

The AppRun script:

* adds `usr/lib` to `LD_LIBRARY_PATH`;
* looks for `roms` beside the AppImage and in `~/Emulation/roms/mslug3`,
  `~/roms/mslug3`, `~/roms`;
* adds `--fullscreen` unless `--headless`, `--scale` or `--fullscreen` was
  already given.

To test without a physical Deck:

```bash
./dist/steamdeck/Metal_Slug_3-SteamDeck-x86_64.AppImage \
    --appimage-extract-and-run --headless --frames 700 \
    --rom-path build-linux/roms
```

## WSL2

The Linux build and AppImage were produced in Ubuntu 22.04 under WSL2:

```powershell
wsl -d Ubuntu-22.04
cd /mnt/c/.../metal\ slug\ 3
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
      -DMSLUG3_ROM_DIR="$PWD/roms-src"
cmake --build build-linux --parallel 8
```

WSLg can run the windowed/fullscreen binary if a display is available.
