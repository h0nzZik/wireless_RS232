#ifndef HEADE_RH
#define HEADER_H


/**
 * RF Communication
 *
 *
 */

/* Byte 0 - Commands */
//#define CMD_DISCONNECT	0
#define RF_SYNC	0x20

#define RF_BOND	0x10
#define RF_DATA	0x30

/**
 * Coordinator is designed as a state machine.
 * State diagram:
 *
 * RESET 	= e2prom =>	STATE_INIT
 * RESET 	= e2prom =>	STATE_WORK
 * STATE_INIT	= auto   =>	STATE_BOND
 * STATE_BOND	= limit  =>	STATE_WORK
 * STATE_*	= SPI    =>	STATE_*
 *
 *
 * List of internal states:
 * STATE_INIT	device is initializing itself (e.g after reset)
 * STATE_BOND	device is being bonded
 * STATE_WORK	device is working and data can be passed through it
 */

/* List of internal states */
#define STATE_NOOP	0x00
#define STATE_INIT	0x10
#define STATE_BOND	0x20
#define STATE_WORK	0x30
#define STATE_SLEEP	0x40	// FIXME: not implemented yet

/*
 * SPI communication protocol
 * * * * * * * * * * * * * * *
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
 *
 * * SPI_CMD_SEND	Sends data through RF. Only on STATE_WORK
 * @flag  RF_NETWORK	Send data to IQMesh network if 1, use P2P mode otherwise	
 * @param SPI[1]	Destination address
 * @param SPI[2:$]	Data to send
 *
 * Implemented replies:
 * * * * * * * * * * * *
 * * SPI_REP_RECV	Data from wireless nework. SPI[1:$] = data
 * @flag  F_NETWORK	Data came from IQMesh network if 1, P2P mode used otherwise.
 * @flag  F_COMPLETE	This was the last part of the packet
 * @param SPI[1]	Address
 * @param SPI[2:$]	Data
 *
 * * SPI_REP_ERR	Something went wrong.
 * @param SPI[1]	Error number
 *
 * Error codes:
 * ERR_STATE		Required operation is not supported in this state
 * ERR_UNKNOWN_CMD	Unknown command
 */


/* Change coordinator's state. SPI[1] = STATE_* */
#define SPI_CMD_CHSTAT	0x10
/* Send data to network. SPI[1:$] = data to send. Only on STATE_WORK */
#define SPI_CMD_SEND	0x20

/* some flags */
#define F_NETWORK	0x01
#define F_COMPLETE	0x02

/* Some error occured. SPI[1] = error number */
#define SPI_REP_ERR	0x10
#define SPI_REP_RECV	0x20

#define ERR_STATE	0x01
#define ERR_UNKNOWN_CMD	0x02

#endif
