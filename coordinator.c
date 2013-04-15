/* Coordinator
 *
 */

#ifndef __FOR_DEVICE
#define __FOR_DEVICE
#endif

#include "header.h"


#pragma packedCdataStrings 0
#pragma cdata[__EEAPPINFO] = "We Love IQRF!*******++++++++++xx"



/* how many nodes do we support? */
#define NODES_MAX	1


#define MAX_SPI_PACKET_DATA	39



struct istat dev;


/* this must be the first */
void APPLICATION()
{

	
	uns8 nodes = 0;
	uns8 nodes_max;
	uns8 x;
	
	setCoordinatorMode();
	enableSPI();
	dev.coord = 1;

	/* Main loop */

#if 1
	dev.state = STATE_NOOP;
	while (1) {
		/**
		 * Check SPI.
		 *
		 */
		clrwdt();
		check_spi();

		/******** State machine ********/


		switch (dev.state) {
			case STATE_NOOP:

				break;
			case STATE_INIT:
				_LEDR = 0;
				_LEDG = 0;
				clearAllBonds();
				enableSPI();
				startSPI(0);
				dev.chstat = STATE_BOND;
				nodes = 0;
				break;

			case STATE_BOND:
				pulseLEDR();
				pulseLEDG();

				/* send P2P broadcast info packet */
				PIN = 0x00;
				DLEN = 1;
				writeToRAM(bufferRF + 0, RF_BOND);
				RFTXpacket();	
				nodes += bondNewNode(0);
				/* change dev.state to 'work' */
				if (nodes >= NODES_MAX)
					dev.chstat = STATE_WORK;
				break;
			case STATE_WORK:
				/* blinking and syncing */
				sync_blink();
				uns8 rv = receive_packet();

				if (rv == 0xFF) {
					/* perform some system/related stuff */

				}

				break;
			case STATE_ERROR:
				_LEDR = 1;
				_LEDG = 1;

				break;
				
			default: /* some error */
				dev.chstat = STATE_INIT;
		}

		/**
		 * Change dev.state and save it to EEPROM
		 *
		 */

		if (dev.chstat) {
			dev.state = dev.chstat;
			dev.chstat = 0;
#if 0
			if (dev.state != STATE_WORK)
				eeWriteByte(E2_BONDED, 0);
			else
				eeWriteByte(E2_BONDED, 1);
#endif

			eeprom_save();

		}

		clrwdt();
	}
#endif
}






void sync_blink(void)
{
	clrwdt();
	if (isDelay() == 0) {
		startDelay(200);
		pulseLEDG();

		/* send IQMESH broadcast sync packet */
		PIN = 0x80;
		/* <test> */
		setCoordinatorMode();
		/* </test> */
		DLEN = 1;
		RX = 0xFF;
		writeToRAM(bufferRF + 0, RF_SYNC);
		RFTXpacket();
	}
}

/* include common.c here */
#include "common.c"

