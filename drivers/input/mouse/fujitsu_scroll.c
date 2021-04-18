// SPDX-License-Identifier: GPL-2.0-only
/*
 * Fujitsu Scroll Devices PS/2 mouse driver
 *
 *   2021 Sam Mertens <smertens@gmail.com>
 *     Modified the original synaptics.c source to support the Fujitsu
 *     scroll devices in the Fujitsu Lifebook T901 laptop
 *
 *   2003 Dmitry Torokhov <dtor@mail.ru>
 *     Added support for pass-through port. Special thanks to Peter Berg Larsen
 *     for explaining various Synaptics quirks.
 *
 *   2003 Peter Osterlund <petero2@telia.com>
 *     Ported to 2.5 input device infrastructure.
 *
 *   Copyright (C) 2001 Stefan Gmeiner <riddlebox@freesurf.ch>
 *     start merging tpconfig and gpm code to a xfree-input module
 *     adding some changes and extensions (ex. 3rd and 4th button)
 *
 *   Copyright (c) 1997 C. Scott Ananian <cananian@alumni.priceton.edu>
 *   Copyright (c) 1998-2000 Bruce Kalk <kall@compass.com>
 *     code for the special synaptics commands (from the tpconfig-source)
 *
 * Trademarks are the property of their respective owners.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/input/mt.h>
#include <linux/serio.h>
#include <linux/libps2.h>
#include <linux/rmi.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include "psmouse.h"
#include "fujitsu_scroll.h"


#ifdef CONFIG_MOUSE_PS2_FUJITSU_SCROLL
static const struct dmi_system_id present_dmi_table[] = {
#if defined(CONFIG_DMI) && defined(CONFIG_X86)
	{
		/* Fujitsu T901  */
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "FUJITSU"),
			DMI_MATCH(DMI_PRODUCT_NAME, "LIFEBOOK T901"),
		},
	},
	/* NOTE: The Fujitsu T900 may have a scroll sensor, but no
	   scroll wheel.  Don't know if it behaves the same and would
	   need confirmation in any event. */
#endif
	{ }
};
#endif

/*****************************************************************************
 *	Stuff we need even when we do not want native Fujitsu Scroll support
 ****************************************************************************/


int fujitsu_scroll_detect(struct psmouse *psmouse, bool set_properties)
{
	struct ps2dev *ps2dev = &psmouse->ps2dev;
	u8 param[4] = { 0 };
	
#ifdef CONFIG_MOUSE_PS2_FUJITSU_SCROLL	
	/*
         * DMI check.  We know these devices exist in the
	 * T901; maybe other laptops.  Don't want the probe to
	 * mess with other systems. T901 does not have a PS/2 port
	 * so no worries there.
	 */
	if (!dmi_check_system(present_dmi_table)) {
	  return -ENODEV;
	}
#endif
	/*
	 * If we pass the DMI check, then we know we have
	 * the scroll devices unless they were somehow yanked
	 * out of the laptop.
	 */
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRES);
	ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);

	/*
	 * Scroll wheel returns  04 43 07
	 * Scroll sensor returns 00 43 07
	 */
	if (param[1] != FUJITSU_SCROLL_ID) {
	     return -ENODEV;
	}

	if (set_properties) {
	    psmouse->vendor = "Fujitsu";
	    switch (param[0]) {
	    case FUJITSU_SCROLL_WHEEL_ID:
	      psmouse->name = "Scroll Wheel";
	      break;
	    case FUJITSU_SCROLL_SENSOR_ID:
	      psmouse->name = "Scroll Sensor";
	      break;
	    default:
	      psmouse->name = "Unknown";
	    }
#if FJS_SEND_EVENTS	    
		__set_bit(BTN_EXTRA, psmouse->dev->keybit);
		__set_bit(REL_WHEEL, psmouse->dev->relbit);
#endif
	}

	return 0;
}

void fujitsu_scroll_init_sequence(struct psmouse *psmouse) {
  struct ps2dev *ps2dev = &psmouse->ps2dev;
  u8 param[4] = { 0 };
  int error;
  int mode = 0xC4;
  FJS_LOG_FUNCTION(psmouse, "fujitsu_scroll_init_sequence");

  /*
   * This is the magic sequence that has been observed to
   * make both scroll devices output data.
   */
  mode = FJS_INIT_MODE;
  error = ps2_sliced_command(ps2dev, mode);
  param[0] = 0x14; // 20; other values don't work
  ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);  
}

