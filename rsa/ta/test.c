/***
*
* FILENAME :
*
*        TA_test.c
*
* DESCRIPTION :
*
*        Sundry test functions
*
* NOTES :
*
*        Copyright Ursus Schneider 2017.  All rights reserved.
*
* AUTHOR :
*
*        Ursus Schneider        START DATE :   15 August 2017
*
* CHANGES :
*
* DATE    		 WHO     				DETAIL
* 15.08.2017    Ursus Schneider 	   Initial release
*
***/

#include <string.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "../types.h"
#include "test.h"
#include "crypto.h"

// test encrypting in the TA
TEE_Result TA_encrypt_command (TEE_Param params[4]) {

   // string to encrypt
   char * in = (char *)params[0].memref.buffer;
   int in_len = strlen (params[0].memref.buffer);

   // will be encrypted into here
   char encrypted [256];
   int encrypted_len;

   DMSG("<<<<<<<<<<<<<<<<<<<<<<<<<<< test_encrypt_ta >>>>>>>>>>>>>>>>>>>>>>>>> ");
   encrypt_using_private_key (in, in_len, encrypted, &encrypted_len);
   memcpy(params[1].memref.buffer, encrypted, encrypted_len);
   params[2].value.a = 0;
   DMSG ("");
   DMSG ("In value:            %s", in);
   DMSG ("In len:              %i", in_len);
   DMSG ("SW Encryted value:   %s", encrypted);
   DMSG ("SW Encryted len:     %i", encrypted_len);
   DMSG ("");
   DMSG("<<<<<<<<<<<<<<<<<<<<<<<<<<< end of test >>>>>>>>>>>>>>>>>>>>>>>>> ");
   return TEE_SUCCESS;

}

// test encrypting in the TA
TEE_Result TA_decrypt_command (TEE_Param params[4]) {

   // string to encrypt
   char * in = (char *)params[0].memref.buffer;
   int in_len = params[0].memref.size;

   // will be encrypted into here
   char decrypted [256];
   int decrypted_len;

   DMSG("<<<<<<<<<<<<<<<<<<<<<<<<<<< test_decrypt_ta >>>>>>>>>>>>>>>>>>>>>>>>> ");
   decrypt_using_public_key  (in, in_len, decrypted, &decrypted_len);
   memcpy(params[1].memref.buffer, decrypted, decrypted_len);
   params[1].memref.size = decrypted_len;
   params[2].value.a = 0;
   DMSG ("");
   DMSG ("NW encrypt value:       %s", in);
   DMSG ("NW encrypt len:         %i", in_len);
   DMSG ("SW decryted value:      %s", decrypted);
   DMSG ("SW decryted len:        %i", decrypted_len);
   DMSG ("");
   DMSG("<<<<<<<<<<<<<<<<<<<<<<<<<<< end of test >>>>>>>>>>>>>>>>>>>>>>>>> ");
   return TEE_SUCCESS;

}

// local encrypt and decrypt test
void local_encrypt_and_decrypt_test (void) {

   // just some testing
   char test_in [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   int test_in_len = strlen (test_in);
   char test_encrypted [256];
   int test_encrypted_len;
   char test_decrypted [256];
   int test_decrypted_len;

   DMSG("<<<<<<<<<<<<<<<<<<<<<<<<<<< check_en_decrypt >>>>>>>>>>>>>>>>>>>>>>>>> ");
   encrypt_using_private_key (test_in,        test_in_len,        test_encrypted, &test_encrypted_len);
   decrypt_using_public_key  (test_encrypted, test_encrypted_len, test_decrypted, &test_decrypted_len);
   DMSG ("");
   DMSG ("In value (22 chars):     %s", test_in);
   DMSG ("In len:                  %i", test_in_len);
   DMSG ("SW encryted value:       %s", test_encrypted);
   DMSG ("SW encryted len:         %i", test_encrypted_len);
   DMSG ("SW decrypted value:      %s", test_decrypted);
   DMSG ("SW decrypted len:        %i", test_decrypted_len);
   DMSG ("");
   DMSG("<<<<<<<<<<<<<<<<<<<<<<<<<<< end of test >>>>>>>>>>>>>>>>>>>>>>>>> ");

}
