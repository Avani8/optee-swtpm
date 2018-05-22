/***
*
* FILENAME :
*
*        crypto.c
*
* DESCRIPTION :
*
*        Crypto functions
*
***/

#include <string.h>

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "../types.h"
#include "crypto.h"

#define SIZE_OF_VEC(vec) (sizeof(vec) - 1)
//
//   These values found using the OS command:
//
//         openssl rsa -in private.pem -text
//
uint8_t modulus[] =
"\xbe\x5c\xe7\x5f\xef\xd6\x8b\x23\xaf\x9f\xa5\x44\xfc\xa4\x9a"
"\x94\x0a\xc8\x67\x57\x30\x6d\x20\x4b\xa0\xee\xd6\x5f\x07\x9b"
"\x4a\x98\x5d\xcf\x9a\xce\xae\xaa\xa9\x9b\xeb\xdf\xdc\xde\xb9"
"\xfc\x3f\x54\xb2\x93\x7d\xe2\x9e\x29\x52\x57\xd4\x3d\xbc\x4c"
"\x89\xa7\xe9\xc5";

uint8_t public_key[] =
"\x01\x00\x01";

uint8_t private_key[] =
"\x48\x30\x89\x19\xcb\xa5\x2b\xac\xc3\xcc\x21\xeb\x90\x77\x87"
"\x9b\x3e\x9f\x92\xf8\xf0\x87\x61\xa8\xec\x85\xc6\x4b\xd1\x61"
"\xa5\x9e\x8b\xc7\xa1\x5a\x72\xf0\x04\xc8\x04\x5d\x5e\x52\x18"
"\x5c\xd4\x68\x82\x21\x17\xdd\xa1\xcc\x42\x87\xe5\x84\xe1\x58"
"\x20\xc2\x03\x7d";

// self explanitory really
BOOLEAN decrypt_using_public_key (char * in, int in_len, char * out, int * out_len) {

   TEE_Result ret = TEE_SUCCESS; // return code
   TEE_ObjectHandle key = (TEE_ObjectHandle) NULL;
   TEE_Attribute rsa_attrs[3];  // array for the keys
   TEE_ObjectInfo info;
   TEE_OperationHandle handle = (TEE_OperationHandle) NULL;

   uint32_t cipher_len = 512; // return encrypted hash length
   void *cipher = NULL; // return encrypted hash

   uint32_t decoded_len = 512; // return decoded hash length
   void *decoded = NULL; // return decoded hash

   // modulo
   rsa_attrs[0].attributeID = TEE_ATTR_RSA_MODULUS;
   rsa_attrs[0].content.ref.buffer = modulus;
   rsa_attrs[0].content.ref.length = SIZE_OF_VEC(modulus);

   // Public key
   rsa_attrs[1].attributeID = TEE_ATTR_RSA_PUBLIC_EXPONENT;
   rsa_attrs[1].content.ref.buffer = public_key;
   rsa_attrs[1].content.ref.length = SIZE_OF_VEC(public_key);

   // Private key ==> I don't have this so just add the public key again!!!
   rsa_attrs[2].attributeID = TEE_ATTR_RSA_PRIVATE_EXPONENT;
   rsa_attrs[2].content.ref.buffer = private_key;
   rsa_attrs[2].content.ref.length = SIZE_OF_VEC(private_key);

   // create a transient object
   //      ret = TEE_AllocateTransientObject(TEE_TYPE_RSA_KEYPAIR, 1024, &key);
   ret = TEE_AllocateTransientObject(TEE_TYPE_RSA_KEYPAIR, 512, &key);
   if (ret != TEE_SUCCESS) {
      return TEE_ERROR_BAD_PARAMETERS;
   }

   // populate the object with your keys
   ret = TEE_PopulateTransientObject(key, (TEE_Attribute *)&rsa_attrs, 3);
   if (ret != TEE_SUCCESS) {
      return TEE_ERROR_BAD_PARAMETERS;
   }

   cipher = TEE_Malloc (cipher_len, 0);
   decoded = TEE_Malloc (decoded_len, 0);
   if (!cipher || !decoded) {
      return TEE_ERROR_BAD_PARAMETERS;
   }

   TEE_MemMove (cipher, in, in_len);
   cipher_len = in_len;

   // setup the info structure about the key
   TEE_GetObjectInfo (key, &info);

   // Allocate the operation
   ret = TEE_AllocateOperation (&handle, TEE_ALG_RSAES_PKCS1_V1_5, TEE_MODE_DECRYPT, info.maxObjectSize);
   if (ret != TEE_SUCCESS) {
      return FALSE;
   }

   // set the key
   ret = TEE_SetOperationKey(handle, key);
   if (ret != TEE_SUCCESS) {
      TEE_FreeOperation(handle);
      return FALSE;
   }

   // decrypt
   ret = TEE_AsymmetricDecrypt (handle, (TEE_Attribute *)NULL, 0, cipher, in_len, decoded, &decoded_len);
   if (ret != TEE_SUCCESS) {
      TEE_FreeOperation(handle);
      return TEE_ERROR_BAD_PARAMETERS;
   }

   // return encrypted
   memcpy (out, decoded, decoded_len);
   *out_len = decoded_len;

   // clean up after yourself
   TEE_FreeOperation(handle);
   TEE_FreeTransientObject (key);
   TEE_Free (cipher);

   // fin
   return TRUE;
}

