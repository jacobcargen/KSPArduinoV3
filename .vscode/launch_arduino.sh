#!/bin/bash
echo "Starting Arduino monitor..."
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200

