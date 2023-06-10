/*
 * 10-18-2021  Tom McDermott, N5EG
 *
 *
 * Copyright 2021, Thomas C. McDermott.
 * Licensed under GNU Public License, version 2 or later.
 *
 * Data Engine to Local Host (DE_LH) Protocol.
 * This implements structures & definitions for the
 * host (DE) side of the LH_DE protocol. See the
 *
 * Local_Host_to_Data_Engine_Protocol_V1.2.pdf
 *
 * document at https://tangerinesdr.com
 * on the Working Documents page.
 *
 */


#ifndef __DELHPROTOCOL_H__
#define __DELHPROTOCOL_H__

/* Nichestack definitions */
#include "ipport.h"
#include "tcpport.h"


/* Each UDP channel created is entered in the table.
 * client_ip - the other computer.
 * host_ip   - us.
 *
 * Per client, there are:
 * 1 - Provisioning channel.
 * 1 - Configuration channel.
 * N - Data channels (probably 8 max per client).
 *
 * The receive socket for discovery is port 1024, which
 * is shared among all clients known and unknown.
 * Other channels create new client IP/Port numbers.
 */
#define MAXUDPCHAN	21		// adjust as needed

struct UDPCHAN {
	int fd;			// Our receive file descriptor. UDP doesn't have one for Tx.
					// We need to explicitly 'sendto()' the client using the client_ip.
	char chantype;	// P = Provisioning, C = Configuration, T = Data, D = Discovery, I = Inactive
	struct sockaddr_in host_ip;
	struct sockaddr_in client_ip;
	int inac_count;		// Inactivity countdown. Expired when zero (or negative).
	};

void initChan();					// Initialize the table
int addChan(struct UDPCHAN *);		// Add a connection to the table
int delChan(struct UDPCHAN *);		// Delete a connection from the table
int findfdChan(int);				// Find a connection in the table
int buildaddChan(int, char, struct sockaddr_in *, struct sockaddr_in *);	// make UDPCHAN struct then add
int findExistingChan(char, struct sockaddr_in *);	// see if we have an existing channel
int allListeners(int *);			// return array of active fd's to listen for


/* Process received channel packet.
 * Discovery - process discovery command
 * Provisioning - process provisioning command
 * ConfigChan - process configuration command
 * TrafficChan - process data - likely gets moved to hardware
 */

int ProcessRxPacket(int);	// Received a packet with index to Connection table
int DiscChanMessage(int);	// Discovery
int ProvChanMessage(int, int);	// Provisioning
int ConfigChanMessage(int, int); // Configuration
int TrafficChanMessage(int, int); // Traffic (data) - this eventually should be handled by hardware.


/*
 * Process Provisioning Channel commands
 */

int ProvisioningLEDCommand(int, int);  // Process the LH_DE LED command
int ProvisioningModuleRegisterCommand(int, unsigned char [], char []);  // Process the LHDE Module read/write command


// Holds parsed provisioning channel command
struct PARSEDCMD {
	char cmd[24];				// The command type (i.e. "MW"or "MR")
	unsigned long module;		// Module number
	unsigned long interface;	// Interface number on that module
	unsigned long address;		// Device address, sometimes unused
	unsigned long data;			// Device write data, sometimes unused
};

/*
 * Handlers for the peripheral devices.
 */

int PCF8574(struct PARSEDCMD *, char resultmessage[], int itfcindex); 		// Handler for TI PCF8574 I2C Latch
int ATECC608(struct PARSEDCMD *, char resultmessage[], int itfcindex); 		// Handler for ATECC 608A ID PROM
int SI5345(struct PARSEDCMD *, char resultmessage[], int itfcindex); 		// Handler for Silicon Labs 5345 synthesizer
int ZEDF9T(struct PARSEDCMD *, char resultmessage[], int itfcindex); 		// Handler for F9T GPS
int AD9648(struct PARSEDCMD *, char resultmessage[], int itfcindex); 		// Handler for ADC AD9648 SPI interface


/* Structure to map Module# / Slot# to a particular interface and particular device type.
 * Need to build a table holding all the valid defined interfaces. Each module needs a unique
 * module number, and each device type, even if on the same I2C wires needs a quique
 * interface number.
 *
 * NOTE:    BASE address #defines from the BSP package / system.h include file.
 */

#define MAXITFC 6				// The DEV_KIT + DataEngine Adaptor + Receiver test system has 6 (I2C+SPI) interfaces
#define CLOCKSYNTHITFC	0		// Interface number of the clock module synthesizer (needed for initialization)
#define GPSINTFC 1				// Interface number of the GPS ZEDF9T (needed for initialization)
#define ADCINTFC 5				// Interface number of the ADC SPI interface (needed forinitialization)

struct ITFCMAP {
	unsigned int module;	// Module number
	unsigned int interface;	// Interface number on that module
	unsigned long base;		// Base address of the interface's control registers
	char name[24];			// name of the device (e.g. "/dev/ic2_ckm_c0")
	int (*handler)(struct PARSEDCMD *, char *, int);	// Pointer to function to handle the specific device type
};

extern struct ITFCMAP ItfcTable[MAXITFC];   // So other modules can find it

int FindItfcIndex(int, int);	// find the index in the ITFCMAP table given module# and slot#
void initItfcMap();				// Initialize the Interface Map table

#endif /* DELHProtocol.h */