// encrypt
BOOLEAN encrypt_using_private_key (char * in, int in_len, char * out, int * out_len) {

   TEE_Result ret = TEE_SUCCESS; // return code
   TEE_ObjectHandle key = (TEE_ObjectHandle) NULL;
   TEE_Attribute rsa_attrs[3];
   void * to_encrypt = NULL;
   uint32_t cipher_len = 256;
   void * cipher = NULL;
   TEE_ObjectInfo info;
   TEE_OperationHandle handle = (TEE_OperationHandle) NULL;

   // modulus
   rsa_attrs[0].attributeID = TEE_ATTR_RSA_MODULUS;
   rsa_attrs[0].content.ref.buffer = modulus;
   rsa_attrs[0].content.ref.length = SIZE_OF_VEC (modulus);
   // Public key
   rsa_attrs[1].attributeID = TEE_ATTR_RSA_PUBLIC_EXPONENT;
   rsa_attrs[1].content.ref.buffer = public_key;
   rsa_attrs[1].content.ref.length = SIZE_OF_VEC (public_key);
   // Private key
   rsa_attrs[2].attributeID = TEE_ATTR_RSA_PRIVATE_EXPONENT;
   rsa_attrs[2].content.ref.buffer = private_key;
   rsa_attrs[2].content.ref.length = SIZE_OF_VEC (private_key);

   // create a transient object
   ret = TEE_AllocateTransientObject(TEE_TYPE_RSA_KEYPAIR, 1024, &key);
   if (ret != TEE_SUCCESS) {
      return TEE_ERROR_BAD_PARAMETERS;
   }

   // populate the object with your keys
   ret = TEE_PopulateTransientObject(key, (TEE_Attribute *)&rsa_attrs, 3);
   if (ret != TEE_SUCCESS) {
      return TEE_ERROR_BAD_PARAMETERS;
   }

   // create your structures to de / encrypt
   to_encrypt = TEE_Malloc (in_len, 0);
   cipher = TEE_Malloc (cipher_len, 0);
   if (!to_encrypt || !cipher) {
      return TEE_ERROR_BAD_PARAMETERS;
   }
   TEE_MemMove(to_encrypt, in, in_len);

   // setup the info structure about the key
   TEE_GetObjectInfo (key, &info);

   // Allocate the operation
   ret = TEE_AllocateOperation (&handle, TEE_ALG_RSAES_PKCS1_V1_5, TEE_MODE_ENCRYPT, info.maxObjectSize);
   if (ret != TEE_SUCCESS) {
      return FALSE;
   }

   // set the key
   ret = TEE_SetOperationKey(handle, key);
   if (ret != TEE_SUCCESS) {
      TEE_FreeOperation(handle);
      return FALSE;
   }

   // encrypt
   ret = TEE_AsymmetricEncrypt (handle, (TEE_Attribute *)NULL, 0, to_encrypt, in_len, cipher, &cipher_len);
   if (ret != TEE_SUCCESS) {
      TEE_FreeOperation(handle);
      return FALSE;
   }

   // finish off
   memcpy (out, cipher, cipher_len);
   *out_len = cipher_len;
   out[cipher_len] = '\0';

   // clean up after yourself
   TEE_FreeOperation(handle);
   TEE_FreeTransientObject (key);
   TEE_Free (cipher);


   // finished
   return TRUE;
}
