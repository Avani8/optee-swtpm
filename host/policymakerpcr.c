/********************************************************************************/
/*										*/
/*			   policymaker						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: policymakerpcr.c 1072 2017-09-11 19:55:31Z kgoldman $		*/
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

/* 
   policymakerpcr calculates a policyPCR term suitable for input to policymaker

   Inputs are:

   a hash algorithm

   a byte mask, totally big endian, e.g. 010000 is PCR 16 

   a file with lines in hexascii representing PCRs, e.g., the output of pcrread with white space
   removed

   This assumes that the byte mask and PCR value file are consistent.
   
   Outputs are:

   if specified, a file with a hex ascii policyPCR line suitable for input to policymaker

   if specified, a print of the hash

   Example: 

   policymakerpcr -bm 010000 -if policies/policypcr16aaasha1.txt -v -pr -of policies/policypcr.txt

   Where policypcr16aaasha1.txt is represents the SHA-1 value of PCR 16
   
   1d47f68aced515f7797371b554e32d47981aa0a0
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <openssl/err.h>
#include <openssl/evp.h>

#ifdef TPM_POSIX
#include <netinet/in.h>
#endif
#ifdef TPM_WINDOWS
#include <winsock2.h>
#endif

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/tsscryptoh.h>
#include <tss2/tsscrypto.h>
#include <tss2/tssmarshal.h>

static void printUsage(void);
static void printPolicyPCR(FILE *out,
			   uint32_t           	sizeInBytes,         		
			   TPML_PCR_SELECTION	*pcrs,
			   TPMT_HA 		*digest);
static int Format_FromHexascii(unsigned char *binary,
			       const char *string,
			       size_t length);
static int Format_ByteFromHexascii(unsigned char *byte,
				   const char *string);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC		rc = 0;
    int			i;    			/* argc iterator */
    char 		*prc = NULL;		/* pointer return code */
    const char 		*inFilename = NULL;
    const char 		*outFilename = NULL;
    FILE 		*inFile = NULL;
    FILE 		*outFile = NULL;
    uint32_t           	sizeInBytes;		/* hash algorithm mapped to size */
    uint32_t	  	pcrmask = 0xffffffff;	/* pcr register mask */
    TPML_PCR_SELECTION	pcrs;
    unsigned int 	pcrCount = 0;
    TPMU_HA		pcr[IMPLEMENTATION_PCR];	/* all the PCRs */
    int			pr = FALSE;
    TPMT_HA 		digest;
    uint8_t		pcrBytes[IMPLEMENTATION_PCR * MAX_DIGEST_SIZE];
    uint16_t		pcrLength;

    /* command line defaults */
    digest.hashAlg = TPM_ALG_SHA256;

    ERR_load_crypto_strings ();
    OpenSSL_add_all_algorithms ();

    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-halg") == 0) {
	    i++;
	    if (i < argc) {
		if (strcmp(argv[i],"sha256") == 0) {
		    digest.hashAlg = TPM_ALG_SHA256;
		}
		else if (strcmp(argv[i],"sha1") == 0) {
		    digest.hashAlg = TPM_ALG_SHA1;
		}
		else {
		    printf("Bad parameter for -halg\n");
		    printUsage();
		}
	    }
	    else {
		printf("Missing parameter for -hi\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-bm") == 0) {
	    i++;
	    if (i < argc) {
		if (1 != sscanf(argv[i], "%x", &pcrmask)) {
		    printf("Invalid -bm argument '%s'\n", argv[i]);
		    printUsage();
		}
	    }
	    else {
		printf("-bm option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-of") == 0) {
	    i++;
	    if (i < argc) {
		outFilename = argv[i];
	    }
	    else {
		printf("-of option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-if") == 0) {
	    i++;
	    if (i < argc) {
		inFilename = argv[i];
	    }
	    else {
		printf("-if option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-pr") == 0) {
	    pr = TRUE;
	}
	else if (strcmp(argv[i],"-h") == 0) {
	    printUsage();
	}
	else if (strcmp(argv[i],"-v") == 0) {
	    verbose = TRUE;
	    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "2");
	}
	else {
	    printf("\n%s is not a valid option\n", argv[i]);
	    printUsage();
	}
    }
    if (pcrmask == 0xffffffff) {
	printf("Missing or illegal pcr byte mask parameter -bm\n");
	printUsage();
    }
    if ((pcrmask != 0) && (inFilename == NULL)) {
	printf("Missing file name parameter -if\n");
	printUsage();
    }
    if ((pcrmask == 0) && (inFilename != NULL)) {
	printf("Unnecessary file name parameter -if\n");
	printUsage();
    }
    /* open the input file if needed */
    if ((rc == 0) && (pcrmask != 0)) {
	inFile = fopen(inFilename, "r");
	if (inFile == NULL) {
	    printf("Error opening %s for %s, %s\n", inFilename, "r", strerror(errno));
	    rc = EXIT_FAILURE;
	}
    }
    if (rc == 0) {
	sizeInBytes = TSS_GetDigestSize(digest.hashAlg);
    }
    /* Table 102 - Definition of TPML_PCR_SELECTION Structure */
    if (rc == 0) {
	pcrs.count = 1;		/* hard code one hash algorithm */
	/* Table 85 - Definition of TPMS_PCR_SELECTION Structure - pcrSelections */
	pcrs.pcrSelections[0].hash = digest.hashAlg;
	pcrs.pcrSelections[0].sizeofSelect= 3;	/* hard code 24 PCRs */
	/* TCG always marshals lower PCR first */
	pcrs.pcrSelections[0].pcrSelect[0] = (pcrmask >>  0) & 0xff;
	pcrs.pcrSelections[0].pcrSelect[1] = (pcrmask >>  8) & 0xff;
	pcrs.pcrSelections[0].pcrSelect[2] = (pcrmask >> 16) & 0xff;
    }
    /* read the input file to the PCR array, assumes the PCR select bm has the correct number of
       bits */
    /* iterate through each line */
    for (pcrCount = 0 ;
	 (rc == 0) && (pcrCount < IMPLEMENTATION_PCR) && (inFile != NULL) ;
	 pcrCount++) {
	
	char 		lineString[256];		/* returned line in hex ascii */
	uint32_t	lineLength;			

	if (rc == 0) {
	    prc = fgets(lineString, sizeof(lineString), inFile);
	}
	/* no more lines, pcrCount is number of PCRs processed */
	if (rc == 0) {
	    if (prc == NULL) {
		break;
	    }
	}
	if (rc == 0) {
	    lineLength = strlen(lineString);
	    if (lineLength == 0) {
		break;
	    }
	    if (lineString[lineLength-1] == '\n') {
		lineString[lineLength-1] = '0';
		lineLength--;
	    }
	}
	if (rc == 0) {
	    if (lineLength != (sizeInBytes *2)) {
		printf("Line length %u is not twice digest size %u\n", lineLength, sizeInBytes);
		rc = -1;
	    }
	}	
	/* convert hex ascii to binary */ 
	if ((rc == 0) && (prc != NULL)) {
	    rc = Format_FromHexascii((uint8_t *)&pcr[pcrCount],
				     lineString, lineLength/2);
	}
	if (rc == 0) {
	    if (verbose) printf("PCR %u\n", pcrCount);
	    if (verbose) TSS_PrintAll("PCR", (uint8_t *)&pcr[pcrCount], sizeInBytes);
	}
    }
    /* serialize PCRs */
    if (rc == 0) {
	unsigned int pc;
	uint8_t *buffer = pcrBytes;
	INT32 size = IMPLEMENTATION_PCR * MAX_DIGEST_SIZE;
	pcrLength = 0;
	for (pc = 0 ; (rc == 0) && (pc < pcrCount) ; pc++) {
	    rc = TSS_Array_Marshal((uint8_t *)&pcr[pc], sizeInBytes, &pcrLength, &buffer, &size);
	}
    }
    /* hash the marshaled PCR array */
    if (rc == 0) {
	rc = TSS_Hash_Generate(&digest,
			       pcrLength, pcrBytes,
			       0, NULL);
    }
    if (rc == 0) {
	if (verbose) TSS_PrintAll("PCR composite digest", (uint8_t *)&digest.digest, sizeInBytes);
    }
    if ((rc == 0) && pr) {
	printPolicyPCR(stdout,
		       sizeInBytes,
		       &pcrs,
		       &digest);
    }
    if (outFilename != NULL) {
	if (rc == 0) {
	    outFile = fopen(outFilename, "wb");
	    if (outFile == NULL) {
		printf("Error opening %s for %s, %s\n", outFilename , "W", strerror(errno));
		rc = EXIT_FAILURE;
	    }
	}
	if (rc == 0) {
	    printPolicyPCR(outFile,
			   sizeInBytes,
			   &pcrs,
			   &digest);
	}
    }
    if (inFile != NULL) {
	fclose(inFile);
    }
    if (outFile != NULL) {
	fclose(outFile);
    }
    if (rc != 0) {
	rc = EXIT_FAILURE;
    }
    return rc;
}

