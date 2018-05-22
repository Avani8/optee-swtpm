/********************************************************************************/
/*										*/
/*			   RSA_Encrypt						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: rsaencrypt.c 1072 2017-09-11 19:55:31Z kgoldman $		*/
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

static void printRsaEncrypt(RSA_Encrypt_Out *out);
static void printUsage(void);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC			rc = 0;
    int				i;    /* argc iterator */
    TSS_CONTEXT			*tssContext = NULL;
    RSA_Encrypt_In 		in;
    RSA_Encrypt_Out 		out;
    TPMI_DH_OBJECT		keyHandle = 0;
    const char			*decryptFilename = NULL;
    const char			*encryptFilename = NULL;

    uint16_t			written = 0;
    size_t 			length = 0;
    uint8_t			*buffer = NULL;	/* for the free */
    uint8_t			*buffer1 = NULL;	/* for marshaling */

    setvbuf(stdout, 0, _IONBF, 0);      /* output may be going through pipe to log file */
    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "1");

    /* command line argument defaults */
    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-hk") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x",&keyHandle);
	    }
	    else {
		printf("Missing parameter for -hk\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-id") == 0) {
	    i++;
	    if (i < argc) {
		decryptFilename = argv[i];
	    }
	    else {
		printf("-id option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-oe") == 0) {
	    i++;
	    if (i < argc) {
		encryptFilename = argv[i];
	    }
	    else {
		printf("-oe option needs a value\n");
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
    if (keyHandle == 0) {
	printf("Missing handle parameter -hk\n");
	printUsage();
    }
    if (decryptFilename == NULL) {
	printf("Missing decrypted file -id\n");
	printUsage();
    }
    if (rc == 0) {
	rc = TSS_File_ReadBinaryFile(&buffer,     /* must be freed by caller */
				     &length,
				     decryptFilename);
    }
    if (rc == 0) {
	if (length > MAX_RSA_KEY_BYTES) {
	    printf("Input data too long %u\n", (unsigned int)length);
	    rc = TSS_RC_INSUFFICIENT_BUFFER;
	}
    }
    if (rc == 0) {
	/* Handle of key that will perform rsaencrypting */
	in.keyHandle = keyHandle;

	/* Table 158 - Definition of {RSA} TPM2B_PUBLIC_KEY_RSA Structure */
	{
	    in.message.t.size = (uint16_t)length;	/* cast safe, range tested above */
	    memcpy(in.message.t.buffer, buffer, length);
	}
	/* padding scheme */
	{
	    /* Table 157 - Definition of {RSA} TPMT_RSA_DECRYPT Structure */
	    in.inScheme.scheme = TPM_ALG_NULL;
	}
	/* label */
	{
	    /* Table 73 - Definition of TPM2B_DATA Structure */
	    in.label.t.size = 0;
	}
    }
    free (buffer);
    buffer = NULL;
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
			 TPM_CC_RSA_Encrypt,
			 TPM_RH_NULL, NULL, 0);
    }
    {
	TPM_RC rc1 = TSS_Delete(tssContext);
	if (rc == 0) {
	    rc = rc1;
	}
    }
    if ((rc == 0) && (encryptFilename != NULL)) {
	written = 0;
	rc = TSS_TPM2B_PUBLIC_KEY_RSA_Marshal(&out.outData, &written, NULL, NULL);
    }    
    if ((rc == 0) && (encryptFilename != NULL)) {
	buffer = realloc(buffer, written);
	buffer1 = buffer;
	written = 0;
	rc = TSS_TPM2B_PUBLIC_KEY_RSA_Marshal(&out.outData, &written, &buffer1, NULL);
    }    
    if ((rc == 0) && (encryptFilename != NULL)) {
	rc = TSS_File_WriteBinaryFile(buffer + sizeof(uint16_t),
				      written - sizeof(uint16_t),
				      encryptFilename); 
    }    
    free(buffer);
    if (rc == 0) {
	if (verbose) printRsaEncrypt(&out);
	if (verbose) printf("rsaencrypt: success\n");
    }
    else {
	const char *msg;
	const char *submsg;
	const char *num;
	printf("rsaencrypt: failed, rc %08x\n", rc);
	TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
	printf("%s%s%s\n", msg, submsg, num);
	rc = EXIT_FAILURE;
    }
    return rc;
}

static void printRsaEncrypt(RSA_Encrypt_Out *out)
{
    TSS_PrintAll("outData", out->outData.t.buffer, out->outData.t.size);
}

static void printUsage(void)
{
    printf("\n");
    printf("rsaencrypt\n");
    printf("\n");
    printf("Runs TPM2_RSA_Encrypt\n");
    printf("\n");
    printf("\t-hk key handle\n");
    printf("\t-id decrypt file name\n");
    printf("\t[-oe encrypt file name (default do not save)]\n");
    exit(1);	
}
