#include <wx/log.h>
#include <wx/string.h>
#include <wx/debug.h>
#include <wx/utils.h>
#include "communication.h"
#include "ppi.h"
#include "serbb.h"
#include "usb.h"

SpokePOVComm *CommFactory(wxString type, wxString port) {
  if (type.IsSameAs("serial"))
    return new SerialComm(port);

#if defined(WIN32NATIVE)	
  if (type.IsSameAs("parallel"))
    return new ParallelComm(port);
#endif

  if (type.IsSameAs("usb"))
    return new USBComm();

  wxLogMessage("Unknown port type "+type);
  return NULL;
}

void msleep (int ms) {
  volatile int del = ms ;

  while (del > 0) {
    del--;
    usleep(1000);
  }
}

void SpokePOVComm::reset(void) {
  setpin(RESET, 0);
  msleep(100);
  setpin(RESET, 1);
}

int SpokePOVComm::getfd(void) {
  return fd;
}

bool SpokePOVComm::readbyte(unsigned int addr, unsigned char *byte) {
  unsigned char ret = 0xFF;

  spi_xfer(COMP_CMD_RDEEPROM);
  spi_xfer((addr >> 8) & 0xFF);
  spi_xfer(addr & 0xFF);
  if (!spi_get(byte) || !spi_get(&ret) || (ret != 0x80)) {
    wxLogDebug("failed %02x", ret);
    return false;
  }
  //wxLogDebug("read 0x%2x", *byte); 
  return true;
}



bool SpokePOVComm::read16byte(unsigned int addr, unsigned char *buff) {
  unsigned char ret = 0xFF;

  spi_xfer(COMP_CMD_RDEEPROM16);
  spi_xfer((addr >> 8) & 0xFF);
  spi_xfer(addr & 0xFF);
  for (int i=0; i<16; i++) {
    if (!spi_get(buff+i))
      return false;
    //wxLogDebug("read 0x%02x", buff[i]); 
  }
  if (!spi_get(&ret) || (ret != 0x80)) {
    wxLogDebug("failed %02x", ret);
    return false;
  }
  return true;
}

bool SpokePOVComm::write16byte(unsigned int addr, unsigned char *data) {
  unsigned char ret = 0xFF;

  spi_xfer(COMP_CMD_WREEPROM16);
  spi_xfer((addr >> 8) & 0xFF);
  spi_xfer(addr & 0xFF);
  for (int i=0; i<16; i++)
    spi_xfer(*(data+i));
  if (!spi_get(&ret) || (ret != 0x80)) {
    wxLogDebug("failed %02x", ret);
    return false;
  }
  return true;
}

bool SpokePOVComm::writebyte(unsigned int addr, unsigned char data) {
  unsigned char ret = 0xFF;

  spi_xfer(COMP_CMD_WREEPROM);
  spi_xfer((addr >> 8) & 0xFF);
  spi_xfer(addr & 0xFF);
  spi_xfer(data);
  if (!spi_get(&ret) || (ret != 0x80)) {
    wxLogDebug("failed %02x", ret);
    return false;
  }
  return true;
}

bool SpokePOVComm::spi_xfer(unsigned char c) {

  // if its USB do it the quick way
  if (type == USB) {
    return ((USBComm *)this)->usbspi_xfer(c);
  }

  for (int x=7; x >=0; x--) {
    setpin(MOSI, c & (1 << x));
    setpin(CLK, true);
    usleep(delay);
    setpin(CLK, false);
    usleep(delay);
  }
  return true;
}


bool SpokePOVComm::spi_get(unsigned char *c) {

  // if its USB do it the quick way
  if (type == USB) {
    return ((USBComm *)this)->usbspi_get(c);
  }

  setpin(MOSI, false);
 
  for (int x=7; x >=0; x--) {
    setpin(CLK, true);
    usleep(delay);
    *c <<= 1;
    if (!getpin(MISO)) { // BUSY is inverted
      *c |= 0x1;
    }
    setpin(CLK, false);
    usleep(delay);
  }
  return true;
}

USBComm::USBComm(void) {
  // from USBtiny
  struct usb_bus*	bus;
  struct usb_device*	dev = 0;

  usb_handle = 0;

  this->type = USB;
  usb_init();
  usb_find_busses();
  usb_find_devices();
  for	( bus = usb_busses; bus; bus = bus->next ) {
    for	( dev = bus->devices; dev; dev = dev->next ) {
      if (dev->descriptor.idVendor == USBDEV_VENDOR
	  && dev->descriptor.idProduct == USBDEV_PRODUCT) {
	usb_handle = usb_open( dev );
	if ( ! usb_handle ) {
	    wxLogDebug("Cannot open USB device: %s\n", usb_strerror() );
	    return;
	}
	wxLogDebug("Found device!");
	usb_control( USBTINY_POWERUP, (delay > 250) ? 250 : delay, RESET_HIGH );

 
	return;
      }
    }
  }
  wxLogDebug("Could not find USB device 0x%x/0x%x\n", 
	     USBDEV_VENDOR, USBDEV_PRODUCT );
}

USBComm::~USBComm(void) {
  if	( ! usb_handle )
    {
      return;
    }
  usb_control( USBTINY_POWERDOWN, 0, 0 );
  usb_close( usb_handle );
  usb_handle = NULL;
  wxLogDebug("closed");
}


