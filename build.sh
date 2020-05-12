. $HOME/esp/esp-idf/export.sh

# Move src to main to conform to ESP-IDF expectations
# mv src main

idf.py build

# mv main src