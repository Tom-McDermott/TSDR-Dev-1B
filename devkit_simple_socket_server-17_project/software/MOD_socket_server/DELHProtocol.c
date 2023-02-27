/*
 * 11-05-2022	Tom McDermott, N5EG
 *
 * Copyright 2021-2022, Thomas C. McDermott.
 * Licensed under GNU Public License, version 2 or later.
 *
 * Implementation of the Data Engine (host) side of
 * the LH_DE protocol for TangerineSDR.
 *
 * This dynamically creates connections as described by
 * the protocol. Future feature: clear connections when
 * they are inactive for a long time.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "system.h"

#include "alt_error_handler.h"
#include "DELHProtocol.h"
#include "ClockModInit.h"

#include "simple_socket_server.h"	// LED Queue

#include "altera_avalon_i2c.h"		// I2C interface
#include "altera_avalon_spi.h"		// SPI interface


/* Allocate the channel table in global memory so
 * the timer task can manipulate the timers in it.
 * The LED command also needs to be in global memory
 * for the same reason.
 */
static struct UDPCHAN Connections[MAXUDPCHAN];
struct ITFCMAP ItfcTable[MAXITFC];
static INT32U LEDCommand;


/* Initialize the connection table,
 * Add a connection to the table,
 * Delete a connection from the table.
 * Find a connection in the table.
 * Build a UDPCONN struct from pieces and add.
 * Return array of all active fd's in the table.
 * fd == -1 implies an empty connection slot.
 *
 * NOTE: fd = 0 could be stdin or stdout ?
 */

void initChan()
{
	for (int i=0; i < MAXUDPCHAN; i++)
	{
		Connections[i].fd = -1;
		Connections[i].chantype = 'I';	// Inactive
	}
};

int addChan(struct UDPCHAN * Conn)
{
	if (Conn->fd < 0)
		return(-2);	// bad fd value to add.

	for (int i=0; i < MAXUDPCHAN; i++)
	{
		if (Connections[i].fd == -1)
		{
			memcpy(&Connections[i], Conn, sizeof(struct UDPCHAN));
			return(i);
		}
	}

	return(-1);  // out of space in the table
};

int delChan(struct UDPCHAN * Conn)
{
	if (Conn->fd < 0)
		return(-2);	// bad fd value to delete.

	for (int i=0; i < MAXUDPCHAN; i++)
	{
		if (Connections[i].fd == Conn->fd)
		{
			Connections[i].fd = -1;
			Connections[i].chantype = 'I';
			return(i);
		}
	}

	return(-1);  // didn't find in the table
};

int findfdChan(int fd)
{
	if (fd < 0)
		return(-2);	// bad fd value to search for.

	for (int i=0; i < MAXUDPCHAN; i++)
	{

		if (Connections[i].fd == fd)
			return(i);  // index of the matching connection
	}

	return(-1);  // didn't find in the table
};

int findExistingChan(char chantype, struct sockaddr_in * client)
{
	// See if we already have a channel to the client IP

	for (int i=0; i < MAXUDPCHAN; i++)
	{
		if ((Connections[i].chantype == chantype)      &&
		   (memcmp(&(Connections[i].client_ip.sin_addr), &(client->sin_addr), sizeof(struct in_addr) ) == 0))
		{
			return(i);	// have existing channel
		}
	}

	return(-1);		// didn't find in the table

}

int buildaddChan(int fd, char type, struct sockaddr_in * host, struct sockaddr_in * client)
{
	struct UDPCHAN conn;

	if (fd < 0)
		return(-2);	// bad fd value

	conn.fd = fd;
	conn.chantype = type;
	memcpy(&conn.host_ip, host, sizeof(struct sockaddr_in));
	memcpy(&conn.client_ip, client, sizeof(struct sockaddr_in));

	return addChan(&conn);
};

int allListeners(int * theList)			// return array of active channels to listen for
{
	int count = 0;

	//memset(theList, -1, MAXUDPCHAN);	// clear the returned list

	for(int i=0; i<MAXUDPCHAN; i++)
		if (Connections[i].fd > 0)		// valid fd
			theList[count++] = Connections[i].fd;

	return count;
};


/*************************************************************************************************
 * Handle received packets according to the type of channel they are received on.
 * Also transmit packets in response as necessary.
 * ***********************************************************************************************/

#define MAXLINE 1500
unsigned char buffer[MAXLINE+1];  // room to hold string terminating \0


