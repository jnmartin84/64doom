#!./bash

########################
# variable definitions #
########################
export ROOTDIR=.
export PWD=$ROOTDIR/pwd
export CAT=$ROOTDIR/cat
export RM=$ROOTDIR/rm
export N64TOOL=$ROOTDIR/n64tool
export CHKSUM64=$ROOTDIR/chksum64

export MIDIPATH_HIGH=$ROOTDIR/ADoom_Ins
export MIDIPATH=$MIDIPATH_HIGH
export MIDIFILE=MIDI_Instruments
export MIDI_ROM_OFFSET=1048576B

export IDPATH=$ROOTDIR/identifiers
export IDFILE=identifier_ULTIMATEDOOM
export ID_ROM_OFFSET_HIGH=5242878B
export ID_ROM_OFFSET=$ID_ROM_OFFSET_HIGH

export WADPATH=$ROOTDIR/wadfiles
export WADFILE=DOOMU.WAD
export WAD_ROM_OFFSET_HIGH=5242894B
export WAD_ROM_OFFSET=$WAD_ROM_OFFSET_HIGH

# WAD_ROM_OFFSET = ID_ROM_OFFSET + 16B

# don't change the following three values
export HEADERNAME=header
export CART_NAME=DOOMN64
export PROG_NAME=doom

################
# begin script #
################

# show menu
echo "64DOOM ROM BUILDER -- jnmartin84"
echo "Enter the number of the ROM to build:"
echo "1) Shareware"
echo "2) Registered"
echo "3) Ultimate"
echo "4) Doom 2"
echo "5) Plutonia"
echo "6) TNT"
# read menu selection from keyboard at terminal
while read -r opt; do
    case $opt in
        1)
           echo "You chose Doom 1 - Shareware.";
           export CART_NAME="DOOMSHAREWARE";
           export IDFILE=identifier_DOOMSHAREWARE
           break
           ;;
        2)
           echo "You chose Doom 1 - Registered.";
           export CART_NAME="DOOM1";
           export IDFILE=identifier_DOOM
           break
           ;;
        3)
           echo "You chose Ultimate Doom.";
           export CART_NAME="ULTIMATEDOOM";
           export IDFILE=identifier_ULTIMATEDOOM
           break
           ;;
        4)
           echo "You chose Doom 2.";
           export CART_NAME="DOOM2";
           export IDFILE=identifier_DOOM2
           break
           ;;
        5)
           echo "You chose Plutonia.";
           export CART_NAME="PLUTONIA";
           export IDFILE=identifier_PLUTONIA
           break
           ;;
        6)
           echo "You chose TNT.";
           export CART_NAME="TNT";
           export IDFILE=identifier_TNT
           break
           ;;
        *)
           echo "invalid option"
           ;;
    esac
done

# set $TRIMMED_WADFILE_NAME to the contents of $IDFILE, used to include correct WAD file
TRIMMED_WADFILE_NAME=$($CAT $IDPATH/$IDFILE)
# remove leading whitespace characters (should not be any, but just in case)
TRIMMED_WADFILE_NAME="${TRIMMED_WADFILE_NAME#"${TRIMMED_WADFILE_NAME%%[![:space:]]*}"}"
# remove trailing whitespace characters (these exist due to string padding in the identifier file)
TRIMMED_WADFILE_NAME="${TRIMMED_WADFILE_NAME%"${TRIMMED_WADFILE_NAME##*[![:space:]]}"}"

# set $WADFILE to value from $IDFILE corresponding to menu choice
export WADFILE=$TRIMMED_WADFILE_NAME
export CWD=$($PWD)
# this is the path of the Z64 ROM file generated below
export FULL_PATH_TO_Z64_FILE="$CWD/$CART_NAME.Z64"

# show some information about the files being included in the ROM
echo "Using IDENTIFIER FILE $IDPATH/$IDFILE"
echo $($CAT $IDPATH/$IDFILE)
echo "Using WADFILE $WADPATH/$WADFILE"
echo ""

# generate N64TOOL command line with corresponding arguments
export COMMAND_N64TOOL="$N64TOOL -l 32M -t \"$CART_NAME\" -h $HEADERNAME -o $CART_NAME.Z64 $PROG_NAME.bin -s $MIDI_ROM_OFFSET $MIDIPATH/$MIDIFILE -s $ID_ROM_OFFSET $IDPATH/$IDFILE -s $WAD_ROM_OFFSET $WADPATH/$WADFILE"
echo "Executing: $COMMAND_N64TOOL"

# execute N64TOOL
$COMMAND_N64TOOL

# result of running N64TOOL
RESULT_N64TOOL=$?

# N64TOOL succeeeded
if [ $RESULT_N64TOOL -eq 0 ]; then
  # generate CHKSUM64 command line
  export COMMAND_CHKSUM64="$CHKSUM64 $CART_NAME.Z64"
  echo "Executing: $COMMAND_CHKSUM64"
  # execute CHKSUM64
  $COMMAND_CHKSUM64

  # result of running CHKSUM64
  RESULT_CHKSUM64=$?

  # CHKSUM64 succeeded
  if [ $RESULT_CHKSUM64 -eq 0 ]; then

    echo "Successfully built ROM; the full path to the new ROM is:"
    echo $FULL_PATH_TO_Z64_FILE
    echo ""
    echo "Enjoy."

  # CHKSUM64 failed
  else

    echo "^                                          "
    echo "|                                          "
    echo "|                                          "
    echo "---- !!!     POSSIBLE ERROR MESSAGE     !!!"
    echo "     !!!          DO NOT IGNORE         !!!"
    echo "     !!! SEE BELOW OUTPUT FOR MORE INFO !!!"
    echo ""

    # CHKSUM64 failed and Z64 file exists, remove it
    if [ -e $FULL_PATH_TO_Z64_FILE ]; then

      echo "CHKSUM64 failed to execute properly."
      echo "Removing $FULL_PATH_TO_Z64_FILE from filesystem."
      $RM $FULL_PATH_TO_Z64_FILE
      echo "Check for other on-screen error messages and try again."

    # CHKSUM64 failed but no Z64 file exists, do nothing
    else

      echo "CHKSUM64 failed to execute properly."
      echo "Check for other on-screen error messages and try again."

    fi

  fi

# N64TOOL failed
else

  echo "^                                          "
  echo "|                                          "
  echo "|                                          "
  echo "---- !!!     POSSIBLE ERROR MESSAGE     !!!"
  echo "     !!!          DO NOT IGNORE         !!!"
  echo "     !!! SEE BELOW OUTPUT FOR MORE INFO !!!"
  echo ""

  # N64TOOL failed and Z64 file exists, remove it
  if [ -e $FULL_PATH_TO_Z64_FILE ]; then

    echo "N64TOOL failed to execute properly and left behind an unusable Z64 file."
    echo "Removing $FULL_PATH_TO_Z64_FILE from filesystem."
    $RM $FULL_PATH_TO_Z64_FILE
    echo "Check for other on-screen error messages and try again."
  # N64TOOL failed but no Z64 file exists, do nothing
  else

    echo "N64TOOL failed to execute properly."
    echo "Check for other on-screen error messages and try again."

  fi

fi

##############
# end script #
##############
