/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2000, 2001, 2002, 2003  Brian S. Dean <bsd@bsdhome.com>
 * Copyright (C) 2005 Michael Holzt <kju-avr@fqdn.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id: serbb_posix.cpp,v 1.1 2007/02/18 06:11:30 lady_ada Exp $ */

/*
 * Posix serial bitbanging interface for avrdude.
 */

#if !defined(WIN32NATIVE)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "serbb.h"

#undef DEBUG

 char *progname = "SpokePOV";
struct termios oldmode;

/*
  serial port/pin mapping

  1	cd	<-
  2	rxd	<-
  3	txd	->
  4	dtr	->
  5	dsr	<-
  6	rts	->
  7	cts	<-
*/

static int serregbits[] =
{ TIOCM_CD, 0, 0, TIOCM_DTR, TIOCM_DSR, TIOCM_RTS, TIOCM_CTS };

#ifdef DEBUG
static char *serpins[7] =
  { "CD", "RXD", "TXD ~RESET", "DTR MOSI", "DSR", "RTS SCK", "CTS MISO" };
#endif

int serbb_setpin(int fd, int pin, int value)
{
  unsigned int	ctl;

  if ( pin < 1 || pin > 7 )
    return -1;

  pin--;

#ifdef DEBUG
  printf("%s to %d\n",serpins[pin],value);
#endif

  switch ( pin )
  {
    case 2:  /* txd */
             ioctl(fd, value ? TIOCSBRK : TIOCCBRK, 0);
             return 0;

    case 3:  /* dtr, rts */
    case 5:  ioctl(fd, TIOCMGET, &ctl);
             if ( value )
               ctl |= serregbits[pin];
             else
               ctl &= ~(serregbits[pin]);
             ioctl(fd, TIOCMSET, &ctl);
             return 0;

    default: /* impossible */
             return -1;
  }
}

int serbb_getpin(int fd, int pin)
{
  unsigned int	ctl;
  unsigned char invert;
invert = 0;

  if ( pin < 1 || pin > 7 )
    return(-1);

  pin --;

  switch ( pin )
  {
    case 1:  /* rxd, currently not implemented, FIXME */
             return(-1);

    case 0:  /* cd, dsr, dtr, rts, cts */
    case 3:
    case 4:
    case 5:
    case 6:  ioctl(fd, TIOCMGET, &ctl);
             if ( !invert )
             {
#ifdef DEBUG
               printf("%s is %d\n",serpins[pin],(ctl & serregbits[pin]) ? 1 : 0 );
#endif
               return ( (ctl & serregbits[pin]) ? 1 : 0 );
             }
             else
             {
#ifdef DEBUG
               printf("%s is %d (~)\n",serpins[pin],(ctl & serregbits[pin]) ? 0 : 1 );
#endif
               return (( ctl & serregbits[pin]) ? 0 : 1 );
             }

    default: /* impossible */
             return(-1);
  }
}

int serbb_open(char *port)
{
  struct termios mode;
  int flags;

  /* adapted from uisp code */

  int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd < 0)
    return(-1);

  tcgetattr(fd, &mode);
  oldmode = mode;

  mode.c_iflag = IGNBRK | IGNPAR;
  mode.c_oflag = 0;
  mode.c_cflag = CLOCAL | CREAD | CS8 | B9600;
  mode.c_cc [VMIN] = 1;
  mode.c_cc [VTIME] = 0;

  tcsetattr(fd, TCSANOW, &mode);

  /* Clear O_NONBLOCK flag.  */
  flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    {
      fprintf(stderr, "%s: Can not get flags\n", progname);
      return(-1);
    }
  flags &= ~O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1)
    {
      fprintf(stderr, "%s: Can not clear nonblock flag\n", progname);
      return(-1);
    }

  return(fd);
}

 void serbb_close(int fd)
{
  tcsetattr(fd, TCSANOW, &oldmode);
  return;
}

#endif  /* WIN32NATIVE */