int ProcessRxPacket(int index)
{

    int                 len, bufflen;
    struct sockaddr_in  client_addr;

	// Discovery channel has to process the receive buffer itself

	if (Connections[index].chantype == 'D')  // Discovery
		return DiscChanMessage(index);

	// Get the received buffer
    memset(&client_addr, 0, sizeof(struct sockaddr_in));

    len = sizeof(struct sockaddr_in);
	bufflen = recvfrom(Connections[index].fd, (char *)buffer, MAXLINE,
			/*MSG_WAITALL*/ 0, (struct sockaddr *) &client_addr, &len);
	buffer[bufflen] = '\0';

	if (Connections[index].chantype == 'P')  // Provisioning
		return ProvChanMessage(index, bufflen);

	if (Connections[index].chantype == 'C')  // Configuration
		return ConfigChanMessage(index, bufflen);

	if (Connections[index].chantype == 'T')  // Traffic (Data rx'd from Ethernet to be RF transmitted)
		return TrafficChanMessage(index, bufflen);

	return -1;		// unknown or Idle chantype
};


int DiscChanMessage(int index)
{
    /* Received HPSDR discovery request.
     * If it's a new one, create a provisioning channel and reply
     * using it.  Else reply using existing provisioning channel.
     * If it's not a request message, we don't know what to do with it.
     */

    int                 len, n, fd_prov_tx;
    struct sockaddr_in  client_addr, host_addr;

    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    memset(&host_addr, 0, sizeof(struct sockaddr_in));

    /* Discovery reply format:
     * 0xEF 0xFE 0x02 (not sending data yet) 6-byte ourMACaddress code_ver
     * board_id (0x07 from TSDR)
     */
    uint8_t disc_resp[64] = { 0xfe, 0xef, 0x02, 0x00, 0x07, 0xed, 0x2a, 0x22, 0x16, 0x07 };
    memset(&disc_resp[10], 0, 54);  // discovery reply is padded with zeros

    len = sizeof(struct sockaddr_in);
	n = recvfrom(Connections[index].fd, (char *)buffer, MAXLINE,
			/*MSG_WAITALL*/ 0, (struct sockaddr *) &client_addr, &len);
	buffer[n] = '\0';

    /* check for 0xef 0xfe 0x02 signifying discovery request */
	if ((buffer[0] == 0xef) && (buffer[1] == 0xfe) && (buffer[2] == 0x02))
	  printf("Valid OpenHPSDR Discovery request received\n");
	else
		return -1;	// unknown discovery message - throw away

	printf("From: %s   Port: %u\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	printf("Received buffer : 0x%2x 0x%2x 0x%2x 0x%2x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

    // See if we already have a Provisioning Channel open from this client

    int existingIndex = findExistingChan('P', &client_addr);
    if (existingIndex >= 0)		// Have an existing provisioning channel - send a duplicate reply
    {
    	int result = sendto(Connections[existingIndex].fd, (void *)disc_resp, sizeof(disc_resp),
				/*MSG_CONFIRM*/ 0, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in));
    	return result;
    }

    // Don't have an existing channel. Create a new Provisioning channel to send the discovery reply

    if ((fd_prov_tx = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
      alt_NetworkErrorHandler(EXPANDED_DIAGNOSIS_CODE,"[TSDR_task] Provisioning Socket creation failed");
    }

    // Use a new sending socket. sendto implicitly binds a new (random) receive port number

    sendto(fd_prov_tx, (void *)disc_resp, sizeof(disc_resp),
    		/*MSG_CONFIRM*/ 0, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_in));

    len = sizeof(struct sockaddr_in);
    if (getsockname(fd_prov_tx, (struct sockaddr *)&host_addr, &len) == -1)
    {
    	perror("getsockname() error\n");
    	return -1;	// Error
    }
    else
    {
    	printf("Provisioning port number: %d\n", ntohs(host_addr.sin_port));
    	int addresult = buildaddChan(fd_prov_tx, 'P', &host_addr, &client_addr);
    	printf("Result of adding new provisioning channel to table: %u\n", addresult);
    	return addresult;   // return the result of adding the channel to the table
    }
};

int ProvChanMessage(int index, int bufflen)
{

    /* Received a Provisioning Channel message command:
	 * MR - Module Read
	 * MW - Module Write
	 * CC - Create a Config Chan
	 * UC - Uncreate (Delete) a Config chan
	 * XR - Reboot DE
	 * S? - status request
	 * Yn - Turn on LED n.  The DEVKIT has 4 LEDs
	 * Xn - Turn off LED n
	 */

	int result;
	char resultmessage[82];	// 80 + \n + \0

	printf("Received Provisioning Channel Message: %s\n", buffer);

	if (buffer[0] == 'Y' || buffer[0] == 'X')
		result = ProvisioningLEDCommand(index, bufflen);


	if (buffer[0] == 'M')
		result = ProvisioningModuleRegisterCommand(index, buffer, resultmessage);

	if (result < 0)
	{
        sendto(Connections[index].fd, (void *)"NAK\n", 4, 0, (struct sockaddr *) &Connections[index].client_ip, sizeof(struct sockaddr_in));
        return result;
	}

    sendto(Connections[index].fd, (void *)"AK\n", 4, 0, (struct sockaddr *) &Connections[index].client_ip, sizeof(struct sockaddr_in));

    if (result > 0)
        sendto(Connections[index].fd, (void *)resultmessage, strlen(resultmessage), 0, (struct sockaddr *) &Connections[index].client_ip, sizeof(struct sockaddr_in));

	return result;
};

