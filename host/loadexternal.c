/********************************************************************************/
/*										*/
/*			   Load External					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: loadexternal.c 1098 2017-11-27 23:07:26Z kgoldman $		*/
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
  DER example:

  Create a key pair in PEM format
  
  > openssl genrsa -out keypair.pem -aes256 -passout pass:rrrr 2048
  > openssl ecparam -name prime256v1 -genkey -noout -out tmpkeypairecc.pem

  Convert to plaintext DER format

  > openssl rsa -inform pem -outform der -in keypair.pem -out keypair.der -passin pass:rrrr
  > openssl ec -inform pem -outform der -in tmpkeypairecc.pem -out tmpkeypairecc.der -passin pass:rrrr > run.out
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/tssresponsecode.h>
#include <tss2/Unmarshal_fp.h>
#include "objecttemplates.h"
#include "cryptoutils.h"
#include "ekutils.h"

static void printUsage(void);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC			rc = 0;
    int				i;    /* argc iterator */
    TSS_CONTEXT			*tssContext = NULL;
    LoadExternal_In 		in;
    LoadExternal_Out 		out;
    char 			hierarchyChar = 0;
    TPMI_RH_HIERARCHY		hierarchy = TPM_RH_NULL;
    int				keyType = TYPE_SI;
    TPMI_ALG_SIG_SCHEME 	scheme = TPM_ALG_RSASSA;
    uint32_t 			keyTypeSpecified = 0;
    TPMI_ALG_PUBLIC 		algPublic = TPM_ALG_RSA;
    TPMI_ALG_HASH		halg = TPM_ALG_SHA256;
    TPMI_ALG_HASH		nalg = TPM_ALG_SHA256;
    const char			*publicKeyFilename = NULL;
    const char			*derKeyFilename = NULL;
    const char			*pemKeyFilename = NULL;
    const char			*keyPassword = NULL;
    int				userWithAuth = TRUE;
    unsigned int		inputCount = 0;
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
	if (strcmp(argv[i],"-hi") == 0) {
	    i++;
	    if (i < argc) {
		if (argv[i][0] != 'e' && argv[i][0] != 'o' &&
		    argv[i][0] != 'p' && argv[i][0] != 'h') {
		    printUsage();
		}
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
	else if (strcmp(argv[i],"-nalg") == 0) {
	    i++;
	    if (i < argc) {
		if (strcmp(argv[i],"sha1") == 0) {
		    nalg = TPM_ALG_SHA1;
		}
		else if (strcmp(argv[i],"sha256") == 0) {
		    nalg = TPM_ALG_SHA256;
		}
		else if (strcmp(argv[i],"sha384") == 0) {
		    nalg = TPM_ALG_SHA384;
		}
		else {
		    printf("Bad parameter for -nalg\n");
		    printUsage();
		}
	    }
	    else {
		printf("-nalg option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i], "-rsa") == 0) {
	    algPublic = TPM_ALG_RSA;
	}
	else if (strcmp(argv[i], "-ecc") == 0) {
	    algPublic = TPM_ALG_ECC;
	}
	else if (strcmp(argv[i],"-scheme") == 0) {
            i++;
	    if (i < argc) {
		if (strcmp(argv[i],"rsassa") == 0) {
		    scheme = TPM_ALG_RSASSA;
		}
		else if (strcmp(argv[i],"rsapss") == 0) {
		    scheme = TPM_ALG_RSAPSS;
		}
		else {
		    printf("Bad parameter %s for -scheme\n", argv[i]);
		    printUsage();
		}
	    }
        }
	else if (strcmp(argv[i], "-st") == 0) {
	    keyType = TYPE_ST;
	    scheme = TPM_ALG_NULL;
	    keyTypeSpecified++;
	}
	else if (strcmp(argv[i], "-si") == 0) {
	    keyType = TYPE_SI;
	    keyTypeSpecified++;
	}
	else if (strcmp(argv[i],"-ipu") == 0) {
	    i++;
	    if (i < argc) {
		publicKeyFilename = argv[i];
		inputCount++;
	    }
	    else {
		printf("-ipu option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ipem") == 0) {
	    i++;
	    if (i < argc) {
		pemKeyFilename = argv[i];
		inputCount++;
	    }
	    else {
		printf("-ipem option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ider") == 0) {
	    i++;
	    if (i < argc) {
		derKeyFilename = argv[i];
		inputCount++;
	    }
	    else {
		printf("-ider option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-pwdk") == 0) {
	    i++;
	    if (i < argc) {
		keyPassword = argv[i];
	    }
	    else {
		printf("-pwdk option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i], "-uwa") == 0) {
	    userWithAuth = FALSE;
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
    if (inputCount != 1) {
	printf("Missing or too many parameters -ipu, -ipem, -ider\n");
	printUsage();
    }
    if (keyTypeSpecified > 1) {
	printf("Too many key attributes\n");
	printUsage();
    }
    if (derKeyFilename == NULL) {
	if (keyPassword != NULL) {
	    printf("Password only valid for -ider keypair\n");
	    printUsage();
	}
    }
    /* loadexternal key pair cannot be restricted (storage key) and must have NULL symmetric
       scheme*/
    if (derKeyFilename != NULL) {
	if (keyType == TYPE_ST) {
	    keyType = TYPE_DEN;
	}
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
    }
    if (rc == 0) {
	in.inPrivate.t.size = 0;	/* default - mark optional inPrivate not used */
	/* TPM format key, output from create */
	if (publicKeyFilename != NULL) {
	    rc = TSS_File_ReadStructure(&in.inPublic,
					(UnmarshalFunction_t)TPM2B_PUBLIC_Unmarshal,
					publicKeyFilename);
	}
	/* PEM format, output from e.g. openssl, readpublic, createprimary, create */
	else if (pemKeyFilename != NULL) {
	    if (algPublic == TPM_ALG_RSA) {
		rc = convertRsaPemToPublic(&in.inPublic,
					   keyType,
					   scheme,
					   nalg,
					   halg,
					   pemKeyFilename);
	    }
	    /* TPM_ALG_ECC */
	    else {
		rc = convertEcPemToPublic(&in.inPublic,
					  keyType,
					  scheme,
					  nalg,
					  halg,
					  pemKeyFilename);
	    }
	}
	/* DER format key pair */
	else if (derKeyFilename != NULL) {
	    if (algPublic == TPM_ALG_RSA) {
		rc = convertRsaDerToKeyPair(&in.inPublic,
					    &in.inPrivate,
					    keyType,
					    scheme,
					    nalg,
					    halg,
					    derKeyFilename,
					    keyPassword);
	    }
	    /* TPM_ALG_ECC */
	    else {
		rc = convertEcDerToKeyPair(&in.inPublic,
					   &in.inPrivate,
					   keyType,
					   scheme,
					   nalg,
					   halg,
					   derKeyFilename,
					   keyPassword);
	    }
	    in.inPrivate.t.size = 1;		/* mark that private area should be loaded */
	}
	else {
	    printf("Failure parsing -ipu, -ipem, -ider\n");
	    printUsage();
	}
    }
    if (rc == 0) {
	if (!userWithAuth) {
	    in.inPublic.publicArea.objectAttributes.val &= ~TPMA_OBJECT_USERWITHAUTH;
	}
	in.hierarchy = hierarchy;
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
			 TPM_CC_LoadExternal,
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
    if (rc == 0) {
	printf("Handle %08x\n", out.objectHandle);
	if (verbose) printf("loadexternal: success\n");
    }
    else {
	const char *msg;
	const char *submsg;
	const char *num;
	printf("loadexternal: failed, rc %08x\n", rc);
	TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
	printf("%s%s%s\n", msg, submsg, num);
	rc = EXIT_FAILURE;
    }
    return rc;
}

static void printUsage(void)
{
    printf("\n");
    printf("loadexternal\n");
    printf("\n");
    printf("Runs TPM2_LoadExternal\n");
    printf("\n");
    printf("\t[-hi hierarchy (e, o, p, n) (default NULL)]\n");
    printf("\t[-nalg name hash algorithm (sha1, sha256, sha384) (default sha256)]\n");
    printf("\t[-halg (sha1, sha256, sha384) (default sha256)]\n");
    printf("\t[Asymmetric Key Algorithm]\n");
    printf("\t\t-rsa (default)\n");
    printf("\t\t-ecc\n");
    printf("\t-ipu TPM2B_PUBLIC public key file name\n");
    printf("\t-ipem PEM format public key file name\n");
    printf("\t-ider DER format plaintext key pair file name\n");
    printf("\t\t[-pwdk password for key (default empty)]\n");
    printf("\t\t[-uwa userWithAuth attribute clear (default set)]\n");
    printf("\t[-si signing (default) RSA default RSASSA scheme]\n");
    printf("\t\t[-scheme]\n");
    printf("\t\t\trsassa\n");
    printf("\t\t\trsapss\n");
    printf("\t[-st storage (default NULL scheme)]\n");
    printf("\n");
    printf("\t-se[0-2] session handle / attributes (default NULL)\n");
    printf("\t\t01 continue\n");
    printf("\t\t20 command decrypt\n");
    printf("\t\t40 response encrypt\n");
    printf("\t\t80 audit\n");
    exit(1);	
}
