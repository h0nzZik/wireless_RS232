#ifndef HEADER_H
#define HEADER_H

/* see the manual */
#define SPI_MAX_PACKET_SIZE	41
#define RF_MAX_PACKET_SIZE	64

/* Both SPI_REP_RECV and SPI_CMD_SEND packets
 * have 2 bytes header, so maximum data size is smaller */
#define SPI_MAX_DATA	(SPI_MAX_PACKET_SIZE - 2)

/* RF_DATA packet has 1 byte long header */
#define RF_MAX_DATA	(RF_MAX_PACKET_SIZE - 1)

/***********************************************************************
 * < This part of header is intended for coordinator/node only,
 * not for an external device >
 */
#ifdef __FOR_DEVICE
#include <template-basic.h>

#define SPI_WAIT_MS	5
#define SPI_WAIT_ITER	200

/**
 * Internal state of a device
 * 
 */

struct istat {
	/* coord == 0 -> node,
	 * coord == 1 -> coordinator
	 */
	uns8 coord:1;
	uns8 bonded:1;
	uns8 ack:1;
	
	/* state number (see STATE_*) */
	uns8 state;
	
	/* Change state?
	 * cstat == 0 -> nothing,
	 * cstat != 0 -> state = cstat;
	 */
	uns8 chstat;

	/**
	 * Packet Identification
	 * This variable is increased after each data packet is sent.
	 * This value will also be used as PID field in NTWINFO block.
	 */
	uns8 pid;
	
	/**
	 * 16 is max 
	 * 
	 */
	uns8 last_pid[2];
};

extern struct istat dev;

/* some common functions (see common.c) */
void sync_blink(void);
uns8 receive_packet(void);
uns8 check_spi();

/* common inline functions */
inline void common_init(void);
inline void change_state(void);
inline void eeprom_load(void);
inline void eeprom_save(void);

#endif /* __FOR_DEVICE */
/***********************************************************************
 * </ Only for device >
 */


/* Byte 0 - Commands */


#define RF_BOND	0x10
#define RF_SYNC	0x20
#define RF_DATA	0x30
#define RF_ACK	0x40

/**
 * Wireless comunication
 *
 * * RF_ACK
 * @flag F_ACK		packet was received and passed to end device
 * @flag F_NACK		packet was received and lost
 *
 *
 */


/**
 * Device is designed as a state machine.
 * After reset device is in STATE_NOOP or STATE_WORK.
 *
 * List of internal states:
 * STATE_NOOP	Device is doing almost nothing, but SPI is running.
 * 				This is the default state.
 * 				
 * STATE_INIT	Device is initializing itself.
 * 				If device is switched into this state, device is removed
 * 				from the network (and network is lost in most cases).
 * 				State is immediately changed to STATE_BOND.
 * 				
 * STATE_BOND	Device is bonding itself to network.
 * STATE_WORK	device is working and data can be passed through it
 */

/* List of internal states */
#define STATE_NOOP	0x00
#define STATE_INIT	0x10
#define STATE_BOND	0x20
#define STATE_WORK	0x30
#define STATE_SLEEP	0x40	// FIXME: not implemented yet
#define STATE_ERROR	0x50

