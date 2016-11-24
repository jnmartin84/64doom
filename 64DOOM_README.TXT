---------------
-64DOOM README-
---------------

At long last, the full source + ROM building toolkit release of 64Doom, the best DOOM source port for the Nintendo 64, ever.


-------------------------
-SVN repository contents-
-------------------------

"64DOOM_README.TXT" is this file

"64DOOM_BUILDER.ZIP" is a zip of the full 64DOOM ROM builder toolkit including DOOM.BIN built from the latest source

"tools" directory contains the contents of the "MAKE YOUR OWN 64DOOM ROM" toolkit, used to turn that .bin file
  (and user-provided .WAD files) into a working .Z64 file for use with 64Drive/Everdrive64 or MESS (emulators, yuck)

"n64memory" directory contains C source, header file and Makefile used to build libn64memory required to build 64Doom

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

- Jason (jnmartin84), 2016/11/23