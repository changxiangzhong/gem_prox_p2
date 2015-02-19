#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tlp224.h"

static int _tlp224_lrc(TLP224* const this)
{
	int i;
	// TLP224::lrc is the XOR of all the items;
	this -> lrc = this -> ack;// [payload size] + [ack 1 byte] + [length byte 1byte]
	this -> lrc ^= this -> ln;
	for(i = 0; i < this -> ln; i++)
		this -> lrc ^= (this -> payload)[i];

	return 1;
}

static int _tlp224_verify_checksum(TLP224* const this)
{
	int ret;
	unsigned char temp = this -> lrc;
	_tlp224_lrc(this);
	ret = temp == this -> lrc;
	if (!ret)
		printf("checksum inconsistency! expected=%02X, result=%02X\n", this -> lrc, temp);
	this -> lrc = temp;
	return ret;
}

int tlp224_init(TLP224* const this)
{
	printf("Enter tlp224_init\n");
	assert(this != NULL);

	// Bypass the const limitiation for the payload
	unsigned char** bypass = (unsigned char**)(& this -> payload);
	this -> ack = 0x60; // Ack by default
	this -> ln = 0;
	this -> lrc = 0;
	*bypass = calloc(TLP224_MAX_PL, sizeof(unsigned char));
	assert(this -> payload != NULL);
	printf("Exit tlp224_init\n");
	return 0;
}

int tlp224_destroy(TLP224* const this)
{
	printf("Enter tlp224_destroy\n");
	assert(this != NULL);
	free(this -> payload);
	printf("Exit tlp224_destroy\n");

	return 1;
}

int tlp224_payload(TLP224* const this, int offset, unsigned const char* const data, int length)
{
	printf("Enter tlp224_payload\n");
	assert(this != NULL);
	memcpy(this -> payload + offset, data, length);
	this -> ln = offset + length;
	_tlp224_lrc(this);
	printf("Exit tlp224_payload\n");

	return 1;
}

/*
 * This is not thread safe!
 */
#define PB_MAX (TLP224_PACKET_MAX * 2 + 1) // add one for the EOT
char* tlp224_print(const TLP224* const this)
{
	int i;
	static char PB[PB_MAX];
	assert(this != NULL);
	memset(PB, 0, PB_MAX);
	sprintf(PB, "%02X%02X", this -> ack, this -> ln);
	for(i = 0; i < this -> ln; i++)
		sprintf(PB + 4 + 2 * i, "%02X", (this -> payload)[i]);
	
	sprintf(PB + 4 + 2 * i,"%02X", this->lrc);

	return PB;
}

#define EOT 0x03
int tlp224_serialize(TLP224* const this, int fos)
{
	int i, n;
	printf("Enter tlp224_serialize. The packet length is %d\n\n\n", tlp224_len(this));
	_tlp224_lrc(this);

	char* ascii = tlp224_print(this);
	
	printf("=======>%s\n\n\n", ascii);
	
	int len = strlen(ascii) + 1;

	// set the EOT
	ascii[len - 1] = EOT;

	for(i = 0; i < len; i++)
		printf("0x%02X\t", ascii[i]);

	printf("\n");
	
	n = write(fos, ascii, len);
	assert(n == len);
	printf("write successfully!\n");
	printf("Exit tlp224_serialize\n");
	
	return n;
}	

int tlp224_deserialize(TLP224* const this, int fis)
{
	static char IN_BUFF[PB_MAX];
	int i;
	int n = 0;
	int m = 0;

	assert(this != NULL);


	memset(IN_BUFF, 0x0, PB_MAX);

	// read until get the last EOT	
	while(n == 0 || IN_BUFF[n - 1] != EOT) 
		n += read(fis, IN_BUFF + n, PB_MAX - n);

	m = (n - 2) / 2;// The binary array size is.

	// TODO: actually we could have issue only one sscanf to handle all
	sscanf(IN_BUFF, "%2hhx", &this->ack);
	sscanf(IN_BUFF + 2, "%2hhx", &this->ln);
	
	for(i = 0; i < m - 2; i++) 
		sscanf(IN_BUFF + 4 + 2 * i, "%2hhx", &this->payload[i]);
	sscanf(IN_BUFF + 4 + 2 * i, "%2hhx", &this->lrc);

	printf("\n\n\n<=========%s\n\n\n", tlp224_print(this));

	assert(_tlp224_verify_checksum(this));

//	printf("=======>Read from the fis:[%s]\n", IN_BUFF);
//
	// TODO: handle the error return;
	return 1;
}
