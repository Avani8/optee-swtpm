/********************************************************************************/
/*										*/
/*			   policymaker						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: policymaker.c 1072 2017-09-11 19:55:31Z kgoldman $		*/
/*										*/
/* (c) Copyright IBM Corporation 2015, 2017					*/
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
   policymaker calculates a TPM2 policy hash

   Inputs are:

   a hash algorithm
   a file with lines in hexascii, to be extended into the policy digest, big endian

   NOTE: Empty lines (lines with just a newline character) are permitted and cause a double hash.
   This is useful for e.g. TPM2_PolicySigned when the policyRef is empty.

   Outputs are:

   if specified, a file with a binary digest
   if specified, a print of the hash

   Example input: policy command code with a command code of NV write

   0000016c00000137
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <openssl/err.h>
#include <openssl/evp.h>

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/tsscryptoh.h>
#include <tss2/tsscrypto.h>

static void printUsage(void);
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
    int			pr = FALSE;
    int			nz = FALSE;
    TPMT_HA 		digest;
    uint32_t           	sizeInBytes;		/* hash algorithm mapped to size */
    uint32_t           	startSizeInBytes;	/* starting buffer for extend */
    FILE 		*inFile = NULL;
    FILE 		*outFile = NULL;

	/* command line defaults */
    digest.hashAlg = TPM_ALG_SHA256;

    ERR_load_crypto_strings ();
    OpenSSL_add_all_algorithms ();

    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-halg") == 0) {
	    i++;
	    if (i < argc) {
		if (strcmp(argv[i],"sha1") == 0) {
		    digest.hashAlg = TPM_ALG_SHA1;
		}
		else if (strcmp(argv[i],"sha256") == 0) {
		    digest.hashAlg = TPM_ALG_SHA256;
		}
		else if (strcmp(argv[i],"sha384") == 0) {
		    digest.hashAlg = TPM_ALG_SHA384;
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
	else if (strcmp(argv[i],"-pr") == 0) {
	    pr = TRUE;
	}
	else if (strcmp(argv[i],"-nz") == 0) {
	    nz = TRUE;
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
    if (inFilename == NULL) {
	printf("Missing input file parameter -if\n");
	printUsage();
    }
    /* open the input file */
    if (rc == 0) {
	inFile = fopen(inFilename, "r");
	if (inFile == NULL) {
	    printf("Error opening %s for %s, %s\n", inFilename, "r", strerror(errno));
	    rc = EXIT_FAILURE;
	}
    }
    if (rc == 0) {
	sizeInBytes = TSS_GetDigestSize(digest.hashAlg);
	/* startauthsession sets session digest to zero */
	if (!nz) {
	    startSizeInBytes = sizeInBytes;
	    memset((uint8_t *)&digest.digest, 0, sizeInBytes);
	}
	else {	/* nz TRUE, start with empty buffer */
	    startSizeInBytes = 0;
	}
    }
    /* iterate through each line */
    do {
	char 		lineString[256];		/* returned line in hex ascii */
	unsigned char 	lineBinary[128];		/* returned line in binary */
	size_t		lineLength;			

	if (rc == 0) {
	    prc = fgets(lineString, sizeof(lineString), inFile);
	}
	/* convert hex ascii to binary */ 
	if ((rc == 0) && (prc != NULL)) {
	    lineLength = strlen(lineString);
	    rc = Format_FromHexascii(lineBinary,
				     lineString, lineLength/2);
	}
	/* hash extend */
	if ((rc == 0) && (prc != NULL)) {
	    TSS_Hash_Generate(&digest,
			      startSizeInBytes, (uint8_t *)&digest.digest,	/* extend */
			      lineLength /2, lineBinary,
			      0, NULL);
	}
	if ((rc == 0) && (prc != NULL)) {
	    if (verbose) TSS_PrintAll("intermediate policy digest",
				      (uint8_t *)&digest.digest, sizeInBytes);
	}
    }
    while ((rc == 0) && (prc != NULL));

    if ((rc == 0) && pr) {
	TSS_PrintAll("policy digest", (uint8_t *)&digest.digest, sizeInBytes);
    }
    /* open the output file */
    if ((rc == 0) && (outFilename != NULL)) {
	outFile = fopen(outFilename, "wb");
	if (outFile == NULL) {
	    printf("Error opening %s for %s, %s\n", outFilename , "W", strerror(errno));
	    rc = EXIT_FAILURE;
	}
    }
    if ((rc == 0) && (outFilename != NULL)) {
	fwrite((uint8_t *)&digest.digest, 1, sizeInBytes, outFile);
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
    printf("policymaker\n");
    printf("\n");
    printf("[-halg hash algorithm (sha1 sha256 sha384) (default sha256)\n");
    printf("[-nz do not extend starting with zeros, just hash the last line]\n");
    printf("-if input policy statements in hex ascii\n");
    printf("[-of] output file - policy hash in binary\n");
    printf("[-pr] stdout - policy hash in hex ascii\n");
    printf("\n");
    exit(1);	
}
