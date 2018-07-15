/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PLATFORM_H
#define __PLATFORM_H

#include <libftdi1/ftdi.h>

#include "timing.h"

#ifndef _WIN32
#	include <alloca.h>
#else
#	ifndef alloca
#		define alloca __builtin_alloca
#	endif
#endif

#define FT2232_VID	0x0403
#define FT2232_PID	0x6010

#define PLATFORM_HAS_DEBUG

#define PLATFORM_IDENT "FTDI/MPSSE"
#define SET_RUN_STATE(state)
#define SET_IDLE_STATE(state)
#define SET_ERROR_STATE(state)

extern struct ftdi_context *ftdic;

void platform_buffer_flush(void);
int platform_buffer_write(const uint8_t *data, int size);
int platform_buffer_read(uint8_t *data, int size);

typedef struct cable_desc_s {
	int vendor;
	int product;
	int interface;
	uint8_t dbus_data;
	uint8_t dbus_ddr;
	uint8_t cbus_data;
	uint8_t cbus_ddr;
	uint8_t bitbang_tms_in_port_cmd;
	uint8_t bitbang_tms_in_pin;
	uint8_t bitbang_swd_dbus_read_data;
	uint8_t bitbang_swd_direct;
	/* dbus_data, dbus_ddr, cbus_data, cbus_ddr value to assert SRST.
	 *	E.g. with CBUS Pin 1 low asserting SRST,
	 *	give {0, 0, ~PIN1, PIN1} */
	uint8_t assert_srst[4];
	/* dbus_data, dbus_ddr, cbus_data, cbus_ddr value to release SRST.
	 *	E.g. with CBUS Pin 1 floating with internal pull up,
	 *	give {0, 0, PIN1, ~PIN1} */
	uint8_t deassert_srst[4];
	/* Command to read back SRST. Keep 0 if no dedicated pin.*/
	uint8_t srst_get_port_cmd;
	/* PIN to read back as SRST. Keep 0 if no dedicated pin.
	*  Use PINX if active high, use Complement (~PINX) if active low*/
	uint8_t srst_get_pin;
	/* bitbang_swd_dbus_read_data is same as dbus_data,
	 * as long as CBUS is not involved.*/
	char *description;
	char * name;
}cable_desc_t;

extern cable_desc_t *active_cable;

static inline int platform_hwversion(void)
{
	        return 0;
}

#define MPSSE_TCK 1
#define PIN0      1
#define MPSSE_TDI 2
#define PIN1      2
#define MPSSE_TDO 4
#define PIN2      4
#define MPSSE_TMS 8
#define PIN3      8
#define PIN4      0x10
#define PIN5      0x20
#define PIN6      0x40
#define PIN7      0x80
#endif

