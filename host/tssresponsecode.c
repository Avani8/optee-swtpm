/********************************************************************************/
/*										*/
/*			     TPM2 Response Code Printer				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tssresponsecode.c 1098 2017-11-27 23:07:26Z kgoldman $	*/
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

#include <stdint.h>
#include <stdlib.h>

#include <tss2/tssresponsecode.h>
#include <tss2/tsserror.h>
#include <tss2/tssprint.h>

/* The intended usage is:

   const char *msg;
   const char *submsg;
   const char *num;

   TSS_ResponseCode_toString(&msg, &submsg, &num, rc);

   printf("%s%s%s\n", msg, submsg, num);
*/

/* 39.4	Response Code Details */

/* tables to map response code to text */

typedef struct {
    TPM_RC rc;
    const char *text;
} RC_TABLE;

static const char *TSS_ResponseCode_RcToText(const RC_TABLE *table, size_t tableSize, TPM_RC rc);
static const char *TSS_ResponseCode_NumberToText(unsigned int num);

const RC_TABLE ver1Table [] = {
    {TPM_RC_INITIALIZE, "TPM_RC_INITIALIZE - TPM not initialized by TPM2_Startup or already initialized"},
    {TPM_RC_FAILURE, "TPM_RC_FAILURE - commands not being accepted because of a TPM failure"},
    {TPM_RC_SEQUENCE, "TPM_RC_SEQUENCE - improper use of a sequence handle"},
    {TPM_RC_PRIVATE, "TPM_RC_PRIVATE - not currently used"},
    {TPM_RC_HMAC, "TPM_RC_HMAC - HMAC failure"},
    {TPM_RC_DISABLED, "TPM_RC_DISABLED - the command is disabled"},
    {TPM_RC_EXCLUSIVE, "TPM_RC_EXCLUSIVE - command failed because audit sequence required exclusivity"},
    {TPM_RC_AUTH_TYPE, "TPM_RC_AUTH_TYPE - authorization handle is not correct for command"},
    {TPM_RC_AUTH_MISSING, "TPM_RC_AUTH_MISSING - command requires an authorization session"},
    {TPM_RC_POLICY, "TPM_RC_POLICY - policy failure in math operation or an invalid authPolicy value"},
    {TPM_RC_PCR, "TPM_RC_PCR - PCR check fail"},
    {TPM_RC_PCR_CHANGED, "TPM_RC_PCR_CHANGED - PCR have changed since checked."},
    {TPM_RC_UPGRADE, "TPM_RC_UPGRADE - TPM is in field upgrade mode"},
    {TPM_RC_TOO_MANY_CONTEXTS, "TPM_RC_TOO_MANY_CONTEXTS - context ID counter is at maximum."},
    {TPM_RC_AUTH_UNAVAILABLE, "TPM_RC_AUTH_UNAVAILABLE - authValue or authPolicy is not available for selected entity."},
    {TPM_RC_REBOOT, "TPM_RC_REBOOT - a _TPM_Init and Startup(CLEAR) is required"},
    {TPM_RC_UNBALANCED, "TPM_RC_UNBALANCED - the protection algorithms (hash and symmetric) are not reasonably balanced"},
    {TPM_RC_COMMAND_SIZE, "TPM_RC_COMMAND_SIZE - command commandSize value is inconsistent with contents of the command buffer"},
    {TPM_RC_COMMAND_CODE, "TPM_RC_COMMAND_CODE - command code not supported"},
    {TPM_RC_AUTHSIZE, "TPM_RC_AUTHSIZE - the value of authorizationSize is out of range"},
    {TPM_RC_AUTH_CONTEXT, "TPM_RC_AUTH_CONTEXT - use of an authorization session with a command that cannot have an authorization session"},
    {TPM_RC_NV_RANGE, "TPM_RC_NV_RANGE - NV offset+size is out of range."},
    {TPM_RC_NV_SIZE, "TPM_RC_NV_SIZE - Requested allocation size is larger than allowed."},
    {TPM_RC_NV_LOCKED, "TPM_RC_NV_LOCKED - NV access locked."},
    {TPM_RC_NV_AUTHORIZATION, "TPM_RC_NV_AUTHORIZATION - NV access authorization fails"},
    {TPM_RC_NV_UNINITIALIZED, "TPM_RC_NV_UNINITIALIZED - an NV Index is used before being initialized"},
    {TPM_RC_NV_SPACE, "TPM_RC_NV_SPACE - insufficient space for NV allocation"},
    {TPM_RC_NV_DEFINED, "TPM_RC_NV_DEFINED - NV Index or persistent object already defined"},
    {TPM_RC_BAD_CONTEXT, "TPM_RC_BAD_CONTEXT - context in TPM2_ContextLoad() is not valid"},
    {TPM_RC_CPHASH, "TPM_RC_CPHASH - cpHash value already set or not correct for use"},
    {TPM_RC_PARENT, "TPM_RC_PARENT - handle for parent is not a valid parent"},
    {TPM_RC_NEEDS_TEST, "TPM_RC_NEEDS_TEST - some function needs testing."},
    {TPM_RC_NO_RESULT, "TPM_RC_NO_RESULT - internal function cannot process a request due to an unspecified problem."},
    {TPM_RC_SENSITIVE, "TPM_RC_SENSITIVE - the sensitive area did not unmarshal correctly after decryption"},
};

