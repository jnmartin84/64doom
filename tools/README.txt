BUILD YOUR OWN 64DOOM --- Updated 2020/04/11
--------------------------------------------------


In order to build a working 64DOOM Z64 file, you 
will need at least one WAD file from the various
versions of Doom. Make sure the WAD file is named 
as described in the following section, and located 
under the "wadfiles" sub-directory under the "tools" 
directory in the copy of the 64Doom Subversion 
repository that you cloned.


The supported game types (and their associated, 
required WAD filenames) are as follows:
--------------------------------------------------
Doom Shareware  -- DOOM1.WAD
Doom Registered -- DOOM.WAD
Ultimate Doom   -- DOOMU.WAD
Doom 2          -- DOOM2.WAD
Plutonia        -- PLUTONIA.WAD
TNT             -- TNT.WAD


then just run "make_doom.sh" as follows:
1) From cygwin, bash make_doom.sh
2) From Windows, double click MAKE_DOOM.BAT
3) From Linux, build or find copies of N64TOOL
    and CHKSUM64 and then try 1)


You will be prompted to enter a number from 1 to 6
(the script prints a listing of games with them);
input a number and press the ENTER key.

If everything is successful, you will end up with
a Z64 file named after the game you picked in the
directory you ran the script from.

If there was a failure, an error message will be 
printed on screen with hints on what to do. 
In the event that a Z64 file was generated before
the failure occurred, it will be removed.

Enjoy.

- Jason (jnmartin84)
