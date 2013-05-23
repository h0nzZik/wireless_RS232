/**
 * File common.c
 *
 */
#ifndef __FOR_DEVICE
#define __FOR_DEVICE
#endif
#include "header.h"

inline void set_net_mode()
{
	PIN = 0x80;
	if (dev.coord) {
		setCoordinatorMode();
	} else {
		setNodeMode();
	}
}


inline void send_info(void)
{
	bufferCOM[0] = SPI_REP_WHOAMI;
	/* coordinator or node? */
	bufferCOM[1] = 0;
	if (dev.coord)
		bufferCOM[1] |= INFO_COORD;
	if (dev.bonded)
		bufferCOM[1] |= INFO_BONDED;

	/* address */
	getNetworkParams();
	bufferCOM[2] = param2;

	/* state */
	bufferCOM[3] = dev.state;
	
	/* packet identification */
	bufferCOM[4] = dev.pid;
	
	startSPI(5);
}

inline void eeprom_load(void)
{
	uns8 x;
	x = eeReadByte(E2_MAGIC);
	if (x != MAGIC)
		return;
	x = eeReadByte(E2_BONDED);
	if (x == 1) {
		dev.state = STATE_WORK;
		dev.bonded = 1;
	} else {
		dev.state = STATE_NOOP;
		dev.bonded = 0;
	}
}

inline void eeprom_save(void)
{
	/* store magic number */
	eeWriteByte(E2_MAGIC, MAGIC);

	/* can I work? */
	if (dev.bonded == 1) {
		eeWriteByte(E2_BONDED, 1);
	} else {
		eeWriteByte(E2_BONDED,0);
	}
}

/**
 * Changes the state and saves some info into eeprom
 */
inline void change_state(void)
{
		if (dev.chstat) {
			dev.state = dev.chstat;
			dev.chstat = 0;
			if (dev.state == STATE_INIT)
				dev.bonded = 0;
			if (dev.state == STATE_WORK)
				dev.bonded = 1;
			eeprom_save();
		}
}

/**
 * Wait for SPI is not busy
 * @param data	1 -> wait for incoming data
 * 				0 -> wait only for finishing the transmit
 */

inline uns8 wait_spi(uns8 data)
{
	uns8 d;
	d = SPI_WAIT_ITER;
	while((d) && (getStatusSPI() || (data && (_SPIRX == 0)))) {
		waitMS(SPI_WAIT_MS);
		clrwdt();
		d--;
	}
	if (getStatusSPI())
		return 1;
	return 0;
}

/**
 * Sends an acknowledge packet
 */
inline void reply_ack(uns8 flags)
{
	if (_ACKF == 0)
		return;

	set_net_mode();
	RX = TX;
	bufferRF[0] = RF_ACK | (flags & 0x0F);
	DLEN = 1;

	_ACKF = 0;
	RFTXpacket();
	clrwdt();
	return;
}


/**
 * Some initialization stuff
 */

inline void common_init(void)
{
	
	/* clear leds after start */
	_LEDG = 0;
	_LEDR = 0;
	
	/* watchdog expired - some error happened */
	if (TO == 0) {
		_LEDR = 1;
		_LEDG = 1;
		while(1)
			clrwdt();		

	}
	
	/* prepare dev structure */
	dev.ack = 1;
	dev.bonded = 0;
	dev.last_pid[0] = 0;
	dev.last_pid[1] = 0;
	dev.state = STATE_NOOP;
	dev.chstat = 0;
		
	/* Stay in RX mode && wait packet end */
	setRFmode(0b11000000);

	eeprom_load();
	enableSPI();
	
	/*
	if (dev.coord) {
		setCoordinatorMode();
	} else {
		setNodeMode();
	}
	*/
	set_net_mode();
	send_info();
	clrwdt();
	return;
}


/**
 * Receive RF packet.
 * If packet is intended for external device (see header.h),
 * SPI is stopped (so current packet is lost if any) and data are sent to the master using SPI.
 *
 * @return	Zero if packet has been processed sucessfully, first byte of data otherwise.
 * 1st level
 */
