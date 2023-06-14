/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Fujitsu Scroll device PS/2 mouse driver
 *
 */

#ifndef _FUJITSU_SCROLL_H
#define _FUJITSU_SCROLL_H

#ifdef CONFIG_MOUSE_PS2_FUJITSU_SCROLL

#define FJS_ALLOW_WHITELIST_ONLY 0

#define FJS_RANGE        0x01000

/*
 * The maximum possible range that can be reported
 */
#define FJS_MAX_POS (FJS_RANGE - 1)

#define FJS_PACKET_SIZE             6

#define FUJITSU_SCROLL_ID           0x43
#define FUJITSU_SCROLL_WHEEL_ID     04
#define FUJITSU_SCROLL_SENSOR_ID    00

/*
 * The minimum weight to register an actual finger touch.
 * Capacitance can range up to 6 bits (0x3F)
 */
#define FJS_CAPACITANCE_THRESHOLD           0x09

#define FJS_WHEEL_AXIS                 REL_WHEEL
#define FJS_SENSOR_AXIS                REL_HWHEEL

/*
 * How much movement should occur to consider it an event
 * Movement is measured as a change in angle, which is 12 bits.
 */
#define FJS_POSITION_CHANGE_THRESHOLD  0x04

/*
 * How much the movement value should be bitshifted right
 */
#define FJS_MOVEMENT_BITSHIFT          3

/*
 * FJS_INIT_MODE - the mode byte to send to enable data packets
 */
#define FJS_INIT_MODE              0x80

#define FJS_MAX_POS_CHG  (FJS_MAX_POS / 2)

enum fujitsu_scroll_device_type {
  FUJITSU_SCROLL_WHEEL,
  FUJITSU_SCROLL_SENSOR
};

struct fujitsu_scroll_data {
  enum fujitsu_scroll_device_type type;
  unsigned int axis;
  unsigned int last_event_position;
  unsigned int finger_down:1;
};

void fujitsu_scroll_module_init(void);
int fujitsu_scroll_detect(struct psmouse *psmouse, bool set_properties);
int fujitsu_scroll_init(struct psmouse *psmouse);

#endif /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */

#endif /* _FUJITSU_SCROLL_H */
