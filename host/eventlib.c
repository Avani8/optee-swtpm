/********************************************************************************/
/*										*/
/*		     	TPM2 Measurement Log Common Routines			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: eventlib.c 1072 2017-09-11 19:55:31Z kgoldman $		*/
/*										*/
/* (c) Copyright IBM Corporation 2016, 2017.					*/
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tss2/tssprint.h>
#include <tss2/Unmarshal_fp.h>
#include <tss2/tssmarshal.h>
#include <tss2/tsscryptoh.h>
#include <tss2/tsscrypto.h>

#include "eventlib.h"

static uint16_t Uint16_Convert(uint16_t in);
static uint32_t Uint32_Convert(uint32_t in);
static TPM_RC UINT16LE_Unmarshal(uint16_t *target, BYTE **buffer, int32_t *size);
static TPM_RC UINT32LE_Unmarshal(uint32_t *target, BYTE **buffer, int32_t *size);

static void TSS_EVENT_EventType_Trace(uint32_t eventType);
static TPM_RC TSS_SpecIdEventAlgorithmSize_Unmarshal(TCG_EfiSpecIdEventAlgorithmSize *algSize,
						     uint8_t **buffer,
						     int32_t *size);
static void TSS_SpecIdEventAlgorithmSize_Trace(TCG_EfiSpecIdEventAlgorithmSize *algSize);

/* TSS_EVENT_Line_Read() reads a TPM 1.2 SHA-1 event line from a binary file inFile.

 */