void fujitsu_scroll_reset(struct psmouse *psmouse)
{
  FJS_LOG_FUNCTION(psmouse, "fujitsu_scroll_reset");
	/* reset touchpad back to relative mode, gestures enabled */
  //	fujitsu_scroll_mode_cmd(psmouse, 0);
}

#if defined(CONFIG_MOUSE_PS2_FUJITSU_SCROLL)


static int fujitsu_scroll_query_hardware(struct psmouse *psmouse)
{
    struct ps2dev *ps2dev = &psmouse->ps2dev;
    struct fujitsu_scroll_data *priv = psmouse->private; 
  u8 i;
  u8 param[4];
  //	int error;

	FJS_LOG_FUNCTION(psmouse, "fujitsu_scroll_query_hardware");

	ps2_sliced_command(ps2dev, 0);
	ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);
	psmouse_info(psmouse, "Parameter 00 : %02x %02x %02x",
		     param[0], param[1], param[2]);
	if (param[0] == FUJITSU_SCROLL_WHEEL_ID) {
	  priv->type = FUJITSU_SCROLL_WHEEL;
	} else {
	  priv->type = FUJITSU_SCROLL_SENSOR;
	}
	
	for (i = 1; i < 16; i++) {
	  ps2_sliced_command(ps2dev, i);
	  ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);
	  psmouse_info(psmouse, "Parameter %02x : %02x %02x %02x",
		       i, param[0], param[1], param[2]);
	}
	// Not going to save any of that information unless/until we
	// can decipher what it means.
	
    return 0;
}

#endif /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */

#ifdef CONFIG_MOUSE_PS2_FUJITSU_SCROLL


/*****************************************************************************
 *	Communications functions
 ****************************************************************************/

static void fujitsu_scroll_set_rate(struct psmouse *psmouse, unsigned int rate)
{
    struct ps2dev *ps2dev = &psmouse->ps2dev;
    u8 param[4];
    
    FJS_LOG_FUNCTION(psmouse, "fujitsu_scroll_set_rate: %d.", rate);
	psmouse->rate = rate;
	// Standard rates: 10, 20, 40, 60, 80, 100, 200 
	param[0] = rate;
	ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);
}


/*****************************************************************************
 *	Functions to interpret the packets
 ****************************************************************************/


/*
 *  called for each full received packet from the touchpad
 */
static void fujitsu_scroll_process_packet(struct psmouse *psmouse)
{
	struct input_dev *dev = psmouse->dev;
	struct fujitsu_scroll_data *priv = psmouse->private;

	unsigned int weight;
	unsigned int position;
	int movement;
	bool pressed;

#if FJS_LOG_PACKETS
	psmouse_info(psmouse, "%s: %02x|%02x/%02x/%02x %02x/%02x/%02x",
		     psmouse->name,
		     psmouse->packet[0] & 0xC0,
		     psmouse->packet[0] & 0x3F,
		     psmouse->packet[1], psmouse->packet[2],
		     psmouse->packet[3], psmouse->packet[4],
		     psmouse->packet[5]);
#endif
	
	position = ((psmouse->packet[1] & 0x0f) << 8) +
	  psmouse->packet[2];
	weight = psmouse->packet[0] & 0x3f;
	pressed = (00 != psmouse->packet[4]);

	if (weight >= FJS_WEIGHT_THRESHOLD) {
	  if (!priv->finger_down) {
	    FJS_LOG(psmouse, "FINGER TOUCH");
	    priv->finger_down = 1;
	    priv->last_event_position = position;
	  } else {
	    movement = position - priv->last_event_position;
	    if (priv->type == FUJITSU_SCROLL_WHEEL) {
	      // handle rolling over 0
	      if (movement > MAX_POSITION_CHANGE) {
		movement =  -((FUJITSU_SCROLL_MAX_POSITION - position) +
			      priv->last_event_position);
	      } else if (movement < -MAX_POSITION_CHANGE) {
		movement += FUJITSU_SCROLL_MAX_POSITION;
	      }
	    } else {
	      // scroll sensor
	      movement = -movement;
	    }
#if FJS_SEND_EVENTS	    
	    input_report_rel(dev, REL_WHEEL,
			     -sign_extend32(movement >> FJS_MOVEMENT_BITSHIFT, 4));
#endif
	    
	    if (movement > FJS_POSITION_CHANGE_THRESHOLD) {
	      FJS_LOG(psmouse, "SCROLL DOWN");
	      priv->last_event_position = position;
	    } else if (movement < -FJS_POSITION_CHANGE_THRESHOLD) {
	      FJS_LOG(psmouse, "SCROLL UP");
	      priv->last_event_position = position;
	    }
	  }
	}
	
	else if (1 == priv->finger_down) {
	  FJS_LOG(psmouse, "FINGER LIFT");
	  priv->finger_down = 0;
	}

	if (pressed != priv->pressed) {
	  if (pressed) {
	    FJS_LOG(psmouse, "BUTTON PRESS 0x%02x", psmouse->packet[4]);
	  } else {
	    FJS_LOG(psmouse, "BUTTON RELEASE");
	  }
	  priv->pressed = pressed;
	}
	
#if FJS_SEND_EVENTS	
	input_report_key(dev, BTN_EXTRA, pressed);
	
	input_sync(dev);
#endif	
}


