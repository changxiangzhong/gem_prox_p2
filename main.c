#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <termios.h>
#include <ctype.h>
#include <fcntl.h>
#include "tlp224.h"
#include "serial.h"
#include "common.h"
#include "iso7816.h"

const unsigned char GET_FIRST_CARD_A[] = {0xe2, 0x01, 0x64};
const unsigned char GET_FIRST_TCL_CARD_A[] = {0xe2, 0x03, 0x00, 0x64};
const unsigned char SHORT_BEEP[] = {0x33, 0x04};
const unsigned char LONG_BEEP[] = {0x33, 0x20};
unsigned char const EXCHANGE_7816_SELECT[] = {0xe4, 0x10, 0x01, 0x00,           
	    // The ISO7816 APDU                                                         
		0x00, 0xa4, 0x04, 0x00, 0x07, 0xf0, 0x39, 0x41, 0x48, 0x14,	0x81, 0x00};

// TODO: handle the the returns of 
// 1. tlp224_serialize
// 2. tlp224_deserialize
int exchange_command(const unsigned char* cmd, int cmd_len, TLP224* const req, TLP224* const resp, const int fd)
{
	tlp224_payload(req, 0, cmd, cmd_len);

	tlp224_serialize(req, fd);
	
	tlp224_deserialize(resp, fd);

	//TODO: handle the return code
	return 1;
}

bool verify_uid(int uid) {
	if (uid == 0x9000 || uid == 0x32A96DB3 || uid == 0xE4CDDEC4 || uid == 0x1e95bed2)
		return true;
	
	if (0x05F5E100 <= uid && uid <= 0x05F767A0)
		return true;

	return false;
}

#define TAPP_INTERVAL 3
int do_loop (int fd, TLP224* const req, TLP224* const resp)
{
	int m, n, i;
	const unsigned char* beep;
	unsigned char uid[10];

	// set the serial attributes
	n = serial_set_interface_attribs(fd, B9600, true);
	assert(n == 0);

	printf("set serial attr successfully!\n");

	// set block mode
	serial_set_blocking(fd, true);
	
	printf("set block/unblock mode successfully!\n");

	while (true) {
		beep = NULL;
		memset(uid, 0, sizeof(uid));
		exchange_command(GET_FIRST_TCL_CARD_A, sizeof(GET_FIRST_TCL_CARD_A), req, resp, fd);

		if (resp -> ln > 0 && (resp -> payload)[0] == TLP224_STATUS_CODE_OK) {
			// success! the card support ISO-14443-4 protocol
			printf("===========================================\nISO-14443-4\n==============================================\n");

			// Issue an ISO-7816 select APDU
			exchange_command(EXCHANGE_7816_SELECT, sizeof(EXCHANGE_7816_SELECT), req, resp, fd);
			if (resp -> ln > 0 && resp->payload[0] == TLP224_STATUS_CODE_OK) {
				// the return code of the reader is OK

				//TODO: check the SW1 & SW2
				int sw1sw2 = (resp->payload)[resp->ln-1] | (resp->payload)[resp->ln-2] << 8;
				printf("\n\n\n%sThe SW1SW2 is[0x%08X]%s\n\n\n", KGRN, sw1sw2, KNRM);

				if (sw1sw2 != ISO7816_SW1SW2_SUCCESS) {
					printf("\n\n\nThe SW1SW2 returned is not 0x9000\n\n\n");
					beep = LONG_BEEP;
					goto BEEP;
				}


				m = 0;
				//TODO: the magic number 2 is for the SW1 & SW2
				for(i = resp -> ln - 1 - 2; i >= 1; i--)
					uid[m++] = (resp->payload)[i];
				
				unsigned int* uid1 = (unsigned int*) uid;
				printf("\n\n\n%sThe uid is[0x%08X]%s\n\n\n", KGRN, *uid1, KNRM);
				
				if (verify_uid(*uid1))
					beep = SHORT_BEEP;
				else 
					beep = LONG_BEEP;

			} else  {
				beep = LONG_BEEP;
				printf("\n\n\n%sFailed!%s\n\n\n", KRED, KNRM);
			}

			// exchange_apdu();
		} else if (resp -> ln > 0 && (resp -> payload)[0] == TLP224_STATUS_CODE_ISO_14443_4_NOT_COMPLIANT) {
			printf("Reading a ISO-14443-4 !!NOT!! compliant card\n");
			// Issue a command for Mifare
			exchange_command(GET_FIRST_CARD_A, sizeof(GET_FIRST_CARD_A), req, resp, fd);
			if (resp -> ln > 0 && (resp -> payload)[0] == TLP224_STATUS_CODE_OK) { 
				// Mifare card
				printf("================================================\nMifare Card\n=============================================\n");
				
				m = 0;
				// uid starts from the 3rd in reverse order to 6th
				for(i = resp -> ln - 3; i > 5; i--)
				// for (i = 6; i < resp -> ln - 2; i++)// reversed order 
					uid[m++] = (resp->payload)[i];


				unsigned int* uid1 = (unsigned int*) uid;
				unsigned long* uid2 = (unsigned long*) uid;
				printf("\n\n\nThe uid is[0x%08X]", *uid1);
				printf("%sThe uid is [0x%016lX]%s\n\n\n", KGRN, *uid2, KNRM);
				
				if (verify_uid(*uid1))
					beep = SHORT_BEEP;
				else 
					beep = LONG_BEEP;	

			} else {
				// TODO: Neither Mifare Nor 14443-4 Compliant
				beep = LONG_BEEP;
			}
		}

BEEP:	if (beep) {
			// Not matter short or long beep the sizeof() is the same)
			exchange_command(beep, sizeof(SHORT_BEEP), req, resp, fd);
			sleep (TAPP_INTERVAL);
		} else 
			printf("===========================================\nCard Not Found!\n==============================================\n");
	} // End of the huge loop
}

int main(int argc, char* argv[])
{
	TLP224 request, response;
	tlp224_init(&request);
	tlp224_init(&response);
	
	// open the serial file
	int fd = open("/dev/ttyS0", O_RDWR|O_NOCTTY|O_SYNC);
	assert(fd != -1);

	printf("open serial successfully!\n");


	do_loop(fd, &request, &response);

	close(fd);
	tlp224_destroy(&response);
	tlp224_destroy(&request);
	return 0;
}