int TSS_EVENT_Line_Read(TCG_PCR_EVENT *event,
			int *endOfFile,
			FILE *inFile)
{
    int rc = 0;
    size_t readSize;
    *endOfFile = FALSE;

    /* read the PCR index */
    if (rc == 0) {
	readSize = fread(&(event->pcrIndex),
			 sizeof(((TCG_PCR_EVENT *)NULL)->pcrIndex), 1, inFile);
	if (readSize != 1) {
	    if (feof(inFile)) {
		*endOfFile = TRUE;;
	    }
	    else {
		printf("TSS_EVENT_Line_Read: Error, could not read pcrIndex, returned %lu\n",
		       (unsigned long)readSize);
		rc = ERR_STRUCTURE;
	    }
	}
    }
    /* do the endian conversion from stream to uint32_t */
    if (!*endOfFile && (rc == 0)) {
	event->pcrIndex = Uint32_Convert(event->pcrIndex);
    }
    /* read the event type */
    if (!*endOfFile && (rc == 0)) {
	readSize = fread(&(event->eventType),
			 sizeof(((TCG_PCR_EVENT *)NULL)->eventType), 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT_Line_Read: Error, could not read eventType, returned %lu\n",
		   (unsigned long) readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* do the endian conversion from stream to uint32_t */
    if (!*endOfFile && (rc == 0)) {
	event->eventType = Uint32_Convert(event->eventType);
    }
    /* read the digest */
    if (!*endOfFile && (rc == 0)) {
	readSize = fread(&(event->digest),
			 sizeof(((TCG_PCR_EVENT *)NULL)->digest), 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT_Line_Read: Error, could not read digest, returned %lu\n",
		   (unsigned long)readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* read the event data size */
    if (!*endOfFile && (rc == 0)) {
	readSize = fread(&(event->eventDataSize),
			 sizeof(((TCG_PCR_EVENT *)NULL)->eventDataSize), 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT_Line_Read: Error, could not read event data size, returned %lu\n",
		   (unsigned long)readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* do the endian conversion from stream to uint32_t */
    if (!*endOfFile && (rc == 0)) {
	event->eventDataSize = Uint32_Convert(event->eventDataSize);
    }
    /* bounds check the event data length */
    if (!*endOfFile && (rc == 0)) {
	if (event->eventDataSize > sizeof(((TCG_PCR_EVENT *)NULL)->event)) {
	    printf("TSS_EVENT_Line_Read: Error, event data length too big: %u\n",
		   event->eventDataSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* read the event */
    if (!*endOfFile && (rc == 0)) {
	memset(event->event , 0, sizeof(((TCG_PCR_EVENT *)NULL)->event));
	readSize = fread(&(event->event),
			 event->eventDataSize, 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT_Line_Read: Error, could not read event, returned %lu\n",
		   (unsigned long)readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    return rc;
}

void TSS_EVENT_Line_Trace(TCG_PCR_EVENT *event)
{
    printf("TSS_EVENT_Line_Trace: PCR index %u\n", event->pcrIndex);
    TSS_EVENT_EventType_Trace(event->eventType);
    TSS_PrintAll("TSS_EVENT_Line_Trace: PCR",
		 event->digest, sizeof(((TCG_PCR_EVENT *)NULL)->digest));
    TSS_PrintAll("TSS_EVENT_Line_Trace: event",
		 event->event, event->eventDataSize);
    return;
}

/* TSS_SpecIdEvent_Unmarshal() unmarshals the TCG_EfiSpecIDEvent structure.

   The size and buffer are not moved, since this is the only structure in the event.
*/

TPM_RC TSS_SpecIdEvent_Unmarshal(TCG_EfiSpecIDEvent *specIdEvent,
				  uint32_t eventSize,
				  uint8_t *event)
{
    TPM_RC	rc = 0;
    int32_t	size = eventSize;	/* copy, because size and buffer are not moved */
    uint8_t	*buffer = event;
    uint32_t 	i;

    if (rc == 0) {
	rc = Array_Unmarshal(specIdEvent->signature, sizeof(specIdEvent->signature),
			     &buffer, &size);
    }
    if (rc == 0) {
	rc = UINT32LE_Unmarshal(&(specIdEvent->platformClass), &buffer, &size);
    }
    if (rc == 0) {
	rc = UINT8_Unmarshal(&(specIdEvent->specVersionMinor), &buffer, &size);
    }
    if (rc == 0) {
	rc = UINT8_Unmarshal(&(specIdEvent->specVersionMajor), &buffer, &size);
    }
    if (rc == 0) {
	rc = UINT8_Unmarshal(&(specIdEvent->specErrata), &buffer, &size);
    }
    if (rc == 0) {
	rc = UINT8_Unmarshal(&(specIdEvent->uintnSize), &buffer, &size);
    }
    if (rc == 0) {
	rc = UINT32LE_Unmarshal(&(specIdEvent->numberOfAlgorithms), &buffer, &size);
    }
    for (i = 0 ; (rc == 0) && (i < specIdEvent->numberOfAlgorithms) ; i++) {
	rc = TSS_SpecIdEventAlgorithmSize_Unmarshal(&(specIdEvent->digestSizes[i]),
						    &buffer, &size);
    }	    
    if (rc == 0) {
	rc = UINT8_Unmarshal(&(specIdEvent->vendorInfoSize), &buffer, &size);
    }
    if (rc == 0) {
	rc = Array_Unmarshal(specIdEvent->vendorInfo, specIdEvent->vendorInfoSize,
			     &buffer, &size);
    }
    return rc;
}

/* TSS_SpecIdEventAlgorithmSize_Unmarshal() unmarshals the TCG_EfiSpecIdEventAlgorithmSize
   structure */

static TPM_RC TSS_SpecIdEventAlgorithmSize_Unmarshal(TCG_EfiSpecIdEventAlgorithmSize *algSize,
						     uint8_t **buffer,
						     int32_t *size)
{
    TPM_RC	rc = 0;

    if (rc == 0) {
	rc = UINT16LE_Unmarshal(&(algSize->algorithmId), buffer, size);
    }
    if (rc == 0) {
	rc = UINT16LE_Unmarshal(&(algSize->digestSize), buffer, size);
    } 
    if (rc == 0) {
	uint16_t mappedDigestSize = TSS_GetDigestSize(algSize->algorithmId);
	if (mappedDigestSize != 0) {
	    if (mappedDigestSize != algSize->digestSize) {
		printf("TSS_SpecIdEventAlgorithmSize_Unmarshal: "
		       "Error, inconsistent digest size, algorithm %04x size %u\n",
		       algSize->algorithmId, algSize->digestSize);
		rc = ERR_STRUCTURE;
	    }
	}
    }
    return rc;
}

void TSS_SpecIdEvent_Trace(TCG_EfiSpecIDEvent *specIdEvent)
{
    uint32_t 	i;

    /* normal case */
    if (specIdEvent->signature[15] == '\0')  {
	printf("TSS_SpecIdEvent_Trace: signature: %s\n", specIdEvent->signature);
    }
    /* error case */
    else {
	TSS_PrintAll("TSS_SpecIdEvent_Trace: signature",
		     specIdEvent->signature, sizeof(specIdEvent->signature));
    }
    printf("TSS_SpecIdEvent_Trace: platformClass %08x\n", specIdEvent->platformClass);
    printf("TSS_SpecIdEvent_Trace: specVersionMinor %02x\n", specIdEvent->specVersionMinor);
    printf("TSS_SpecIdEvent_Trace: specVersionMajor %02x\n", specIdEvent->specVersionMajor);
    printf("TSS_SpecIdEvent_Trace: specErrata %02x\n", specIdEvent->specErrata);
    printf("TSS_SpecIdEvent_Trace: uintnSize %02x\n", specIdEvent->uintnSize);
    printf("TSS_SpecIdEvent_Trace: numberOfAlgorithms %u\n", specIdEvent->numberOfAlgorithms);
    for (i = 0 ; (i < specIdEvent->numberOfAlgorithms) ; i++) {
	TSS_SpecIdEventAlgorithmSize_Trace(&(specIdEvent->digestSizes[i]));
    }
    /* try for a printable string */
    if (specIdEvent->vendorInfo[specIdEvent->vendorInfoSize-1] == '\0')  {
	printf("TSS_SpecIdEvent_Trace: vendorInfo: %s\n", specIdEvent->vendorInfo);
    }
    /* if not, trace the bytes */
    else {
	TSS_PrintAll("TSS_SpecIdEvent_Trace: vendorInfo",
		     specIdEvent->vendorInfo, specIdEvent->vendorInfoSize);
    }
    return;
}

static void TSS_SpecIdEventAlgorithmSize_Trace(TCG_EfiSpecIdEventAlgorithmSize *algSize)
{
    printf("TSS_SpecIdEventAlgorithmSize_Trace: algorithmId %04x\n", algSize->algorithmId);
    printf("TSS_SpecIdEventAlgorithmSize_Trace: digestSize %u\n", algSize->digestSize);
    return;
}

/* TSS_EVENT2_Line_Read() reads a TPM2 event line from a binary file inFile.

*/

int TSS_EVENT2_Line_Read(TCG_PCR_EVENT2 *event,
			 int *endOfFile,
			 FILE *inFile)
{
    int rc = 0;
    size_t readSize;
    *endOfFile = FALSE;

    /* read the PCR index */
    if (rc == 0) {
	readSize = fread(&(event->pcrIndex),
			 sizeof(((TCG_PCR_EVENT2 *)NULL)->pcrIndex), 1, inFile);
	if (readSize != 1) {
	    if (feof(inFile)) {
		*endOfFile = TRUE;
	    }
	    else {
		printf("TSS_EVENT2_Line_Read: Error, could not read pcrIndex, returned %lu\n",
		       (unsigned long)readSize);
		rc = ERR_STRUCTURE;
	    }
	}
    }
    /* do the endian conversion from stream to uint32_t */
    if (!*endOfFile && (rc == 0)) {
	event->pcrIndex = Uint32_Convert(event->pcrIndex);
    }
    /* read the event type */
    if (!*endOfFile && (rc == 0)) {
	readSize = fread(&(event->eventType),
			 sizeof(((TCG_PCR_EVENT2 *)NULL)->eventType), 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT2_Line_Read: Error, could not read eventType, returned %lu\n",
		   (unsigned long)readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* do the endian conversion from stream to uint32_t */
    if (!*endOfFile && (rc == 0)) {
	event->eventType = Uint32_Convert(event->eventType);
    }
    /* read the TPML_DIGEST_VALUES count */
    uint32_t maxCount; 
    if (!*endOfFile && (rc == 0)) {
	maxCount = sizeof((TPML_DIGEST_VALUES *)NULL)->digests / sizeof(TPMT_HA);
	readSize = fread(&(event->digests.count),
			 sizeof(((TPML_DIGEST_VALUES *)NULL)->count), 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT2_Line_Read: Error, could not read digest count, returned %lu\n",
		   (unsigned long)readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* do the endian conversion from stream to uint32_t */
    if (!*endOfFile && (rc == 0)) {
	event->digests.count = Uint32_Convert(event->digests.count);
    }
    /* range check the digest count */
    if (!*endOfFile && (rc == 0)) {
	if (event->digests.count > maxCount) {
	    printf("TSS_EVENT2_Line_Read: Error, digest count %u is greater than structure %u\n",
		   event->digests.count, maxCount);
	    rc = ERR_STRUCTURE;
	}
	else if (event->digests.count == 0) {
	    printf("TSS_EVENT2_Line_Read: Error, digest count is zero\n");
	    rc = ERR_STRUCTURE;
	}
    }
    uint32_t count;
    /* read all the TPMT_HA, loop through all the digest algorithms */
    for (count = 0 ; !*endOfFile && (count < event->digests.count) ; count++) {
	/* read the digest algorithm */
	if (rc == 0) {
	    readSize = fread(&(event->digests.digests[count].hashAlg),
			     sizeof((TPMT_HA *)NULL)->hashAlg, 1, inFile);
	    if (readSize != 1) {
		printf("TSS_EVENT2_Line_Read: "
		       "Error, could not read digest algorithm, returned %lu\n",
		       (unsigned long)readSize);
		rc = ERR_STRUCTURE;
	    }
	}
	/* do the endian conversion of the hash algorithm from stream to uint16_t */
	if (rc == 0) {
	    event->digests.digests[count].hashAlg =
		Uint16_Convert(event->digests.digests[count].hashAlg);
	}
	/* map from the digest algorithm to the digest length */
	uint16_t digestSize;
	if (rc == 0) {
	    digestSize = TSS_GetDigestSize(event->digests.digests[count].hashAlg);
	    if (digestSize == 0) {
		printf("TSS_EVENT2_Line_Read: Error, unknown digest algorithm %04x*\n",
		       event->digests.digests[count].hashAlg);
		rc = ERR_STRUCTURE;
	    }
	}
	/* read the digest */
	if (rc == 0) {
	    readSize = fread((uint8_t *)&(event->digests.digests[count].digest),
			     digestSize, 1, inFile);
	    if (readSize != 1) {
		printf("TSS_EVENT2_Line_Read: Error, could not read digest, returned %lu\n",
		       (unsigned long)readSize);
		rc = ERR_STRUCTURE;
	    }
	}
    }
    /* read the event size */
    if (!*endOfFile && (rc == 0)) {
	readSize = fread(&(event->eventSize),
			 sizeof(((TCG_PCR_EVENT2 *)NULL)->eventSize), 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT2_Line_Read: Error, could not read event size, returned %lu\n",
		   (unsigned long)readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* do the endian conversion from stream to uint32_t */
    if (!*endOfFile && (rc == 0)) {
	event->eventSize = Uint32_Convert(event->eventSize);
    }
    /* bounds check the event size */
    if (!*endOfFile && (rc == 0)) {
	if (event->eventSize > sizeof(((TCG_PCR_EVENT2 *)NULL)->event)) {
	    printf("TSS_EVENT2_Line_Read: Error, event size too big: %u\n",
		   event->eventSize);
	    rc = ERR_STRUCTURE;
	}
    }
    /* read the event */
    if (!*endOfFile && (rc == 0)) {
	memset(event->event , 0, sizeof(((TCG_PCR_EVENT2 *)NULL)->event));
	readSize = fread(&(event->event),
			 event->eventSize, 1, inFile);
	if (readSize != 1) {
	    printf("TSS_EVENT2_Line_Read: Error, could not read event, returned %lu\n",
		   (unsigned long)readSize);
	    rc = ERR_STRUCTURE;
	}
    }
    return rc;
}

/* TSS_EVENT2_Line_Marshal() marshals a TCG_PCR_EVENT2 structure */

TPM_RC TSS_EVENT2_Line_Marshal(TCG_PCR_EVENT2 *source,
			       uint16_t *written, uint8_t **buffer, int32_t *size)
{
    TPM_RC rc = 0;

    if (rc == 0) {
	rc = TSS_UINT32_Marshal(&source->pcrIndex, written, buffer, size);
    }
    if (rc == 0) {
	rc = TSS_UINT32_Marshal(&source->eventType, written, buffer, size);
    }
    if (rc == 0) {
	rc = TSS_TPML_DIGEST_VALUES_Marshal(&source->digests, written, buffer, size);
    }
    if (rc == 0) {
	rc = TSS_UINT32_Marshal(&source->eventSize, written, buffer, size);
    }
    if (rc == 0) {
	rc = TSS_Array_Marshal((uint8_t *)source->event, source->eventSize, written, buffer, size);
    }
    return rc;
}

/* TSS_EVENT2_Line_Unmarshal() unmarshals a TCG_PCR_EVENT2 structure */


TPM_RC TSS_EVENT2_Line_Unmarshal(TCG_PCR_EVENT2 *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = 0;

    if (rc == 0) {
	rc = UINT32_Unmarshal(&target->pcrIndex, buffer, size);
    }
    if (rc == 0) {
	rc = UINT32_Unmarshal(&target->eventType, buffer, size);
    }
    if (rc == 0) {
	rc = TPML_DIGEST_VALUES_Unmarshal(&target->digests, buffer, size);
    }
    if (rc == 0) {
	rc = UINT32_Unmarshal(&target->eventSize, buffer, size);
    }
    if (rc == 0) {
	rc = Array_Unmarshal((uint8_t *)target->event, target->eventSize, buffer, size);
    }
    return rc;
}

/* TSS_EVENT2_PCR_Extend() extends a PCR digest with the digest from the TCG_PCR_EVENT2 event log
   entry.

   FIXME - currently handles only PCR 0-7 and SHA-256.
*/

TPM_RC TSS_EVENT2_PCR_Extend(TPMT_HA pcrs[8],
			     TCG_PCR_EVENT2 *event2)
{
    TPM_RC rc = 0;
    uint32_t i;				/* iterator though hash algorithms */
    int foundSha256 = FALSE;
    
    /* validate PCR number */
    if (rc == 0) {
	if (event2->pcrIndex > 7) {
	    printf("ERROR: TSS_EVENT2_PCR_Extend: PCR number %u out of range\n", event2->pcrIndex);
	    rc = 1;
	}
    }
    /* validate event count */
    if (rc == 0) {
	uint32_t maxCount = sizeof(((TPML_DIGEST_VALUES *)NULL)->digests) / sizeof(TPMT_HA);
	if (event2->digests.count > maxCount) {
	    printf("ERROR: TSS_EVENT2_PCR_Extend: PCR count %u out of range, max %u\n",
		   event2->digests.count, maxCount);
	    rc = 1;
	}	    
    }
    /* search for the SHA-256 digest entry */
    for (i = 0; (rc == 0) && (i < event2->digests.count) && !foundSha256 ; i++) {
	if (event2->digests.digests[i].hashAlg == TPM_ALG_SHA256) {
	    rc = TSS_Hash_Generate(&pcrs[event2->pcrIndex],
				   SHA256_DIGEST_SIZE,
				   (uint8_t *)&pcrs[event2->pcrIndex].digest,
				   SHA256_DIGEST_SIZE,
				   &event2->digests.digests[i].digest,
				   0, NULL);
	    foundSha256 = TRUE;
	}
    }
    if ((rc == 0) && !foundSha256) {
	printf("ERROR: TSS_EVENT2_PCR_Extend: no SHA-256 entry in event record, PCR %u\n",
	       event2->pcrIndex);
	rc = 1;
    }
    return rc;
}

/* Uint16_Convert() converts a little endian uint16_t (from an input stream) to host byte order
 */

static uint16_t Uint16_Convert(uint16_t in)
{
    uint16_t out = 0;
    unsigned char *inb = (unsigned char *)&in;
    
    /* little endian input */
    out = (inb[0] <<  0) |
	  (inb[1] <<  8);
    return out;
}

/* Uint32_Convert() converts a little endian uint32_t (from an input stream) to host byte order
 */

static uint32_t Uint32_Convert(uint32_t in)
{
    uint32_t out = 0;
    unsigned char *inb = (unsigned char *)&in;
    
    /* little endian input */
    out = (inb[0] <<  0) |
	  (inb[1] <<  8) |
	  (inb[2] << 16) |
	  (inb[3] << 24);
    return out;
}

/* UINT16LE_Unmarshal() unmarshals a little endian 4-byte array from buffer into a HBO uint16_t */

static TPM_RC
UINT16LE_Unmarshal(uint16_t *target, BYTE **buffer, int32_t *size)
{
    if ((uint16_t)*size < sizeof(uint16_t)) {
	return TPM_RC_INSUFFICIENT;
    }
    *target = ((uint16_t)((*buffer)[0]) <<  0) |
	      ((uint16_t)((*buffer)[1]) <<  8);
    *buffer += sizeof(uint16_t);
    *size -= sizeof(uint16_t);
    return TPM_RC_SUCCESS;
}

/* uint32LE_Unmarshal() unmarshals a little endian 4-byte array from buffer into a HBO uint32_t */

static TPM_RC
UINT32LE_Unmarshal(uint32_t *target, BYTE **buffer, int32_t *size)
{
    if ((uint32_t)*size < sizeof(uint32_t)) {
	return TPM_RC_INSUFFICIENT;
    }
    *target = ((uint32_t)((*buffer)[0]) <<  0) |
	      ((uint32_t)((*buffer)[1]) <<  8) |
	      ((uint32_t)((*buffer)[2]) << 16) |
	      ((uint32_t)((*buffer)[3]) << 24);
    *buffer += sizeof(uint32_t);
    *size -= sizeof(uint32_t);
    return TPM_RC_SUCCESS;
}


void TSS_EVENT2_Line_Trace(TCG_PCR_EVENT2 *event)
{
    printf("TSS_EVENT2_Line_Trace: PCR index %u\n", event->pcrIndex);
    TSS_EVENT_EventType_Trace(event->eventType);
    printf("TSS_EVENT2_Line_Trace: digest count %u\n", event->digests.count);
    uint32_t count;
    for (count = 0 ; count < event->digests.count ; count++) {
	printf("TSS_EVENT2_Line_Trace: digest %u algorithm %04x\n",
	       count, event->digests.digests[count].hashAlg);
	uint16_t digestSize = TSS_GetDigestSize(event->digests.digests[count].hashAlg);
	TSS_PrintAll("TSS_EVENT2_Line_Trace: PCR",
		     (uint8_t *)&event->digests.digests[count].digest, digestSize);
    }
    TSS_PrintAll("TSS_EVENT2_Line_Trace: event",
		 event->event, event->eventSize);
    return;
}

/* tables to map eventType to text */

typedef struct {
    uint32_t eventType;
    const char *text;
} EVENT_TYPE_TABLE;

const EVENT_TYPE_TABLE eventTypeTable [] = {
    {EV_PREBOOT_CERT, "EV_PREBOOT_CERT"},
    {EV_POST_CODE, "EV_POST_CODE"},
    {EV_UNUSED, "EV_UNUSED"},
    {EV_NO_ACTION, "EV_NO_ACTION"},
    {EV_SEPARATOR, "EV_SEPARATOR"},
    {EV_ACTION, "EV_ACTION"},
    {EV_EVENT_TAG, "EV_EVENT_TAG"},
    {EV_S_CRTM_CONTENTS, "EV_S_CRTM_CONTENTS"},
    {EV_S_CRTM_VERSION, "EV_S_CRTM_VERSION"},
    {EV_CPU_MICROCODE, "EV_CPU_MICROCODE"},
    {EV_PLATFORM_CONFIG_FLAGS, "EV_PLATFORM_CONFIG_FLAGS"},
    {EV_TABLE_OF_DEVICES, "EV_TABLE_OF_DEVICES"},
    {EV_COMPACT_HASH, "EV_COMPACT_HASH"},
    {EV_IPL, "EV_IPL"},
    {EV_IPL_PARTITION_DATA, "EV_IPL_PARTITION_DATA"},
    {EV_NONHOST_CODE, "EV_NONHOST_CODE"},
    {EV_NONHOST_CONFIG, "EV_NONHOST_CONFIG"},
    {EV_NONHOST_INFO, "EV_NONHOST_INFO"},
    {EV_OMIT_BOOT_DEVICE_EVENTS, "EV_OMIT_BOOT_DEVICE_EVENTS"},
    {EV_EFI_EVENT_BASE, "EV_EFI_EVENT_BASE"},
    {EV_EFI_VARIABLE_DRIVER_CONFIG, "EV_EFI_VARIABLE_DRIVER_CONFIG"},
    {EV_EFI_VARIABLE_BOOT, "EV_EFI_VARIABLE_BOOT"},
    {EV_EFI_BOOT_SERVICES_APPLICATION, "EV_EFI_BOOT_SERVICES_APPLICATION"},
    {EV_EFI_BOOT_SERVICES_DRIVER, "EV_EFI_BOOT_SERVICES_DRIVER"},
    {EV_EFI_RUNTIME_SERVICES_DRIVER, "EV_EFI_RUNTIME_SERVICES_DRIVER"},
    {EV_EFI_GPT_EVENT, "EV_EFI_GPT_EVENT"},
    {EV_EFI_ACTION, "EV_EFI_ACTION"},
    {EV_EFI_PLATFORM_FIRMWARE_BLOB, "EV_EFI_PLATFORM_FIRMWARE_BLOB"},
    {EV_EFI_HANDOFF_TABLES, "EV_EFI_HANDOFF_TABLES"},
    {EV_EFI_HCRTM_EVENT, "EV_EFI_HCRTM_EVENT"},
    {EV_EFI_VARIABLE_AUTHORITY, "EV_EFI_VARIABLE_AUTHORITY"}
};

static void TSS_EVENT_EventType_Trace(uint32_t eventType)
{
    size_t i;

    for (i = 0 ; i < sizeof(eventTypeTable) / sizeof(EVENT_TYPE_TABLE) ; i++) {
	if (eventTypeTable[i].eventType == eventType) {
	    printf("TSS_EVENT_EventType_Trace: %08x %s\n",
		   eventTypeTable[i].eventType, eventTypeTable[i].text);
	    return;
	}
    }
    printf("TSS_EVENT_EventType_Trace: %08x Unknown\n", eventType);
    return;
}

const char *TSS_EVENT_EventTypeToString(uint32_t eventType)
{
    const char *crc = NULL;
    size_t i;

    for (i = 0 ; i < sizeof(eventTypeTable) / sizeof(EVENT_TYPE_TABLE) ; i++) {
	if (eventTypeTable[i].eventType == eventType) {
	    crc = eventTypeTable[i].text;
	}
    }
    if (crc == NULL) {
	crc = "Unknown event type";
    }
    return crc;
}

