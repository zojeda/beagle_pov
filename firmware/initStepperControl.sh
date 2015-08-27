#!/bin/bash
echo BB-BEAGLEPOV | sudo tee /sys/devices/bone_capemgr.*/slots
echo 49 | sudo tee /sys/class/gpio/export
echo "out" | sudo tee /sys/class/gpio/gpio49/direction
