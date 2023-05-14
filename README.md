---------------
-64DOOM README-
---------------


Updated 2023/05/14


-------------------------
-git repository contents-
-------------------------

`README.md` is this file

`src` directory contains all 64Doom C source, header files, menu graphics and Makefile used to build 64Doom

`doc` directory contains many text files including

  `ORIGINAL_README.TXT` is the original `README.TXT` from the 1990s DOOM open source release
  
  `DOOMLIC.TXT` is the DOOM open source license documentation
  
  `CREDITS.TXT` which provides attribution for various components and code contributors
  
  `LICENSE_generic-hashtable.TXT`, `README_generic-hashtable.TXT` required attribution for Hashtable implementation used
  
  `GPLV3.TXT` is a copy of the GPL V3 License as required

--------------
-how to build-
--------------
Setup libdragon: https://github.com/DragonMinded/libdragon/wiki/Installing-libdragon

Get a copy of a supported version of Doom (Doom shareware, retail, Ultimate Doom, Doom II, Plutonia, TNT).

Export two environment variables:

`IWAD_DIRECTORY` -- the path of the directory that contains your IWAD file

`IWAD_FILENAME` -- the actual IWAD filename (one of `DOOM1.WAD`, `DOOM.WAD`, `DOOMU.WAD`, `DOOM2.WAD`, `PLUTONIA.WAD`, `TNT.WAD`)

And run make.

Example for building DOOM2 from repo directory, with IWAD files located in your home directory under `IWADS`:

`cd src`

`env IWAD_DIRECTORY="~/IWADS" IWAD_FILENAME="DOOM2.WAD" make`

If you want to build for a different game version, be sure to run make clean first. Environment variables must be set for make clean as well.

`env IWAD_DIRECTORY="." IWAD_FILENAME="." make clean`

----------
-CONTROLS-
----------

D-PAD UP / ANALOG STICK UP :: move forward

D-PAD DOWN / ANALOG STICK DOWN :: move backward

D-PAD LEFT / ANALOG STICK LEFT :: turn left

D-PAD RIGHT / ANALOG STICK RIGHT :: turn right

L TRIGGER :: strafe left

R TRIGGER :: strafe right

C LEFT :: switch to previous weapon

C RIGHT :: switch to next weapon

C UP :: toggle auto-map

C DOWN :: ENTER key

Z :: toggle run on/off

A :: shoot

B :: use (open doors, flip switches)

START :: ESCAPE key

L + R :: enable/disable debug output and GOD MODE


Enjoy.