bool USBComm::IsOK(void) {
  if (usb_handle)
    return true;
  return false;
}


bool USBComm::usbspi_get(unsigned char *c) {
  unsigned char buffer[4];
  int ret;

  ret = usb_in(USBTINY_SPI1, 0, 0, buffer, 1, 200);
  if (ret < 0) {
    wxLogDebug("Unable to send vendor request, ret = %d...\n", ret);
    return false;
  } 
  c[0] = buffer[0];
  return true;
}

bool USBComm::usbspi_xfer(unsigned char c) {
  unsigned char buffer[4];
  int ret;

  ret = usb_in(USBTINY_SPI1, c, 0, buffer, 1, 200);
  if (ret < 0) {
    wxLogDebug("Unable to send vendor request, ret = %d...\n", ret);
    return false;
  } 
  return true;
}


void USBComm::setpin(int bit, bool val) {
  //wxLogDebug("setting %d to %d", bit, val);
  switch (bit) {
  case MOSI:
    usb_control( (val ? USBTINY_SET : USBTINY_CLR), 5, 0);
    break;
    
  case CLK:
    usb_control( (val ? USBTINY_SET : USBTINY_CLR), 7, 0);
    break;
    
  case RESET:
    usb_control( (val ? USBTINY_SET : USBTINY_CLR), 4, 0);
    break;
  }
  fflush(stderr);
}

bool USBComm::getpin(int pin) {
  unsigned char buff[4];

  switch (pin) {
  case MISO:
    usb_in(USBTINY_READ, 0, 0, buff, 1, USB_TIMEOUT); 
    return !( buff[0] & (1 << 6) ); // inverted
  }
  return false;
}

void USBComm::usb_control ( int req, int val, int index )
{
  usb_control_msg( usb_handle,
		   USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		   req, val, index, NULL, 0, USB_TIMEOUT );
 }
 
 int USBComm::usb_in ( int req, int val, int index, unsigned char *buf, int buflen, int umax )
 {
  int	n;
  int	timeout;
  
  timeout = USB_TIMEOUT + (buflen * umax) / 1000;
  n = usb_control_msg( usb_handle,
		       USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		       req, val, index, (char*) buf, buflen, timeout );
  if	( n != buflen )
    {
      fprintf( stderr, "USB read error: expected %d, got %d\n", buflen, n );
      return -1;
    }
  return n;
}

int USBComm::usb_out ( int req, int val, int index, unsigned char* buf, int buflen, int umax )
{
  int	n;
  int	timeout;
  
  timeout = USB_TIMEOUT + (buflen * umax) / 1000;
  n = usb_control_msg( usb_handle,
		       USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		       req, val, index, (char*) buf, buflen, timeout );
  if	( n != buflen )
    {
      fprintf( stderr, "USB write error: expected %d, got %d\n", buflen, n );
      return -1;
    }
  return 0;
}

/***********************************************************************/

SerialComm::SerialComm(wxString port) {
  this->port = port;
  this->type = SERIAL;
  if (fd > 0)
    serbb_close(fd);
  fd = serbb_open((char *)port.c_str());
  wxLogDebug("got fd = 0x%x", fd);
  if (fd < 0)
    wxLogDebug("Failed to open port: "+port);

}

SerialComm::~SerialComm(void) {
  serbb_close(fd);
  fd = -1;
}

void SerialComm::setpin(int bit, bool val) {
  //wxLogDebug("setting %d to %d", bit, val);
  switch (bit) {
  case MOSI:
    serbb_setpin(fd, 3, val);
    break;
    
  case CLK:
    serbb_setpin(fd, 4, val);
    break;
    
  case RESET:
    serbb_setpin(fd, 6, val);
    break;
  }
  fflush(stderr);
}

bool SerialComm::getpin(int pin) {
  int i;

  switch (pin) {
  case MISO:
    i = serbb_getpin(fd, 7);
    //wxLogDebug("Got %d", i);
    return !i;
  }
  return false;
}


bool SerialComm::IsOK(void) {
  if (fd > 0)
    return true;
  return false;
}

#if defined(WIN32NATIVE)
ParallelComm::ParallelComm(wxString port) {
  this->port = port;
  this->type = PARALLEL;
  fd = ppi_open((char *)port.c_str());
  if (fd < 0)
    wxLogDebug("Failed to open port! "+port);

  delay = 1000;
}

ParallelComm::~ParallelComm(void) {
  ppi_close(fd);
  fd = -1;
}


bool ParallelComm::IsOK(void) {
  if (fd > 0)
    return true;
  return false;
}

void ParallelComm::setpin(int pin, bool val) {
  switch (pin) {
  case MOSI:
    if (val) {
      ppi_set(fd, PPIDATA, 0x01);
    } else {
      ppi_clr(fd, PPIDATA, 0x01);
    }
    break;
  
  case CLK:
    if (val) {
      ppi_set(fd, PPIDATA, 0x08);
    } else {
      ppi_clr(fd, PPIDATA, 0x08);
    }
    break;
    
  case RESET:
    if (val) {
      ppi_set(fd, PPIDATA, 0x04);
    } else {
      ppi_clr(fd, PPIDATA, 0x04);
    }
    break;
  }
}

bool ParallelComm::getpin(int pin) {
  switch (pin) {
  case MISO:
    return ppi_get(fd, PPISTATUS, 0x80);
  }
  return false;
}
#endif
