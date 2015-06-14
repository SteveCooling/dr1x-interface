dr1x-interface
==============

Microcontroller interface to control operating modes of the Yaesu DR-1X Repeater.

Author LA1FTA Morten <la1fta@la3m.no>. Copyright (c) 2015. Please see LICENSE file.

Purpose
-------

Allows remote control of the Yaesu DR-1X Repeater.
PTT and operating mode can be controlled remotely via a USB virtual serial port.
The squelch input can be sensed using the same serial line.

The code samples the SQL pin and sends information about state changes to the computer.
It also sequences outputs for the EXT I/O, MODE, and PTT pins when commanded to.

![Interface test circuit](https://raw.githubusercontent.com/SteveCooling/dr1x-interface/master/testcircuit.jpg)

Line protocol
-------------

### Commands

- MODE (AUTOAUTO,AUTOFM,AUTOAUTO)
  Set standby operating mode of the repeater (while not keyed from this interface)

- PTT (0,1)
  Externally control the repeater's transmitter
  Setting the PTT will sequentially set EXT I/O, MODE FM/FM and then PTT.
  Resetting PTT will reverse the sequence.

- DEBUG (0,1)
  Output debug output on the serial line along with the other signals.

### Input signals

- SQL (1,0)
  The interface sends "SQL 1" if the squelch input pin is pulled low, and "SQL 0" when released.

Compatibility
-------------

Code is tested on Teensy 3.1 and Arduino Pro Micro (Leonardo), but should work on anything you can program from the Arduino IDE that appears as a serial port to the connected computer.

This interface can be used to link several Yaesu DR-1X repeaters together, and to provide Echolink connectivity to one or more such repeaters. To do this you need to use my patched version of TheLinkBox from here: https://github.com/SteveCooling/thelinkbox
