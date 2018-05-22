/********************************************************************************/
/*										*/
/*			    VerifySignature					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: verifysignature.c 1098 2017-11-27 23:07:26Z kgoldman $	*/
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

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <openssl/objects.h>
#include <openssl/rsa.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/Unmarshal_fp.h>
#include <tss2/tsscryptoh.h>
#include <tss2/tsscrypto.h>
#include <tss2/tssmarshal.h>
#include <tss2/tssresponsecode.h>

#include "cryptoutils.h"

static void printUsage(void);
TPM_RC rawUnmarshal(TPMT_SIGNATURE *target,
		    TPMI_ALG_PUBLIC algPublic,
		    TPMI_ALG_HASH halg,
		    uint8_t *buffer, size_t length);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC			rc = 0;
    int				i;    /* argc iterator */
    TSS_CONTEXT			*tssContext = NULL;
    VerifySignature_In 		in;
    VerifySignature_Out 	out;
    TPMI_DH_OBJECT		keyHandle = 0;
    const char			*pemFilename = NULL;
    const char			*signatureFilename = NULL;
    TPMI_ALG_HASH		halg = TPM_ALG_SHA256;
    TPMI_ALG_PUBLIC 		algPublic = TPM_ALG_RSA;
    const char			*messageFilename = NULL;
    int				doHash = TRUE;
    const char			*ticketFilename = NULL;
    int				raw = FALSE;	/* default TPMT_SIGNATURE */
    unsigned char 		*data = NULL;	/* message */
    size_t 			dataLength;
    uint8_t			*buffer = NULL;		/* for the free */
    uint8_t			*buffer1 = NULL;	/* for marshaling */
    size_t 			length = 0;
    uint32_t           		sizeInBytes;	/* hash algorithm mapped to size */
    TPMT_HA 			digest;		/* digest of the message */
    TPMI_SH_AUTH_SESSION    	sessionHandle0 = TPM_RH_NULL;
    unsigned int		sessionAttributes0 = 0;
    TPMI_SH_AUTH_SESSION    	sessionHandle1 = TPM_RH_NULL;
    unsigned int		sessionAttributes1 = 0;
    TPMI_SH_AUTH_SESSION    	sessionHandle2 = TPM_RH_NULL;
    unsigned int		sessionAttributes2 = 0;

    setvbuf(stdout, 0, _IONBF, 0);      /* output may be going through pipe to log file */
    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "1"); 

    /* command line argument defaults */
    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-hk") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &keyHandle);
	    }
	    else {
		printf("Missing parameter for -hk\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ipem") == 0) {
	    i++;
	    if (i < argc) {
		pemFilename = argv[i];
	    }
	    else {
		printf("-ipem option needs a value\n");
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
	else if (strcmp(argv[i], "-rsa") == 0) {
	    algPublic = TPM_ALG_RSA;
	}
	else if (strcmp(argv[i], "-ecc") == 0) {
	    algPublic = TPM_ALG_ECC;
	}
	else if (strcmp(argv[i],"-if") == 0) {
	    i++;
	    if (i < argc) {
		messageFilename = argv[i];
	    }
	    else {
		printf("-if option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ih") == 0) {
	    i++;
	    if (i < argc) {
		messageFilename = argv[i];
		doHash = FALSE;
	    }
	    else {
		printf("-ih option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-is") == 0) {
	    i++;
	    if (i < argc) {
		signatureFilename = argv[i];
	    }
	    else {
		printf("-is option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-raw") == 0) {
	    raw = TRUE;
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
	else if (strcmp(argv[i],"-se0") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &sessionHandle0);
	    }
	    else {
		printf("Missing parameter for -se0\n");
		printUsage();
	    }
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &sessionAttributes0);
		if (sessionAttributes0 > 0xff) {
		    printf("Out of range session attributes for -se0\n");
		    printUsage();
		}
	    }
	    else {
		printf("Missing parameter for -se0\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-se1") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &sessionHandle1);
	    }
	    else {
		printf("Missing parameter for -se1\n");
		printUsage();
	    }
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &sessionAttributes1);
		if (sessionAttributes1 > 0xff) {
		    printf("Out of range session attributes for -se1\n");
		    printUsage();
		}
	    }
	    else {
		printf("Missing parameter for -se1\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-se2") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &sessionHandle2);
	    }
	    else {
		printf("Missing parameter for -se2\n");
		printUsage();
	    }
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &sessionAttributes2);
		if (sessionAttributes2 > 0xff) {
		    printf("Out of range session attributes for -se2\n");
		    printUsage();
		}
	    }
	    else {
		printf("Missing parameter for -se2\n");
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
    if ((keyHandle == 0) && (pemFilename == NULL)) {
	printf("Missing handle parameter -hk or PEM file name -ipem\n");
	printUsage();
    }
    if (messageFilename == NULL) {
	printf("Missing message file name -if or hash file name -ih\n");
	printUsage();
    }
    if (signatureFilename == NULL) {
	printf("Missing signature parameter -is\n");
	printUsage();
    }
    if (rc == 0) {
       rc = TSS_File_ReadBinaryFile(&data,     /* freed @1 */
				    &dataLength,
				    messageFilename);
    }
    /* hash the file */
    if (rc == 0) {
	if (doHash) {
	    if (rc == 0) {
		if (verbose) printf("verifysignature: Hashing message file %s\n", messageFilename);
		digest.hashAlg = halg;
		sizeInBytes = TSS_GetDigestSize(digest.hashAlg);
		rc = TSS_Hash_Generate(&digest,
				       dataLength, data,
				       0, NULL);
	    }
	    if (rc == 0) {
		if (verbose) printf("verifysignature: Hashing message\n");
		/* digest to be verified */
		in.digest.t.size = sizeInBytes;
		memcpy(&in.digest.t.buffer, (uint8_t *)&digest.digest, sizeInBytes);
	    }
	}
	else {
	    if (verbose) printf("verifysignature: Using hash input file %s\n", messageFilename);
	    in.digest.t.size = dataLength;
	    memcpy(&in.digest.t.buffer, (uint8_t *)data, dataLength);
	}
	if (verbose) TSS_PrintAll("verifysignature: hash",
				  (uint8_t *)&in.digest.t.buffer, in.digest.t.size);
    }
    if (rc == 0) {
	rc = TSS_File_ReadBinaryFile(&buffer,     /* freed @2 */
				     &length,
				     signatureFilename);
    }
    if (rc == 0) {
	if (!raw) {
	    int32_t ilength = length;	/* values that can move during the unmarshal */
	    buffer1 = buffer;
	    /* input is TPMT_SIGNATURE */
	    rc = TPMT_SIGNATURE_Unmarshal(&in.signature, &buffer1, &ilength, NO);
	}
	else {
	    /* input is raw bytes */
	    rc = rawUnmarshal(&in.signature, algPublic, halg, buffer, length);
	}
    }
    if (keyHandle != 0) {
	if (rc == 0) {
	    /* Handle of key that will perform verifying */
	    in.keyHandle = keyHandle;
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
			     TPM_CC_VerifySignature,
			     sessionHandle0, NULL, sessionAttributes0,
			     sessionHandle1, NULL, sessionAttributes1,
			     sessionHandle2, NULL, sessionAttributes2,
			     TPM_RH_NULL, NULL, 0);
	}
	{
	    TPM_RC rc1 = TSS_Delete(tssContext);
	    if (rc == 0) {
		rc = rc1;
	    }
	}
	if ((rc == 0) && (ticketFilename != NULL)) {
	    rc = TSS_File_WriteStructure(&out.validation,
					 (MarshalFunction_t)TSS_TPMT_TK_VERIFIED_Marshal,
					 ticketFilename);
	}
    }
    if (pemFilename != NULL) {
	if (rc == 0) {
	    rc = verifySignatureFromPem((uint8_t *)&in.digest.t.buffer,
					in.digest.t.size,
					&in.signature,
					halg,
					pemFilename);
	}
    }
    if (rc == 0) {
	if (verbose) printf("verifysignature: success\n");
    }
    else {
	const char *msg;
	const char *submsg;
	const char *num;
	printf("verifysignature: failed, rc %08x\n", rc);
	TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
	printf("%s%s%s\n", msg, submsg, num);
	rc = EXIT_FAILURE;
    }
    free(data);		/* @1 */
    free(buffer);	/* @2 */
    return rc;
}

