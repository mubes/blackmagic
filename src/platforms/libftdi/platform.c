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
#include "general.h"
#include "gdb_if.h"
#include "version.h"
#include "platform.h"

#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

struct ftdi_context *ftdic;

#define BUF_SIZE 4096
static uint8_t outbuf[BUF_SIZE];
static uint16_t bufptr = 0;

cable_desc_t *active_cable;

cable_desc_t cable_desc[] = {
	{
		/* Direct connection from FTDI to Jtag/Swd.*/
		.vendor = 0x0403,
		.product = 0x6010,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x1B,
		.bitbang_tms_in_port_cmd = GET_BITS_LOW,
		.bitbang_tms_in_pin = MPSSE_TMS,
		.description = "FLOSS-JTAG",
		.name = "flossjtag"
	},
	{
		/* Buffered connection from FTDI to Jtag/Swd.
		 * TCK and TMS not independant switchable!
		 * SWD not possible. */
		.vendor = 0x0403,
		.product = 0x6010,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x1B,
		.description = "FTDIJTAG",
		.name = "ftdijtag"
	},
	{
/* UART/SWO on Interface A
 * JTAG and control on INTERFACE_B
 * Bit 5 high selects SWD-READ (TMS routed to TDO)
 * Bit 6 high selects JTAG vs SWD (TMS routed to TDI/TDO)
 * BCBUS 1 (Output) N_SRST
 * BCBUS 2 (Input) V_ISO available
 *
 * For bitbanged SWD, set Bit 5 low and select SWD read with
 * Bit 6 low. Read Connector TMS as FTDI TDO.
 *
 * TDO is routed to Interface 0 RXD as SWO or with Uart
 * Connector pin 10 pulled to ground will connect Interface 0 RXD
 * to UART connector RXD
 */
		.vendor = 0x0403,
		.product = 0x6010,
		.interface = INTERFACE_B,
		.dbus_data = 0x6A,
		.dbus_ddr  = 0x6B,
		.cbus_data = 0x02,
		/* Keep nSRST Floating. Needed for I_KEPCO_MCA.
		 * Internal Pull Up will keep nSRST high on
		 * normal FTDISWD. */
		.cbus_ddr  = 0x00,
		.bitbang_tms_in_port_cmd = GET_BITS_LOW,
		.bitbang_tms_in_pin = MPSSE_TDO, /* keep bit 5 low*/
		.bitbang_swd_dbus_read_data = 0x02,
		.assert_srst   = {0, 0, ~PIN1,  PIN1},
		.deassert_srst = {0, 0,  PIN1, ~PIN1},
		.name = "ftdiswd"
	},
	{
		.vendor = 0x15b1,
		.product = 0x0003,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x1B,
		.name = "olimex"
	},
	{
		/* Buffered connection from FTDI to Jtag/Swd.
		 * TCK and TMS not independant switchable!
		 * => SWD not possible. */
		.vendor = 0x0403,
		.product = 0xbdc8,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x1B,
		.name = "turtelizer"
	},
	{
		/* https://reference.digilentinc.com/jtag_hs1/jtag_hs1
		 * No schmeatics available.
		 * Buffered from FTDI to Jtag/Swd announced
		 * Independant switch for TMS not known
		 * => SWD not possible. */
		.vendor = 0x0403,
		.product = 0xbdc8,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x1B,
		.name = "jtaghs1"
	},
	{
		/* Direct connection from FTDI to Jtag/Swd assumed.*/
		.vendor = 0x0403,
		.product = 0xbdc8,
		.interface = INTERFACE_A,
		.dbus_data = 0xA8,
		.dbus_ddr  = 0xAB,
		.bitbang_tms_in_port_cmd = GET_BITS_LOW,
		.bitbang_tms_in_pin = MPSSE_TMS,
		.name = "ftdi"
	},
	{
		/* Product name not unique! Assume SWD not possible.*/
		.vendor = 0x0403,
		.product = 0x6014,
		.interface = INTERFACE_A,
		.dbus_data = 0x88,
		.dbus_ddr  = 0x8B,
		.cbus_data = 0x20,
		.cbus_ddr  = 0x3f,
		.name = "digilent"
	},
	{
		/* Direct connection from FTDI to Jtag/Swd assumed.*/
		.vendor = 0x0403,
		.product = 0x6014,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x0B,
		.bitbang_tms_in_port_cmd = GET_BITS_LOW,
		.bitbang_tms_in_pin = MPSSE_TMS,
		.name = "ft232h"
	},
	{
		/* Direct connection from FTDI to Jtag/Swd assumed.*/
		.vendor = 0x0403,
		.product = 0x6011,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x0B,
		.bitbang_tms_in_port_cmd = GET_BITS_LOW,
		.bitbang_tms_in_pin = MPSSE_TMS,
		.name = "ft4232h"
	},
	{
		/* http://www.olimex.com/dev/pdf/ARM-USB-OCD.pdf.
		 * BDUS 4 global enables JTAG Buffer.
		 * => TCK and TMS not independant switchable!
		 * => SWD not possible. */
		.vendor = 0x15ba,
		.product = 0x002b,
		.interface = INTERFACE_A,
		.dbus_data = 0x08,
		.dbus_ddr  = 0x1B,
		.cbus_data = 0x00,
		.cbus_ddr  = 0x08,
		.name = "arm-usb-ocd-h"
	},
};

