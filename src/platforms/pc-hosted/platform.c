/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 * Additions by Dave Marples <dave@marples.net>
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
#include "remote.h"

#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Allow 100mS for responses to reach us */
#define RESP_TIMEOUT (1000)


static int f;  /* File descriptor for connection to GDB remote */

void platform_init(int argc, char **argv)
{
  int c;
  char *serial = NULL;
  while((c = getopt(argc, argv, "s:")) != -1) {
    switch(c)
      {
      case 's':
	serial = optarg;
	break;
      }
  }
  
  printf("\nBlack Magic Probe (" FIRMWARE_VERSION ")\n");
  printf("Copyright (C) 2019  Black Sphere Technologies Ltd.\n");
  printf("License GPLv3+: GNU GPL version 3 or later "
	 "<http://gnu.org/licenses/gpl.html>\n\n");
  
  assert(gdb_if_init() == 0);
  
  f=open(serial,O_RDWR);
  if (f<0)
    {
      fprintf(stderr,"Couldn't open serial port %s\n",serial);
      exit(-1);
    }
}

void platform_srst_set_val(bool assert)
{
  (void)assert;
  platform_buffer_flush();
}

bool platform_srst_get_val(void) { return false; }

void platform_buffer_flush(void)
{
  
}

int platform_buffer_write(const uint8_t *data, int size)
{
  int s;
  
  s=write(f,data,size);
  if (s<0)
    {
      fprintf(stderr,"Failed to write\n");
      exit(-2);
    }
  
  return size;
}

int platform_buffer_read(uint8_t *data, int maxsize)
  
{
  uint8_t *c;
  int s;
  int ret;
  uint32_t endTime;
  fd_set  rset;
  struct timeval tv;
  
  c=data;
  tv.tv_sec=0;
  
  endTime=platform_time_ms()+RESP_TIMEOUT;
  tv.tv_usec=1000*(endTime-platform_time_ms());
  
  /* Look for start of response */
  do
    {
      FD_ZERO(&rset);
      FD_SET(f, &rset);
      
      ret = select(f + 1, &rset, NULL, NULL, &tv);
      if (ret < 0)
	{
	  fprintf(stderr,"Failed on select\n");
	  exit(-4);
	}
      if(ret == 0)
	{
	  fprintf(stderr,"Timeout on read\n");
	  exit(-3);
	}
      
      s=read(f,c,1);
    }
  while ((s>0) && (*c!=REMOTE_RESP));
  
  /* Now collect the response */
  do
    {
      FD_ZERO(&rset);
      FD_SET(f, &rset);
      ret = select(f + 1, &rset, NULL, NULL, &tv);
      if (ret < 0)
	{
	  fprintf(stderr,"Failed on select\n");
	  exit(-4);
	}
      if(ret == 0)
	{
	  fprintf(stderr,"Timeout on read\n");
	  exit(-3);
	}
      s=read(f,c,1);
      if (*c==REMOTE_EOM)
	{
	  *c=0;
	  return (c-data);
	}
      else
	c++;
    }
  while ((s>=0) && (c-data<maxsize));
  
  fprintf(stderr,"Failed to read\n");
  exit(-3);
  return 0;
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
