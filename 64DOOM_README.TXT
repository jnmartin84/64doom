---------------
-64DOOM README-
---------------

At long last, the full source + ROM building toolkit release of 64Doom, the best DOOM source port for the Nintendo 64, ever.

This latest release adds 640x480 high resolution mode.

High detail renders the game screen at 640x400 without status bar, scaling down from there. Until you get to a +3 border, it is still higher res than normal 320x200 high detail Doom.
It is not fast but is playable.

Low detail renders the game screen at 320x400 without status bar, scaling down from there.
It is smooth and nicely playable on all screen sizes.

Audio mixing is entirely fixed point now, avoiding floating point instructions entirely (on the N64, they share the integer hardware and destroy performance).

The bug where music stopped playing correctly after a level or two is fixed now, solid now.

Save game logic is better now too, no more weirdness about not being able to save or load more than once per boot.

Optimizations in rendering code, mostly to draw doubled pixels as full words where possible.

That's it.

-------------------------
-SVN repository contents-
-------------------------

"64DOOM_README.TXT" is this file

"64DOOM_BUILDER.ZIP" is a zip of the full 64DOOM ROM builder toolkit including DOOM.BIN built from the latest source

"tools" directory contains the contents of the "MAKE YOUR OWN 64DOOM ROM" toolkit, used to turn that .bin file
  (and user-provided .WAD files) into a working .Z64 file for use with 64Drive/Everdrive64 or MESS (emulators, yuck)

"n64memory" directory contains C source, header file and Makefile used to build libn64memory required to build 64Doom

"libdragon" directory contains C source, header file and Makefile used to build customized libdragon required to build 64Doom

"src" directory contains all 64Doom C source, header files and Makefile used to build a .bin file that can be turned
  into a working 64Doom Z64 ROM file

"doc" directory contains many text files including
  ORIGINAL_README.TXT is the original README.TXT from the 1990s DOOM open source release
  DOOMLIC.TXT is the DOOM open source license documentation
  CREDITS.TXT which provides attribution for various components and code contributors
  LICENSE_generic-hashtable.TXT, README_generic-hashtable.TXT required attribution for Hashtable implementation used
  GPLV3.TXT is a copy of the GPL V3 License as required


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
L + Z :: press 5 times in a row to skip to next level
R + Z :: increase gamma correction


Enjoy.

- Jason (jnmartin84), 2020/03/28
