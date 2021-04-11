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

/*
 * Set the Fujitsu scroll mode byte by special commands
 */
static int fujitsu_scroll_mode_cmd(struct psmouse *psmouse, u8 mode)
{
#if 0  
	u8 param[1];
	int error;
	psmouse_info(psmouse, "Setting mode: %02x", mode);
	error = ps2_sliced_command(&psmouse->ps2dev, mode);
	if (error)
		return error;

	param[0] = SYN_PS_SET_MODE2;
	error = ps2_command(&psmouse->ps2dev, param, PSMOUSE_CMD_SETRATE);
	if (error)
		return error;
#endif
	return 0;
}

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
	}

	return 0;
}

void fujitsu_scroll_init_sequence(struct psmouse *psmouse) {
  struct ps2dev *ps2dev = &psmouse->ps2dev;
  u8 param[4] = { 0 };
  int error;
  psmouse_info(psmouse, "fujitsu_scroll_init_sequence");

  /*
   * This is the magic sequence that has been observed to
   * make both scroll devices output data.  We should see
   * if it can't be pared down.
   */
  error = ps2_sliced_command(ps2dev, 0x00);
  ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);
  error = ps2_sliced_command(ps2dev, 0x03);
  ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);
  error = ps2_sliced_command(ps2dev, 0x0a);
  ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);
  error = ps2_sliced_command(ps2dev, 0x02);
  ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);
  error = ps2_sliced_command(ps2dev, 0x08);
  ps2_command(ps2dev, param, PSMOUSE_CMD_GETINFO);
  error = ps2_sliced_command(ps2dev, 0xc4);
  
  param[0] = 0x14;
  ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);
  error = ps2_sliced_command(ps2dev, 0xc4);
  ps2_command(ps2dev, param, PSMOUSE_CMD_SETRATE);
  
}

void fujitsu_scroll_reset(struct psmouse *psmouse)
{
	/* reset touchpad back to relative mode, gestures enabled */
	fujitsu_scroll_mode_cmd(psmouse, 0);
}

#if defined(CONFIG_MOUSE_PS2_FUJITSU_SCROLL)


static int fujitsu_scroll_query_hardware(struct psmouse *psmouse)
{
    struct ps2dev *ps2dev = &psmouse->ps2dev;
  u8 i;
  u8 param[4];
  //	int error;

	psmouse_info(psmouse, "fujitsu_scroll_query_hardware");

	for (i = 0; i < 16; i++) {
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
  //	struct fujitsu_scroll_data *priv = psmouse->private;

	psmouse_info(psmouse, "fujitsu_scroll_set_rate: %d.  Ignoring.", rate);
	if (rate >= 80) {
	  //		priv->mode |= SYN_BIT_HIGH_RATE;
		psmouse->rate = 80;
	} else {
	  //		priv->mode &= ~SYN_BIT_HIGH_RATE;
		psmouse->rate = 40;
	}

	//	fujitsu_scroll_mode_cmd(psmouse, priv->mode);
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
	//	struct fujitsu_scroll_data *priv = psmouse->private;
	//	struct fujitsu_scroll_device_info *info = &priv->info;
	struct fujitsu_scroll_hw_state hw;

	psmouse_info(psmouse, "%s: %02x|%02x/%02x/%02x %02x/%02x/%02x",
		     psmouse->name,
		     psmouse->packet[0] & 0xC0,
		     psmouse->packet[0] & 0x3F,
		     psmouse->packet[1], psmouse->packet[2],
		     psmouse->packet[3], psmouse->packet[4],
		     psmouse->packet[5]);

	hw.position = ((psmouse->packet[1] & 0x0f) << 8) +
	  psmouse->packet[2];
	hw.weight = psmouse->packet[0] & 0x3f;
	hw.pressed = psmouse->packet[4] >> 7;
	/*
	 * TODO: Something interesting with hw
	 */
	input_sync(dev);
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
	psmouse_info(psmouse, "fujitsu_scroll_disconnect");
	fujitsu_scroll_reset(psmouse);
	kfree(priv);
	psmouse->private = NULL;
}

static int fujitsu_scroll_reconnect(struct psmouse *psmouse)
{
  //	struct fujitsu_scroll_data *priv = psmouse->private;
  //	struct fujitsu_scroll_device_info info;
  //	u8 param[2];
	int retry = 0;
	int error = 0;
	psmouse_info(psmouse, "fujitsu_scroll_reconnect");
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
	// TODO: declare inputs
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
  psmouse_info(psmouse, "ENOSYS fujitsu_scroll_setup_ps2");
	return -ENOSYS;
}

#endif /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */

#if defined(CONFIG_MOUSE_PS2_FUJITSU_SCROLL)

int fujitsu_scroll_init(struct psmouse *psmouse)
{
	psmouse_info(psmouse, "fujitsu_scroll_init");	
	psmouse_reset(psmouse);
	return fujitsu_scroll_init_ps2(psmouse);
}

#else /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */

int fujitsu_scroll_init(struct psmouse *psmouse)
{
  psmouse_info(psmouse, "ENOSYS fujitsu_scroll_init");
	return -ENOSYS;
}

#endif /* CONFIG_MOUSE_PS2_FUJITSU_SCROLL */
