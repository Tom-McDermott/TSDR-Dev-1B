/******************************************************************************
* Copyright (c) 2006 Altera Corporation, San Jose, California, USA.           *
* All rights reserved. All use of this software and documentation is          *
* subject to the License Agreement located at the end of this file below.     *
*******************************************************************************
* Date - October 24, 2006                                                     *
* Module - simple_socket_server.c                                             *
*                                                                             *
******************************************************************************/
 
/******************************************************************************
 * Simple Socket Server (SSS) example. 
 * 
 * This example demonstrates the use of MicroC/OS-II running on NIOS II.       
 * In addition it is to serve as a good starting point for designs using       
 * MicroC/OS-II and Altera NicheStack TCP/IP Stack - NIOS II Edition.                                          
 *                                                                             
 * -Known Issues                                                             
 *     None.   
 *      
 * Please refer to the Altera NicheStack Tutorial documentation for details on this 
 * software example, as well as details on how to configure the NicheStack TCP/IP 
 * networking stack and MicroC/OS-II Real-Time Operating System.
 *
 *
 * Extensively revised to use UDP sockets only. This implements a stripped down
 * version of the TangerineSDR DE_LH protocol just for module test.
 *
 * TCP (telnet) is disconnected and unused now.
 *
 */
 
#include <stdio.h>
#include <string.h>
#include <ctype.h> 

/* MicroC/OS-II definitions */
#include "includes.h"

/* Simple Socket Server definitions */
#include "simple_socket_server.h"                                                                    
#include "alt_error_handler.h"

/* Nichestack definitions */
#include "ipport.h"
#include "tcpport.h"

/* TSDR protocol definitions */
#include "DELHProtocol.h"

/* Initialize TSDR processor-controlled peripherals */
#include "InitializePeriph.h"

 
/*
 * Global handles (pointers) to our MicroC/OS-II resources. All of resources 
 * beginning with "SSS" are declared and created in this file.
 */

/*
 * This SSSLEDCommandQ MicroC/OS-II message queue will be used to communicate 
 * between the simple socket server task and Nios Development Board LED control 
 * tasks.
 *
 * Handle to our MicroC/OS-II Command Queue and variable definitions related to 
 * the Q for sending commands received on the TCP-IP socket from the 
 * SSSSimpleSocketServerTask to the LEDManagementTask.
 */
OS_EVENT  *SSSLEDCommandQ;
#define SSS_LED_COMMAND_Q_SIZE  30  /* Message capacity of SSSLEDCommandQ */
void *SSSLEDCommandQTbl[SSS_LED_COMMAND_Q_SIZE]; /*Storage for SSSLEDCommandQ*/


/*
 * Handle to our MicroC/OS-II LED Event Flag.  Each flag corresponds to one of
 * the LEDs on the Nios Development board, D0 - D7. 
 */
OS_FLAG_GRP *SSSLEDEventFlag;


/* Definition of Task Stacks for tasks not invoked by TK_NEWTASK 
 * (do not use NicheStack) 
 */

OS_STK    LEDManagementTaskStk[TASK_STACKSIZE];

/*
 * Create our MicroC/OS-II resources. All of the resources beginning with 
 * "SSS" are declared in this file, and created in this function.
 */
void SSSCreateOSDataStructs(void)
{
  INT8U error_code;
  
  /*
  * Create the resource for our MicroC/OS-II Queue for sending commands 
  * received on the TCP/IP socket from the SSSSimpleSocketServerTask()
  * to the LEDManagementTask().
  */
  SSSLEDCommandQ = OSQCreate(&SSSLEDCommandQTbl[0], SSS_LED_COMMAND_Q_SIZE);
  if (!SSSLEDCommandQ)
  {
     alt_uCOSIIErrorHandler(EXPANDED_DIAGNOSIS_CODE, 
     "Failed to create SSSLEDCommandQ.\n");
  }
  
 /*
  * Create our MicroC/OS-II LED Event Flag.  Each flag corresponds to one of
  * the LEDs on the Nios Development board, D0 - D7. 
  */   
  SSSLEDEventFlag = OSFlagCreate(0, &error_code);
  if (!SSSLEDEventFlag)
  {
     alt_uCOSIIErrorHandler(error_code, 0);
  }
}

/* This function creates tasks used in this example which do not use sockets.
 * Tasks which use Interniche sockets must be created with TK_NEWTASK.
 */
 