void platform_init(int argc, char **argv)
{
	int err;
	int c;
	unsigned index = 0;
	char *serial = NULL;
	char * cablename =  "ftdi";
	while((c = getopt(argc, argv, "c:s:")) != -1) {
		switch(c) {
		case 'c':
			cablename =  optarg;
			break;
		case 's':
			serial = optarg;
			break;
		}
	}

	for(index = 0; index < sizeof(cable_desc)/sizeof(cable_desc[0]);
		index++)
		 if (strcmp(cable_desc[index].name, cablename) == 0)
		 break;

	if (index == sizeof(cable_desc)/sizeof(cable_desc[0])){
		fprintf(stderr, "No cable matching %s found\n",cablename);
		exit(-1);
	}

	active_cable = &cable_desc[index];

	printf("\nBlack Magic Probe (" FIRMWARE_VERSION ")\n");
	printf("Copyright (C) 2015  Black Sphere Technologies Ltd.\n");
	printf("License GPLv3+: GNU GPL version 3 or later "
	       "<http://gnu.org/licenses/gpl.html>\n\n");

	if(ftdic) {
		ftdi_usb_close(ftdic);
		ftdi_free(ftdic);
		ftdic = NULL;
	}
	if((ftdic = ftdi_new()) == NULL) {
		fprintf(stderr, "ftdi_new: %s\n",
			ftdi_get_error_string(ftdic));
		abort();
	}
	if((err = ftdi_set_interface(ftdic, active_cable->interface)) != 0) {
		fprintf(stderr, "ftdi_set_interface: %d: %s\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}
	if((err = ftdi_usb_open_desc(
		ftdic, active_cable->vendor, active_cable->product,
		active_cable->description, serial)) != 0) {
		fprintf(stderr, "unable to open ftdi device: %d (%s)\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}

	if((err = ftdi_set_latency_timer(ftdic, 1)) != 0) {
		fprintf(stderr, "ftdi_set_latency_timer: %d: %s\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}
	if((err = ftdi_set_baudrate(ftdic, 1000000)) != 0) {
		fprintf(stderr, "ftdi_set_baudrate: %d: %s\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}
	if((err = ftdi_write_data_set_chunksize(ftdic, BUF_SIZE)) != 0) {
		fprintf(stderr, "ftdi_write_data_set_chunksize: %d: %s\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}
	assert(gdb_if_init() == 0);
}

void platform_srst_set_val(bool assert)
{
	uint8_t *srst_tupple;
	uint8_t cmd[3] = {0};
	if (assert)
		srst_tupple = active_cable->assert_srst;
	else
		srst_tupple = active_cable->deassert_srst;
	if (srst_tupple[0] || srst_tupple[1]) {
		/* SRST is on dbus. Modify dbus_data and dbus_ddr.*/
		if ((srst_tupple[0] < 0x7f) ||
			(srst_tupple[0] == PIN7)) {
			active_cable->dbus_data |= srst_tupple[0];
		} else
			active_cable->dbus_data &= srst_tupple[0];
		if ((srst_tupple[1] < 0x7f) ||
			(srst_tupple[1] == PIN7)) {
			active_cable->dbus_ddr  |= srst_tupple[1];
		} else
			active_cable->dbus_ddr  &= srst_tupple[1];
		cmd[0] = SET_BITS_LOW;
		cmd[1] = active_cable->dbus_data;
		cmd[2] = active_cable->dbus_ddr;
		platform_buffer_write(cmd, 3);
		platform_buffer_flush();
	} else if (srst_tupple[2] || srst_tupple[3]) {
		/* SRST is on dbus. Modify dbus_data and dbus_ddr.*/
		if ((srst_tupple[2] < 0x7f) ||
			(srst_tupple[2] == PIN7)) {
			active_cable->cbus_data |= srst_tupple[2];
		} else
			active_cable->cbus_data &= srst_tupple[2];
		if ((srst_tupple[3] < 0x7f) ||
			(srst_tupple[3] == PIN7)) {
			active_cable->cbus_ddr  |= srst_tupple[3];
		} else
			active_cable->cbus_ddr  &= srst_tupple[3];
		cmd[0] = SET_BITS_HIGH;
		cmd[1] = active_cable->cbus_data;
		cmd[2] = active_cable->cbus_ddr;
	}
	if (cmd[0]) {
		platform_buffer_write(cmd, 3);
		platform_buffer_flush();
	}
}

bool platform_srst_get_val(void)
{
	uint8_t cmd[1] = {0};
	uint8_t pin = 0;
	if (active_cable->srst_get_port_cmd && active_cable->srst_get_pin) {
		cmd[0]= active_cable->srst_get_port_cmd;
		pin   =  active_cable->srst_get_pin;
	} else if (active_cable->assert_srst[0] && active_cable->assert_srst[1]) {
		cmd[0]= GET_BITS_LOW;
		pin   = active_cable->assert_srst[0];
	} else if (active_cable->assert_srst[2] && active_cable->assert_srst[3]) {
		cmd[0]= GET_BITS_HIGH;
		pin   = active_cable->assert_srst[2];
	}else {
		return false;
	}
	platform_buffer_write(cmd, 1);
	uint8_t data[1];
	platform_buffer_read(data, 1);
	bool res = false;
	if (((pin < 0x7f) || (pin == PIN7)))
		res = data[0] & pin;
	else
		res = !(data[0] & ~pin);
	return res;
}

void platform_buffer_flush(void)
{
#if defined(USE_USB_VERSION_BIT)
static struct ftdi_transfer_control *tc_write = NULL;
	if (tc_write)
		ftdi_transfer_data_done(tc_write);
	tc_write = ftdi_write_data_submit(ftdic, outbuf, bufptr);
#else
	assert(ftdi_write_data(ftdic, outbuf, bufptr) == bufptr);
//	printf("FT2232 platform_buffer flush: %d bytes\n", bufptr);
#endif
	bufptr = 0;
}

int platform_buffer_write(const uint8_t *data, int size)
{
	if((bufptr + size) / BUF_SIZE > 0) platform_buffer_flush();
	memcpy(outbuf + bufptr, data, size);
	bufptr += size;
	return size;
}

int platform_buffer_read(uint8_t *data, int size)
{
#if defined(USE_USB_VERSION_BIT)
	struct ftdi_transfer_control *tc;
	outbuf[bufptr++] = SEND_IMMEDIATE;
	platform_buffer_flush();
	tc = ftdi_read_data_submit(ftdic, data, size);
	ftdi_transfer_data_done(tc);
#else
	int index = 0;
	outbuf[bufptr++] = SEND_IMMEDIATE;
	platform_buffer_flush();
	while((index += ftdi_read_data(ftdic, data + index, size-index)) != size);
#endif
	return size;
}

#if defined(_WIN32) && !defined(__MINGW32__)
#warning "This vasprintf() is dubious!"
int vasprintf(char **strp, const char *fmt, va_list ap)
{
	int size = 128, ret = 0;

	*strp = malloc(size);
	while(*strp && ((ret = vsnprintf(*strp, size, fmt, ap)) == size))
		*strp = realloc(*strp, size <<= 1);

	return ret;
}
#endif

const char *platform_target_voltage(void)
{
	return "not supported";
}

void platform_delay(uint32_t ms)
{
	usleep(ms * 1000);
}

uint32_t platform_time_ms(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

