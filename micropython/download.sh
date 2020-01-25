#!/bin/bash
esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash --flash_size=detect 0 esp32-idf3-20191220-v1.12.bin 

