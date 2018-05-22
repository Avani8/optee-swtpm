/********************************************************************************/
/*										*/
/*			     	IMA Routines					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: imalib.h 1020 2017-06-09 18:46:45Z kgoldman $			*/
/*										*/
/* (c) Copyright IBM Corporation 2016.						*/
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

#ifndef IMA_H
#define IMA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <sys/param.h>

#include <tss2/TPM_Types.h>

#define IMA_PCR 		10

/* FIXME need better error codes */

#define ERR_STRUCTURE  		1 	/* this is not the stream for the structure to be parsed */
#define ERR_HASH_ALGORITHM	2 	/* unsupported hash algorithm */
#define TCG_EVENT_NAME_LEN_MAX	255	/* FIXME need verification */

#define TCG_TEMPLATE_DATA_LEN_MAX				\
sizeof(uint32_t) +		/* hash length */		\
65 +				/* hash algorithm as text */	\
32 +				/* file data hash */		\
sizeof(uint32_t) +	 	/* file name length */		\
MAXPATHLEN+1 +			/* file name */			\
sizeof(uint32_t) +		/* signature length */		\
32 + 256			/* signature */

/* from security/integrity/integrity.h: */

enum evm_ima_xattr_type {
    IMA_XATTR_DIGEST = 0x01,
    EVM_XATTR_HMAC,
    EVM_IMA_XATTR_DIGSIG,
    IMA_XATTR_DIGEST_NG,
    IMA_XATTR_LAST
};

/* from include/uapi/linux/hash_info.h: */

enum hash_algo {
    HASH_ALGO_MD4,
    HASH_ALGO_MD5,
    HASH_ALGO_SHA1,
    HASH_ALGO_RIPE_MD_160,
    HASH_ALGO_SHA256,
    HASH_ALGO_SHA384,
    HASH_ALGO_SHA512,
    HASH_ALGO_SHA224,
    HASH_ALGO_RIPE_MD_128,
    HASH_ALGO_RIPE_MD_256,
    HASH_ALGO_RIPE_MD_320,
    HASH_ALGO_WP_256,
    HASH_ALGO_WP_384,
    HASH_ALGO_WP_512,
    HASH_ALGO_TGR_128,
    HASH_ALGO_TGR_160,
    HASH_ALGO_TGR_192,
    HASH_ALGO__LAST
};

#define IMA_UNSUPPORTED	0
#define IMA_NG		1
#define IMA_SIG		2

//typedef TPM_DIGEST TPM_PCRVALUE;        	/* The value inside of the PCR */

typedef struct ImaEvent {
    uint32_t pcrIndex;
    uint8_t digest[SHA1_DIGEST_SIZE];		/* IMA hard coded to SHA-1 */
    uint32_t name_len;
    char name[TCG_EVENT_NAME_LEN_MAX + 1];
    unsigned int nameInt;			/* integer for template data handler */
    struct ima_template_desc *template_desc; 	/* template descriptor */
    uint32_t template_data_len;
    uint8_t *template_data;			/* template related data */
} ImaEvent;

typedef struct ImaTemplateData {
    uint32_t hashLength;
    char hashAlg[64+1];		/* FIXME need verification */
    int hashNid;
    uint32_t fileDataHashLength;
    uint8_t fileDataHash[SHA256_DIGEST_SIZE];
    uint32_t fileNameLength;
    uint8_t fileName[MAXPATHLEN+1];
    uint32_t sigLength;
    uint32_t sigHeaderLength;
    uint8_t sigHeader[9];	/* FIXME need verification, length and contents */
    uint16_t signatureSize;
    uint8_t signature[256];	/* FIXME need verification */
} ImaTemplateData;

#ifdef __cplusplus
extern "C" {
#endif

    void IMA_Event_Init(ImaEvent *imaEvent);
    void IMA_Event_Free(ImaEvent *imaEvent);
    void IMA_Event_Trace(ImaEvent *imaEvent, int traceTemplate);
    void IMA_TemplateData_Trace(ImaTemplateData *imaTemplateData,
				unsigned int nameInt);
    uint32_t IMA_Event_ReadFile(ImaEvent *imaEvent,
				int *endOfFile,
				FILE *infile,
				int littleEndian);
    uint32_t IMA_Event_ReadBuffer(ImaEvent *imaEvent,
				  size_t *length,
				  uint8_t **buffer,
				  int *endOfBuffer,
				  int littleEndian,
				  int getTemplate);
    uint32_t IMA_TemplateData_ReadBuffer(ImaTemplateData *imaTemplateData,
					 ImaEvent *imaEvent,
					 int littleEndian);
    uint32_t IMA_Event_Write(ImaEvent *imaEvent,
			     FILE *outFile);
    uint32_t IMA_Extend(TPMT_HA *imapcr,
			ImaEvent *imaEvent,
			TPMI_ALG_HASH hashAlg);
    uint32_t IMA_VerifyImaDigest(uint32_t *badEvent,
				 ImaEvent *imaEvent,
				 int eventNum);
    TPM_RC ImaEvent_Marshal(ImaEvent *source,
			    uint16_t *written, uint8_t **buffer, int32_t *size);

#if 0
    uint32_t IMA_Event_ToString(char **eventString,
				ImaEvent *imaEvent);
#endif

#ifdef __cplusplus
}
#endif

#endif
