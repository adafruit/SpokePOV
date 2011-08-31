/*
 * avrdude - A Downloader/Uploader for AVR device programmers
 * Copyright (C) 2003, 2004  Martin J. Thomas  <mthomas@rhrk.uni-kl.de>
 * Copyright (C) 2005 Michael Holzt <kju-avr@fqdn.org>
 * Copyright (C) 2005 Joerg Wunsch <j@uriah.heep.sax.de>
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
/* $Id: serbb_win32.cpp,v 1.1 2007/02/18 06:11:30 lady_ada Exp $ */

/*
 * Win32 serial bitbanging interface for avrdude.
 */

#if defined(WIN32NATIVE)

#include <windows.h>
#include <stdio.h>
#include "serbb.h"

#define verbose 0

/* cached status lines */
static int dtr, rts, txd;

#define W32SERBUFSIZE 1024

/*
  serial port/pin mapping

  1	cd	<-
  2	rxd	<-
  3	txd	->
  4	dtr	->
  5	dsr	<-
  6	rts	->
  7	cts	<-

  Negative pin # means negated value.
*/

int serbb_setpin(int fd, int pin, int value)
{
  HANDLE hComPort = (HANDLE)fd;
  LPVOID lpMsgBuf;
  DWORD dwFunc;
  const char *name;

  if (pin < 1 || pin > 7)
    return -1;
  
  pin--;

  switch (pin)
    {
    case 2:  /* txd */
      dwFunc = value? SETBREAK: CLRBREAK;
      name = value? "SETBREAK": "CLRBREAK";
      txd = value;
      break;

    case 3:  /* dtr */
      dwFunc = value? SETDTR: CLRDTR;
      name = value? "SETDTR": "CLRDTR";
      dtr = value;
      break;
      
    case 5:  /* rts */
      dwFunc = value? SETRTS: CLRRTS;
      name = value? "SETRTS": "CLRRTS";
      break;
      
    default:
      if (verbose) 
	fprintf(stderr,
		"serbb_setpin(): unknown pin %d\n",  pin + 1);
      return -1;
    }
  
  //usleep(1000);
  //printf("delay %d\n\r", pgm->bb_delay);
  
  if (verbose > 4)
    fprintf(stderr,
	    "serbb_setpin(): EscapeCommFunction(%s)\n",  name);
  if (!EscapeCommFunction(hComPort, dwFunc))
    {
      FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
		    FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_IGNORE_INSERTS,
		    NULL,
		    GetLastError(),
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		    (LPTSTR) &lpMsgBuf,
		    0,
		    NULL);
      fprintf(stderr,
	      "serbb_setpin(): SetCommState() failed: %s\n",
	      (char *)lpMsgBuf);
      CloseHandle(hComPort);
      LocalFree(lpMsgBuf);
      exit(1);
    }
  return 0;
}

int serbb_getpin(int fd, int pin)
{
  HANDLE hComPort = (HANDLE)fd;
  LPVOID lpMsgBuf;
  int invert, rv;
  const char *name;
  DWORD modemstate;
  

  invert = 0;
  
  if (pin < 1 || pin > 7)
    return -1;
  
  pin --;
  
  if (pin == 0 /* cd */ || pin == 4 /* dsr */ || pin == 6 /* cts */)
    {
      if (!GetCommModemStatus(hComPort, &modemstate))
	{
	  FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
	  fprintf(stderr,
		  "serbb_setpin(): GetCommModemStatus() failed: %s\n",
		  (char *)lpMsgBuf);
	  CloseHandle(hComPort);
	  LocalFree(lpMsgBuf);
	  exit(1);
	}
      if (verbose > 4)
	fprintf(stderr,
		"serbb_getpin(): GetCommState() => 0x%lx\n",
		 modemstate);
      switch (pin)
	{
	case 0:
	  modemstate &= MS_RLSD_ON;
	  break;
	case 4:
	  modemstate &= MS_DSR_ON;
	  break;
	case 6:
	  modemstate &= MS_CTS_ON;
	  break;
	}
      rv = modemstate != 0;
      if (invert)
	rv = !rv;
      
      return rv;
    }
  
  switch (pin)
    {
    case 2: /* txd */
      rv = txd;
      name = "TXD";
      break;
    case 3: /* dtr */
      rv = dtr;
      name = "DTR";
      break;
    case 5: /* rts */
      rv = rts;
      name = "RTS";
      break;
    default:
      if (verbose)
	fprintf(stderr,
		" serbb_getpin(): unknown pin %d\n", pin + 1);
      return -1;
    }
  if (verbose > 4)
    fprintf(stderr,
	    "serbb_getpin(): return cached state for %s\n",
	     name);
  if (invert)
    rv = !rv;
  
  return rv;
}

int serbb_open(char *port)
{
  DCB dcb;
  LPVOID lpMsgBuf;
  HANDLE hComPort = INVALID_HANDLE_VALUE;
  
  hComPort = CreateFile(port, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  
  if (hComPort == INVALID_HANDLE_VALUE) {
    FormatMessage(
		  FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_FROM_SYSTEM |
		  FORMAT_MESSAGE_IGNORE_INSERTS,
		  NULL,
		  GetLastError(),
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		  (LPTSTR) &lpMsgBuf,
		  0,
		  NULL);
    fprintf(stderr, "ser_open(): can't open device \"%s\": %s\n",
	     port, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
    return -1;
  }
  
  if (!SetupComm(hComPort, W32SERBUFSIZE, W32SERBUFSIZE))
    {
      CloseHandle(hComPort);
      fprintf(stderr, "ser_open(): can't set buffers for \"%s\"\n",
	       port);
      return -1;
    }
  
  
  ZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  dcb.BaudRate = CBR_9600;
  dcb.fBinary = 1;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  
  if (!SetCommState(hComPort, &dcb))
    {
      CloseHandle(hComPort);
      fprintf(stderr, "ser_open(): can't set com-state for \"%s\"\n",
	      port);
      return -1;
    }
  if (verbose > 2)
    fprintf(stderr,
	    "ser_open(): opened comm port \"%s\", handle 0x%x\n",
	    port, (int)hComPort);
  
  
  
  dtr = rts = txd = 0;
  
  return (int)hComPort;

}

void serbb_close(int fd)
{
  HANDLE hComPort=(HANDLE)fd;
  if (hComPort != INVALID_HANDLE_VALUE)
    CloseHandle (hComPort);
  if (verbose > 2)
    fprintf(stderr,
	    "ser_close(): closed comm port handle 0x%x\n",
	    (int)hComPort);
  
  hComPort = INVALID_HANDLE_VALUE;
}

#endif  /* WIN32NATIVE */
