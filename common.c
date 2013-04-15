/**
 * File common.c
 *
 */
#ifndef __FOR_DEVICE
#define __FOR_DEVICE
#endif
#include "header.h"

/**
 * Receive RF packet.
 * If packet is intended for external device (see header.h),
 * SPI is stopped (so current packet is lost if any) and data are sent to the master using SPI.
 *
 * @return	Zero if packet has been processed sucessfully, first byte of data otherwise.
 */
uns8 receive_packet(void)
{
	clrwdt();
	/* nothing much */
	if (RFRXpacket() == 0)
		return 0;
	/* ignore empty packets */
	if (DLEN == 0)
		return 0;
	/* ignore non-network packets */
	if (_NTWPACKET == 0)
		return 0;
	/* filter packets without RF_DATA header */
	if (bufferRF[0] != RF_DATA)
		return bufferRF[0];

	uns8 remain;	/* remaining bytes in bufferRF */
	uns8 offset;
	uns8 addr;

	remain = DLEN-1;
	offset = 1;

	_LEDR = 1;

	stopSPI();
	do {
		uns8 bytes;
		clrwdt();
		bytes = remain;
		if (bytes > MAX_SPI_PACKET_DATA)
			bytes = MAX_SPI_PACKET_DATA;
		remain -= bytes;

		uns8 head;
		head = SPI_REP_RECV;
		if (!remain)
			head |= F_COMPLETE;
		if (_NTWPACKET)
			head |= F_NETWORK;


		/* Create packet. See header.h */

		bufferCOM[0] = head;
		bufferCOM[1] = addr;
		copyMemoryBlock(bufferRF + offset, bufferCOM+2, bytes);

		/* problem */
		if (2+bytes > 41) {
			return 0xFF;
		}
		startSPI(2+bytes);

		offset += bytes;
		/* wait for finishing the transfer */
		if (remain) {
			uns8 d;
			d = 100;
			do  {
				clrwdt();
				waitMS(5);	// FIXME: wait only 100 ms (slow libusb)
				d--;
			} while(d  && !(getStatusSPI() == 0  && _SPIRX));

			if (d == 0 ||  !(bufferCOM[0] == (SPI_CMD_MORE | F_YES))) {
				stopSPI();
				startSPI(0);
				break;
			}
		}


	} while (remain != 0);

	_LEDR = 0;

	/* acknoledge */
	if (_ACKF) {
		

	}
	return 0;
}


uns8 check_spi()
{
	uns8 reply_len;

	
	clrwdt();
	/* SPI is busy */
	if (getStatusSPI())
		return 0;
	/* nothing received */
	if (_SPIRX == 0)
		return 0;


	/* invalid CRC */
	if (_SPICRCok == 0) {
		startSPI(0);
		return 0;
	}
	pulseLEDR();
	reply_len = 0;
	switch (bufferCOM[0] & 0xF0) {
		/* change state */
		case SPI_CMD_CHSTAT:
			pulseLEDG();
			pulseLEDR();
			dev.chstat = bufferCOM[1];
			break;

		/* send some data */
		case SPI_CMD_SEND:
			/* error handling */
			if (dev.state != STATE_WORK) {
				reply_len = 2;
				bufferCOM[0] = SPI_REP_ERR;
				bufferCOM[1] = ERR_STATE;
				break;
			}
			/* Copy data to send to  */
			copyMemoryBlock(bufferCOM+2, bufferRF+1, SPIpacketLength - 2);
			bufferRF[0] = RF_DATA;
//			writeToRAM(bufferRF+0, RF_DATA);

			/* IQMesh or P2P ? */
			if (bufferCOM[0] & F_NETWORK) {
				PIN = 0x80;
				if (dev.coord)
					setCoordinatorMode();
				else
					setNodeMode();
			} else {
				PIN = 0x00;
				setNonetMode();
			}

			/* send it */
			DLEN = SPIpacketLength - 1;
			RX = bufferCOM[1];
			clrwdt();
			RFTXpacket();
			break;

		/* who we are? */
		case SPI_CMD_WRU:
			bufferCOM[0] = SPI_REP_WHOAMI;
			bufferCOM[1] = 0;
			if (dev.coord)
				bufferCOM[1] |= 1;
			reply_len = 2;
			break;
			
		default:
			/* an error */
			bufferCOM[0] = SPI_REP_ERR;
			bufferCOM[1] = ERR_UNKNOWN_CMD;
			reply_len = 2;


	} /* switch */
	/* send a reply */
	if (reply_len == 0) {
		bufferCOM[0] = SPI_REP_OK;
		bufferCOM[1] = 'K';
		reply_len = 2;

	}
	startSPI(reply_len);
	clrwdt();
	return 0;
}
#if 0
/* Some EEPROM - related defines */
#define MAGIC	0x5A

/* EEPROM memory. 0xA0 is first byte available for coordinator */
#define E2_MAGIC	0xA0
#define E2_BONDED	0xA1


void eeprom_load(void)
{
	uns8 x;
	x = eeReadByte(E2_MAGIC);
	if (x == MAGIC) {
		x = eeReadByte(E2_BONDED);
		if (x == 1)
			dev.state = STATE_WORK;
	}


}
#endif

void eeprom_save(void)
{

	;
}
