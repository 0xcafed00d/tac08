. $HOME/esp/esp-idf/export.sh

COMPORT=/dev/cu.SLAB_USBtoUART
BINNAME=tac08-go

idf_monitor.py --port $COMPORT build/$BINNAME.elf
