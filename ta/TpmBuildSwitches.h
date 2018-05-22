/********************************************************************************/
/*										*/
/*			    Build Switches	 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmBuildSwitches.h 1091 2017-10-31 20:31:59Z kgoldman $	*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2016, 2017				*/
/*										*/
/********************************************************************************/

#ifndef TPMBUILDSWITCHES_H
#define TPMBUILDSWITCHES_H

/* This file contains the build switches. This contains switches for multiple versions of the
   crypto-library so some may not apply to your environment. */
/* The switches are guarded so that they can either be set on the command line or set here. */
/* Many of the #defines are guarded so that they can be set on the command line without causing
   consternation in the compiler. */
#ifndef INLINE_FUNCTIONS
//#  define INLINE_FUNCTIONS
#endif
/* Don't move this include ahead of the INLINE_FUNCTIONS definition. */
#include "CompilerDependencies.h"
/* This definition is required for the re-factored code */
#define USE_BN_ECC_DATA
/* Comment these out as needed */
/* Comment out to generate actual random numbers */
#ifndef SIMULATION
#  define SIMULATION
#endif
/* Define this to run the function that checks the format compatibility for the chosen big number
   math library. Not all ports use this. */
#if !defined LIBRARY_COMPATIBILITY_CHECK && defined SIMULATION
#   define LIBRARY_COMPATABILITY_CHECK
#endif
#ifndef FIPS_COMPLIANT
//#  define FIPS_COMPLIANT
#endif
/* Definition to allow alternate behavior for non-orderly startup. If there is a chance that the TPM
   could not update failedTries */
#ifndef USE_DA_USED
#   define USE_DA_USED
#endif
/* Define TABLE_DRIVEN_DISPATCH to use tables rather than case statements for command dispatch and
   handle unmarshaling */
#ifndef TABLE_DRIVEN_DISPATCH
#  define TABLE_DRIVEN_DISPATCH
#endif
/* This switch is used to enable the self-test capability in AlgorithmTests.c */
#ifndef SELF_TEST
#define SELF_TEST
#endif
/* Enable the generation of RSA primes using a sieve. */
#ifndef RSA_KEY_SIEVE
#  define RSA_KEY_SIEVE
#endif
/* Enable the instrumentation of the sieve process. This is used to tune the sieve variables.*/
#if !defined RSA_INSTRUMENT && defined RSA_KEY_SIEVE && defined SIMULATION
//#define RSA_INSTRUMENT
#endif
#if defined RSA_KEY_SIEVE && !defined NDEBUG && !defined RSA_INSTRUMENT
//# define RSA_INSTRUMENT
#endif
/* This switch enables the RNG state save and restore */
#ifndef _DRBG_STATE_SAVE
#  define _DRBG_STATE_SAVE        // Comment this out if no state save is wanted
#endif
/* Switch added to support packed lists that leave out space associated with unimplemented
   commands. Comment this out to use linear lists. */
/* NOTE: if vendor specific commands are present, the associated list is always in compressed
   form. */
#ifndef COMPRESSED_LISTS
#   define COMPRESSED_LISTS
#endif
/* This switch indicates where clock epoch value should be stored. If this value defined, then it is
   assumed that the timer will change at any time so the nonce should be a random number kept in
   RAM. When it is not defined, then the timer only stops during power outages. */
#ifndef CLOCK_STOPS
//#   define CLOCK_STOPS
#endif
/* The switches in this group can only be enabled when running a simulation */
#ifdef SIMULATION
/* Enables use of the key cache */
#   ifndef USE_RSA_KEY_CACHE
#       define USE_RSA_KEY_CACHE
#   endif
#   if defined USE_RSA_KEY_CACHE && !defined USE_KEY_CACHE_FILE
#       define USE_KEY_CACHE_FILE
#   endif
#   if !defined NDEBUG && !defined USE_DEBUG_RNG
/* This provides fixed seeding of the RNG when doing debug on a simulator. This should allow
   consistent results on test runs as long as the input parameters to the functions remains the
   same. */
#       define USE_DEBUG_RNG
#   endif
#else
#   undef USE_RSA_KEY_CACHE
#   undef USE_KEY_CACHE_FILE
#   undef USE_DEBUG_RNG
#   undef RSA_INSTRUMENT
#endif  // SIMULATION
#ifndef NDEBUG
/* In some cases, the relationship between two values may be dependent on things that change based
   on various selections like the chosen cryptographic libraries. It is possible that these
   selections will result in incompatible settings. These are often detectable by the compiler but
   it isn't always possible to do the check in the preprocessor code. For example, when the check
   requires use of 'sizeof()' then the preprocessor can't do the comparison. For these cases, we
   include a special macro that, depending on the compiler will generate a warning to indicate if
   the check always passes or always fails because it involves fixed constants. To run these checks,
   define COMPILER_CHECKS. */
#ifndef COMPILER_CHECKS
//#   define COMPILER_CHECKS
#endif
/* Some of the values (such as sizes) are the result of different options set in
   Implementation.h. The combination might not be consistent. A function is defined
   (TpmSizeChecks()) that is used to verify the sizes at run time. To enable the function, define
   this parameter. */
#ifndef RUNTIME_SIZE_CHECKS
#define RUNTIME_SIZE_CHECKS
#endif

/* If doing debug, can set the DRBG to print out the intermediate test values. Before enabling this,
   make sure that the dbgDumpMemBlock() function has been added someplace (preferably, somewhere in
   CryptRand.c) */
#ifndef DRBG_DEBUG_PRINT
//#  define DRBG_DEBUG_PRINT
#endif
/* If an assertion event it not going to produce any trace information (function and line number)
   then define NO_FAIL_TRACE */
#ifndef NO_FAIL_TRACE
//#   define NO_FAIL_TRACE
#endif
#endif // NDEBUG
/* If the implementation is going to give lockout time credit for time up to the last orderly
   shutdown, then uncomment this variable */
#ifndef ACCUMULATE_SELF_HEAL_TIMER
#define ACCUMULATE_SELF_HEAL_TIMER
#endif // ACCUMULATE_SELF_HEAL_TIMER
/* If the implementation is to compute the sizes of the proof and primary seed size values based on
   the implemented algorithms, then use this define. */
#ifndef USE_SPEC_COMPLIANT_PROOFS
#define  USE_SPEC_COMPLIANT_PROOFS
#endif
/* Comment this out to allow compile to continue even though the chosen proof values do not match
   the compliant values. This is written so that someone would have to proactively ignore errors. */
#ifndef SKIP_PROOF_ERRORS
//#define SKIP_PROOF_ERRORS
#endif
/* This define is used to eliminate the use of bit-fields. It can be enable for big- or
   little-endian machines but is required of big-endian system that number bits in registers from
   left to right. Little-endian machines number from right to left with the least significant bit
   having assigned a bit number of 0. These are LSb0() machines (they are also little-endian so they
   are also least-significant byte 0 (LSB0) machines. Big-endian (MSB0) machines may number in
   either direction (MSb0() or LSb0()). For an MSB0+MSb0() machine this value should be defined */
#ifndef NO_BIT_FIELD_STRUCTURES
//#   define NO_BIT_FIELD_STRUCTURES
#endif
/* Change these definitions to turn all algorithms or commands ON or OFF. That is, to turn all
   algorithms on, set ALG_NO to YES. This is mostly useful as a debug feature. */
#define      ALG_YES      YES
#define      ALG_NO       NO
#define      CC_YES       YES
#define      CC_NO        NO
#endif // _TPM_BUILD_SWITCHES_H_