uns8 receive_packet(void)
{
	clrwdt();
//	toutRF = 16;
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
	addr = TX;


	/* FIXME: maximal number of nodes */
	if (addr > 1)
		return 0;

	uns8 last_pid;
	last_pid = readFromRAM(dev.last_pid + addr);
	
	/* remote maybe didn't get acknoledge? */
	if ((_ACKF == 1) && (last_pid != 0) && (last_pid == PID)) {
		reply_ack(F_SAME);
		return 0;
		
	}

	/*
	 * SPI is too busy to do something
	 */
	if (wait_spi(0) || _SPIRX) {
		reply_ack(F_NACK);
		return 0;
	}
	/*
	 * So protect it
	 */
	stopSPI();

	
//	_LEDR = 1;





	/*
	 * If the packet is big, split it into smaller parts
	 */
	do {
		uns8 bytes;
		clrwdt();
		bytes = remain;
		if (bytes > SPI_MAX_DATA)
			bytes = SPI_MAX_DATA;
		remain -= bytes;

		uns8 head;
		head = SPI_REP_RECV;
		if (!remain)
			head |= F_COMPLETE;
		if (_NTWPACKET)
			head |= F_NETWORK;


		/* Create SPI packet. See header.h */

		bufferCOM[0] = head;
		bufferCOM[1] = addr;
		copyMemoryBlock(bufferRF + offset, bufferCOM+2, bytes);

	
		startSPI(2+bytes);

		offset += bytes;
		
		/* wait for finishing the transfer */
		if (wait_spi(0)) {
			stopSPI();
			startSPI(0);
			reply_ack(F_NACK);
			goto recv_end;
		}

	} while (remain != 0);


	/* Packet was processed successfully
	 * so we can save it's PID*/
	if ((_ACKF == 1) && (PID != 0)) {
		writeToRAM(dev.last_pid + addr, PID);
	}


	/* acknoledge. Use the same PID */
	if (_ACKF) {
		reply_ack(F_ACK);
		goto recv_end;
	}
	
recv_end:

	_LEDR = 0;
	return 0;
}

/**
 * Checks spi
 * * first level of subroutine calling
 */

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

