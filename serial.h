#ifndef __serial_h__
#define __serial_h__

/*
 * tty_serial.h component defines the COMMON operations againt serial port
 */
void serial_set_blocking (int fd, int should_block);
int serial_set_interface_attribs (int fd, int speed, int parity);

#endif
