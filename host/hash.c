/********************************************************************************/
/*										*/
/*			    Hash						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: hash.c 1112 2017-12-13 21:55:26Z kgoldman $			*/
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

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/tssresponsecode.h>
#include <tss2/tssmarshal.h>

static void printUsage(void);
static void printHash(Hash_Out *out);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC			rc = 0;
    int				i;    /* argc iterator */
    TSS_CONTEXT			*tssContext = NULL;
    Hash_In 			in;
    Hash_Out 			out;
    char 			hierarchyChar = 'n';
    TPMI_RH_HIERARCHY		hierarchy = TPM_RH_NULL;
    TPMI_ALG_HASH		halg = TPM_ALG_SHA256;
    const char			*inFilename = NULL;
    const char 			*inString = NULL;
    const char			*hashFilename = NULL;
    const char			*ticketFilename = NULL;
 
    size_t 			length = 0;
    uint8_t			*buffer = NULL;	/* for the free */

    setvbuf(stdout, 0, _IONBF, 0);      /* output may be going through pipe to log file */
    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "1");

    /* command line argument defaults */
    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-hi") == 0) {
	    i++;
	    if (i < argc) {
		hierarchyChar = argv[i][0];
	    }
	    else {
		printf("Missing parameter for -hi\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-halg") == 0) {
	    i++;
	    if (i < argc) {
		if (strcmp(argv[i],"sha1") == 0) {
		    halg = TPM_ALG_SHA1;
		}
		else if (strcmp(argv[i],"sha256") == 0) {
		    halg = TPM_ALG_SHA256;
		}
		else if (strcmp(argv[i],"sha384") == 0) {
		    halg = TPM_ALG_SHA384;
		}
		else {
		    printf("Bad parameter for -halg\n");
		    printUsage();
		}
	    }
	    else {
		printf("-halg option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ic") == 0) {
	    i++;
	    if (i < argc) {
		inString = argv[i];
	    }
	    else {
		printf("-ic option needs a value\n");
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
 	else if (strcmp(argv[i],"-oh") == 0) {
	    i++;
	    if (i < argc) {
		hashFilename = argv[i];
	    }
	    else {
		printf("-oh option needs a value\n");
		printUsage();
	    }
	}
 	else if (strcmp(argv[i],"-tk") == 0) {
	    i++;
	    if (i < argc) {
		ticketFilename = argv[i];
	    }
	    else {
		printf("-tk option needs a value\n");
		printUsage();
	    }
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
    if ((inFilename == NULL) && (inString == NULL)) {
	printf("Input file -if or input string -ic must be specified\n");
	printUsage();
    }
    if ((inFilename != NULL) && (inString != NULL)) {
	printf("Input file -if and input string -ic cannot both be specified\n");
	printUsage();
    }
    /* Table 50 - TPMI_RH_HIERARCHY primaryHandle */
    if (rc == 0) {
	if (hierarchyChar == 'e') {
	    hierarchy = TPM_RH_ENDORSEMENT;
	}
	else if (hierarchyChar == 'o') {
	    hierarchy = TPM_RH_OWNER;
	}
	else if (hierarchyChar == 'p') {
	    hierarchy = TPM_RH_PLATFORM;
	}
	else if (hierarchyChar == 'n') {
	    hierarchy = TPM_RH_NULL;
	}
	else {
	    printf("Bad parameter %c for -hi\n", hierarchyChar);
	    printUsage();
	}
 	in.hierarchy = hierarchy;
    }
    if (inFilename != NULL) {
	if (rc == 0) {
	    rc = TSS_File_ReadBinaryFile(&buffer,     /* must be freed by caller */
					 &length,
					 inFilename);
	}
	if (rc == 0) {
	    if (length > MAX_DIGEST_BUFFER) {
		printf("Input data too long %lu\n", (unsigned long)length);
		rc = TSS_RC_INSUFFICIENT_BUFFER;
	    }
	}
	if (rc == 0) {
	    /* data to be hashed */
	    in.data.t.size = (uint16_t)length;	/* cast safe, range tested above */
	    memcpy(in.data.t.buffer, buffer, length);
	}
    }
    if (inString != NULL) {
	if (rc == 0) {
	    length = strlen(inString);
	    if (length > MAX_DIGEST_BUFFER) {
		printf("Input data too long %lu\n", (unsigned long)length);
		rc = TSS_RC_INSUFFICIENT_BUFFER;
	    } 
	}
	if (rc == 0) {
	    /* data to be hashed */
	    in.data.t.size = (uint16_t)length;	/* cast safe, range tested above */
	    memcpy(in.data.t.buffer, inString, length);
	}
    }
    if (rc == 0) {
	in.hashAlg = halg;
    }
    /* Start a TSS context */
    if (rc == 0) {
	rc = TSS_Create(&tssContext);
    }
    /* call TSS to execute the command */
    if (rc == 0) {
	rc = TSS_Execute(tssContext,
			 (RESPONSE_PARAMETERS *)&out, 
			 (COMMAND_PARAMETERS *)&in,
			 NULL,
			 TPM_CC_Hash,
			 TPM_RH_NULL, NULL, 0);
    }
    {
	TPM_RC rc1 = TSS_Delete(tssContext);
	if (rc == 0) {
	    rc = rc1;
	}
    }
    if ((rc == 0) && (hashFilename != NULL)) {
	rc = TSS_File_WriteBinaryFile(out.outHash.t.buffer,
				      out.outHash.t.size,
				      hashFilename); 
    }
    if ((rc == 0) && (ticketFilename != NULL)) {
	rc = TSS_File_WriteStructure(&out.validation,
				     (MarshalFunction_t)TSS_TPMT_TK_HASHCHECK_Marshal,
				     ticketFilename);
    }
    free(buffer);
    if (rc == 0) {
	if (verbose) printHash(&out);
	if (verbose) printf("hash: success\n");
    }
    else {
	const char *msg;
	const char *submsg;
	const char *num;
	printf("hash: failed, rc %08x\n", rc);
	TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
	printf("%s%s%s\n", msg, submsg, num);
	rc = EXIT_FAILURE;
    }
    return rc;
}

static void printHash(Hash_Out *out)
{
    TSS_PrintAll("Hash", out->outHash.t.buffer, out->outHash.t.size);
}

static void printUsage(void)
{
    printf("\n");
    printf("hash\n");
    printf("\n");
    printf("Runs TPM2_Hash\n");
    printf("\n");
    printf("\t[-hi hierarchy (e, o, p, n) (default null)]\n");
    printf("\t\te endorsement, o owner, p platform, n null\n");
    printf("\t[-halg (sha1, sha256, sha384) (default sha256)]\n");
    printf("\t-if input file to be hashed\n");
    printf("\t-ic data string to be hashed\n");
    printf("\t[-oh hash file name (default do not save)]\n");
    printf("\t[-tk ticket file name (default do not save)]\n");
    exit(1);	
}
