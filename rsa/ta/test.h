/***
*
* FILENAME :
*
*        TA_test.h
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

#ifndef TEST_H
#define TEST_H

// function prototypes
void local_encrypt_and_decrypt_test (void);
TEE_Result TA_encrypt_command (TEE_Param params[4]);
TEE_Result TA_decrypt_command (TEE_Param params[4]);

#endif
