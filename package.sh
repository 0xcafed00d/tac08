#!/bin/bash
. $HOME/esp/esp-idf/export.sh

BINNAME=tac08
SIZE=1048576

idf.py build
release=`date +%Y%m%d`;
../odroid-go-multi-firmware/tools/mkfw/mkfw "$BINNAME ($release)" tile.raw 0 16 $SIZE app build/$BINNAME.bin 
mv firmware.fw "${BINNAME}_${release}.fw"
