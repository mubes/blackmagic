/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2019  Black Sphere Technologies Ltd.
 * Written by Dave Marples <dave@marples.net>
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
#include "remote.h"
#include "gdb_packet.h"
#include "swdptap.h"
#include "jtagtap.h"
#include "gdb_if.h"
#include <stdarg.h>

#define HTON(x) ((x<='9')?x-'0':((TOUPPER(x))-'A'+10))
#define TOUPPER(x) ((((x)>='a') && ((x)<='z'))?((x)-('a'-'A')):(x))
#define ISHEX(x) (				\
		  (((x)>='0') && ((x)<='9')) || \
                  (((x)>='A') && ((x)<='F')) || \
                  (((x)>='a') && ((x)<='f'))	\
		 )


uint32_t remotehston(uint32_t limit, char *s)

/* Return numeric version of string, until illegal hex digit, or limit */
  
{
  uint32_t ret=0;
  char c;

  while (limit--)
    {
      c=*s++;
      if (!ISHEX(c))
	return ret;
      ret=(ret<<4)|HTON(c);
    }

  return ret;
}

static void _respond(char respCode, uint32_t param)

/* Send response to far end */
  
{
  char buf[20];
  int size;
  int i=0;
  gdb_if_putchar(REMOTE_RESP,0);
  size=sprintf(buf,REMOTE_RESP_FORMAT,respCode,param);
  while (size--)
    gdb_if_putchar(buf[i++], 0);
  gdb_if_putchar(REMOTE_EOM,1);  
}

void remotePacketProcess(uint8_t i, char *packet)

{
  uint8_t ticks;
  uint32_t param;
  bool badParity;
  
  switch (packet[0])
    {
    case REMOTE_SWDP_PACKET: /* SWDP Commands */
      switch (packet[1])
	{
	case REMOTE_SWDP_INIT: /* SS = initialise ================================= */
	  if (i==2)
	    {
	      swdptap_init();
	      _respond(REMOTE_RESP_OK, 0); 
	    }
	  else
	    {
	      _respond(REMOTE_RESP_ERR,REMOTE_ERROR_WRONGLEN);
	    }
	  break;
	  
	case REMOTE_SWDP_IN_PAR: /* SI = In parity ================================== */
	  ticks=remotehston(2,&packet[2]);
	  badParity=swdptap_seq_in_parity(&param, ticks);
	  _respond(badParity?REMOTE_RESP_PARERR:REMOTE_RESP_OK,param);
	  break;
	  
	case REMOTE_SWDP_IN: /* Si = In ========================================= */
	  ticks=remotehston(2,&packet[2]);
	  param=swdptap_seq_in(ticks);
	  _respond(REMOTE_RESP_OK,param);	  
	  break;
	  
	case REMOTE_SWDP_OUT: /* So = Out ======================================== */
	  ticks=remotehston(2,&packet[2]);
	  param=remotehston(-1, &packet[4]);
	  swdptap_seq_out(param, ticks);
	  _respond(REMOTE_RESP_OK, 0);
	  break;
	  
	case REMOTE_SWDP_OUT_PAR: /* SO = Out parity ================================= */
	  ticks=remotehston(2,&packet[2]);
	  param=remotehston(-1, &packet[4]);
	  swdptap_seq_out_parity(param, ticks);
	  _respond(REMOTE_RESP_OK, 0);
	  break;
	  
	default:
	  _respond(REMOTE_RESP_ERR,REMOTE_ERROR_UNRECOGNISED);	  
	  break;
	}
      break;

    default: /* Oh dear, unrecognised, return an error */
      _respond(REMOTE_RESP_ERR,REMOTE_ERROR_UNRECOGNISED);
      break;
    }
}
