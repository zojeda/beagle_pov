#!/usr/bin/python
""" blinkled.py - test script for the PyPRUSS library
It blinks the user leds ten times
"""

import sys
import pypruss

delay = int(600000)

pypruss.modprobe()                                  # This only has to be called once pr boot
pypruss.init()                                      # Init the PRU
pypruss.open(0)                                     # Open PRU event 0 which is PRU0_ARM_INTERRUPT
pypruss.pruintc_init()                              # Init the interrupt controller
pypruss.pru_write_memory(0, 0, [delay])                # Load data in the pru RAM
pypruss.exec_program(0, "./bin/beagle_pov.bin")      # Load firmware "beagle_pov.bin" on PRU 0
pypruss.exit()                                      # Exit, don't know what this does.