/* RC_FMT1 response code to text */

const RC_TABLE fmt1Table [] = {
    {TPM_RC_ASYMMETRIC, "TPM_RC_ASYMMETRIC - asymmetric algorithm not supported or not correct"},
    {TPM_RC_ATTRIBUTES, "TPM_RC_ATTRIBUTES - inconsistent attributes"},
    {TPM_RC_HASH, "TPM_RC_HASH - hash algorithm not supported or not appropriate"},
    {TPM_RC_VALUE, "TPM_RC_VALUE - value is out of range or is not correct for the context"},
    {TPM_RC_HIERARCHY, "TPM_RC_HIERARCHY - hierarchy is not enabled or is not correct for the use"},
    {TPM_RC_KEY_SIZE, "TPM_RC_KEY_SIZE - key size is not supported"},
    {TPM_RC_MGF, "TPM_RC_MGF - mask generation function not supported"},
    {TPM_RC_MODE, "TPM_RC_MODE - mode of operation not supported"},
    {TPM_RC_TYPE, "TPM_RC_TYPE - the type of the value is not appropriate for the use"},
    {TPM_RC_HANDLE, "TPM_RC_HANDLE - the handle is not correct for the use"},
    {TPM_RC_KDF, "TPM_RC_KDF - unsupported key derivation function or function not appropriate for use"},
    {TPM_RC_RANGE, "TPM_RC_RANGE - value was out of allowed range."},
    {TPM_RC_AUTH_FAIL, "TPM_RC_AUTH_FAIL - the authorization HMAC check failed and DA counter incremented"},
    {TPM_RC_NONCE, "TPM_RC_NONCE - invalid nonce size or nonce value mismatch"},
    {TPM_RC_PP, "TPM_RC_PP - authorization requires assertion of PP"},
    {TPM_RC_SCHEME, "TPM_RC_SCHEME - unsupported or incompatible scheme"},
    {TPM_RC_SIZE, "TPM_RC_SIZE - structure is the wrong size"},
    {TPM_RC_SYMMETRIC, "TPM_RC_SYMMETRIC - unsupported symmetric algorithm or key size, or not appropriate for instance"},
    {TPM_RC_TAG, "TPM_RC_TAG - incorrect structure tag"},
    {TPM_RC_SELECTOR, "TPM_RC_SELECTOR - union selector is incorrect"},
    {TPM_RC_INSUFFICIENT, "TPM_RC_INSUFFICIENT - the TPM was unable to unmarshal a value because there were not enough octets in the input buffer"},
    {TPM_RC_SIGNATURE, "TPM_RC_SIGNATURE - the signature is not valid"},
    {TPM_RC_KEY, "TPM_RC_KEY - key fields are not compatible with the selected use"},
    {TPM_RC_POLICY_FAIL, "TPM_RC_POLICY_FAIL - a policy check failed"},
    {TPM_RC_INTEGRITY, "TPM_RC_INTEGRITY - integrity check failed"},
    {TPM_RC_TICKET, "TPM_RC_TICKET - invalid ticket"},
    {TPM_RC_RESERVED_BITS, "TPM_RC_RESERVED_BITS - reserved bits not set to zero as required"},
    {TPM_RC_BAD_AUTH, "TPM_RC_BAD_AUTH - authorization failure without DA implications"},
    {TPM_RC_EXPIRED, "TPM_RC_EXPIRED - the policy has expired"},
    {TPM_RC_POLICY_CC, "TPM_RC_POLICY_CC - the commandCode in the policy is not the commandCode of the command"},
    {TPM_RC_BINDING, "TPM_RC_BINDING - public and sensitive portions of an object are not cryptographically bound"},
    {TPM_RC_CURVE, "TPM_RC_CURVE - curve not supported	"},
    {TPM_RC_ECC_POINT, "TPM_RC_ECC_POINT - point is not on the required curve."},
};