static psmouse_ret_t fujitsu_scroll_process_byte(struct psmouse *psmouse)
{
	if (psmouse->pktcnt >= 6) { /* Full packet received */
		fujitsu_scroll_process_packet(psmouse);
		return PSMOUSE_FULL_PACKET;
	}

	return PSMOUSE_GOOD_DATA;
}


/*****************************************************************************
 *	Driver initialization/cleanup functions
 ****************************************************************************/

static void fujitsu_scroll_disconnect(struct psmouse *psmouse)
{
	struct fujitsu_scroll_data *priv = psmouse->private;
	FJS_LOG_FUNCTION(psmouse, "fujitsu_scroll_disconnect");
	fujitsu_scroll_reset(psmouse);
	kfree(priv);
	psmouse->private = NULL;
}

static int fujitsu_scroll_reconnect(struct psmouse *psmouse)
{
	int retry = 0;
	int error = 0;
	FJS_LOG_FUNCTION(psmouse, "fujitsu_scroll_reconnect");
	do {
		psmouse_reset(psmouse);
		if (retry) {
			/*
			 * On some boxes, right after resuming, the touchpad
			 * needs some time to finish initializing (I assume
			 * it needs time to calibrate) and start responding
			 * to Synaptics-specific queries, so let's wait a
			 * bit.
			 */
			ssleep(1);
		}
		//		ps2_command(&psmouse->ps2dev, param, PSMOUSE_CMD_GETID);
		//		error = fujitsu_scroll_detect(psmouse, 0);
	} while (error && ++retry < 3);

	fujitsu_scroll_init_sequence(psmouse);
		
	if (error)
		return error;

	if (retry > 1) {
		psmouse_info(psmouse, "reconnected after %d tries\n", retry);
	}
	
	return 0;
}


void __init fujitsu_scroll_module_init(void)
{

}

static int fujitsu_scroll_init_ps2(struct psmouse *psmouse)
{
	struct fujitsu_scroll_data *priv;
	int err;

	psmouse->private = priv = kzalloc(sizeof(struct fujitsu_scroll_data), GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}
	
	psmouse->protocol_handler = fujitsu_scroll_process_byte;
	psmouse->pktsize = 6;

	psmouse->set_rate = fujitsu_scroll_set_rate;
	psmouse->disconnect = fujitsu_scroll_disconnect;
	psmouse->reconnect = fujitsu_scroll_reconnect;
	psmouse->cleanup = fujitsu_scroll_reset;
	/* TODO: see if resync_time needs to be adjusted */
	psmouse->resync_time = 0;
#if 0
        ps2_command(&psmouse->ps2dev, NULL, PSMOUSE_CMD_ENABLE);

	fujitsu_scroll_reconnect(psmouse);
#else
	fujitsu_scroll_query_hardware(psmouse);
#if FJS_SEND_EVENTS	
        input_set_capability(psmouse->dev, EV_KEY, BTN_EXTRA);
#endif	
	fujitsu_scroll_init_sequence(psmouse);
#endif	
	return 0;

	// init_fail:
	kfree(priv);
	return err;
}


#else /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */

void __init fujitsu_scroll_module_init(void)
{
}

static int __maybe_unused
fujitsu_scroll_setup_ps2(struct psmouse *psmouse)
{
  FJS_LOG_FUNCTION(psmouse, "ENOSYS fujitsu_scroll_setup_ps2");
	return -ENOSYS;
}

#endif /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */

#if defined(CONFIG_MOUSE_PS2_FUJITSU_SCROLL)

int fujitsu_scroll_init(struct psmouse *psmouse)
{
	FJS_LOG_FUNCTION(psmouse, "fujitsu_scroll_init");	
	psmouse_reset(psmouse);
	return fujitsu_scroll_init_ps2(psmouse);
}

#else /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */

int fujitsu_scroll_init(struct psmouse *psmouse)
{
  FJS_LOG_FUNCTION(psmouse, "ENOSYS fujitsu_scroll_init");
	return -ENOSYS;
}

#endif /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */
