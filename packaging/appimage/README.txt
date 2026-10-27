Metal Slug 3 - native static recompilation
=========================================

This AppImage contains the recompiled game, the SDL2 window/audio layer and
the build-time ROM decrypt tool. It does not contain game ROMs or the Neo Geo
BIOS: supply your own legally obtained dump.

Quick start
-----------
1. Put the decrypted runtime ROM files in a folder named "roms" next to the
   AppImage (or in ~/.local/share/mslug3/roms, ~/Emulation/roms/mslug3,
   ~/roms/mslug3, or ~/roms). The AppRun script finds those automatically.

   To prepare them from the MAME "mslug3" and "neogeo" sets:

     mkdir -p roms
     unzip mslug3.zip -d roms
     unzip neogeo.zip -d roms
     ./usr/bin/mslug3_decrypt --rom-path roms --out roms

   (Run that from the extracted AppDir, or pass --appimage-extract first.)
   mslug3_decrypt reads the encrypted NEO-SMA program, the NEO-CMC42
   graphics and the 256-m1/256-v1..v4 files, and writes the decrypted,
   runtime-ready set. It never writes outside the output directory.

2. Run the AppImage. It starts in fullscreen with a 4:3 pillarboxed image.

Controls
--------
  Arrows / left stick    move
  Z X C V or pad A B X Y Neo Geo A B C D
  5 or pad Back          insert coin
  1 or pad Start         start
  3 or pad left shoulder select
  9                      service
  F2                     test switch
  F11                    toggle fullscreen
  Esc                    quit

Notes
-----
* The MVS BIOS is the real sp-s2.sp1 system ROM; there is no BIOS HLE.
* The game's 68000 code is statically recompiled. Code reached only through
  swapped bank windows runs in the bundled Musashi interpreter; the measured
  share is reported in the project README.
* Audio: the YM2610/M1 path is corrected for the M1 bank mask. The final
  WAV is continuous from attract through character select and Mission 1
  (peak 29,938 over 4,000 frames); subjective mix/volume was not auditioned.
