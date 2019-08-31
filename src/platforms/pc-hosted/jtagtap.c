/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2008  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 * Modified by Dave Marples <dave@marples.net>
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

/* Low level JTAG implementation using FT2232 with libftdi.
 *
 * Issues:
 * Should share interface with swdptap.c or at least clean up...
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <assert.h>

#include "general.h"
#include "jtagtap.h"

#define MAX_MSG_SIZE (60)

/* See remote.c/.h for protocol information */

/*
 *  DANGER - THIS IS INCOMPLETE */

int jtagtap_init(void)
{
  platform_buffer_write((uint8_t *)"!JS#",4);

  return 0;
}

void jtagtap_reset(void)
{
  platform_buffer_write((uint8_t *)"!JR#",4);
}

void
jtagtap_tms_seq(uint32_t MS, int ticks)

{
  uint8_t construct[MAX_MSG_SIZE];
  int s;

  s=sprintf((char *)construct,"!JM#%02x%x",ticks,MS);
  platform_buffer_write(construct,s);
}

void
jtagtap_tdi_tdo_seq(uint8_t *DO, const uint8_t final_tms, const uint8_t *DI, int ticks)
{
	uint8_t construct[MAX_MSG_SIZE];
	int s;

	if(!ticks) return;
	if (!DI && !DO) return;

	// We assume we're always clocking 32 bits or less, let's be certain
	assert(ticks<33);

	s=sprintf((char *)construct,"!JM#%02x%02x",ticks,final_tms);
	platform_buffer_write(construct,s);
	
	
#if 0

//	printf("ticks: %d\n", ticks);
	if(final_tms) ticks--;
	rticks = ticks & 7;
	ticks >>= 3;
	uint8_t data[3];
	uint8_t cmd =  ((DO)? MPSSE_DO_READ : 0) | ((DI)? (MPSSE_DO_WRITE | MPSSE_WRITE_NEG) : 0) | MPSSE_LSB;
	rsize = ticks;
	if(ticks) {
		data[0] = cmd;
		data[1] = ticks - 1;
		data[2] = 0;
		platform_buffer_write(data, 3);
		if (DI)
			platform_buffer_write(DI, ticks);
	}
	if(rticks) {
		int index = 0;
		rsize++;
		data[index++] = cmd | MPSSE_BITMODE;
		data[index++] = rticks - 1;
		if (DI)
			data[index++] = DI[ticks];
		platform_buffer_write(data, index);
	}
	if(final_tms) {
		int index = 0;
		rsize++;
		data[index++] = MPSSE_WRITE_TMS | ((DO)? MPSSE_DO_READ : 0) | MPSSE_LSB | MPSSE_BITMODE | MPSSE_WRITE_NEG;
		data[index++] = 0;
		if (DI)
			data[index++] = (DI[ticks]) >> rticks?0x81 : 0x01;
		platform_buffer_write(data, index);
	}
	if (DO) {
		int index = 0;
		uint8_t *tmp = alloca(ticks);
		platform_buffer_read(tmp, rsize);
		if(final_tms) rsize--;

		while(rsize--) {
			/*if(rsize) printf("%02X ", tmp[index]);*/
			*DO++ = tmp[index++];
		}
		if (rticks == 0)
			*DO++ = 0;
		if(final_tms) {
			rticks++;
			*(--DO) >>= 1;
			*DO |= tmp[index] & 0x80;
		} else DO--;
		if(rticks) {
			*DO >>= (8-rticks);
		}
		/*printf("%02X\n", *DO);*/
	}
#endif
}

void jtagtap_tdi_seq(const uint8_t final_tms, const uint8_t *DI, int ticks)

{
  return jtagtap_tdi_tdo_seq(NULL,  final_tms, DI, ticks);
}

uint8_t jtagtap_next(uint8_t dTMS, uint8_t dTDI)

{
  uint8_t construct[MAX_MSG_SIZE];
  int s;

  s=sprintf((char *)construct,"!JN#%02x",(dTDI?0x80:0) | (dTMS?0x01:0));
  platform_buffer_write(construct,s);

  return 0;
}

