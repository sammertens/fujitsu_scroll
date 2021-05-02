/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Fujitsu Scroll device PS/2 mouse driver
 *
 * Scroll Wheel packets
 *  Bytes 0,1,2:  1 0 c c c c c c  0 0 0 0 a a a a  a a a a a a a a
 *  Bytes 3,4,5:  1 1 0 0 0 0 0 0  0 0 0 p 0 0 0 0  0 0 0 0 0 0 0 0
 *
 *  c - 6 bits, capacitance, how much capacitance is detected.
 *              Seems to be highest around a ring inside the wheel
 *              circle, decreasing the farther the finger is from
 *              that ring
 *  a - 12 bits, angle, the angle of the wheel where touch is sensed
 *               the upper 4 bits are 'f' when the touch is at the
 *               top of the wheel (12 o'clock), and loops back to 0.
 *               Increased in a clockwise direction.
 *  p - 1 bit,  pressed, is the wheel being touched in the center.
 *
 *  Multitouch is not supported.  If one or more fingers are added to
 *  a first one that's touching, the reported angle will stay the same
 *  but the capacitance and center press may change.
 *
 *
 * Scroll Sensor packets
 *  Same as scroll wheel, except the 'a' angle is now position, with
 *  lower values to the right and higher values to the left, and 'p'
 *  now indicates whether a spot on the bezel about 0.5cm left of the
 *  sensor is being touched.  There is no indication on the bezel where
 *  this spot is.  Unlike the wheel, it is very possible for 'p' to be 1
 *  and both 'c' and 'a' to be 0. (Although they may not be if both this
 *  spot and the sensor are being touched simultaneously.)
 */

#ifndef _FUJITSU_SCROLL_H
#define _FUJITSU_SCROLL_H

/* TODO: This will have to be moved to a kernel config option */
#ifndef CONFIG_MOUSE_PS2_FUJITSU_SCROLL

#define CONFIG_MOUSE_PS2_FUJITSU_SCROLL

#endif

#define FUJITSU_SCROLL_RANGE        0x01000
#define FUJITSU_SCROLL_MAX_POSITION (FUJITSU_SCROLL_RANGE - 1)

#define FUJITSU_SCROLL_ID           0x43
#define FUJITSU_SCROLL_WHEEL_ID     04
#define FUJITSU_SCROLL_SENSOR_ID    00

/*
 * FJS_LOG_PACKETS - enable log all incoming packets
 */
#define FJS_LOG_PACKETS                0

/*
 * FJS_LOG_GETINFO - enable to log the GETINFO data on init
 */
#define FJS_LOG_GETINFO               0

/*
 * FJS_SEND_EVENTS - disable to not actually generate events
 */
#define FJS_SEND_EVENTS                1

/*
 * The minimum weight to register an actual finger touch
 */
#define FJS_WEIGHT_THRESHOLD           0x08

#define FJS_WHEEL_AXIS                 REL_WHEEL
#define FJS_WHEEL_PRESS                BTN_EXTRA

#define FJS_SENSOR_AXIS                REL_HWHEEL
#define FJS_SENSOR_PRESS               BTN_EXTRA

/*
 * How much movement should occur to consider it an event
 */
#define FJS_POSITION_CHANGE_THRESHOLD  0x100

/*
 * How much the movement value should be bitshifted right
 */
#define FJS_MOVEMENT_BITSHIFT          6

/*
 * FJS_MODE_ENABLE - This bit must be set in the 'mode' byte in order
 * for packets to be generated
 */
#define FJS_MODE_ENABLE            0x80

/*
 * FJS_MODE_PRESS_ONLY - This bit, if set, will suppress all weight and
 * position data.  Only the 'press' bit will be reported.
 */
#define FJS_MODE_PRESS_ONLY        0x20

/*
 * FJS_INIT_MODE - the mode byte to send upon initialization
 */
#define FJS_INIT_MODE              (FJS_MODE_ENABLE)


#define MAX_POSITION_CHANGE  (FUJITSU_SCROLL_MAX_POSITION / 2)

enum fujitsu_scroll_device_type {
  FUJITSU_SCROLL_WHEEL,
  FUJITSU_SCROLL_SENSOR
};

struct fujitsu_scroll_data {
  enum fujitsu_scroll_device_type type;
  unsigned int last_event_position;
  unsigned int finger_down:1;
  bool pressed;
  unsigned int axis;
  unsigned int button;
};

void fujitsu_scroll_module_init(void);
int fujitsu_scroll_detect(struct psmouse *psmouse, bool set_properties);
int fujitsu_scroll_init(struct psmouse *psmouse);
void fujitsu_scroll_reset(struct psmouse *psmouse);

#endif /* _FUJITSU_SCROLL_H */
