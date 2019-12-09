#!/bin/bash

#NOTE: u might want to wait a bit before running this script..
# 'right after' upload system reports that the port is not ready yet!
# so, we don't support here.. 'upload-n-monitor.sh' option.

pio device monitor -p /dev/cu.usbmodem32231601 -b 9600
