.PHONY: flash
flash:
	cd controller && \
	idf.py -p /dev/ttyACM0 flash monitor

.PHONY: erase
erase:
	cd controller && \
	idf.py -p /dev/ttyACM0 erase-flash

isp-idf-export:
	. ${IDF_PATH}/export.sh