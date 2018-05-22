/***
*
* FILENAME :
*
*        TA.h
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
* DATE         WHO               DETAIL
* 26.06.2017    Ursus Schneider  Initial release
*
***/
#ifndef TA_H
#define TA_H

/* This UUID is generated with uuidgen
the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html */

/* Version 4 UUID Generator used on 15.08.2017 @ 23:00
d6f85138-131c-4f86-b746-a066b2536e69     */

#define TEST_TA_UUID { 0xd6f85138, 0x131c, 0x4f86, {0xb7, 0x46, 0xa0, 0x66, 0xb2, 0x53, 0x6e, 0x69} }

/* The Trusted Application Function ID(s) implemented in this TA */
#define TEST_EN_DE_CRYPT_COMMAND 0
#define TEST_ENCRYPT_IN_TA_COMMAND 1
#define TEST_DECRYPT_IN_TA_COMMAND 2

#endif /* TA_H */
