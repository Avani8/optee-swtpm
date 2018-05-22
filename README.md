# OP-TEE Sample Applications
## Contents
1. [Introduction](#1-introduction)
2. [List of sample applications](#2-list-of-sample-applications)
3. [How to build a Trusted Application](#3-how-to-build-a-trusted-application)


## 1. Introduction
This document describes the POC implementation  of TPM2.0 as TA in the OP-TEE,
that aim to showcase specific functionality and use case.

For sake of simplicity, all tpm test application are prefixed with
`tpm_.

---
## 2. List of sample applications

Directory **hello_world/**:
* A very simple Trusted Application to answer a hello command and incrementing
an integer value.
* Test application: `tpm_hello_world`
* Trusted application UUID: 8aaaf200-2450-11e4-abe2-0002a5d5c51b

Directory **random/**:
* Generates a random UUID using capabilities of TEE API (`TEE_GenerateRandom()`).
* Test application: `tpm_random`
* Trusted application UUID: b6c53aba-9669-4668-a7f2-205629d00f86

Directory **aes/**:
* Runs an AES encryption and decryption from a TA using the GPD TEE Internal
Core API. Non secure test application provides the key, initial vector and
ciphered data.
* Test application: `tpm_aes`
* Trusted application UUID: 5dbac793-f574-4871-8ad3-04331ec17f24

Directory **sha/**:
* Runs  sha (sha1,sha128,sha256,sha384, sha512)  hashing algorithms  from a TA using the GPD TEE Internal
Core API. Non secure test application provides the key, initial vector and
data.
* Test application: `tpm_sha`sha1
* Trusted application UUID: 5dbac793-f574-4871-8ad3-04331ec17f24

Directory **rsa/**:
* Runs an rsa encryption and decryption from a TA using the GPD TEE Internal
Core API. Non secure test application provides the key, initial vector and
ciphered data.
* Test application: `tpm_rsa`
* Trusted application UUID: 5dbac793-f574-4871-8ad3-04331ec17f24

Directory */hotp**:
* Runs an hotp from a TA using the GPD TEE Internal
Core API. Non secure test application provides the key, initial vector and
ciphered data.
* Test application: `tpm_hotp`
* Trusted application UUID: 5dbac793-f574-4871-8ad3-04331ec17f24

## 3. How to build a Trusted Application
This documentation presents the basics for implementing and building
an OP-TEE trusted application.

updating documentation is underprocess 

