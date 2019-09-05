# Black Magic Probe with FTDI MPSSE capable chips

## Compiling on windows

You can crosscompile blackmagic for windows with mingw or on windows
with cygwin. For compilation, headers for libftdi1 and libusb-1.0 are
needed. For running, libftdi1.dll and libusb-1.0.dll are needed and
the executable must be able to find them. Mingw on cygwin does not provide
a libftdi package yet.

To prepare libusb access to the ftdi device, run zadig https://zadig.akeo.ie/.
Choose WinUSB(libusb-1.0) for the BMP Ftdi device.

Running cygwin/blackmagic in a cygwin console, the program does not react
on ^C. In another console, run "ps ax" to find the WINPID of the process
and then "taskkill /F ?PID (WINPID)".

## Device matching
As other USB dongles already connected to the host PC may use FTDI chips,
cable descriptions are provided to match with the dongle.
To match the dongle, at least USB VID/PID  that must match.
If a description is given, the USB device must provide that string. If a
serial number string is given on the command line, that number must match
with serial number in the USB descriptor of the device.

## Connection possibilities:

### Direct Connection
 MPSSE_SK --> JTAG_TCK
 MPSSE_DO --> JTAG_TDI
 MPSSE_DI <-- JTAG_TDO
 MPSSE_CS <-> JTAG_TMS

+ JTAG and bitbanging SWD is possible
- No level translation, no buffering, no isolation
Example: [Flossjtag](https://randomprojects.org/wiki/Floss-JTAG).

### Resistor SWD
 MPSSE_SK ---> JTAG_TCK
 MPSSE_DO -R-> JTAG_TMS
 MPSSE_DI <--- JTAG_TMS

BMP would allow direct MPSSE_DO ->JTAG_TMS connections as BMP tristates DO
when reading. Resistor defeats contentions anyways. R is typical choosen
in the range of 470R

+ MPSSE SWD possible
- No Jtag, no level translation, no buffering, no isolation

### Direct buffered Connection
 MPSSE_SK -B-> JTAG_TCK
 MPSSE_DO -B-> JTAG_TDI
 MPSSE_DI <-B- JTAG_TDO
 MPSSE_CS -B-> JTAG_TMS

+ Only Jtag, buffered, possible level translation and isolation
- No SWD

Example: [Turtelizer]http://www.ethernut.de/en/hardware/turtelizer/index.html)
[schematics](http://www.ethernut.de/pdf/turtelizer20c-schematic.pdf)

The 'r' command line arguments allows to specify an external SWD
resistor connection added to some existing cable. Jtag is not possible
together with the 'r' argument.

### Many variants possible
As the FTDI has more pins, these pins may be used to control
enables of buffers and multiplexer selection in many variants.

### Reset, Traget voltage readback etc
The additional pins may also control Reset functionality, provide
information if target voltage is applied. etc.

## Cable descriptions
Please help to verify the cable description and give feedback on the
cables already listed and propose other cable. A link to the schematics
is welcome.
