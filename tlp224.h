#ifndef __tlp224_h__
#define __tlp224_h__

// Per 'GemProx Reference Manual.pdf' the maximum of the Payload is 255
#define TLP224_MAX_PL 255
#define TLP224_HEADER 2
#define TLP224_TAILOR 1

#define TLP224_STATUS_CODE_OK 0x00
#define TLP224_STATUS_CODE_ISO_14443_4_NOT_COMPLIANT 0xFA

#define TLP224_PACKET_MAX (TLP224_MAX_PL + TLP224_HEADER + TLP224_TAILOR)
/*
 * The expected sequence sequence is as following:
 * 	1. tlp224_init();
 * 	2. setting values;
 * 	3. tlp224_lrc();
 * 	4. tlp224_to_bytes();
 * 	5. tlp224_destroy();
 */
typedef struct {
	unsigned char ack;
	unsigned char ln;
	unsigned char* const payload;
	unsigned char lrc;
	unsigned char time; // processing time in milli-second
} TLP224;

/*
 * Return the length of the whole packet
 */
static inline int tlp224_len(TLP224* const this)
{
	return this -> ln + 3;
}

/*
 * Constructor of TLP224
 */
int tlp224_init(TLP224* const this);

/*
 * Destructor of TLP224
 */
int tlp224_destroy(TLP224* const this);

/*
 * Calculate the checksum after setting all the values
 */
int tlp224_payload(TLP224* const this, int offset, unsigned const char* const data, int length);

/*
 * The 2nd phase of TLP224 protocol processing
 */
int tlp224_serialize(TLP224* const this, int fos);

/*
 * Deserialize it. The 0x03 is the special EOT signal.
 * It would be blocked until receiving it.
 */
int tlp224_deserialize(TLP224* const this, int fis);

/*
 * Print out the TLP224 protocol
 */
char* tlp224_print(const TLP224* const this);
#endif
