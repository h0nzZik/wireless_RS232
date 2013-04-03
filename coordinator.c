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

/* how many nodes do we support? */
#define NODES_MAX	1


#define MAX_SPI_PACKET_DATA	39


void APPLICATION()
{
	uns8 nodes = 0;
	uns8 nodes_max;
	uns8 state;
	uns8 x;
	uns8 change_state = 0;
	
	setCoordinatorMode();
	enableSPI();
	state = STATE_INIT;

#if 0
	/* try to load configuration */
	x = eeReadByte(E2_MAGIC);
	if (x == MAGIC) {
		x = eeReadByte(E2_BONDED);
		if (x == 1)
			state = STATE_WORK;
	}
#endif

	/* Main loop */


	state = STATE_NOOP;
	while (1) {
		/**
		 * Check SPI.
		 *
		 */
		clrwdt();
		if (getStatusSPI() == 0  && _SPIRX) {
			pulseLEDG();
			uns8 reply_len = 0;

			/* what to do ? */
			switch (bufferCOM[0] & 0xF0) {
				case SPI_CMD_CHSTAT:
					pulseLEDG();
					change_state = bufferCOM[1];
					break;

				case SPI_CMD_SEND:
					/* error handling */
					if (state != STATE_WORK) {
						reply_len = 2;
						bufferCOM[0] = SPI_REP_ERR;
						bufferCOM[1] = ERR_STATE;
						break;

					}
					/* Copy data to send to  */
					copyMemoryBlock(bufferCOM+1, bufferRF+2, SPIpacketLength - 2);
					copyBufferCOM2RF();
					writeToRAM(bufferRF+0, RF_DATA);

					/* IQMesh or P2P ? */
					if (bufferCOM[0] & F_NETWORK)
						PIN = 0x80;
					else
						PIN = 0x00;

					DLEN = SPIpacketLength - 1;
					RX = bufferCOM[1];
					RFTXpacket();
					break;

				default:
					/* an error */


			}


			/* reply */
			startSPI(reply_len);
			clrwdt();
		}

		/******** State machine ********/


		switch (state) {
			case STATE_NOOP:

				break;
			case STATE_INIT:
				clearAllBonds();
				enableSPI();
				startSPI(0);
				change_state = STATE_BOND;
				nodes = 0;
				break;

			case STATE_BOND:
				pulseLEDR();

				/* send P2P broadcast info packet */
				PIN = 0x00;
				DLEN = 1;
				writeToRAM(bufferRF + 0, RF_BOND);
				RFTXpacket();	
				nodes += bondNewNode(0);
				/* change state to 'work' */
				if (nodes >= NODES_MAX)
					change_state = STATE_WORK;
				break;
			case STATE_WORK:
				/* blinking and syncing */
				if (isDelay() == 0) {
					startDelay(200);
					pulseLEDG();

					/* send IQMESH broadcast sync packet */
					PIN = 0x80;
					DLEN = 1;
					RX = 0xFF;
					writeToRAM(bufferRF + 0, RF_SYNC);
					RFTXpacket();
				}

				/* Try to receive something */
				if (RFRXpacket()) {
					uns8 head;
					uns8 bytes;
					bit fraction;

					stopSPI();
					/* network packet? */
					head = SPI_REP_RECV;
					if (_NTWPACKET == 1)
						head |= F_NETWORK;

					/* must it be splitted? */
					bytes = DLEN - 1;
					if (bytes > MAX_SPI_PACKET_DATA) {
						bytes = MAX_SPI_PACKET_DATA;
						fraction = 1;
					}
					else {
						head |= F_COMPLETE;
						fraction = 0;
					}

					/* send it */
					copyMemoryBlock(bufferRF + 1, bufferCOM + 2, bytes);
					writeToRAM(bufferCOM + 0, head);
					writeToRAM(bufferCOM + 1, TX);
					startSPI(bytes + 2);


					if (fraction) {
						/* wait for SPI completetion for 1s */
						uns8 d;
						d = 100;
						while (d && getStatusSPI()) {
							waitMS(10);
							d--;
						}
						
						/* can we send the data */
						if (getStatusSPI() == 0) {
							stopSPI();
							bytes = DLEN - 1 - MAX_SPI_PACKET_DATA;
							/* copy remaining data */
							copyMemoryBlock(bufferRF + 1 + MAX_SPI_PACKET_DATA,
									bufferCOM+2, bytes);
							head = SPI_REP_RECV | F_COMPLETE;
							if (_NTWPACKET)
								head |= F_NETWORK;
							bufferCOM[0] = head;
							bufferCOM[1] = bytes;
							startSPI(2+bytes);



						}



					}



				}

				break;
			default: /* some error */
				change_state = STATE_INIT;
		}

		/**
		 * Change state and save it to EEPROM
		 *
		 */

		if (change_state) {
			state = change_state;
			change_state = 0;
			if (state != STATE_WORK)
				eeWriteByte(E2_BONDED, 0);
			else
				eeWriteByte(E2_BONDED, 1);
		}

		clrwdt();
	}
}

