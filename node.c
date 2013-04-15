/* Node
 *
 */

#ifndef __FOR_DEVICE
#define __FOR_DEVICE
#endif

#include "header.h"

#pragma packedCdataStrings 0
#pragma cdata[__EEAPPINFO] = "We Love IQRF!*******++++++++++xx"



/* Some EEPROM - related defines */
#define MAGIC	0x5A

/* EEPROM memory. 0xA0 is first byte available for coordinator */
#define E2_MAGIC	0xA0
#define E2_BONDED	0xA1

#define MAX_SPI_PACKET_DATA	39



#define isupper(x) (((x) >= 'A') && ((x) <= 'Z'))
struct istat dev;

void send_dummy();


void APPLICATION()
{
	uns8 i;


	_LEDG = 0;
	_LEDR = 0;
	/* watchdog expired - some error */
	if (TO == 0) {
		_LEDR = 1;
		_LEDG = 1;
		while(1)
			clrwdt();		

	}

	setNodeMode();
	enableSPI();
	dev.state = STATE_INIT;
	i=0;
	removeBond();
	while(1) {
		clrwdt();

		/* change status */
		if (dev.chstat) {
			dev.state = dev.chstat;
			dev.chstat = 0;
		}


		/* reset it all */
		if (buttonPressed) {
			waitDelay(1000);
			if (buttonPressed)
				dev.chstat = STATE_INIT;
			clrwdt();
			continue;
		}

		check_spi();
		if (dev.chstat)
			continue;


		switch(dev.state){
			case STATE_NOOP:
				break;
			case STATE_INIT:
				dev.chstat = STATE_BOND;
				removeBond();
				_LEDG=1;
				break;
			case STATE_BOND:
				if (bondRequest()) {
					dev.chstat = STATE_WORK;
					_LEDG = 0;
				}
				break;
			case STATE_WORK:
				setNodeMode();
				uns8 rcvd;
				rcvd = receive_packet();
				if ((rcvd & 0xF0) == RF_SYNC) {
					pulseLEDG();
	//				send_dummy();

				}


				break;

			default:
				dev.chstat = STATE_INIT;
		}
		clrwdt();
	}
}

uns8 reply_counter;
void send_dummy(void)
{
	clrwdt();
	/* network packet */
	/* send a reply */
	reply_counter++;
	reply_counter &= 0x0F;

	bufferRF[0] = RF_DATA;
//	writeToRAM(bufferRF + 0, RF_DATA);
	uns8 h,l;

	h = reply_counter / 10;
	l = reply_counter % 10;
	bufferRF[1] = h + '0';
	bufferRF[2] = l + '0';
//	writeToRAM(bufferRF + 1, '0' + h);
//	writeToRAM(bufferRF + 2, '0' + l);
	uns8 i;
	for (i=0; i<50; i++) {
		uns8 c;
		c = 'A' + i;
		if (!isupper(c))
			c = c - 'Z' + 'a';
		writeToRAM(bufferRF + 3 + i, c);	
	}
	bufferRF[3+49] = '#';
//	writeToRAM(bufferRF + 3 + 49, '#');
	DLEN = 2;
	DLEN = 53;
	PIN = 0x80;
	setNodeMode();	
	RFTXpacket();

	clrwdt();

}


/* common functions */
#include "common.c"
