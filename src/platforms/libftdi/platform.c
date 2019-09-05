/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Copyright (C) 2018  Uwe Bonnes(bon@elektron.ikp.physik.tu-darmstadt.de)
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
data_desc_t active_state;

cable_desc_t cable_desc[] = {
	{
		/* Direct connection from FTDI to Jtag/Swd.
		 Pin 6 direct connected to RST.*/
		.vendor = 0x0403,
		.product = 0x6010,
		.interface = INTERFACE_A,
		.init.data_low = PIN6, /* PULL nRST high*/
		.bb_swdio_in_port_cmd = GET_BITS_LOW,
		.bb_swdio_in_pin = MPSSE_CS,
		.assert_srst.data_low   = ~PIN6,
		.assert_srst.ddr_low    =  PIN6,
		.deassert_srst.data_low =  PIN6,
		.deassert_srst.ddr_low  = ~PIN6,
		.description = "FLOSS-JTAG",
		.name = "flossjtag"
	},
	{
		/* MPSSE_SK (DB0) ----------- SWDCK/JTCK
		 * MPSSE-DO (DB1) -- 470 R -- SWDIO/JTMS
		 * MPSSE-DI (DB2) ----------- SWDIO/JTMS
		 * DO is tristated with SWD read, so
		 * resistor is not necessary, but protects
		 * from contentions in case of errors.
		 * JTAG not possible
		 * PIN6     (DB6) ----------- NRST */
		.vendor  = 0x0403,
		.product = 0x6010,/*FT2232H*/
		.interface = INTERFACE_B,
		.init.data_low = PIN4, /* Pull up pin 4 */
		.init.ddr_low  = PIN4, /* Pull up pin 4 */
		.mpsse_swd_read.set_data_low  = MPSSE_DO,
		.mpsse_swd_write.set_data_low = MPSSE_DO,
		.assert_srst.data_low   = ~PIN6,
		.assert_srst.ddr_low    =  PIN6,
		.deassert_srst.data_low =  PIN6,
		.deassert_srst.ddr_low  = ~PIN6,
		.target_voltage_cmd  = GET_BITS_LOW,
		.target_voltage_pin  = PIN4, /* Always read as target voltage present.*/
		.description = "USBMATE",
		.name = "usbmate"
	},
	{
		/* MPSSE_SK (DB0) ----------- SWDCK/JTCK
		 * MPSSE-DO (DB1) -- 470 R -- SWDIO/JTMS
		 * MPSSE-DI (DB2) ----------- SWDIO/JTMS
		 * DO is tristated with SWD read, so
		 * resistor is not necessary, but protects
		 * from contentions in case of errors.
		 * JTAG not possible.*/
		.vendor  = 0x0403,
		.product = 0x6014,/*FT232H*/
		.interface = INTERFACE_A,
		.mpsse_swd_read.set_data_low  = MPSSE_DO,
		.mpsse_swd_write.set_data_low = MPSSE_DO,
		.name = "ft232h_resistor_swd"
	},
	{
		/* Buffered connection from FTDI to Jtag/Swd.
		 * TCK and TMS not independant switchable!
		 * SWD not possible.
		 * PIN4 low enables buffers
		 * PIN5 Low indicates VRef applied
		 * PIN6 reads back SRST
		 * CBUS PIN1 Sets SRST
		 * CBUS PIN2 low drives SRST
		 */
		.vendor = 0x0403,
		.product = 0x6010,
		.interface = INTERFACE_A,
		.init.data_low = PIN4,
		.init.data_high = PIN4 | PIN3 | PIN2,
		.init.ddr_high = PIN4 | PIN3 | PIN2 | PIN1 | PIN0,
		.assert_srst.data_high   = ~PIN2,
		.deassert_srst.data_high =  PIN2,
		.srst_get_port_cmd = GET_BITS_LOW,
		.srst_get_pin = PIN6,
		.description = "FTDIJTAG",
		.name = "ftdijtag"
	},
	{
/* UART/SWO on Interface A
 * JTAG and control on INTERFACE_B
 * Bit 5 high selects SWD-WRITE (TMS routed to MPSSE_DI)
 * Bit 6 high selects JTAG vs SWD (TMS routed to MPSSE_CS)
 * BCBUS 1 (Output) N_SRST
 * BCBUS 2 (Input/ Internal Pull Up) V_ISO available
 *
 * For bitbanged SWD, set Bit 5 low and select SWD read with
 * Bit 6 low. Read Connector TMS as MPSSE_DI.
 *
 * TDO is routed to Interface 0 RXD as SWO or with Uart
 * Connector pin 10 pulled to ground will connect Interface 0 RXD
 * to UART connector RXD
 */
		.vendor = 0x0403,
		.product = 0x6010,
		.interface = INTERFACE_B,
		.init.data_low = PIN6 | PIN5,
		.init.ddr_low  = PIN6 | PIN5,
		.init.data_high = PIN1 | PIN2,
		.assert_srst.data_high     = ~PIN1,
		.assert_srst.ddr_high      =  PIN1,
		.deassert_srst.data_high   =  PIN1,
		.deassert_srst.ddr_high    = ~PIN1,
		.mpsse_swd_read.clr_data_low  = PIN5 | PIN6,
		.mpsse_swd_write.set_data_low = PIN5,
		.mpsse_swd_write.clr_data_low = PIN6,
		.jtag.set_data_low            = PIN6,
		.target_voltage_cmd  = GET_BITS_HIGH,
		.target_voltage_pin  = ~PIN2,
		.name = "ftdiswd"
	},
	{
		.vendor = 0x15b1,
		.product = 0x0003,
		.interface = INTERFACE_A,
		.init.ddr_low  = PIN5,
		.name = "olimex"
	},
	{
		/* Buffered connection from FTDI to Jtag/Swd.
		 * TCK and TMS not independant switchable!
		 * => SWD not possible.
		 * DBUS PIN4 / JTAGOE low enables buffers
		 * DBUS PIN5 / TRST high drives nTRST low OC
		 * DBUS PIN6 / RST high drives nSRST low OC
		 * CBUS PIN0 reads back SRST
		 */
		.vendor = 0x0403,
		.product = 0xbdc8,
		.interface = INTERFACE_A,
		/* Drive low to activate JTAGOE and deassert TRST/RST.*/
		.init.data_low  = PIN6,
		.init.ddr_low  = PIN6 | PIN5 | PIN4,
		.init.ddr_high = PIN2, /* ONE LED */
		.assert_srst.data_low = PIN6,
		.deassert_srst.data_low = ~PIN6,
		.srst_get_port_cmd = GET_BITS_HIGH,
		.srst_get_pin = PIN0,
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
		.name = "jtaghs1"
	},
	{
		/* Direct connection from FTDI to Jtag/Swd assumed.*/
		.vendor = 0x0403,
		.product = 0xbdc8,
		.interface = INTERFACE_A,
		.init.data_low = MPSSE_CS | MPSSE_DO | MPSSE_DI,
		.init.ddr_low  = MPSSE_CS | MPSSE_DO | MPSSE_SK,
		.bb_swdio_in_port_cmd = GET_BITS_LOW,
		.bb_swdio_in_pin = MPSSE_CS,
		.name = "ftdi"
	},
	{
		/* Product name not unique! Assume SWD not possible.*/
		.vendor = 0x0403,
		.product = 0x6014,
		.interface = INTERFACE_A,
		.init.data_low = PIN7,
		.init.ddr_low = PIN7,
		.init.data_high = PIN5,
		.init.ddr_high = PIN5 | PIN4 | PIN3 | PIN2 | PIN1 | PIN0,
		.name = "digilent"
	},
	{
		/* Direct connection from FTDI to Jtag/Swd assumed.*/
		.vendor = 0x0403,
		.product = 0x6014,
		.interface = INTERFACE_A,
		.init.data_low = MPSSE_CS | MPSSE_DO | MPSSE_DI,
		.init.ddr_low  = MPSSE_CS | MPSSE_DO | MPSSE_SK,
		.bb_swdio_in_port_cmd = GET_BITS_LOW,
		.bb_swdio_in_pin = MPSSE_CS,
		.name = "ft232h"
	},
	{
		/* Direct connection from FTDI to Jtag/Swd assumed.*/
		.vendor = 0x0403,
		.product = 0x6011,
		.interface = INTERFACE_A,
		.bb_swdio_in_port_cmd = GET_BITS_LOW,
		.bb_swdio_in_pin = MPSSE_CS,
		.name = "ft4232h"
	},
	{
		/* http://www.olimex.com/dev/pdf/ARM-USB-OCD.pdf.
		 * DBUS 4 global enables JTAG Buffer.
		 * => TCK and TMS not independant switchable!
		 * => SWD not possible. */
		.vendor = 0x15ba,
		.product = 0x002b,
		.interface = INTERFACE_A,
		.init.ddr_low = PIN4,
		.init.data_high = PIN3 | PIN1 | PIN0,
		.init.ddr_high =  PIN4 | PIN3 | PIN1 | PIN0,
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
	bool external_resistor_swd = false;
	while ((c = getopt(argc, argv, "c:s:r")) != -1) {
		switch(c) {
		case 'c':
			cablename =  optarg;
			break;
		case 's':
			serial = optarg;
			break;
		case 'r':
			external_resistor_swd = true;
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
	memcpy(&active_state, &active_cable->init, sizeof(data_desc_t));
	/* If swd_(read|write) is not given for the selected cable and
	   the 'r' command line argument is give, assume resistor SWD
	   connection.*/
	if (external_resistor_swd &&
		(active_cable->mpsse_swd_read.set_data_low  == 0) &&
		(active_cable->mpsse_swd_read.clr_data_low  == 0) &&
		(active_cable->mpsse_swd_read.set_data_high == 0) &&
		(active_cable->mpsse_swd_read.clr_data_high == 0) &&
		(active_cable->mpsse_swd_write.set_data_low  == 0) &&
		(active_cable->mpsse_swd_write.clr_data_low  == 0) &&
		(active_cable->mpsse_swd_write.set_data_high == 0) &&
		(active_cable->mpsse_swd_write.clr_data_high == 0)) {
			DEBUG("Using external resistor SWD\n");
			active_cable->mpsse_swd_read.set_data_low = MPSSE_DO;
			active_cable->mpsse_swd_write.set_data_low = MPSSE_DO;
		}

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
	assert(ftdic != NULL);
	err = ftdi_usb_purge_buffers(ftdic);
	if (err != 0) {
		fprintf(stderr, "ftdi_usb_purge_buffer: %d: %s\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}
	/* Reset MPSSE controller. */
	err = ftdi_set_bitmode(ftdic, 0,  BITMODE_RESET);
	if (err != 0) {
		fprintf(stderr, "ftdi_set_bitmode: %d: %s\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}
	/* Enable MPSSE controller. Pin directions are set later.*/
	err = ftdi_set_bitmode(ftdic, 0, BITMODE_MPSSE);
	if (err != 0) {
		fprintf(stderr, "ftdi_set_bitmode: %d: %s\n",
			err, ftdi_get_error_string(ftdic));
		abort();
	}
	uint8_t ftdi_init[9];
	ftdi_init[0]= TCK_DIVISOR;
	/* Use CLK/2 for about 50 % SWDCLK duty cycle on FT2232c.*/
	ftdi_init[1]= 1;
	ftdi_init[3]= SET_BITS_LOW;
	ftdi_init[4]= active_state.data_low;
	ftdi_init[5]= active_state.ddr_low;
	ftdi_init[6]= SET_BITS_HIGH;
	ftdi_init[7]= active_state.data_high;
	ftdi_init[8]= active_state.ddr_high;
	platform_buffer_write(ftdi_init, 9);
	platform_buffer_flush();

	assert(gdb_if_init() == 0);
}

void platform_set_data(data_desc_t* data)
{
	uint8_t cmd[6];
	int index = 0;
	if ((data->data_low) || (data->ddr_low)) {
		if (data->data_low > 0)
			active_state.data_low |= (data->data_low & 0xff);
		else
			active_state.data_low &= (data->data_low & 0xff);
		if (data->ddr_low > 0)
			active_state.ddr_low  |= (data->ddr_low  & 0xff);
		else
			active_state.ddr_low  &= (data->ddr_low  & 0xff);
		cmd[index++] = SET_BITS_LOW;
		cmd[index++] = active_state.data_low;
		cmd[index++] = active_state.ddr_low;
	}
	if ((data->data_high) || (data->ddr_high)) {
		if (data->data_high > 0)
			active_state.data_high |= (data->data_high & 0xff);
		else
			active_state.data_high &= (data->data_high & 0xff);
		if (data->ddr_high > 0)
			active_state.ddr_high  |= (data->ddr_high  & 0xff);
		else
			active_state.ddr_high  &= (data->ddr_high  & 0xff);
		cmd[index++] = SET_BITS_HIGH;
		cmd[index++] = active_state.data_high;
		cmd[index++] = active_state.ddr_high;
	}
	if (index) {
		platform_buffer_write(cmd, index);
		platform_buffer_flush();
	}
}

void platform_srst_set_val(bool assert)
{
	if (assert)
		platform_set_data(&active_cable->assert_srst);
	else
		platform_set_data(&active_cable->deassert_srst);
}

bool platform_srst_get_val(void)
{
	uint8_t cmd[1] = {0};
	uint8_t pin = 0;
	if (active_cable->srst_get_port_cmd && active_cable->srst_get_pin) {
		cmd[0]= active_cable->srst_get_port_cmd;
		pin   =  active_cable->srst_get_pin;
	} else if (active_cable->assert_srst.data_low &&
			   active_cable->assert_srst.ddr_low) {
		cmd[0]= GET_BITS_LOW;
		pin   = active_cable->assert_srst.data_low;
	} else if (active_cable->assert_srst.data_high &&
			   active_cable->assert_srst.ddr_high) {
		cmd[0]= GET_BITS_HIGH;
		pin   = active_cable->assert_srst.data_high;
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
	uint8_t pin = active_cable->target_voltage_pin;
	if (active_cable->target_voltage_cmd && pin) {
		platform_buffer_write(&active_cable->target_voltage_cmd, 1);
		uint8_t data[1];
		platform_buffer_read(data, 1);
		bool res = false;
		if (((pin < 0x7f) || (pin == PIN7)))
			res = data[0] & pin;
		else
			res = !(data[0] & ~pin);
		if (res)
			return "Present";
		else
			return "Absent";
	}
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

