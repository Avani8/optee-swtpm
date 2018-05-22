/********************************************************************************/
/*										*/
/*			    PolicySigned	 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: policysigned.c 1069 2017-08-29 17:11:32Z kgoldman $		*/
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

#ifdef TPM_POSIX
#include <netinet/in.h>
#endif
#ifdef TPM_WINDOWS
#include <winsock2.h>
#endif

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <tss2/tss.h>
#include <tss2/tssutils.h>
#include <tss2/tsscryptoh.h>
#include <tss2/tsscrypto.h>
#include <tss2/tssresponsecode.h>
#include <tss2/tssmarshal.h>

static void printUsage(void);
static TPM_RC signAHash(TPM2B_PUBLIC_KEY_RSA *signature,
			TPMT_HA *aHash,
			const char *signingKeyFilename,
			const char *signingKeyPassword);

int verbose = FALSE;

int main(int argc, char *argv[])
{
    TPM_RC			rc = 0;
    int				i;    /* argc iterator */
    TSS_CONTEXT			*tssContext = NULL;
    PolicySigned_In 		in;
    PolicySigned_Out 		out;
    TPMI_DH_OBJECT		authObject = 0;
    TPMI_SH_POLICY		policySession = 0;
    const char 			*nonceTPMFilename = NULL;
    const char 			*cpHashAFilename = NULL;
    const char			*policyRefFilename = NULL;
    const char			*ticketFilename = NULL;
    const char			*timeoutFilename = NULL;
    INT32			expiration = 0;
    const char			*signingKeyFilename = NULL;
    const char			*signingKeyPassword = NULL;
    const char			*signatureFilename = NULL;
    uint8_t			*signature = NULL;
    size_t			signatureLength;
    TPMI_ALG_HASH		halg = TPM_ALG_SHA256;
    TPMT_HA 			aHash;
    
    setvbuf(stdout, 0, _IONBF, 0);      /* output may be going through pipe to log file */
    TSS_SetProperty(NULL, TPM_TRACE_LEVEL, "1");
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    /* command line argument defaults */

    in.nonceTPM.b.size = 0;	/* three of the components to aHash are optional */
    in.cpHashA.b.size = 0;
    in.policyRef.b.size = 0;

    for (i=1 ; (i<argc) && (rc == 0) ; i++) {
	if (strcmp(argv[i],"-hk") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &authObject);
	    }
	    else {
		printf("Missing parameter for -hk\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-ha") == 0) {
	    i++;
	    if (i < argc) {
		sscanf(argv[i],"%x", &policySession);
	    }
	    else {
		printf("Missing parameter for -ha\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-in") == 0) {
	    i++;
	    if (i < argc) {
		nonceTPMFilename = argv[i];
	    }
	    else {
		printf("-in option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-cp") == 0) {
	    i++;
	    if (i < argc) {
		cpHashAFilename = argv[i];
	    }
	    else {
		printf("-cp option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-pref") == 0) {
	    i++;
	    if (i < argc) {
		policyRefFilename = argv[i];
	    }
	    else {
		printf("-pref option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-exp") == 0) {
	    i++;
	    if (i < argc) {
		expiration = atoi(argv[i]);
	    }
	    else {
		printf("Missing parameter for -exp\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-sk") == 0) {
	    i++;
	    if (i < argc) {
		signingKeyFilename = argv[i];
	    }
	    else {
		printf("-sk option needs a value\n");
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
	else if (strcmp(argv[i],"-to") == 0) {
	    i++;
	    if (i < argc) {
		timeoutFilename = argv[i];
	    }
	    else {
		printf("-to option needs a value\n");
		printUsage();
	    }
	}
	else if (strcmp(argv[i],"-pwdk") == 0) {
	    i++;
	    if (i < argc) {
		signingKeyPassword = argv[i];
	    }
	    else {
		printf("-pwdk option needs a value\n");
		printUsage();
	    }
	}
 	else if (strcmp(argv[i],"-halg") == 0) {
	    i++;
	    if (i < argc) {
		if (strcmp(argv[i],"sha256") == 0) {
		    halg = TPM_ALG_SHA256;
		}
		else if (strcmp(argv[i],"sha1") == 0) {
		    halg = TPM_ALG_SHA1;
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
    if (authObject == 0) {
	printf("Missing handle parameter -hk\n");
	printUsage();
    }
    if (policySession == 0) {
	printf("Missing handle parameter -ha\n");
	printUsage();
    }
    if ((signingKeyFilename == NULL) && (signatureFilename == NULL)) {
	printf("Missing signing key -sk or signature -is\n");
	printUsage();
    }
    if ((signingKeyFilename != NULL) && (signatureFilename != NULL)) {
	printf("Cannot have both signing key -sk and signature -is\n");
	printUsage();
    }
    if (rc == 0) {
	in.authObject = authObject;
	in.policySession = policySession;
    }
    /* read the optional components - nonceTPM, cpHashA, policyRef */ 
    if ((rc == 0) && (nonceTPMFilename != NULL)) {
	rc = TSS_File_Read2B(&in.nonceTPM.b,
			     sizeof(TPMU_HA),
			     nonceTPMFilename);
    }
    if ((rc == 0) && (cpHashAFilename != NULL)) {
	rc = TSS_File_Read2B(&in.cpHashA.b,
			     sizeof(TPMU_HA),
			     cpHashAFilename);
    }
    if ((rc == 0) && (policyRefFilename != NULL)) {
	rc = TSS_File_Read2B(&in.policyRef.b,
			     sizeof(TPMU_HA),
			     policyRefFilename);
    }
    if (rc == 0) {
	in.expiration = expiration;
	in.auth.sigAlg = TPM_ALG_RSASSA;	/* sample uses RSASSA */
	in.auth.signature.rsassa.hash = halg;
    }
    /* sample code using a PEM key to sign */
    if (signingKeyFilename != NULL) {
	/* calculate the digest from the 4 components according to the TPM spec Part 3. */
	/* aHash = HauthAlg(nonceTPM || expiration || cpHashA || policyRef)	(13) */
	if (rc == 0) {
	    INT32 expirationNbo = htonl(in.expiration);
	    aHash.hashAlg = halg;
	    /* This varargs function takes length / array pairs.  It skips pairs with a length of
	       zero.  This handles the three optional components (default length zero) with no
	       special handling. */
	    rc = TSS_Hash_Generate(&aHash,		/* largest size of a digest */
				   in.nonceTPM.t.size, in.nonceTPM.t.buffer,
				   sizeof(INT32), &expirationNbo,
				   in.cpHashA.t.size, in.cpHashA.t.buffer,
				   in.policyRef.t.size, in.policyRef.t.buffer,
				   0, NULL);
	}
	/* sign aHash */
	if (rc == 0) {
	    rc = signAHash(&in.auth.signature.rsassa.sig,	/* sample uses RSASSA */
			   &aHash,
			   signingKeyFilename, signingKeyPassword);
	}
    }
    /* sample code where the signature has been generated externally */
    if (signatureFilename != NULL) {
	if (rc == 0) {
	    rc = TSS_File_ReadBinaryFile((unsigned char **)&signature,     /* freed @1 */
					 &signatureLength,
					 signatureFilename);
	}
	if (rc == 0) {
	    if (signatureLength > sizeof(in.auth.signature.rsassa.sig.t.buffer)) {
		printf("Signature length %lu is greater than buffer %lu\n",
		       (unsigned long)signatureLength,
		       (unsigned long)sizeof(in.auth.signature.rsassa.sig.t.buffer));
		rc = TSS_RC_RSA_SIGNATURE;
	    }
	}
	if (rc == 0) {
	    in.auth.signature.rsassa.sig.t.size = signatureLength;
	    memcpy(&in.auth.signature.rsassa.sig.t.buffer, signature, signatureLength); 
	}
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
			 TPM_CC_PolicySigned,
			 TPM_RH_NULL, NULL, 0);
    }
    {
	TPM_RC rc1 = TSS_Delete(tssContext);
	if (rc == 0) {
	    rc = rc1;
	}
    }
    if ((rc == 0) && (ticketFilename != NULL)) {
	rc = TSS_File_WriteStructure(&out.policyTicket,
				     (MarshalFunction_t)TSS_TPMT_TK_AUTH_Marshal,
				     ticketFilename);
    }
    if ((rc == 0) && (timeoutFilename != NULL)) {
	rc = TSS_File_WriteBinaryFile(out.timeout.b.buffer,
				      out.timeout.b.size,
				      timeoutFilename); 
    }
    if (rc == 0) {
	if (verbose) printf("policysigned: success\n");
    }
    else {
	const char *msg;
	const char *submsg;
	const char *num;
	printf("policysigned: failed, rc %08x\n", rc);
	TSS_ResponseCode_toString(&msg, &submsg, &num, rc);
	printf("%s%s%s\n", msg, submsg, num);
	rc = EXIT_FAILURE;
    }
    free(signature);	/* @1 */
    return rc;
}

/* signAHash() signs digest, returns signature.  The signature TPM2B_PUBLIC_KEY_RSA is a member of
   the TPMT_SIGNATURE command parameter.

   This sample signer uses a pem file signingKeyFilename with signingKeyPassword.

*/

TPM_RC signAHash(TPM2B_PUBLIC_KEY_RSA *signature,
		 TPMT_HA *aHash,
		 const char *signingKeyFilename,
		 const char *signingKeyPassword)
{
    TPM_RC		rc = 0;
    int			irc;
    RSA			*rsaKey = NULL;
    FILE		*keyFile = NULL;
    int			nid;			/* openssl hash algorithm */
    uint32_t  		sizeInBytes;		/* hash algorithm mapped to size */
    unsigned int 	length;			/* RSA_Sign() output */

    if (rc == 0) {
	sizeInBytes = TSS_GetDigestSize(aHash->hashAlg);
#if 0
	if (verbose) {
	    TSS_PrintAll("signAHash: aHash",
			 (uint8_t *)(&aHash->digest), sizeInBytes);
	}
#endif
    }
    /* map the hash algorithm to the openssl NID */
    if (rc == 0) {
	switch (aHash->hashAlg) {
	  case TPM_ALG_SHA256:
	    nid = NID_sha256;
	    break;
	  case TPM_ALG_SHA1:
	    nid = NID_sha1;
	    break;
	  default:
	    printf("signAHash: Error, hash algorithm %04hx unsupported\n", aHash->hashAlg);
	    rc = -1;
	}
    }
    /* read the PEM format private key into the OpenSSL structure */
    if (rc == 0) {
	keyFile = fopen(signingKeyFilename, "r");
	if (keyFile == NULL) {
	    printf("signAHash: Error opening %s\n", signingKeyFilename);
	    rc = -1;
	}
    }
    if (rc == 0) {
	rsaKey = PEM_read_RSAPrivateKey(keyFile, NULL, NULL, (void *)signingKeyPassword);
	if (rsaKey == NULL) {
	    printf("signAHash: Error in OpenSSL PEM_read_RSAPrivateKey()\n");
	    ERR_print_errors_fp(stdout);
	    rc = -1;
	}
    }
    /* validate that the length of the resulting signature will fit in the
       TPMT_SIGNATURE->TPMU_SIGNATURE->TPMS_SIGNATURE_RSASSA->
       TPMS_SIGNATURE_RSA->TPM2B_PUBLIC_KEY_RSA structure */
    if (rc == 0) {
	unsigned int keySize = RSA_size(rsaKey);
	if (keySize > sizeof(signature->t.buffer)) {
	    printf("signAHash: Error, private key length %u > signature buffer %u\n",
		   keySize, (unsigned int) sizeof(signature->t.buffer));
	    rc = -1;
	}
    }
    if (rc == 0) {
	irc = RSA_sign(nid,
		       (uint8_t *)(&aHash->digest), sizeInBytes,
		       signature->t.buffer, &length,
		       rsaKey);
	if (irc != 1) {
	    printf("signAHash: Error in OpenSSL RSA_sign()\n");
	    ERR_print_errors_fp(stdout);
	    rc = -1;
	}
    }
    if (rc == 0) {
	signature->t.size = length;	/* length of RSA key checked above */
#if 0
	if (verbose) TSS_PrintAll("signAHash: signature",
				  signature->t.buffer, signature->t.size);
#endif
    }
    if (keyFile != NULL) {
	fclose(keyFile);
    }
    if (rsaKey != NULL) {
	RSA_free(rsaKey);
    }
    return rc;
}


static void printUsage(void)
{
    printf("\n");
    printf("policysigned\n");
    printf("\n");
    printf("Runs TPM2_PolicySigned\n");
    printf("\n");
    printf("\t-hk signature verification key handle\n");
    printf("\t-ha policy session handle\n");
    printf("\t[-in nonceTPM file (default none)]\n");
    printf("\t[-cp cpHash file (default none)]\n");
    printf("\t[-pref policyRef file (default none)]\n");
    printf("\t[-exp expiration in decimal (default none)]\n");
    printf("\t[-halg (sha1, sha256) (default sha256)]\n");
    printf("\t-sk RSA signing key file name (PEM format)\n");
    printf("\t\tUse this signing key.\n");
    printf("\t-is signature file name\n");
    printf("\t\tUse this signature from e.g., a smart card or other HSM.\n");
    printf("\t[-pwdk signing key password (default null)]\n");
    printf("\t[-tk ticket file name]\n");
    printf("\t[-to timeout file name]\n");
    exit(1);	
}