/**
 * SPI communication protocol
 * * * * * * * * * * * * * * *
 * TR module communicates with external circuits using SPI. For better
 * background understanding, see User's Guide [1].
 * TR module works only as a slave device. Communication is always
 * initiated by master, so master is supposed to periodically
 * check module's SPI status (see [1]).
 * If module is in communication mode, master can send one of the commands
 * (SPI_CMD_*) below. In general, master should wait for a reply after it.
 * Sometimes happens that SPI data can be divided into two (or more) separate packets.
 * In that case TR module sends a reply only after the last packet.
 * If the command processing is successfull, module sends SPI_REP_OK packet,
 * if an error occurs than module send SPI_REP_ERR packet.
 * 
 * 
 * 
 * 
 * After reset, TR module sends master an SPI_REP_INFO packet, so it can know
 * whether module is bonded and working or not.
 * 
 * $ - packet length
 *
 * Packet format:
 * * * * * * * * *
 * SPI[0] & 0xF0	SPI_CMD_* or SPI_REP_*
 * SPI[0] & 0x0F	Flags
 * SPI[1:$]		Parameters and/or Data
 * SPI_CMD_*		= commands send by master
 * SPI_REP_*		= replies
 *
 * Supported commands:
 * * * * * * * * * * *
 * * SPI_CMD_CHSTAT	Change internal state.
 * @param SPI[1]	Value to change state to
 * Comment:	This is primary intended for initializing the device after first start.
 *
 * * SPI_CMD_SEND	Sends data through RF. Only on STATE_WORK
 * @flag  RF_NETWORK	Send data to IQMesh network if 1, use P2P mode otherwise
 * @flag  F_COMPLETE	This is the last packet fragment.
 * 						If this flag is not set, device is waiting
 * 						for next SPI_CMD_SEND packet. Data from the packet
 * 						are appended after previous fragment. 						
 * @param SPI[1]	Destination address
 * @param SPI[2:$]	Data fragment to send
 * Comment:	Device sends the data to adressee. If device receives
 * positive acknowledge (or acknowledge is off), it replies with SPI_REP_OK.
 * Otherwise an error is signaled using SPI_REP_ERR reply.
 *
 * * SPI_CMD_WRU	Who are you?
 * Comment:	Module will send SPI_REP_WHOAMI reply.
 *
 * Implemented replies:
 * * * * * * * * * * * *
 * * SPI_REP_RECV	Data from wireless nework. SPI[1:$] = data
 * @flag  F_NETWORK	Data came from IQMesh network if 1, P2P mode used otherwise.
 * @flag  F_COMPLETE	This is the last fragment of the packet.
 * @param SPI[1]	Address
 * @param SPI[2:$]	Data
 * Comment:	If F_COMPLETE flag is not set, device will send next fragment
 * in next SPI_REP_RECV reply.
 *
 * * SPI_REP_ERR	Something went wrong.
 * @param SPI[1]	Error number
 * @param SPI[2:$]	Additional information
 * Error number can be one of the following:
 * ERR_STATE		
 * ERR_PACKET_LOST	Packet has been probably lost
 * @param SPI[2]	LOST_NO_ACK or LOST_NACK or LOST_SAME
 * 
 * * SPI_REP_OK		Everything is OK
 * @param SPI[1:$]	whatever
 *
 * * SPI_REP_INFO
 * @param SPI[1]	flags INFO_COORD and/or INFO_BONDED
 * @param SPI[2]	network address
 * @param SPI[3]	internal state
 * @param SPI[4]	current packet ID
 * 
 * Error codes:
 * ERR_STATE		Required operation is not supported in this state
 * 
 * ERR_UNKNOWN_CMD	Unknown command
 * @param SPI[2]	first byte of command (old SPI[0])
 * 
 * ERR_PACKET_LOST	Reply to the SPI_CMD_SEND command. Data are probably lost.
 * @param SPI[2]	Reason. Can be one of LOST_NO_ACK or LOST_NACK or LOST_SAME.
 */

/**
 * SPI commands
 */
#define SPI_CMD_CHSTAT	0x10
#define SPI_CMD_SEND	0x20
//#define SPI_CMD_MORE	0x30
#define SPI_CMD_WRU	0x40



/**
 * SPI replies
 */
#define SPI_REP_ERR	0x10
#define SPI_REP_RECV	0x20
#define SPI_REP_OK	0x30
#define SPI_REP_WHOAMI	0x40



/**
 *  for INFO reply SPI[1]
 */
#define INFO_BONDED	0x01
#define INFO_COORD	0x02


/**
 * Error codes
 */
#define ERR_STATE	0x01
#define ERR_UNKNOWN_CMD	0x02
#define ERR_PACKET_LOST	0x04



/**
 * Additional flags
 */

/* use with SPI_CMD_SEND */
#define F_NETWORK	0x01
#define F_COMPLETE	0x02

/* use with RF_ACK */
#define F_ACK		0x01
#define F_NACK		0x02
#define F_SAME		0x04

/* use with ERR_PACKET_LOST */
#define LOST_NO_ACK	0x01
#define	LOST_NACK	0x02
#define LOST_SAME	0x04


/**
 * EEPROM
 * 
 */


/* Some EEPROM - related defines */
#define MAGIC	0x76

/* EEPROM memory. 0xA0 is first byte available for coordinator */
#define E2_MAGIC	0xA0
#define E2_BONDED	0xA1


/**
 * References:
 * [1]	SPI specification www.iqrf.org/weben/downloads.php?id=85
 * 
 * 
 */


#endif
