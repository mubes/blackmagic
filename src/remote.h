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

#ifndef _REMOTE_
#define _REMOTE_

#include <inttypes.h>
#include "general.h"

/*
 * Commands to remote end, and responses
 * =====================================
 * 
 * All commands as sent as ASCII and begin with !, ending with #.
 *  Parameters are hex digits and format is per command.
 *
 * !<CMD><PARAM>#
 *   <CMD>   - 2 digit ASCII value
 *   <PARAM> - x digits (according to command) ASCII value
 *
 * So, for example;
 *
 *  SI - swdptap_seq_in_parity
 *         tt - Ticks
 *       e.g. SI21 : Request input with parity, 33 ticks
 *       resp: K<PARAM> - hex value returned.
 *       resp: F<PARAM> - hex value returned, bad parity.
 *             X<err>   - error occured
 *
 * The whole protocol is defined in this header file. Parameters have
 * to be marshalled in remote.c, swdptap.c and jtagtap.c, so be 
 * careful to ensure the parameter handling matches the protocol
 * definition when anything is changed.
 */

/* Protocol error messages */
#define REMOTE_ERROR_UNRECOGNISED 1
#define REMOTE_ERROR_WRONGLEN     2

/* Start and end of message identifiers */
#define REMOTE_SOM         '!'
#define REMOTE_EOM         '#'
#define REMOTE_RESP        '&'

/* SWDP protocol elements */
#define REMOTE_SWDP_PACKET 'S'
#define REMOTE_SWDP_INIT   'S'
#define REMOTE_SWDP_INIT_STR (char []){ '+',REMOTE_EOM, REMOTE_SOM, REMOTE_SWDP_PACKET, REMOTE_SWDP_INIT, REMOTE_EOM, 0 }

#define REMOTE_SWDP_IN_PAR 'I'
#define REMOTE_SWDP_IN_PAR_STR (char []){ REMOTE_SOM, REMOTE_SWDP_PACKET, REMOTE_SWDP_IN_PAR, \
                                          '%','0','2','x',REMOTE_EOM, 0 }

#define REMOTE_SWDP_IN 'i'
#define REMOTE_SWDP_IN_STR (char []){ REMOTE_SOM, REMOTE_SWDP_PACKET, REMOTE_SWDP_IN, \
                                      '%','0','2','x',REMOTE_EOM, 0 }

#define REMOTE_SWDP_OUT 'o'
#define REMOTE_SWDP_OUT_STR (char []){ REMOTE_SOM, REMOTE_SWDP_PACKET, REMOTE_SWDP_OUT, \
                                       '%','0','2','x','%','x',REMOTE_EOM, 0 }

#define REMOTE_SWDP_OUT_PAR 'O'
#define REMOTE_SWDP_OUT_PAR_STR (char []){ REMOTE_SOM, REMOTE_SWDP_PACKET, REMOTE_SWDP_OUT_PAR, \
                                           '%','0','2','x','%','x',REMOTE_EOM, 0 }

/* Protocol response options */
#define REMOTE_RESP_FORMAT "%c%1" PRIx32
#define REMOTE_RESP_OK     'K'
#define REMOTE_RESP_PARERR 'P'
#define REMOTE_RESP_ERR    'E'

uint32_t remotehston(uint32_t limit, char *s);
void remotePacketProcess(uint8_t i, char *packet);

#endif
