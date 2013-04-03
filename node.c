/* Coordinator
 *
 */

#include <template-basic.h>
// *********************************************************************
#include "header.h"

#pragma packedCdataStrings 0
#pragma cdata[__EEAPPINFO] = "We Love IQRF!*******++++++++++xx"



/* Some EEPROM - related defines */
#define MAGIC	0x5A

/* EEPROM memory. 0xA0 is first byte available for coordinator */
#define E2_MAGIC	0xA0
#define E2_BONDED	0xA1

#define MAX_SPI_PACKET_DATA	39


void APPLICATION()
{
	uns8 state;
	uns8 i;

	setNodeMode();
	state = STATE_INIT;
	i=0;
	while(1) {

		switch(state){
			case STATE_NOOP:
				break;
			case STATE_INIT:
				state = STATE_BOND;
				removeBond();
				break;
			case STATE_BOND:
				pulseLEDR();
				if (bondRequest())
					state = STATE_WORK;
				else
					setNodeMode();		// must be restored after 'bondRequest()'
				break;
			case STATE_WORK:
				if (RFRXpacket()) {
					if (_NTWPACKET == 0)
						pulseLEDR();
					else
						pulseLEDG();
/*
					if ((bufferRF[0] & 0xF0) == RF_SYNC)
						pulseLEDG();
					else
						*/
//						pulseLEDG();
					
				}
				break;

			default:
				state = STATE_INIT;
		}
		clrwdt();
	}
}