//	pulseLEDR();


	reply_len = 0xFF;

	uns8 cmd;
	cmd = bufferCOM[0] & 0xF0;
	switch (cmd) {

		/**
		 * Change state. No checks are performed,
		 * so if invalid state is set, ...
		 */

		case SPI_CMD_CHSTAT:
			pulseLEDG();
			pulseLEDR();
			dev.chstat = bufferCOM[1];
			goto sw_end;
		
		/**
		 * Who are you?
		 * tell master who I am
		 */
		case SPI_CMD_WRU:
			send_info();
//			pulseLEDR();
			reply_len = 0;	// it was already handled
			goto sw_end;

		/**
		 * SPI_CMD_SEND
		 * Send a packet through RF.
		 * If 'dev.ack' flag is set, wait for acknowledge,
		 * set PID and increase its stored value.
		 */
		case SPI_CMD_SEND:
			uns8 net = 0;	// use IQMESH network?
			uns8 addr;		// who is this packet intended to?

			/* Can not send the packet
			 * because we are on holidays */
			if (dev.state != STATE_WORK) {
				reply_len = 2;
				bufferCOM[0] = SPI_REP_ERR;
				bufferCOM[1] = ERR_STATE;
				goto sw_end;
			}

			/* Copy data to send to  */
			copyMemoryBlock(bufferCOM+2, bufferRF+1, SPIpacketLength - 2);
			bufferRF[0] = RF_DATA;

			/* IQMesh or P2P ? */
			if (bufferCOM[0] & F_NETWORK)
				net = 1;

			if (net == 1) {
				set_net_mode();
			} else {
				PIN = 0x00;
				setNonetMode();
			}

			DLEN = SPIpacketLength - 1;
			addr = bufferCOM[1];
			RX = addr;

			/* a second part? */
			if ((bufferCOM[0] & F_COMPLETE) == 0) {
				startSPI(0);
				if (wait_spi(1)) {
					stopSPI();
					/* some error */
					bufferCOM[0] = SPI_REP_ERR;
					bufferCOM[1] = 'Z';	/* FIXME: */
					reply_len = 2;
					goto sw_end;
				}
				/* We can't send such a big date */
				if (DLEN + SPIpacketLength - 2 > RF_MAX_PACKET_SIZE) {
					bufferCOM[0] = SPI_REP_ERR;
					bufferCOM[1] = ERR_CANNOT_SEND;
					bufferCOM[2] = DATA_TOO_BIG;
					reply_len = 3;
					/* some error */
					goto sw_end;
				}
				
				copyMemoryBlock(bufferCOM+2, bufferRF + DLEN, SPIpacketLength - 2);
				DLEN += SPIpacketLength - 2;
				
			}

			/* Do we require an acknoledge? */
			uns8 ack = 0;
			if (net == 1 && addr != 255 && dev.ack == 1)
				ack = 1;

			/* set packet ID */
			PID = 0;
			if(ack) {
				_ACKF = 1;	// require acknoledge
				PID = dev.pid;
			}
			
			/* send it */
			RFTXpacket();
			clrwdt();

			/* If no acknoledge is required,
			 * we are almost done */
			if (ack == 0) {
				dev.pid++;
				if (dev.pid == 0)
					dev.pid = 1;
				goto sw_end;
			}
			
			/* wait for the acknoledge.
			 * FIXME: Hope the time is enough */
			uns8 d;
			_LEDR = 1;
			for (d=100; d; d--) {
				clrwdt();
				if (!RFRXpacket())
					continue;
				if (TX != addr)
					continue;
				if (PID != dev.pid)
					continue;
				if ((bufferRF[0] & 0xF0) != RF_ACK)
					continue;
				
				/*  Negative acknowledge */
				if ((bufferRF[0] & 0x0F) == F_NACK) {
					bufferCOM[0] = SPI_REP_ERR;
					bufferCOM[1] = ERR_PACKET_LOST;
					bufferCOM[2] = LOST_NACK;
					reply_len = 3;
					goto sw_end;
				}
				
				/* The same packet? */
				if ((bufferRF[0] & 0x0F) == F_SAME) {
					dev.pid++;
					if (dev.pid == 0)
						dev.pid = 1;
					
					bufferCOM[0] = SPI_REP_ERR;
					bufferCOM[1] = ERR_PACKET_LOST;
					bufferCOM[2] = LOST_SAME;
					reply_len = 3;
					goto sw_end;
				}
				/* Positive acknowledge?
				 * -> increment pid,
				 * -> send OK reply */
				if ((bufferRF[0] & 0x0F) == F_ACK) {
					dev.pid++;
					if (dev.pid == 0)
						dev.pid = 1;
					goto sw_end;
				}

			}
			/*  send an error message via SPI */
			bufferCOM[0] = SPI_REP_ERR;
			bufferCOM[1] = ERR_PACKET_LOST;
			bufferCOM[2] = LOST_NO_ACK;
			reply_len = 3;


			goto sw_end;

			
		/**
		 * Some bad command?
		 * so send an error message 
		 */
		default:
			bufferCOM[0] = SPI_REP_ERR;
			bufferCOM[1] = ERR_UNKNOWN_CMD;
			bufferCOM[2] = cmd;
			reply_len = 3;
			goto sw_end;
	} /* switch */
	
sw_end:	
	/* Default reply
	 * Everything is fine, so send OK reply */
	if (reply_len == 0xFF) {
		bufferCOM[0] = SPI_REP_OK;
		bufferCOM[1] = 'K';
		reply_len = 2;

	}
	if (reply_len != 0) {
		startSPI(reply_len);
	}
	clrwdt();
	_LEDR = 0;
	return 0;
}