/* RC_WARN response code to text */

const RC_TABLE warnTable [] = {
    {TPM_RC_CONTEXT_GAP, "TPM_RC_CONTEXT_GAP - gap for context ID is too large"},
    {TPM_RC_OBJECT_MEMORY, "TPM_RC_OBJECT_MEMORY - out of memory for object contexts"},
    {TPM_RC_SESSION_MEMORY, "TPM_RC_SESSION_MEMORY - out of memory for session contexts"},
    {TPM_RC_MEMORY, "TPM_RC_MEMORY - out of shared object/session memory or need space for internal operations"},
    {TPM_RC_SESSION_HANDLES, "TPM_RC_SESSION_HANDLES - out of session handles - a session must be flushed before a new session may be created"},
    {TPM_RC_OBJECT_HANDLES, "TPM_RC_OBJECT_HANDLES - out of object handles - the handle space for objects is depleted and a reboot is required"},
    {TPM_RC_LOCALITY, "TPM_RC_LOCALITY - bad locality"},
    {TPM_RC_YIELDED, "TPM_RC_YIELDED - the TPM has suspended operation on the command; forward progress was made and the command may be retried."},
    {TPM_RC_CANCELED, "TPM_RC_CANCELED - the command was canceled"},
    {TPM_RC_TESTING, "TPM_RC_TESTING - TPM is performing self-tests"},
    {TPM_RC_REFERENCE_H0, "TPM_RC_REFERENCE_H0 - the 1st handle in the handle area references a transient object or session that is not loaded"},
    {TPM_RC_REFERENCE_H1, "TPM_RC_REFERENCE_H1 - the 2nd handle in the handle area references a transient object or session that is not loaded"},
    {TPM_RC_REFERENCE_H2, "TPM_RC_REFERENCE_H2 - the 3rd handle in the handle area references a transient object or session that is not loaded"},
    {TPM_RC_REFERENCE_H3, "TPM_RC_REFERENCE_H3 - the 4th handle in the handle area references a transient object or session that is not loaded"},
    {TPM_RC_REFERENCE_H4, "TPM_RC_REFERENCE_H4 - the 5th handle in the handle area references a transient object or session that is not loaded"},
    {TPM_RC_REFERENCE_H5, "TPM_RC_REFERENCE_H5 - the 6th handle in the handle area references a transient object or session that is not loaded"},
    {TPM_RC_REFERENCE_H6, "TPM_RC_REFERENCE_H6 - the 7th handle in the handle area references a transient object or session that is not loaded"},
    {TPM_RC_REFERENCE_S0, "TPM_RC_REFERENCE_S0 - the 1st authorization session handle references a session that is not loaded"},
    {TPM_RC_REFERENCE_S1, "TPM_RC_REFERENCE_S1 - the 2nd authorization session handle references a session that is not loaded"},
    {TPM_RC_REFERENCE_S2, "TPM_RC_REFERENCE_S2 - the 3rd authorization session handle references a session that is not loaded"},
    {TPM_RC_REFERENCE_S3, "TPM_RC_REFERENCE_S3 - the 4th authorization session handle references a session that is not loaded"},
    {TPM_RC_REFERENCE_S4, "TPM_RC_REFERENCE_S4 - the 5th session handle references a session that is not loaded"},
    {TPM_RC_REFERENCE_S5, "TPM_RC_REFERENCE_S5 - the 6th session handle references a session that is not loaded"},
    {TPM_RC_REFERENCE_S6, "TPM_RC_REFERENCE_S6 - the 7th authorization session handle references a session that is not loaded"},
    {TPM_RC_NV_RATE, "TPM_RC_NV_RATE - the TPM is rate-limiting accesses to prevent wearout of NV"},
    {TPM_RC_LOCKOUT, "TPM_RC_LOCKOUT - authorizations for objects subject to DA protection are not allowed at this time because the TPM is in DA lockout mode"},
    {TPM_RC_RETRY, "TPM_RC_RETRY - the TPM was not able to start the command"},
    {TPM_RC_NV_UNAVAILABLE, "the command may require writing of NV and NV is not current accessible"}, 
    {TPM_RC_NOT_USED, "TPM_RC_NOT_USED - this value is reserved and shall not be returned by the TPM"},
};
    
