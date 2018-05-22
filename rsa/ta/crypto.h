/***
*
* FILENAME :
*
*        crypto.h
*
* DESCRIPTION :
*
*        Crypto functions
*
***/

#ifndef CRYPTO_H
#define CRYPTO_H

BOOLEAN decrypt_using_public_key (char * in, int in_len, char * out, int * out_len);
BOOLEAN encrypt_using_private_key (char * in, int in_len, char * out, int * out_len);

#endif