static void printPolicyPCR(FILE 		*out,
			   uint32_t           	sizeInBytes,         		
			   TPML_PCR_SELECTION	*pcrs,
			   TPMT_HA 		*digest)
{
    unsigned int i;
    uint8_t *pcrDigest = (uint8_t *)&digest->digest;

    fprintf(out, "%02x", 0xff & (TPM_CC_PolicyPCR >> 24));
    fprintf(out, "%02x", 0xff & (TPM_CC_PolicyPCR >> 16));
    fprintf(out, "%02x", 0xff & (TPM_CC_PolicyPCR >>  8));
    fprintf(out, "%02x", 0xff & (TPM_CC_PolicyPCR >>  0));
    /* NOTE only handles count of 1, 1 hash algorithm */
    fprintf(out, "%08x", pcrs->count);

    fprintf(out, "%02x", 0xff & (pcrs->pcrSelections[0].hash >> 8));
    fprintf(out, "%02x", 0xff & (pcrs->pcrSelections[0].hash >> 0));

    fprintf(out, "%02x", pcrs->pcrSelections[0].sizeofSelect);
    
    fprintf(out, "%02x", pcrs->pcrSelections[0].pcrSelect[0]);
    fprintf(out, "%02x", pcrs->pcrSelections[0].pcrSelect[1]);
    fprintf(out, "%02x", pcrs->pcrSelections[0].pcrSelect[2]);

    for (i = 0 ; i < sizeInBytes ; i++) {
	fprintf(out, "%02x", pcrDigest[i]);
    }
    fprintf(out, "\n");
    return;
}