/* rawUnmarshal() unmarshals a raw openssl signature 'buffer' into the TPMT_SIGNATURE structure.

   It handles RSA and ECC P256.
*/

TPM_RC rawUnmarshal(TPMT_SIGNATURE *tSignature,
		    TPMI_ALG_PUBLIC algPublic,
		    TPMI_ALG_HASH halg,
		    uint8_t *signatureBin, size_t signatureBinLen)
{
    TPM_RC			rc = 0;
    if (algPublic == TPM_ALG_RSA) {
	rc = convertRsaBinToTSignature(tSignature,
				       halg,
				       signatureBin,
				       signatureBinLen);
    }
    /* TPM_ALG_ECC, the raw signature is DER encoded R and S elements */
    else {
	rc = convertEcBinToTSignature(tSignature,
				      halg,
				      signatureBin,
				      signatureBinLen);
    }
    return rc;
}

static void printUsage(void)
{
    printf("\n");
    printf("verifysignature\n");
    printf("\n");
    printf("Runs TPM2_VerifySignature and/or verifies using the PEM public key\n");
    printf("\n");
    printf("\t[-hk key handle]\n");
    printf("\t[-ipem public key PEM format file name to verify signature]\n");
    printf("\t\tOne of -hk, -ipem must be specified\n");
    printf("\t[-halg (sha1, sha256, sha384) (default sha256)]\n");
    printf("\t[asymmetric key algorithm]\n");
    printf("\t\t[-rsa (default)]\n");
    printf("\t\t[-ecc curve (P256)]\n");
    printf("\t-if input message file name\n");
    printf("\t-ih input hash file name\n");
    printf("\t-is signature file name\n");
    printf("\t[-raw (flag) signature specified by -is is in raw format]\n");
    printf("\t\t(default TPMT_SIGNATURE)\n");
    printf("\t[-tk ticket file name (requires -ha)]\n");
    printf("\n");
    printf("\t-se[0-2] session handle / attributes (default NULL)\n");
    printf("\t\t01 continue\n");
    printf("\t\t20 command decrypt\n");
    printf("\t\t80 audit\n");
    exit(1);	
}
