/********************************************************************************/
/*										*/
/*			   Import		 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: import.c 1092 2017-11-03 14:10:10Z kgoldman $		*/
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

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/tssresponsecode.h>
#include <tss2/tssmarshal.h>
#include <tss2/Unmarshal_fp.h>

static void printUsage(void);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC			rc = 0;
    int				i;    /* argc iterator */
    TSS_CONTEXT			*tssContext = NULL;
    Import_In 			in;
    Import_Out 			out;
    TPMI_DH_OBJECT		parentHandle = 0;
    const char			*parentPassword = NULL;
    const char 			*encryptionKeyFilename = NULL;
    const char			*objectPublicFilename = NULL;
    const char			*duplicateFilename = NULL;
    const char			*inSymSeedFilename = NULL;
    const char			*outPrivateFilename = NULL;
    TPMI_SH_AUTH_SESSION    	sessionHandle0 = TPM_RS_PW;
    unsigned int		sessionAttributes0 = 0;
    TPMI_SH_AUTH_SESSION    	sessionHandle1 = TPM_RH_NULL;
    unsigned int		sessionAttributes1 = 0;
    TPMI_SH_AUTH_SESSION    	sessionHandle2 = TPM_RH_NULL;
    unsigned int		sessionAttributes2 = 0;
    
    setvbuf(stdout, 0, _IONBF, 0);      /* output may be going through pipe to log file */
    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "1");

    /* command line argument defaults */
    /* Table 129 - Definition of TPMT_SYM_DEF_OBJECT Structure */
    in.symmetricAlg.algorithm = TPM_ALG_NULL;

    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-hp") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &parentHandle);
	    }
	    else {
		printf("Missing parameter for -hp\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-pwdp") == 0) {
	    i++;
	    if (i < argc) {
		parentPassword = argv[i];
	    }
	    else {
		printf("-pwdp option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ik") == 0) {
	    i++;
	    if (i < argc) {
		encryptionKeyFilename = argv[i];
	    }
	    else {
		printf("-ik option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ipu") == 0) {
	    i++;
	    if (i < argc) {
		objectPublicFilename = argv[i];
	    }
	    else {
		printf("-ipu option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-id") == 0) {
	    i++;
	    if (i < argc) {
		duplicateFilename = argv[i];
	    }
	    else {
		printf("-id option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-iss") == 0) {
	    i++;
	    if (i < argc) {
		inSymSeedFilename = argv[i];
	    }
	    else {
		printf("-iss option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-salg") == 0) {
	    i++;
	    if (i < argc) {
		if (strcmp(argv[i],"aes") == 0) {
		    in.symmetricAlg.algorithm = TPM_ALG_AES;
		    in.symmetricAlg.keyBits.aes = 128;
		    in.symmetricAlg.mode.aes = TPM_ALG_CFB;
		}
		else {
		    printf("Bad parameter for -salg\n");
		    printUsage();
		}
	    }
	    else {
		printf("-salg option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-opr") == 0) {
	    i++;
	    if (i < argc) {
		outPrivateFilename = argv[i];
	    }
	    else {
		printf("-opr option needs a value\n");
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
    if ((in.symmetricAlg.algorithm == TPM_ALG_NULL) &&
	(encryptionKeyFilename != NULL)) {
	printf("-ik needs -salg\n");
	printUsage();
    }
    if ((in.symmetricAlg.algorithm != TPM_ALG_NULL) &&
	(encryptionKeyFilename == NULL)) {
	printf("-salg needs -ik\n");
	printUsage();
    }
    if (parentHandle == 0) {
	printf("Missing or bad object handle parameter -hp\n");
	printUsage();
    }
    if (objectPublicFilename == NULL) {
	printf("Missing parameter -ipu\n");
	printUsage();
    }
    if (duplicateFilename == NULL) {
	printf("Missing parameter -id\n");
	printUsage();
    }
    if (inSymSeedFilename == NULL) {
	printf("Missing parameter -iss\n");
	printUsage();
    }
    if (outPrivateFilename  == NULL) {
	printf("Missing parameter -opr\n");
	printUsage();
    }
    if (rc == 0) {
	in.parentHandle = parentHandle;
    }
    /* optional symmetric encryption key */
    if (rc == 0) {
	if (encryptionKeyFilename != NULL) {
	    rc = TSS_File_Read2B(&in.encryptionKey.b,
				 sizeof(TPMT_HA),
				 encryptionKeyFilename);
	}
	else {
	    in.encryptionKey.t.size = 0;
	}
    }
    if (rc == 0) {
	rc = TSS_File_ReadStructure(&in.objectPublic,
				    (UnmarshalFunction_t)TPM2B_PUBLIC_Unmarshal,
				    objectPublicFilename);
    }
    if (rc == 0) {
	rc = TSS_File_Read2B(&in.duplicate.b,
			     sizeof(_PRIVATE),
			     duplicateFilename);
    }
    if (rc == 0) {
	rc = TSS_File_Read2B(&in.inSymSeed.b,
			     sizeof(TPMU_ENCRYPTED_SECRET),
			     inSymSeedFilename);
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
			 TPM_CC_Import,
			 sessionHandle0, parentPassword, sessionAttributes0,
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
	rc = TSS_File_WriteStructure(&out.outPrivate,
				     (MarshalFunction_t)TSS_TPM2B_PRIVATE_Marshal,
				     outPrivateFilename);
    }
    if (rc == 0) {
	if (verbose) printf("import: success\n");
    }
    else {
	const char *msg;
	const char *submsg;
	const char *num;
	printf("import: failed, rc %08x\n", rc);
	TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
	printf("%s%s%s\n", msg, submsg, num);
	rc = EXIT_FAILURE;
    }
    return rc;
}

static void printUsage(void)
{
    printf("\n");
    printf("import\n");
    printf("\n");
    printf("Runs TPM2_Import\n");
    printf("\n");
    printf("\t-hp parent handle\n");
    printf("\t[-pwdp password for parent (default empty)]\n");
    printf("\t[-ik encryption key in file name]\n");
    printf("\t-ipu object public area file name\n");
    printf("\t-id duplicate file name\n");
    printf("\t-iss symmetric seed file name\n");
    printf("\t[-salg symmetric algorithm (default none)]\n");
    printf("\n");
    printf("\t-opr private area file name\n");
    printf("\t-se[0-2] session handle / attributes (default PWAP)\n");
    printf("\t\t01 continue\n");
    printf("\t\t20 command decrypt\n");
    printf("\t\t40 response encrypt\n");
    exit(1);	
}
