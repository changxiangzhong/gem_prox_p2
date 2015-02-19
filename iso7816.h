#ifndef __iso7816_h__
#define __iso7816_h__

#define ISO7816_SW1SW2_SUCCESS 0X9000

/*
 * Defined in ISO/IEE-7816-4 protocol suit
 * TODO: the lc & le items can be 3 bytes, 
 * which is not handled now.
 */
typedef struct {
	unsigned char cla;
	unsigned char ins;
	unsigned char p1;
	unsigned char p2;
	// can be 0, 1, 3 bytes
	unsigned char lc; 
	unsigned char* data;
	// can be 0, 1, 3, bytes
	unsigned char le; 
} ISO7816;

#endif
