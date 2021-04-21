# Fujitsu_Scroll

## Project Goal
The goal of this project is to develop Linux kernel device drivers for the
scroll devices found in Fujitsu Lifebook T901 laptops.

## History
I was looking for a cheap laptop to run Linux on, and picked up a Fujitsu
Lifebook T901 on EBay for real cheap.  I wasn't expecting a machine with
rotating screen or stylus support, but the price was right.  I started poking
around all the various unexpected features that the laptop had, including
these odd scrolling areas that didn't seem to do anything in Linux.  The sordid
details can be found [here](https://www.linuxquestions.org/questions/slackware-14/fujitsu-lifebook-t901-convertible-laptop-tablet-features-missing-on-default-install-4175637572/page2.html#post6235699) and [here on linuxquestions.org](https://www.linuxquestions.org/questions/linux-hardware-18/fujitsu-lifebook-scroll-device-support-4175692959/#post6236179), where I found one or two other folk
trying to get the most out of the same hardware.

## The Hardware (human interface)
The T901 sports two features that up until now have not had any Linux support.

### The Scroll Wheel
This is a round indentation on the palmrest to the right of the (well
supported) Synaptics touchpad.  Fujitsu documentation says that the user can
trace circles in this indentation to emulate a scroll wheel.

### The Scroll Sensor
This is a linear region on the lower left portion of the screen bezel,
labeled 'Scroll Sensor' with raised bumps.  Similar to the wheel, the user
can slide their finger back and forth across this area to scroll up or down.
In fact, it behaves very much like a second Scroll Wheel that was straightened
out.

In photos I've seen, it appears that the Fujitsu Lifebook T900 may have a
Scroll Sensor, but it does not have a Scroll Wheel.  I do not have access to
a T900 to confirm.  Also, the Fujitsu Lifebook S762 has a scroll wheel but it
was implemented completely differently; at one point a kernel patch was
developed for it but that patch went nowhere.  It may very well be that the
T901 is the only laptop to have both devices.


## The Hardware (System Interface)
Investigation of the sole T901 at my disposal showed that in addition to the
Synaptics touchpad, there are two other mystery devices attached to the
legacy i8042 interface, which the existing psmouse driver identifies as
generic PS/2 mice.  With a stock Linux kernel, they will respond when commands
are written to but will never generate event data on their own.

Eventually, it was determined that the i8042 operates properly in multiplex
mode.  serio1 is not attached to any device.  serio2 is the Scroll Wheel,
serio3 is the Scroll Sensor, serio4 is the Synaptics touchpad.

### OEM Support
None.  Fujitsu provides closed source drivers for Windows XP, 7, and 8.  I
asked for technical documentation and was very promptly and courteously
rebuffed.  I'm sure complete documentation is buried in a filing cabinet
somewhere but Fujitsu Customer Support is not set up to provide it, nor
inclined to go dig it up.  The Synaptics touchpad, which is the main form
of pointer support, is handled very well by the existing Linux Synaptics
driver, which was apparently developed using API information provided by
Synaptics.

### The protocol
Experimentation has shown that data output must be enabled on each scroll
device by sending a 'mode' byte via a sliced command (see PS/2 protocol
documentation for details on sliced commands), followed by a SET RATE command
with a parameter of 20.  This very loosely follows the Synaptics extensions
to the PS/2 mouse protocol.  (Synaptics also has a mode which can be set in
this way, but the individual bits seem to have different meanings.)

Only two bits of the mode byte have been found to be significant.

* BIT 7 (0x80) - set 1 to enable data packets, 0 to disable
* BIT 5 (0x20) - set 1 to inhibit 'position' and 'capacitance' data

It's not obvious that any of the other bits make any difference.

Data packets are written as 6 bytes, again loosely mirroring the Synaptics
6 byte protocol and they will be accepted by code doing casual framing
validation of such.  Trial and error has produced the following definition:

* Byte 0:
  *   bits 7-6: mask 0xc0 - Always set to '10' for framing
  *   bits 5-0: mask 0x3f - Capacitance value; how much finger is being sensed
* Byte 1:
  *   bits 7-4: mask 0xf0 - Always observed to be 0
  *   bits 3-0: mask 0x0f - the top 4 bits of the position value
* Byte 2:
  *   bits 7-0: mask 0xff - the lower 8 bits of the position value
* Byte 3:
  *   bits 7-6: mask 0xc0 - Always set to '11' for framing
  *   bits 5-0: mask 0x3f - Always observed to be '000000'
* Byte 4:
  *   bits 7-5: mask 0xe0 - Always observed to be '000'
  *   bit    4: mask 0x10 - Set if a specific region on the sensor is touched
  *   bits 3-0: mask 0x0f - Always observed to be '0000'
* Byte 5:
  *   bits 7-0: mask 0xff - Always observed to be '00000000'

It's certainly possible some of those 0 fields could hold meaning if the
sensor devices are put into some currently unknown configuration.  Or they
might indicate hardware faults that simply have never been encountered.
There is simply zero documentation.

**Capacitance**: how much finger is being sensed.  This value indicates whether
a sensor is being touched or not.  A few times I've seen a sensor get stuck
reporting a very low, but not 0, value even after my finger has been removed,
so it's probably best to apply a lower bounds filter just in case.  I have
not found any immediately intuitive ways that the end user can adjust this
value as a means of providing more input (say, light touch versus heavy
touch).

**Position**: This value ranges from 0x000 to 0xfff.  On the Scroll Wheel, it
indicates the angle of the circle that the Wheel is being touched, with 0x000
being at the top (12 o'clock), and increasing clockwise to 0xfff (just shy of
12 o'clock).  On the Scroll Sensor, 0x000 is at the right end of the Sensor,
and 0xfff is at the left end.  In practice, the lower 8 bits are probably
too noisy to be of much use and the upper 4 bits will suffice, which divides
each sensor into 16 parts.

**Byte 4, bit 4**: This was unexpected.  Each sensor has a region which, while
touched, will cause this bit to be set.  If bit 5 in the mode byte is set,
this is the only bit which will change/generate data packets.  For the
Scroll Wheel, the sensitive region is dead center.  In practice, I'm not
quite convinced it can be triggered reliably enough to be useful.  For the
Scroll Sensor, the region is about a quarter inch to the left of the actual
sensor; there is no indication on the bezel that this is an area of interest
in any way - the user simply has to know they can touch there.  I've not seen
anything in the Fujitsu documentation to suggest that either region is actually
used (but I haven't scoured all that extensively either).  The most logical
thing to map it to would be the middle mouse button (since the touchpad only
has two buttons), but again, usability is questionable.

With the Scroll Wheel, capacitance is least sensitive in the center of
the circle, then it increases as the finger moves outward, and then decreases
again as the outer circumference is reached.  If it were drawn as a map, it'd
look like a donut.  Because of this, we can't really use capacitance and
position as polar coordinates to get an absolute finger location outside of
what byte 4/bit 4 provides.

If a finger is touching a sensor, and another finger is added to also touch
the sensor, capacitance will go up but position will remain unchanged.
That's the closest we get to multifinger sensing and it doesn't seem very
useful in that regard.

## Current state / TODO

1. Small tweaks may still be needed for the Input subsystem.  It currently
seems usable.  The Wheel does vertical scrolls, the Sensor does
horizontal scrolls, and they both send the 'EXTRA' mouse button, which very
few applications seem to care about.  (Accidental presses of the 'MIDDLE'
button were problematic.)  All of this, plus scrolling sensitivity, can
be adjusted in fujitsu_scroll.h.

2. Figure out why psmouse frequently decides to reset one or the other device,
usually in response to a large number of incoming data packets, and if
possible minimize or eliminate the issue.  This actually doesn't seem to
impact the user experience as much as one might think, and it's possible the
original designers decided it was acceptable enough to ship.

3. General code cleanup, remove the debug parts.  Most of the debug code is
controlled with #define flags, which keeps the compiled code lean but makes
the source code busier to read than necessary.

4. Palm detection on the Scroll Wheel?

Some day:

X. Get the driver in a state that it can be submitted to the kernel for
inclusion.  In the meantime, the good news is that i8042 and PS/2 support is
very much legacy with very little development going on, so it should be
possible to drop this code into new kernel versions with little/no changes.
(Last tested kernel version: 5.12-rc7.)

Y. Consider an out-of-kernel module.  This seems much more feasible (I
imagine kernel maintainers would want more than one or two T901 owners to
affirm that this code works), and would allow an easy way to provide parameters
and different behavior modes.  (For example, the Wheel could be a D-Pad.)