/* parameter and handle number to text */

const char *num_table [] = {
    "unspecified",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15"
};

/* from tsserror.h */

const RC_TABLE tssTable [] = {
    {TSS_RC_OUT_OF_MEMORY, "TSS_RC_OUT_OF_MEMORY - Out of memory (malloc failed)"},
    {TSS_RC_ALLOC_INPUT, "TSS_RC_ALLOC_INPUT - The input to an allocation is not NULL"},
    {TSS_RC_MALLOC_SIZE, "TSS_RC_MALLOC_SIZE - The malloc size is too large or zero"},
    {TSS_RC_INSUFFICIENT_BUFFER, "TSS_RC_INSUFFICIENT_BUFFER - A buffer was insufficient for a copy"},
    {TSS_RC_BAD_PROPERTY, "TSS_RC_BAD_PROPERTY - The property parameter is out of range"},
    {TSS_RC_BAD_PROPERTY_VALUE, "TSS_RC_BAD_PROPERTY_VALUE - The property value is invalid"},
    {TSS_RC_INSUPPORTED_INTERFACE, "TSS_RC_INSUPPORTED_INTERFACE - The TPM interface type is not supported"},
    {TSS_RC_NO_CONNECTION, "TSS_RC_NO_CONNECTION - Failure connecting to lower layer"},
    {TSS_RC_BAD_CONNECTION, "TSS_RC_BAD_CONNECTION - Failure communicating with lower layer"},
    {TSS_RC_MALFORMED_RESPONSE, "TSS_RC_MALFORMED_RESPONSE - A response packet was fundamentally malformed"},
    {TSS_RC_NULL_PARAMETER, "TSS_RC_NULL_PARAMETER - A required parameter was NULL"},
    {TSS_RC_NOT_IMPLEMENTED, "TSS_RC_NOT_IMPLEMENTED - TSS function is not implemented"},
    {TSS_RC_FILE_OPEN, "TSS_RC_FILE_OPEN - The file could not be opened"},
    {TSS_RC_FILE_SEEK, "TSS_RC_FILE_SEEK - A file seek failed"},
    {TSS_RC_FILE_FTELL, "TSS_RC_FILE_FTELL - A file ftell failed"},
    {TSS_RC_FILE_READ, "TSS_RC_FILE_READ - A file read failed"},
    {TSS_RC_FILE_CLOSE, "TSS_RC_FILE_CLOSE - A file close failed"},
    {TSS_RC_FILE_WRITE, "TSS_RC_FILE_WRITE - A file write failed"},
    {TSS_RC_FILE_REMOVE, "TSS_RC_FILE_REMOVE - A file remove failed"},
    {TSS_RC_RNG_FAILURE, "TSS_RC_RNG_FAILURE - The random number generator failed"},
    {TSS_RC_BAD_PWAP_NONCE, "TSS_RC_BAD_PWAP_NONCE - Bad PWAP response nonce"},
    {TSS_RC_BAD_PWAP_ATTRIBUTES, "TSS_RC_BAD_PWAP_ATTRIBUTES - Bad PWAP response attributes"},
    {TSS_RC_BAD_PWAP_HMAC, "TSS_RC_BAD_PWAP_HMAC - Bad PWAP response HMAC"},
    {TSS_RC_NAME_NOT_IMPLEMENTED, "TSS_RC_NAME_NOT_IMPLEMENTED - name calculation not implemented for handle type"},
    {TSS_RC_MALFORMED_NV_PUBLIC, "TSS_RC_MALFORMED_NV_PUBLIC - The NV public structure does not match the name"},
    {TSS_RC_NAME_FILENAME, "TSS_RC_NAME_FILENAME - The name filename function has inconsistent arguments"},
    {TSS_RC_DECRYPT_SESSIONS, "TSS_RC_DECRYPT_SESSIONS - More than one command decrypt session"},
    {TSS_RC_ENCRYPT_SESSIONS, "TSS_RC_ENCRYPT_SESSIONS - More than one response encrypt session"},
    {TSS_RC_NO_DECRYPT_PARAMETER, "TSS_RC_NO_DECRYPT_PARAMETER - and has no decrypt parameter"},
    {TSS_RC_NO_ENCRYPT_PARAMETER, "TSS_RC_NO_ENCRYPT_PARAMETER - nse has no encrypt parameter"},
    {TSS_RC_BAD_DECRYPT_ALGORITHM, "TSS_RC_BAD_DECRYPT_ALGORITHM - ion had an unimplemented decrypt symmetric algorithm"},
    {TSS_RC_BAD_ENCRYPT_ALGORITHM, "TSS_RC_BAD_ENCRYPT_ALGORITHM - ion had an unimplemented encrypt symmetric algorithm"},
    {TSS_RC_AES_ENCRYPT_FAILURE, "TSS_RC_AES_ENCRYPT_FAILURE - AES encryption failed"},
    {TSS_RC_AES_DECRYPT_FAILURE, "TSS_RC_AES_DECRYPT_FAILURE - AES decryption failed\n"
     "\tIf using command line utilities, set env variable TPM_ENCRYPT_SESSIONS to 0\n"
     "\tor see TSS manual for more options"},
    {TSS_RC_BAD_ENCRYPT_SIZE, "TSS_RC_BAD_ENCRYPT_SIZE - Parameter encryption size mismatch"},
    {TSS_RC_AES_KEYGEN_FAILURE, "TSS_RC_AES_KEYGEN_FAILURE - AES key generation failed"},
    {TSS_RC_BAD_SALT_KEY, "TSS_RC_BAD_SALT_KEY - Key is unsuitable for salt"},
    {TSS_RC_KDFA_FAILED, "TSS_RC_KDFA_FAILED - KDFa function failed"},
    {TSS_RC_HMAC, "TSS_RC_HMAC -  An HMAC calculation failed"},
    {TSS_RC_HMAC_SIZE, "TSS_RC_HMAC_SIZE - nse HMAC is the wrong size"},
    {TSS_RC_HMAC_VERIFY, "TSS_RC_HMAC_VERIFY - MAC does not verify"},
    {TSS_RC_BAD_HASH_ALGORITHM, "TSS_RC_BAD_HASH_ALGORITHM - Unimplemented hash algorithm"},
    {TSS_RC_HASH, "TSS_RC_HASH - A hash calculation failed"},
    {TSS_RC_RSA_KEY_CONVERT, "TSS_RC_RSA_KEY_CONVERT - RSA key conversion failed"},
    {TSS_RC_RSA_PADDING, "TSS_RC_RSA_PADDING - RSA add padding failed"},
    {TSS_RC_RSA_ENCRYPT, "TSS_RC_RSA_ENCRYPT - RSA public encrypt failed"},
    {TSS_RC_BIGNUM, "TSS_RC_BIGNUM - NUM operation failed"},
    {TSS_RC_RSA_SIGNATURE, "TSS_RC_RSA_SIGNATURE - RSA signature is bad"},
    {TSS_RC_EC_SIGNATURE, "TSS_RC_EC_SIGNATURE - EC signature is bad"},
    {TSS_RC_EC_KEY_CONVERT, "TSS_RC_EC_KEY_CONVERT - EC key conversion failed"},
    {TSS_RC_X509_ERROR, "TSS_RC_X509_ERROR - X509 parse error"},
    {TSS_RC_BAD_SIGNATURE_ALGORITHM, "TSS_RC_BAD_SIGNATURE_ALGORITHM - Unimplemented signature algorithm"},
    {TSS_RC_COMMAND_UNIMPLEMENTED, "TSS_RC_COMMAND_UNIMPLEMENTED - Unimplemented command"},
    {TSS_RC_IN_PARAMETER, "TSS_RC_IN_PARAMETER - Bad in parameter to TSS_Execute"},
    {TSS_RC_OUT_PARAMETER, "TSS_RC_OUT_PARAMETER - Bad out parameter to TSS_Execute"},
    {TSS_RC_BAD_HANDLE_NUMBER, "TSS_RC_BAD_HANDLE_NUMBER - Bad handle number for this command"},
    {TSS_RC_KDFE_FAILED, "TSS_RC_KDFE_FAILED - KDFe function failed"},
    {TSS_RC_EC_EPHEMERAL_FAILURE, "TSS_RC_EC_EPHEMERAL_FAILURE - Failed while making or using EC ephemeral key"},
    {TSS_RC_NO_SESSION_SLOT, "TSS_RC_NO_SESSION_SLOT - TSS context has no session slot for handle"},
    {TSS_RC_NO_OBJECTPUBLIC_SLOT, "TSS_RC_NO_OBJECTPUBLIC_SLOT - TSS context has no object public slot for handle"},
    {TSS_RC_NO_NVPUBLIC_SLOT, "TSS_RC_NO_NVPUBLIC_SLOT -TSS context has no NV public slot for handle"}
};

