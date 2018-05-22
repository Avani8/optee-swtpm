/***
*
* FILENAME :
*
*        TA.c
*
* DESCRIPTION :
*
*        TA to test en/decrypting
*
* NOTES :
*
*        Copyright Ursus Schneider 2017.  All rights reserved.
*
* AUTHOR :
*
*        Ursus Schneider        START DATE :    15 April 2017
*
* CHANGES :
*
* DATE    			WHO DETAIL
* 26.06.2017   Ursus Schneider 	Initial release
* 16.07.2017   Ursus Schneider   RSA functions
* 12.08.2017   Ursus Schneider   Split file into multiple files
*
***/
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "../types.h"
#include "TA.h"
#include "test.h"
#include "crypto.h"

// Called when the TA is created
TEE_Result TA_CreateEntryPoint(void) {
   DMSG("=============== TA_CreateEntryPoint ================");
   return TEE_SUCCESS;
}

// Called when the TA is destroyed
void TA_DestroyEntryPoint(void) {
   DMSG("=============== TA_DestroyEntryPoint ===============");
}

// open session
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types, TEE_Param __maybe_unused params[4], void __maybe_unused **sess_ctx) {

   DMSG("=========== TA_OpenSessionEntryPoint ===============");

   /* suppress compiler warnings */
   (void)&param_types;
   (void)&params;
   (void)&sess_ctx;

   // finised
   return TEE_SUCCESS;
}

// close session
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx) {

   DMSG("========== TA_CloseSessionEntryPoint ===============");

   /* suppress compiler warnings */
   (void)&sess_ctx;
}

// invoke command
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[4]) {

   TEE_Result rc;

   // suppress compiler warnings
   (void)&sess_ctx;
   (void)&param_types;

   DMSG("Switching to correct command");
   switch (cmd_id) {
   case TEST_EN_DE_CRYPT_COMMAND:
      local_encrypt_and_decrypt_test ();
      return TEE_SUCCESS;
   case TEST_ENCRYPT_IN_TA_COMMAND:
      rc = TA_encrypt_command (params);
      return rc;
   case TEST_DECRYPT_IN_TA_COMMAND:
      rc = TA_decrypt_command (params);
      return rc;
   default:
      return TEE_ERROR_BAD_PARAMETERS;
   }

}
