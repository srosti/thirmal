#!/bin/bash
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash -z 0x1000  esp32-idf3-20191220-v1.12.bin

