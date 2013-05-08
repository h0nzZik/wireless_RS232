/*
 * Node
 *
 */

#ifndef __FOR_DEVICE
#define __FOR_DEVICE
#endif

#include "header.h"

#if 0
#pragma packedCdataStrings 0
#pragma cdata[__EEAPPINFO] = "We Love IQRF!*******++++++++++xx"
#endif


#define isupper(x) (((x) >= 'A') && ((x) <= 'Z'))
struct istat dev;

void send_dummy();


void APPLICATION()
{
	/* node */
	dev.coord = 0;
	common_init();

	while(1) {
		clrwdt();

		/* reset it all */
		if (buttonPressed) {
			waitDelay(1000);
			if (buttonPressed)
				dev.chstat = STATE_INIT;
			clrwdt();
			continue;
		}


		switch(dev.state){
			case STATE_NOOP:
				pulseLEDR();
				waitDelay(30);
				pulseLEDG();
				waitDelay(60);
				break;
			case STATE_INIT:
//				dev.bonded = 0;
				dev.chstat = STATE_BOND;
				removeBond();
				_LEDG=1;
				break;
			case STATE_BOND:
				if (bondRequest()) {
//					dev.bonded = 1;
					dev.chstat = STATE_WORK;
					_LEDG = 0;
					_LEDR = 0;
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

		check_spi();
		change_state();
		clrwdt();
	}
}
#if 0
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
#endif

/* common functions */
#include "common.c"
