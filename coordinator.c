/**
 * Coordinator
 * 
 * Only one node is supported in this time.
 *
 */

/**
 * See "header.h" 
 */
#ifndef __FOR_DEVICE
#define __FOR_DEVICE
#endif
#include "header.h"


// #pragma packedCdataStrings 0
// #pragma cdata[__EEAPPINFO] = "We Love IQRF!*******++++++++++xx"



/* how many nodes do we support? */
#define NODES_MAX	1

struct istat dev;


/* this must be the first */
void APPLICATION()
{	
	uns8 nodes = 0;
	dev.coord = 1;
	common_init();
	
	while (1) {
		clrwdt();


		/******** State machine ********/


		switch (dev.state) {
			case STATE_NOOP:
				pulseLEDG();
				waitDelay(40);
				pulseLEDR();
				waitDelay(70);
				break;

			/**
			 * STATE_INIT
			 * In this state coordinator clears all its bonds
			 * and re-initializes SPI.
			 */
			case STATE_INIT:
				_LEDR = 0;
				_LEDG = 0;
//				dev.bonded = 0;
				clearAllBonds();
				enableSPI();
				startSPI(0);
				dev.chstat = STATE_BOND;
				nodes = 0;
				break;
			
			/**
			 * STATE_BOND
			 * Coordinator is trying to bond some nodes.
			 * Before bonding, RF_BOND packet is send to P2P network,
			 * so that every potential node can deal with it.
			 * After NODES_MAX nodes have been bonded, 
			 * coordinator will change its state to STATE_WORK.
			 * This can be also done manually, using SPI_CMD_CHSTAT command. 
			 */
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
				if (nodes >= NODES_MAX) {
	//				dev.bonded = 1;
					dev.chstat = STATE_WORK;
				}
				break;

			/**
			 * STATE_WORK
			 * Coordinator (or node, respectively) is normally working.
			 * During this state, synchronization packets are being sent
			 * to network once a time, so that every bonded node can know
			 * it is in range.
			 * In this state device (coordinator or node) is receiving
			 * network data packets (RF_DATA) and generating
			 * SPI data packets (see SPI_REP_RECV).
			 * 
			 */
			case STATE_WORK:
				/* blinking and syncing */
				sync_blink();
				uns8 rv = receive_packet();

				if (rv == 0xFF) {
					/* perform some system/related stuff */

				}

				break;
			/**
			 * STATE_ERROR
			 * Something ugly happened, so leds are on.
			 * SPI works but network is ignored.
			 */
			case STATE_ERROR:
				_LEDR = 1;
				_LEDG = 1;

				break;
			
			/**
			 * I hope it never happens
			 */
			default: /* some error */
				dev.chstat = STATE_INIT;
		}

		check_spi();
		change_state();
		clrwdt();
	}
}





uns8 sync_blink_i;
void sync_blink(void)
{
	clrwdt();
//	return;

	sync_blink_i %= 10;
	if (isDelay() == 0) {
		startDelay(150);
		sync_blink_i++;
		sync_blink_i &= 0x03;


		if (sync_blink_i == 0) {
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
}

/* include common.c here */
#include "common.c"

