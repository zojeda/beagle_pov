#!/usr/bin/python
""" blinkled.py - test script for the PyPRUSS library
It blinks the user leds ten times
"""

import sys
import pypruss

steps = int(1)
delay = int(600000)

pypruss.modprobe()                                  # This only has to be called once pr boot
pypruss.init()                                      # Init the PRU
pypruss.open(0)                                     # Open PRU event 0 which is PRU0_ARM_INTERRUPT
pypruss.pruintc_init()                              # Init the interrupt controller
pypruss.pru_write_memory(0, 0, [delay])                # Load data in the pru RAM
pypruss.pru_write_memory(0, 4, [steps])                # Load data in the pru RAM
pypruss.exec_program(0, "./bin/stepperTest.bin")      # Load firmware "blinkled.bin" on PRU 0
pypruss.wait_for_event(0)                           # Wait for event 0 which is connected to PRU0_ARM_INTERRUPT
pypruss.clear_event(0)                              # Clear the event
pypruss.pru_disable(0)                              # Disable PRU 0, this is already done by the firmware
pypruss.exit()                                      # Exit, don't know what this does.