/* Format_FromHexAscii() converts 'string' in hex ascii to 'binary' of 'length'

   It assumes that the string has enough bytes to accommodate the length.
*/

static int Format_FromHexascii(unsigned char *binary,
			       const char *string,
			       size_t length)
{
    int 	rc = 0;
    size_t	i;

    for (i = 0 ; (rc == 0) && (i < length) ; i++) {
	rc = Format_ByteFromHexascii(binary + i,
				     string + (i * 2));
	
    }
    return rc;
}

/* Format_ByteFromHexAscii() converts two bytes of hex ascii to one byte of binary
 */

static int Format_ByteFromHexascii(unsigned char *byte,
				   const char *string)
{
    int 	rc = 0;
    size_t	i;
    char	c;
    *byte 	= 0;
    
    for (i = 0 ; (rc == 0) && (i < 2) ; i++) {
	(*byte) <<= 4;		/* big endian, shift up the nibble */
	c = *(string + i);	/* extract the next character from the string */

	if ((c >= '0') && (c <= '9')) {
	    *byte += c - '0';
	}
	else if ((c >= 'a') && (c <= 'f')) {
	    *byte += c + 10 - 'a';
	}
	else if ((c >= 'A') && (c <= 'F')) {
	    *byte += c + 10 - 'A';
	}
	else {
	    printf("Format_ByteFromHexascii: "
		   "Error: Line has non hex ascii character: %c\n", c);
	    rc = EXIT_FAILURE;
	}
    }
    return rc;
}


static void printUsage(void)
{
    printf("\n");
    printf("policymakerpcr\n");
    printf("\n");
    printf("Creates a policyPCR term suitable for input to policymaker (hex ascii)\n");
    printf("\n");
    printf("Assumes that the byte mask and PCR values are consistent\n");
    printf("\n");
    printf("[-halg hash algorithm  (sha1 sha256 sha384) (default sha256)]\n");
    printf("-bm pcr byte mask in hex, big endian\n");
    printf("\te.g. 010000 selects PCR 16\n");
    printf("\te.g. ffffff selects all 24 PCRs\n");
    printf("-if input file - PCR values, hex ascii, one per line, %u max\n", IMPLEMENTATION_PCR);
    printf("\trequired unless pcr mask is 0\n");
    printf("[-of output file - policy hash in binary]\n");
    printf("[-pr stdout - policy hash in hex ascii]\n");
    printf("\n");
    exit(1);	
}