#define BITS1108	0xf00
#define BITS1108SHIFT	8

#define BITS1008	0x700
#define BITS1008SHIFT	8

#define BITS0600	0x07f
#define BITS0500	0x03f

#define BITS87		0x180
#define BIT11		0x800
#define BIT10		0x400
#define BIT7		0x080
#define BIT6		0x040

#define TSSMASK		0x00ff0000	/* 23:16 */

/* Test cases

   TPM 	1.2	001
   TPM 	param	1c1
   TPM	handle  181
   TPM	session	981
   TSS		b0001
*/

/* TSS namespace starts with bit 16 */
#define TSS_RC_LEVEL_SHIFT 16

/* TSS error level name space */
#define TSS_ERROR_LEVEL (11 << TSS_RC_LEVEL_SHIFT )

/* Figure 26 - Response Code Evaluation */	    

void TSS_ResponseCode_toString(const char **msg, const char **submsg,  const char **num, TPM_RC rc)
{
    *submsg = "";	/* sometimes no sub-message */
    *num = "";		/* sometime no number */

    if (rc == 0) {
	*msg = "TPM_RC_SUCCESS";
    }
    /* if TSS 11 << 16 */
    else if ((rc & TSSMASK) == TSS_ERROR_LEVEL) {
	*msg = TSS_ResponseCode_RcToText(tssTable, sizeof(tssTable) / sizeof(RC_TABLE), rc);
    }
    /* if bits 8:7 are 00 */
    else if ((rc & BITS87) == 0) {
	/* TPM 1.2  x000 0xxx xxxx */
	*msg = "TPM 1.2 response code";
    }
    /* if bits 8:7 are not 00 */
    else {
	/* if bit 7 is 0 */
	if ((rc & BIT7) == 0) {
	    /* if bit 10 is 1 */
	    if ((rc & BIT10) != 0) {
		/* vendor defined x101 0xxx xxxx */
		*msg = "TPM2 vendor defined response code";
	    }
	    /* if bit 10 is 0 */
	    else {
		/* if bit 11 is 1 */
		if ((rc & BIT11) != 0) {
		    /* warning 1001 0xxx xxxx RC_WARN */
		    *msg = TSS_ResponseCode_RcToText(warnTable,
						     sizeof(warnTable) / sizeof(RC_TABLE),
						     rc & (BITS0600 | RC_WARN));
		}
		/* if bit 11 is 0 */
		else {
		    /* error 0001 0xxx xxxx  RC_VER1 */
		    *msg = TSS_ResponseCode_RcToText(ver1Table,
						     sizeof(ver1Table) / sizeof(RC_TABLE),
						     rc & (BITS0600 | RC_VER1));
		}
	    }
	}
	/* if bit 7 is 1 RC_FMT1 */
	else {
	    /* if bit 6 is 1 */
	    if ((rc & BIT6) != 0) {
		/* error xxxx 11xx xxxx */
		*msg = TSS_ResponseCode_RcToText(fmt1Table,
						 sizeof(fmt1Table) / sizeof(RC_TABLE),
						 rc & (BITS0500 | RC_FMT1));
		*submsg = " Parameter number ";
		*num = TSS_ResponseCode_NumberToText((rc & BITS1108) >> BITS1108SHIFT); 
	    }
	    /* if bit 6 is 0 */
	    else {
		/* if bit 11 is 1 */
		if ((rc & BIT11) != 0) {
		    /* error 1xxx 10xx xxxx */
		    *msg = TSS_ResponseCode_RcToText(fmt1Table,
						     sizeof(fmt1Table) / sizeof(RC_TABLE),
						     rc & (BITS0500 | RC_FMT1));
		    *submsg = " Session number ";
		    *num = TSS_ResponseCode_NumberToText((rc & BITS1008) >> BITS1008SHIFT); 
		}
		/* if bit 11 is 0 */
		else {
		    /* error 0xxx 10xx xxxx */
		    *msg = TSS_ResponseCode_RcToText(fmt1Table,
						     sizeof(fmt1Table) / sizeof(RC_TABLE),
						     rc & (BITS0500 | RC_FMT1));
		    *submsg = " Handle number ";
		    *num = TSS_ResponseCode_NumberToText((rc & BITS1008) >> BITS1008SHIFT); 
		}
	    }
	}
    }
    return;
}

static const char *TSS_ResponseCode_RcToText(const RC_TABLE *table, size_t tableSize, TPM_RC rc) 
{
    size_t i;

    for (i = 0 ; i < tableSize ; i++) {
	if (table[i].rc == rc) {
	    return table[i].text;
	}
    }
    return "response code unknown";
}

static const char *TSS_ResponseCode_NumberToText(unsigned int num)
{
    if (num < (sizeof(num_table) / sizeof(const char *))) {
	return num_table[num];
    }
    else {
	return "out of bounds";
    }
}