int ConfigChanMessage(int index, int bufflen)
{
    // Received Configuration Channel message

	printf("Received Configuration Channel Message: %s\n", buffer);
	return -3;	// not yet implemented
};

int TrafficChanMessage(int index, int bufflen)
{
    // Received Traffic (Data) Channel message

	printf("Received Traffic Channel Message: %s\n",buffer);
	return -3;	// not yet implemented
};


int ProvisioningLEDCommand(int index, int bufflen)
{
	if (buffer[0] == 'Y')	// Turn on LED0 - 3
	{
		if (buffer[1] == '0')
		{
			LEDCommand = 0x01;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		if (buffer[1] == '1')
		{
			LEDCommand = 0x02;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		if (buffer[1] == '2')
		{
			LEDCommand = 0x04;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		if (buffer[1] == '3')
		{
			LEDCommand = 0x08;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		return -1;	// unknown LED number
	}

	if (buffer[0] == 'X')	// Turn off LED0 - 3
	{
		if (buffer[1] == '0')
		{
			LEDCommand = 0x10;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		if (buffer[1] == '1')
		{
			LEDCommand = 0x20;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		if (buffer[1] == '2')
		{
			LEDCommand = 0x40;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		if (buffer[1] == '3')
		{
			LEDCommand = 0x80;
			OSQPost(SSSLEDCommandQ, (void *) LEDCommand);
			return 0;
		}
		return -1;	// unknown LED number
	}
	return -2;		// How did we get here?

};

int SI5345(struct PARSEDCMD *cmds, char resultmessage[], int itfcindex) 	// Handler for Silicon Labs 5345 Synthesizer
{
	ALT_AVALON_I2C_DEV_t *i2c_dev; 		//pointer to instance structure
//	ALT_AVALON_I2C_STATUS_CODE status;
	alt_32 status = 0;						// Altera defines this as unsigned, but then uses it as signed

	i2c_dev = alt_avalon_i2c_open(ItfcTable[itfcindex].name);

    // Si5345 slave address is dependent on the value A1 A0 are. A0 supposed to be pulled up.
	//  A1 is on pin 17 of the device (a corner pin).  Then slave address >> 1 is:
	//   if A1 high - 0x6B
	//   if A1 low - 0x69		The Clock Module was reworked to ground pin A1.

	alt_u32 slave_addr = 0x69;  	  // Right shifted by one bit

	alt_avalon_i2c_master_target_set(i2c_dev, slave_addr);

	// Need to set the I2C I/O voltage to 3.3v:  bank 0x09 register 0x43

	alt_u8 bank9[2] = {0x01, 0x09 };	// write 9 to bank 1 (selector)
	status += alt_avalon_i2c_master_tx(i2c_dev, bank9, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

	// write 0x01 to 9:43
	alt_u8 vccio[2] = { 0x43, 0x01 };	// set I2C vccio to 3.3v
	status += alt_avalon_i2c_master_tx(i2c_dev, vccio, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

	alt_u8 banksel[2] = {0x01, 0x00 };	// write 0 (as bank 0 for future reads) to bank 1 (bank selector)
	status += alt_avalon_i2c_master_tx(i2c_dev, banksel, 2, ALT_AVALON_I2C_NO_INTERRUPTS);

	alt_u8 selMSBPN = {0x02 };	// bank 0 : MSB of the part number
	status += alt_avalon_i2c_master_tx(i2c_dev, &selMSBPN, 1, ALT_AVALON_I2C_NO_INTERRUPTS);
	alt_u8 LSBPN;
	status += alt_avalon_i2c_master_rx(i2c_dev, &LSBPN, 1, ALT_AVALON_I2C_NO_INTERRUPTS);

	alt_u8 selLSBPN = {0x03 };	// bank 0 : LSB of the part number
	status += alt_avalon_i2c_master_tx(i2c_dev, &selLSBPN, 1, ALT_AVALON_I2C_NO_INTERRUPTS);
	alt_u8 MSBPN;
	status += alt_avalon_i2c_master_rx(i2c_dev, &MSBPN, 1, ALT_AVALON_I2C_NO_INTERRUPTS);


	// TODO - Need to add code to write and read the Si5345 registers from MW and MR commands.
	// TODO - Likely this isn't useful since a lot of bytes must be written to the Synth to
	// TODO - program it, involving a lot of MW/MR commands.

	return -1;	// TODO - NAK the MW/MR command for now

}
int ZEDF9T(struct PARSEDCMD *cmds, char resultmessage[], int itfcindex) 		// Handler for ZED-F9T GPS
{
	ALT_AVALON_I2C_DEV_t *i2c_dev; 		//pointer to instance structure
//	ALT_AVALON_I2C_STATUS_CODE status;
	alt_32 status;						// Altera defines this as unsigned, but then uses it as signed

	i2c_dev = alt_avalon_i2c_open(ItfcTable[itfcindex].name);

	if (NULL==i2c_dev)
	{
		printf("Device Error: Cannot find: %s\n", ItfcTable[itfcindex].name);
		return -1;
	}

    // ZEDF9T slave address is 0x84. Then slave address >> 1 is:  0x42

	alt_u32 slave_addr = 0x42;  	  // Right shifted by one bit

	alt_avalon_i2c_master_target_set(i2c_dev, slave_addr);



	// Write the binary equivalent of the  UBX-MON-VER command to the I2C.
	//    in hopes that we can actually read something back.
	//   Writes are 2..N bytes, there is no address register for writes.
	// Only do this is the command is a Module Write

	if (cmds->cmd[1] == 'W')
	{

		alt_u8 UbxMonVer[8] = { 0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x0E, 0x34 };
		status = alt_avalon_i2c_master_tx(i2c_dev, UbxMonVer, 8, ALT_AVALON_I2C_NO_INTERRUPTS);

		// sleep for 500 milliseconds
		OSTimeDlyHMSM(0, 0, 0, 500);


// TODO
		// Temporarily put the GPS initialization code in the manually triggered part so that the ZEDF9T
		// has enough time to wake up.   Ultimately move these to a delayed-initializer OS task.
// TODO


//		alt_u8 UbxCmd1[40] = { 0xb5, 0x62, 0x06, 0x31, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00,
//				0x32, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x8c, 0x86,
//				0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x80,
//				0x00, 0x00, 0x00, 0x00, 0xef, 0x00, 0x00, 0x00, 0x52, 0x7e };
//		status = alt_avalon_i2c_master_tx(i2c_dev, UbxCmd1, 40, ALT_AVALON_I2C_NO_INTERRUPTS);
//
//		// sleep for 100 milliseconds
//		OSTimeDlyHMSM(0, 0, 0, 100);
//
//
//		alt_u8 UbxCmd2[40] = { 0xb5, 0x62, 0x06, 0x31, 0x20, 0x00, 0x01, 0x01, 0x00, 0x00,
//				0x32, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
//				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,	0x00, 0x00,
//				0x00, 0x00, 0x00, 0x00, 0xef, 0x00, 0x00, 0x00, 0x85, 0xca };
//		status -= alt_avalon_i2c_master_tx(i2c_dev, UbxCmd2, 40, ALT_AVALON_I2C_NO_INTERRUPTS);

		if (status < 0)
			printf("I2C Error while attempting to initialize ZEDF9T\n");


		return (0);			// Just issue an AK
	}



	// Only 3 addresses in the receive register map are useful:
	// 0xFD, 0xFE - byte count of the message available on port 0xFF, and
	// 0xFF - a stream of characters with the various serial messages. If register 0xFF is
	// read and returns a value of 0xFF, then there is no message waiting. No valid message
	// can begin with the 0xFF byte.
	//
	// To select a register, write that register value to the device.  Nothing else is writable.

	alt_u8 regnum = 0xFD;
	status = alt_avalon_i2c_master_tx(i2c_dev, &regnum, 1, ALT_AVALON_I2C_NO_INTERRUPTS);
	alt_u8 MScount;
	status += alt_avalon_i2c_master_rx(i2c_dev, &MScount, 1, ALT_AVALON_I2C_NO_INTERRUPTS);

//	regnum = 0xFE;  // address auto-increments
	status += alt_avalon_i2c_master_tx(i2c_dev, &regnum, 1, ALT_AVALON_I2C_NO_INTERRUPTS);
	alt_u8 LScount;
	status += alt_avalon_i2c_master_rx(i2c_dev, &LScount, 1, ALT_AVALON_I2C_NO_INTERRUPTS);



	if (status != 0)
	{
		printf("ZEDF9T I2C error, status = %04li\n", status);
		return -1;		// error - return DE_LH NAK
	}

	int Count = 256 * MScount + LScount;

	if (Count == 0 || Count == 65535)		// No message awaiting, return count, force data value of 0xff
	{
		sprintf(resultmessage, "RR 0x%04lx 0x%04lx    0x%04x 0xff\n",	cmds->module, cmds->interface, Count);
		return 1;
	}
	else				// Read the message for Count bytes, then return the message as hex.
	{

//		regnum = 0xFF;	// select the message register  // address auto increments but stops incrementing at 0xff
		status += alt_avalon_i2c_master_tx(i2c_dev, &regnum, 1, ALT_AVALON_I2C_NO_INTERRUPTS);

		alt_u8 msgbuf[120];
		int msglen;
		if (Count <= 78)		// for now format the line to 80 characters (accounting for trailing \n\0)
			msglen = Count;
		else
			msglen = 78;

		status = alt_avalon_i2c_master_rx(i2c_dev, msgbuf, msglen, ALT_AVALON_I2C_NO_INTERRUPTS);

		if (status != 0)
		{
			printf("ZEDF9T I2C message read error, status = %04li\n", status);
			return -1;		// error - return DE_LH NAK
		}

		sprintf(resultmessage, "RR 0x%04lx 0x%04lx    %04x  ", cmds->module, cmds->interface, Count);

		// now append the message to the response
		int position = strlen(resultmessage);

		int i;
		for (i=0; i<msglen; i++)
		{
			resultmessage[position+i] = msgbuf[i];		// hopefully the message is ASCII or something
			if (msgbuf[i] > 127)
				resultmessage[position+i] = '.';

			//if (msgbuf[i] == 0xff)		//  If there is a message byte of 0xff, terminate the printout as it's probably a message boundary.
			//		break;
		}
		resultmessage[position+i+1] = '\n';
		resultmessage[position+i+2] = '\0';

		return 1;
	}

	return -1;	// error
}
int AD9648(struct PARSEDCMD *cmds, char resultmessage[], int itfcindex) 		// Handler for ADC AD9648 SPI interface
{
	// The SPI controller (Qsys) and the bidirectional adaptor (Verilog) have been configured for:
	//   Clock Polarity = 0, Clock Phase = 0, MSB-first, 8-bit data, 1 slave-select signal
	//
	// The Altera SPI driver uses the base address to talk to the correct SPI interface. Altera has
	//   one predefined command to read + write byte data without interrupts.
	//
	// The AD9648 register map has a 9-bit address. The device byte-count is set to one, and the address
	//   plus count plus Read/Write (total 16 bits) is mapped into two consecutive bytes.
	//
	// The slave address is a bit mask allowing simultaneous selection of multiple slave chip select lines.
	//   We only have one chip select, the bit that controls it is bit 0. Set only bit 0 (numeric a value of 1).
	//   But the driver code doesn't seem to work this way.
	//   It load the slave register with  1 << slave.  So slave = 0 should be slave select 0.

	alt_u32 base = ItfcTable[itfcindex].base;	// select the SPi register base address
	alt_u32 slave = 0x00;						// use chip select 0 (the only chip select in our case).
	alt_u32 flags = 0;							// disable scatter/gather merge
	alt_u32	wrlen;								// write length count
	alt_u32 rdlen;								// read length count
	alt_u8 rddata;								// hold read data 0 or 1 bytes
	alt_u8 wrdata[3];							// hold write data 2 or 3 bytes

	wrdata[1] = (0xff & cmds->address);			// low device register address
	wrdata[0] = (0x01 & (cmds->address >> 8));	// high device register address, clear count and r/w fields


	if (cmds->cmd[1] == 'R')
	{
		wrlen = 2;								// write two address bytes
		rdlen = 1;								// read one data byte
		wrdata[0] |= 0x80;					// set the read bit in the address field

		int readcount = alt_avalon_spi_command(base, slave, wrlen, wrdata, rdlen, &rddata, flags);

		if (readcount == 1)
		{
			// form the response to a read
			sprintf(resultmessage, "RR 0x%04lx 0x%04lx    0x%02x\n",
					cmds->module, cmds->interface, rddata);

			return 1;	// return ACK and a result message
		}
		else
			return -1;	// error
	}

	if (cmds->cmd[1] == 'W')
	{
		wrlen = 3;								// write two address bytes plus one data byte
		rdlen = 0;								// don't read any data bytes

		wrdata[2] = (0xff & cmds->data);

		int readcount = alt_avalon_spi_command(base, slave, wrlen, wrdata, rdlen, &rddata, flags);

		if (readcount == 0)
			return 0;	// OK  - ACK only
		else
			return -1;	// error
	}

	return -1;	// Unknown command. Send NAK.
}

int ATECC608(struct PARSEDCMD *cmds, char resultmessage[], int itfcindex)
{
	// Ident ROM Microchip ATECC 608A - used just to read the unique 72-bit serial number.
	// Write not supported.  We need to do a dummy write of 0x00 in order to wake up
	// the device first.

	ALT_AVALON_I2C_DEV_t *i2c_dev; 		//pointer to instance structure
//	ALT_AVALON_I2C_STATUS_CODE status;
	alt_32 status;						// Altera defines this as unsigned, but then uses it as signed

	i2c_dev = alt_avalon_i2c_open(ItfcTable[itfcindex].name);

	if (NULL==i2c_dev)
	{
		printf("Device Error: Cannot find: %s\n", ItfcTable[itfcindex].name);
		return -1;
	}

   // ATECC608 default slave address is 0xC0 (0xC1 is the corresponding read addr)
	alt_u32 slave_addr = 0x60;  	  // Right shifted by one bit

	alt_avalon_i2c_master_target_set(i2c_dev, slave_addr);

	if (cmds->cmd[1] == 'R')
	{

	// wakeup the device from sleep

	alt_u8 txdata = 0x00;
	alt_u32 count = 0x01;
	alt_u8 wakestatus[4];


	status = alt_avalon_i2c_master_tx(i2c_dev, &txdata, count, ALT_AVALON_I2C_NO_INTERRUPTS);

	// we expect the call to fail, but wake up the ATECC device. Other errors are not OK.

	if (status == ALT_AVALON_I2C_NACK_ERR)
	{
		printf("Received NACK error from ATECC608A upon wakeup. That's OK\n");
	}
	else if (status != ALT_AVALON_I2C_SUCCESS)
	{
		printf("i2c_master_tx: ATECC608A wake-up attempt error = %ld\n", status);
		return -1; //FAIL
	}

	// Read the status code from the device. After wakeup it should be 0x11 (608 data sheet page 57).
	// The response should be 4 bytes: count, packet, 2-bytes CRC

	status = alt_avalon_i2c_master_rx(i2c_dev, wakestatus, 4, ALT_AVALON_I2C_NO_INTERRUPTS);

	if (wakestatus[1] != 0x11)
	{
		printf("ATECC608 did not correctly wake up\n");
		return -1;
	}

	// To get the 72-bit serial number we need to read three 32-bit (4 hex digit) values.
	// Each requires writing a command + CRC to the device in the correct zone. Each enables
	// a 4-byte read. All 3 {command+CRC} have been computed externally and are declared
	// as constants in this code.

	alt_u8 first[8], second[8], third[8];  // serial number responses
	alt_u32 rxcount = 8;		// each response is 7 bytes: count 4 digits CRC CRC. Leave room for one extra

	alt_u8 cmd1[8] = { 0x03, 0x07, 0x02, 0x00, 0x00, 0x00, 0x1E, 0x2D } ;  // first four bytes of ID
	alt_u8 cmd2[8] = { 0x03, 0x07, 0x02, 0x00, 0x02, 0x00, 0x18, 0xAD } ;  // second four bytes of ID
	alt_u8 cmd3[8] = { 0x03, 0x07, 0x02, 0x00, 0x03, 0x00, 0x11, 0x2D } ;  // last byte of ID + 3 don't care bytes

	// If the corresponding read is all ones, then the device was not ready and the
	// read needs to be retried.

	// Read the first four bytes of the serial number
	status = alt_avalon_i2c_master_tx(i2c_dev, cmd1, 8, ALT_AVALON_I2C_NO_INTERRUPTS);


	for (int i=0; i<10; i++)
	{
		status = alt_avalon_i2c_master_rx(i2c_dev, first, rxcount, ALT_AVALON_I2C_NO_INTERRUPTS);
		if (status == 0)	// Successful read
			break;
		if (status == ALT_AVALON_I2C_NACK_ERR)	// device busy, try again
			continue;
	}
	if (status < 0)
	{
		printf("Failed to read ATECC608 - first 4 bytes of S/N.\n");
		return -1;
	}


	// Read the second four bytes of the serial number
	status = alt_avalon_i2c_master_tx(i2c_dev, cmd2, 8, ALT_AVALON_I2C_NO_INTERRUPTS);

	for (int i=0; i<10; i++)
	{
		status = alt_avalon_i2c_master_rx(i2c_dev, second, rxcount, ALT_AVALON_I2C_NO_INTERRUPTS);
		if (status == 0)	// Successful read
			break;
		if (status == ALT_AVALON_I2C_NACK_ERR)	// device busy, try again
			continue;
	}
	if (status < 0)
	{
		printf("Failed to read ATECC608 - second 4 bytes of S/N.\n");
		return -1;
	}


	// Read the third four bytes of the serial number
	status = alt_avalon_i2c_master_tx(i2c_dev, cmd3, 8, ALT_AVALON_I2C_NO_INTERRUPTS);

	for (int i=0; i<10; i++)
	{
		status = alt_avalon_i2c_master_rx(i2c_dev, third, rxcount, ALT_AVALON_I2C_NO_INTERRUPTS);
		if (status == 0)	// Successful read
			break;
		if (status == ALT_AVALON_I2C_NACK_ERR)	// device busy, try again
			continue;
	}
	if (status < 0)
	{
		printf("Failed to read ATECC608 - third four bytes of S/N.\n");
		return -1;
	}

	// The response to each request is 7 bytes: count(=7) 4 bytes of data crc-16 0xff(=no data to read since count was only 7)
	// The last one, the data is formatted: one byte of S/N, 3 bytes of other unrelated data

	sprintf(resultmessage, "RR 0x%04lx 0x%04lx    0x%02x%02x %02x%02x %02x%02x %02x%02x %02x\n",
			cmds->module, cmds->interface,
			first[1], first[2], first[3], first[4], second[1], second[2], second[3], second[4], third[1]);


	return 1;		// OK and return the read response string (RR ....)
	}
	if (cmds->cmd[1] == 'W')
	{

		return -3;	// unimplemented - nothing to write to ID Prom
	}

	return -1;		// unknown command
}


int PCF8574(struct PARSEDCMD *cmds, char resultmessage[], int itfcindex)
{
	// TI PCF8574 8-bit I/O latch and driver.

	ALT_AVALON_I2C_DEV_t *i2c_dev; 		//pointer to instance structure
//	ALT_AVALON_I2C_STATUS_CODE status;
	alt_32 status;

	i2c_dev = alt_avalon_i2c_open(ItfcTable[itfcindex].name);

	if (NULL==i2c_dev)
	{
		printf("Device Error: Cannot find: %s\n", ItfcTable[itfcindex].name);
		return -1;
	}

	// PCF8574 Write address is 0x70, 0x71 is the corresponding read addr.
	alt_u32 slave_addr = 0x38;  	  // PCF8574 address - the i2c driver left shifts then appends 0 for
									  // write or 1 for read translating this to 0x70 for write, 0x71 for read
	alt_avalon_i2c_master_target_set(i2c_dev, slave_addr);


	if (cmds->cmd[1] == 'W')
	{
		alt_u8 txdata = cmds->data;
		alt_u32 count = 0x01;				// num bytes to send

		//
		// Receive Module. All data are ACTIVE LOW.
		// Bit7 = Red LED   		Bit6 = Green LED  		Bit5 = Ch2 20dB Atten  	Bit4 = Ch2 10 dB Atten
		// Bit3 = Ch2 Noise Enable 	Bit2 = Ch1 20 dB Atten  Bit1 = Ch1 10 dB Atten  Bit0 = Ch1 Noise Enable


		status = alt_avalon_i2c_master_tx(i2c_dev, &txdata, count, ALT_AVALON_I2C_NO_INTERRUPTS);

		if (status == ALT_AVALON_I2C_NACK_ERR)		// retry the write
		{
			printf("Received NACK error from I2C device. Retry once\n");
			status = alt_avalon_i2c_master_tx(i2c_dev, &txdata, count, ALT_AVALON_I2C_NO_INTERRUPTS);
		}


		if (status != ALT_AVALON_I2C_SUCCESS)
		{
			printf("i2c_master_tx: error = %ld\n", status);
			return -1; //FAIL
		}

		return 0;		// OK


	}
	if (cmds->cmd[1] == 'R')
	{
		alt_u8 rxdata;

		// retrieve one byte of data from the de3vice

		status = alt_avalon_i2c_master_rx(i2c_dev, &rxdata, 1, ALT_AVALON_I2C_NO_INTERRUPTS);

		if (status < 0)
		{
			printf("Received error from I2C device:   %ld\n", status);
			return -1;
		}
		sprintf(resultmessage, "RR 0x%04lx 0x%04lx   0x%02x\n", cmds->module, cmds->interface, rxdata);
		return 1;
	}

	return -1;		// unknown command type
}


int ProvisioningModuleRegisterCommand(int index, unsigned char buffer [], char resultmessage[])
{

	/* Define a table mapping the slot# and interface# to a specific
	 * I2C or SPI interface for the test system based on the DEV_KIT.
	 * For a real system this table needs to be constructed based on
	 * how the DE is actually equipped with modules which probably
	 * needs to be determined at run time.
	 *
	 * The various types of I2C devices are different. The table holds
	 * a pointer to the routine used for the particular device. Note that
	 * two different devices can share a physical I2C interface. Those need to
	 * have different interface numbers, as they use different handlers.
	 *
	 * The name of the low-level drivers for each physical interface
	 * are manually encoded here based 	on direct knowledge of the SOPCINFO
	 * file and Quartus Qsys planner.
	 */

	// Note: buffer gets modified by strtok(), so don't further reuse it.
	//              Cmd  Slot# Intfc#  Addr   Data
	// Module Read:  MR 0xssss 0xiiii 0xaaaa
	// Module Write: MW 0xssss 0xiiii 0xaaaa 0xZZZZ

	struct PARSEDCMD provcmd;		// hold the parsed provisioning command

	char * cmd = strtok((char *)buffer, " ");
	char * module = strtok(NULL, " ");
	char * slot = strtok(NULL, " ");
	char * regaddr = strtok(NULL, " ");
	char * regdata = strtok(NULL, " ");

	unsigned long moduleint, itfcint, regaddrint, regdataint;

	char * ptr;		// dummy used to make strtoul work

	moduleint = strtoul(module, &ptr, 0);
	itfcint = strtoul(slot, &ptr, 0);
	regaddrint = strtoul(regaddr, &ptr, 0);
	if (regdata != NULL)
		regdataint = strtoul(regdata, &ptr, 0);
	else
		regdataint = 0;

	// copy parsed command into struct
	strcpy(provcmd.cmd, cmd);
	provcmd.module = moduleint;
	provcmd.interface = itfcint;
	provcmd.data = regdataint;
	provcmd.address = regaddrint;


	int itfcindex = FindItfcIndex(moduleint, itfcint);	// Find index of requested interface in map

	if (itfcindex == -1)	// Not found
		return -1;		// Unknown Module / Slot combination.  Send NAK.

	// Call the proper device handler. The handler returns:
	//		status < 0 for error, send NAK
	//		status == 0 for success, send AK
	//      status == +1 send AK then send response message defined by the handler back to the Local Host.

	int result = (*ItfcTable[itfcindex].handler)(&provcmd, resultmessage, itfcindex);

	return result;
};


/* Routines to initialize the Interface Map table and to find the index
 * of the entry that matches the module # / interface #
 *
 * Base address defined in the BSP system.h file.
 * Name copied from the BSP generated system.h file.
 * If the Qsys blocks have their names changed, then these must change.
 * Often need to rebuild the Project Indexes when BSP --> system.h is regenerated.
 *
 */

// Initialize the Interface Map Table.
void initItfcMap()
{
	ItfcTable[0].module = 0;	// CKM_C0
	ItfcTable[0].interface = 0;
	ItfcTable[0].base = I2C_CKM_C0_BASE;
	strcpy(&ItfcTable[0].name[0], I2C_CKM_C0_NAME);
	ItfcTable[0].handler = SI5345;

	ItfcTable[1].module = 0;	// CKM_C0
	ItfcTable[1].interface = 1;
	ItfcTable[1].base = I2C_CKM_C0_BASE;
	strcpy(&ItfcTable[1].name[0], I2C_CKM_C0_NAME);
	ItfcTable[1].handler = ZEDF9T;

//	ItfcTable[1].module = 0;	// CKM_C1
//	ItfcTable[1].interface = 1;
//	ItfcTable[1].base = I2C_CKM_C1_BASE;
//	strcpy(&ItfcTable[1].name[0], I2C_CKM_C1_NAME);
//	ItfcTable[1].handler = 2;

	ItfcTable[2].module = 0;	// CKM_ID
	ItfcTable[2].interface = 2;
	ItfcTable[2].base = I2C_CKM_ID_BASE;
	strcpy(&ItfcTable[2].name[0], I2C_CKM_ID_NAME);
	ItfcTable[2].handler = ATECC608;

	ItfcTable[3].module = 1;	// RXM_CTRL
	ItfcTable[3].interface = 0;
	ItfcTable[3].base = I2C_RXM_CTRL_BASE;
	strcpy(&ItfcTable[3].name[0], I2C_RXM_CTRL_NAME);
	ItfcTable[3].handler = PCF8574;

	ItfcTable[4].module = 1;	// RXM_ID
	ItfcTable[4].interface = 1;
	ItfcTable[4].base = I2C_RXM_ID_BASE;
	strcpy(&ItfcTable[4].name[0], I2C_RXM_ID_NAME);
	ItfcTable[4].handler = ATECC608;

	ItfcTable[5].module = 1;	// RXM_SPI_ADC
	ItfcTable[5].interface = 2;
	ItfcTable[5].base = SPI_RXM_BASE;
	strcpy(&ItfcTable[5].name[0], SPI_RXM_NAME);
	ItfcTable[5].handler = AD9648;
};

// Find the index of the Module / Interface in the Interface Map table
int FindItfcIndex(int module, int itfcnum)
{
	for (int i=0; i< MAXITFC; i++)		if ((module == ItfcTable[i].module) && (itfcnum == ItfcTable[i].interface))
			return i;

	return -1;	// Didn't find it in the table
};
