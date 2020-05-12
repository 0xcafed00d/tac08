. $HOME/esp/esp-idf/export.sh

COMPORT=/dev/cu.SLAB_USBtoUART

idf.py -p $COMPORT --baud 921600 flash
