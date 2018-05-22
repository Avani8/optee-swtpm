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
* NOTES :
*
*
***/

#include <stdio.h>
#include <string.h>
#include <openssl/pem.h>

#include "../types.h"
#include "crypto.h"

RSA *createRSA(unsigned char *key, int public);

// global variables
char public_key[] =
"-----BEGIN PUBLIC KEY-----\n"
"MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAL5c51/v1osjr5+lRPykmpQKyGdXMG0g\n"
"S6Du1l8Hm0qYXc+azq6qqZvr39zeufw/VLKTfeKeKVJX1D28TImn6cUCAwEAAQ==\n"
"-----END PUBLIC KEY-----\n";

char private_key[] =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIIBOgIBAAJBAL5c51/v1osjr5+lRPykmpQKyGdXMG0gS6Du1l8Hm0qYXc+azq6q\n"
"qZvr39zeufw/VLKTfeKeKVJX1D28TImn6cUCAwEAAQJASDCJGculK6zDzCHrkHeH\n"
"mz6fkvjwh2Go7IXGS9FhpZ6Lx6FacvAEyARdXlIYXNRogiEX3aHMQoflhOFYIMID\n"
"fQIhAPj4koWd11bSLeR5bI1ojNm/M7y6oKYiWlX/Txbo66L7AiEAw7y+czu2VIdK\n"
"qcUfGnLfI9qVrZPhw4rB14/3oOBXCj8CIQC5yINNwaLW3q/wNcuTGdlBAzSQOJN4\n"
"ZVoTohhaeCSd0QIgGqi0T8GMPcsHckP0zodiuOFmjXOcxiM574AeO/0SHcUCICkw\n"
"Ztd6hrPK/M6HFQL/fGu1MecHNrsKyroMlZNqLmXu\n"
"-----END RSA PRIVATE KEY-----\n";

// local functions
RSA *createRSA (unsigned char *key, int public);

// encrypt in string using the private key
BOOLEAN encrypt_using_private_key (char * in, int in_len, char * out, int * out_len) {

   RSA * rsa;

   rsa = createRSA ((unsigned char *) private_key, 0);
   *out_len = RSA_private_encrypt (in_len, (unsigned char *)in, (unsigned char *)out, rsa, RSA_PKCS1_PADDING);

   if (*out_len == -1) {
      return FALSE;
   } else {
      return TRUE;
   }
}

// decrypt in string using the public key
BOOLEAN decrypt_using_public_key (char * in, int in_len, char * out, int * out_len) {

   RSA * rsa;

   rsa = createRSA ((unsigned char *) public_key, 1);
   *out_len = RSA_public_decrypt (in_len, (unsigned char *)in, (unsigned char *) out, rsa, RSA_PKCS1_PADDING);

   if (*out_len == -1)
      return FALSE;
   else {
      return TRUE;
   }
}

// create RSA object
RSA *createRSA(unsigned char *key, int public) {
   RSA *rsa = NULL;
   BIO *keybio;
   keybio = BIO_new_mem_buf(key, -1);
   if (keybio == NULL) {
      printf("==> Error: Failed to create key BIO");
      return 0;
   }
   if (public) {
      rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
   } else {
      rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
   }
   if (rsa == NULL) {
      printf("==> Error: Failed to create RSA");
   }
   return rsa;
}