void SSSCreateTasks(void)
{
   INT8U error_code;
  
   error_code = OSTaskCreateExt(LEDManagementTask,
                              NULL,
                              (void *)&LEDManagementTaskStk[TASK_STACKSIZE-1],
                              LED_MANAGEMENT_TASK_PRIORITY,
                              LED_MANAGEMENT_TASK_PRIORITY,
                              LEDManagementTaskStk,
                              TASK_STACKSIZE,
                              NULL,
                              0);

   alt_uCOSIIErrorHandler(error_code, 0);

}

/*
 * SSSSimpleSocketServerTask()
 * 
 * This MicroC/OS-II thread spins forever after first establishing an
 * OpenHPSDR discovery socket on port 1024. It listens for any received packet
 * on any established socket and dispatches it accordingly.
 *
 */
void SSSSimpleSocketServerTask()
{
  int fd_disc_recv;		// file descriptor (fd) of the receive discovery socket
  struct sockaddr_in addrhost, nilclient;
  fd_set readfds;

  memset(&addrhost, 0, sizeof(struct sockaddr_in));
  memset(&nilclient, 0, sizeof(struct sockaddr_in));		// discovery request doesn't use client address / port
  
  initChan();			// Clear table of UDP Channels.

  initItfcMap();		// Build the Interface Map table.

  initPeriph();			// Initialize TSDR peripherals


  /*
   * Sockets primer...
   * The socket() call creates an endpoint for TCP of UDP communication. It 
   * returns a descriptor (similar to a file descriptor) that we call fd_listen,
   * or, "the socket we're listening on for connection requests" in our sss
   * server example.
   *
   * Traditionally, in the Sockets API, PF_INET and AF_INET is used for the 
   * protocol and address families respectively. However, there is usually only
   * 1 address per protocol family. Thus PF_INET and AF_INET can be interchanged.
   * In the case of NicheStack, only the use of AF_INET is supported.
   * PF_INET is not supported in NicheStack.
   */ 
  if ((fd_disc_recv = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    alt_NetworkErrorHandler(EXPANDED_DIAGNOSIS_CODE,"[TSDR_task] Discovery Socket creation failed");
  }
  
  /*
   * Sockets primer, continued...
   * Calling bind() associates a socket created with socket() to a particular IP
   * port and incoming address. In this case we're binding to SSS_PORT and to
   * INADDR_ANY address (allowing anyone to connect to us. Bind may fail for 
   * various reasons, but the most common is that some other socket is bound to
   * the port we're requesting. 
   */ 
  addrhost.sin_family = AF_INET;
  addrhost.sin_port = htons(1024);
  addrhost.sin_addr.s_addr = INADDR_ANY;	//  any interface on our host
  
  if ((bind(fd_disc_recv, (struct sockaddr *)&addrhost, sizeof(addrhost))) < 0)
  {
    alt_NetworkErrorHandler(EXPANDED_DIAGNOSIS_CODE,"[TSDR_task] Discovery Bind failed");
  }

  int addresult = buildaddChan(fd_disc_recv, 'D', &addrhost, &nilclient);	// add discovery channel to table
  if (addresult < 0)
  {
    printf("[FATAL] Could not add Discovery Channel.  We're probably dead.\n");
  }


  while(1)
  {
    /* 
     * For those not familiar with sockets programming...
     * The select() call below basically tells the TCPIP stack to return 
     * from this call when any of the events I have expressed an interest 
     * in happen (it blocks until our call to select() is satisfied). 
     * 
     * In the call below we're only interested in either someone trying to 
     * connect to us, or data being available to read on a socket, both of 
     * these are a read event as far as select is called.
     * 
     * The sockets we're interested in are passed in in the readfds 
     * parameter, the format of the readfds is implementation dependent
     * Hence there are standard MACROs for setting/reading the values:
     * 
     *   FD_ZERO  - Zero's out the sockets we're interested in
     *   FD_SET   - Adds a socket to those we're interested in
     *   FD_ISSET - Tests whether the chosen socket is set 
     */
    FD_ZERO(&readfds);

    int activelist[MAXUDPCHAN];							// list of active channels to retrieve
    int listenercount = allListeners(&activelist[0]);	// count of active channels retrieved
    for(int i=0; i<listenercount; i++)
    	FD_SET(activelist[i], &readfds);	// add all active channels to select

    select(MAXUDPCHAN, &readfds, NULL, NULL, NULL);

    for(int i=0; i<listenercount; i++)
    {
    	if (FD_ISSET(activelist[i], &readfds))		// if we got a packet for the listener
    	{
    		int index = findfdChan(activelist[i]);
    		if (index >= 0)
    			ProcessRxPacket(index);				// process the received packet, say which channel it's on
    		else
    			printf("ERROR: Received a Packet but no connection found.\n");
    	}

    }

  } /* while(1) */
} // Task



/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2009 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/
