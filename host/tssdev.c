/********************************************************************************/
/*										*/
/*		Linux Device Transmit and Receive Utilities			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tssdev.c 1072 2017-09-11 19:55:31Z kgoldman $ 		*/
/*										*/
/* (c) Copyright IBM Corporation 2015, 2017.					*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/

#ifdef TPM_POSIX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>

#include <tss2/tssresponsecode.h>
#include <tss2/tsserror.h>
#include <tss2/tssprint.h>
#include "tssproperties.h"

#include "tssdev.h"

/* local prototypes */

static uint32_t TSS_Dev_Open(TSS_CONTEXT *tssContext);
static uint32_t TSS_Dev_SendCommand(int dev_fd, const uint8_t *buffer, uint16_t length,
				    const char *message);
static uint32_t TSS_Dev_ReceiveCommand(int dev_fd, uint8_t *buffer, uint32_t *length);

/* global configuration */

extern int tssVverbose;
extern int tssVerbose;

/* TSS_Dev_Transmit() transmits the command and receives the response.

   Can return device transmit and receive packet errors, but normally returns the TPM response code.
*/

TPM_RC TSS_Dev_Transmit(TSS_CONTEXT *tssContext,
			uint8_t *responseBuffer, uint32_t *read,
			const uint8_t *commandBuffer, uint32_t written,
			const char *message)
{
    TPM_RC rc = 0;
    
    /* open on first transmit */
    if (tssContext->tssFirstTransmit) {	
	if (rc == 0) {
	    rc = TSS_Dev_Open(tssContext);
	}
	if (rc == 0) {
	    tssContext->tssFirstTransmit = FALSE;
	}
    }
    /* send the command to the device.  Error if the device send fails. */
    if (rc == 0) {
	rc = TSS_Dev_SendCommand(tssContext->dev_fd, commandBuffer, written, message);
    }
    /* receive the response from the dev_fd.  Returns dev_fd errors, malformed response errors.
       Else returns the TPM response code. */
    if (rc == 0) {
	rc = TSS_Dev_ReceiveCommand(tssContext->dev_fd, responseBuffer, read);
    }
    return rc;
}

/* TSS_Dev_Open() opens the TPM device (through the device driver) */

static uint32_t TSS_Dev_Open(TSS_CONTEXT *tssContext)
{
    uint32_t rc = 0;
    
    if (rc == 0) {
	if (tssVverbose) printf("TSS_Dev_Open: Opening %s\n", tssContext->tssDevice);
	tssContext->dev_fd = open(tssContext->tssDevice, O_RDWR);
	if (tssContext->dev_fd < 0) {
	    if (tssVerbose) printf("TSS_Dev_Open: Error opening %s\n", tssContext->tssDevice);
	    rc = TSS_RC_NO_CONNECTION;
	}
    }
    return rc;
}

/* TSS_Dev_SendCommand() sends the TPM command buffer to the device.

   Returns an error if the device write fails.
*/

static uint32_t TSS_Dev_SendCommand(int dev_fd,
				    const uint8_t *buffer, uint16_t length,
				    const char *message)
{
    uint32_t rc = 0;
    int irc;
    
    if (message != NULL) {
	if (tssVverbose) printf("TSS_Dev_SendCommand: %s\n", message);
    }
    if ((rc == 0) && tssVverbose) {
	TSS_PrintAll("TSS_Dev_SendCommand",
		     buffer, length);
    }
    if (rc == 0) {
	irc = write(dev_fd, buffer, length);
	if (irc < 0) {
	    if (tssVerbose) printf("TSS_Dev_SendCommand: write error %d %s\n",
				   errno, strerror(errno));
	    rc = TSS_RC_BAD_CONNECTION;
	}
    }
    return rc;
}

/* TSS_Dev_ReceiveCommand() reads a response buffer from the device.

   Returns TPM packet error code.

   Validates that the packet length and the packet responseSize match 
*/

static uint32_t TSS_Dev_ReceiveCommand(int dev_fd, uint8_t *buffer, uint32_t *length)
{
    uint32_t 	rc = 0;
    int 	irc;
    uint32_t 	responseSize = 0;
    uint32_t 	responseCode = 0;

    if (tssVverbose) printf("TSS_Dev_ReceiveCommand:\n");
    /* read the TPM device */
    if (rc == 0) {
	irc = read(dev_fd, buffer, MAX_RESPONSE_SIZE);
	if (irc <= 0) {
	    rc = TSS_RC_BAD_CONNECTION;
	    if (irc < 0) {
		if (tssVerbose) printf("TSS_Dev_ReceiveCommand: read error %d %s\n",
				       errno, strerror(errno));
	    }
	}
    }
    if ((rc == 0) && tssVverbose) {
	TSS_PrintAll("TSS_Dev_ReceiveCommand",
		     buffer, irc);
    }
    /* verify that there is at least a tag, responseSize, and responseCode */
    if (rc == 0) {
	if ((unsigned int)irc < (sizeof(TPM_ST) + sizeof(uint32_t) + sizeof(uint32_t))) {
	    if (tssVerbose) printf("TSS_Dev_ReceiveCommand: read bytes %u < header\n", irc);
	    rc = TSS_RC_MALFORMED_RESPONSE;
	}
    }
    /* get responseSize from the packet */
    if (rc == 0) {
	responseSize = ntohl(*(uint32_t *)(buffer + sizeof(TPM_ST)));
	/* sanity check against the length actually received, the return code */
	if ((uint32_t)irc != responseSize) {
	    if (tssVerbose) printf("TSS_Dev_ReceiveCommand: read bytes %u != responseSize %u\n",
				   (uint32_t)irc, responseSize);
	    rc = TSS_RC_BAD_CONNECTION;
	}
    }
    /* read the TPM return code from the packet */
    if (rc == 0) {
	responseCode = ntohl(*(uint32_t *)(buffer + sizeof(TPM_ST)+ sizeof(uint32_t)));
    }
    if (rc == 0) {
	rc = responseCode;
    }
	
    *length = responseSize;
    if (tssVverbose) printf("TSS_Dev_ReceiveCommand: rc %08x\n", rc);
    return rc;
}	

TPM_RC TSS_Dev_Close(TSS_CONTEXT *tssContext)
{
    if (tssVverbose) printf("TSS_Dev_Close: Closing %s\n", tssContext->tssDevice);
    close(tssContext->dev_fd);
    return 0;
}

#endif	/* TPM_POSIX */
