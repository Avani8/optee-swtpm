#!/bin/bash
#

#################################################################################
#										#
#			TPM2 regression test					#
#			     Written by Ken Goldman				#
#		       IBM Thomas J. Watson Research Center			#
#		$Id: testpolicy.sh 1085 2017-10-19 22:03:04Z kgoldman $		#
#										#
# (c) Copyright IBM Corporation 2015, 2017					#
# 										#
# All rights reserved.								#
# 										#
# Redistribution and use in source and binary forms, with or without		#
# modification, are permitted provided that the following conditions are	#
# met:										#
# 										#
# Redistributions of source code must retain the above copyright notice,	#
# this list of conditions and the following disclaimer.				#
# 										#
# Redistributions in binary form must reproduce the above copyright		#
# notice, this list of conditions and the following disclaimer in the		#
# documentation and/or other materials provided with the distribution.		#
# 										#
# Neither the names of the IBM Corporation nor the names of its			#
# contributors may be used to endorse or promote products derived from		#
# this software without specific prior written permission.			#
# 										#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		#
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		#
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR		#
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		#
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	#
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		#
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,		#
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY		#
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		#
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE		#
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		#
#										#
#################################################################################

# used for the name in policy ticket

if [ -z $TPM_DATA_DIR ]; then
    TPM_DATA_DIR=.
fi


echo ""
echo "Policy Command Code"
echo ""

echo "Create a signing key under the primary key - policy command code - sign"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policyccsign.bin > run.out
checkSuccess $?

echo "Load the signing key under the primary key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Sign a digest"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -pwdk sig > run.out
checkSuccess $?

# sign with correct policy command code
# cc69 18b2 2627 3b08 f5bd 406d 7f10 cf16
# 0f0a 7d13 dfd8 3b77 70cc bcd1 aa80 d811

echo "Start a policy session"
${PREFIX}startauthsession -se p > run.out
checkSuccess $?

echo "Sign a digest - policy, should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Policy command code - sign"
${PREFIX}policycommandcode -ha 03000000 -cc 15d > run.out
checkSuccess $?

echo "Policy get digest - should be cc69 ..."
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Sign a digest - policy and wrong password"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 -pwdk xxx > run.out
checkSuccess $?

echo "Sign a digest - policy, should fail, session used "
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

# quote with bad policy or bad command 

# echo "Start a policy session"
# ${PREFIX}startauthsession -se p > run.out
# checkSuccess $?

echo "Policy command code - sign"
${PREFIX}policycommandcode -ha 03000000 -cc 15d > run.out
checkSuccess $?

echo "Quote - PWAP"
${PREFIX}quote -hp 0 -hk 80000001 -os sig.bin -pwdk sig > run.out
checkSuccess $?

echo "Quote - policy, should fail"
${PREFIX}quote -hp 0 -hk 80000001 -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Policy restart, set back to zero"
${PREFIX}policyrestart -ha 03000000 > run.out 
checkSuccess $?

# echo "Flush the session"
# ${PREFIX}flushcontext -ha 03000000 > run.out
# checkSuccess $?

# echo "Start a policy session"
# ${PREFIX}startauthsession -se p > run.out
# checkSuccess $?

echo "Policy command code - quote"
${PREFIX}policycommandcode -ha 03000000 -cc 158 > run.out
checkSuccess $?

echo "Quote - policy, should fail"
${PREFIX}quote -hp 0 -hk 80000001 -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

# echo "Flush the session"
# ${PREFIX}flushcontext -ha 03000000 > run.out
# checkSuccess $?

echo "Flush the signing key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

echo ""
echo "Policy Command Code and Policy Password / Authvalue"
echo ""

echo "Create a signing key under the primary key - policy command code - sign, auth"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policyccsign-auth.bin > run.out
checkSuccess $?

echo "Load the signing key under the primary key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

# policypassword

echo "Policy restart, set back to zero"
${PREFIX}policyrestart -ha 03000000 > run.out 
checkSuccess $?

echo "Sign a digest - policy, should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Policy command code - sign"
${PREFIX}policycommandcode -ha 03000000 -cc 15d > run.out
checkSuccess $?

echo "Sign a digest - policy, should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Policy password"
${PREFIX}policypassword -ha 03000000 > run.out
checkSuccess $?

echo "Sign a digest - policy, no password should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Sign a digest - policy, password"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 -pwdk sig > run.out
checkSuccess $?

# policyauthvalue

# echo "Start a policy session"
# ${PREFIX}startauthsession -se p > run.out
# checkSuccess $?

echo "Policy command code - sign"
${PREFIX}policycommandcode -ha 03000000 -cc 15d > run.out
checkSuccess $?

echo "Policy authvalue"
${PREFIX}policyauthvalue -ha 03000000 > run.out
checkSuccess $?

echo "Sign a digest - policy, no password should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Sign a digest - policy, password"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 -pwdk sig > run.out
checkSuccess $?

echo "Flush the signing key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

echo ""
echo "Policy Password and Policy Authvalue flags"
echo ""

for COMMAND in policypassword policyauthvalue 

do

    echo "Create a signing key under the primary key - policy command code - sign, auth"
    ${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policyccsign-auth.bin > run.out
    checkSuccess $?

    echo "Load the signing key under the primary key"
    ${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
    checkSuccess $?

    echo "Start a policy session"
    ${PREFIX}startauthsession -se p > run.out
    checkSuccess $?

    echo "Policy command code - sign"
    ${PREFIX}policycommandcode -ha 03000000 -cc 15d > run.out
    checkSuccess $?

    echo "Policy ${COMMAND}"
    ${PREFIX}${COMMAND} -ha 03000000 > run.out
    checkSuccess $?

    echo "Sign a digest - policy, password"
    ${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 -pwdk sig > run.out
    checkSuccess $?

    echo "Flush signing key"
    ${PREFIX}flushcontext -ha 80000001 > run.out
    checkSuccess $?

    echo "Create a signing key under the primary key - policy command code - sign"
    ${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policyccsign.bin > run.out
    checkSuccess $?

    echo "Load the signing key under the primary key"
    ${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
    checkSuccess $?

    echo "Policy command code - sign"
    ${PREFIX}policycommandcode -ha 03000000 -cc 15d > run.out
    checkSuccess $?

    echo "Sign a digest - policy and wrong password"
    ${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 -pwdk xxx > run.out
    checkSuccess $?

    echo "Flush signing key"
    ${PREFIX}flushcontext -ha 80000001 > run.out
    checkSuccess $?

    echo "Flush policy session"
    ${PREFIX}flushcontext -ha 03000000 > run.out
    checkSuccess $?

done

echo ""
echo "Policy Signed"
echo ""

# create rsaprivkey.pem
# > openssl genrsa -out rsaprivkey.pem -aes256 -passout pass:rrrr 2048
# extract the public key
# > openssl pkey -inform pem -outform pem -in rsaprivkey.pem -passin pass:rrrr -pubout -out rsapubkey.pem 
# sign a test message msg.bin
# > openssl dgst -sha1 -sign rsaprivkey.pem -passin pass:rrrr -out pssig.bin msg.bin
#
# create the policy:
# after loadexternal, get the name from readpublic -ho 80000001 -v

# sha1
# 00 04 42 34 c2 4f c1 b9 de 66 93 a6 24 53 41 7d 
# 27 34 d7 53 8f 6f 

# sha256
# 00 0b 64 ac 92 1a 03 5c 72 b3 aa 55 ba 7d b8 b5 
# 99 f1 72 6f 52 ec 2f 68 20 42 fc 0e 0d 29 fa e8 
# 17 99 

# 00000160 plus the above name as text, add a blank line for empty policyRef
# to create policies/policysigned$HALG.txt
#
# 0000016000044234c24fc1b9de6693a62453417d2734d7538f6f
# 00000160000b64ac921a035c72b3aa55ba7db8b599f1726f52ec2f682042fc0e0d29fae81799
#
# makes sha256 policy by default, policy digest algorithm is separate from Name and signature hash algorithm
#
# > policymaker -if policies/policysigned$HALG.txt -of policies/policysigned$HALG.bin -pr
#
# 9d 81 7a 4e e0 76 eb b5 cf ee c1 82 05 cc 4c 01 
# b3 a0 5e 59 a9 b9 65 a1 59 af 1e cd 3d bf 54 fb 
#
# de bf 9d fa 3c 98 08 0b f1 7d d1 d0 7b 54 fd e1 
# 07 93 7f e5 40 50 9e 70 96 aa 73 27 53 b3 83 31 
#
# 80000000 primary key
# 80000001 verification public key
# 80000002 signing key with policy
# 03000000 policy session

for HALG in sha1 sha256
do

    echo "Load external just the public part of PEM at 80000001 - $HALG"
    ${PREFIX}loadexternal -halg $HALG -nalg $HALG -ipem policies/rsapubkey.pem > run.out
    checkSuccess $?

    echo "Sign a test message with openssl - $HALG"
    openssl dgst -$HALG -sign policies/rsaprivkey.pem -passin pass:rrrr -out pssig.bin msg.bin

    echo "Verify the signature with 80000001 - $HALG"
    ${PREFIX}verifysignature -hk 80000001 -halg $HALG -if msg.bin -is pssig.bin -raw > run.out
    checkSuccess $?

    echo "Create a signing key under the primary key - policy signed - $HALG"
    ${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policysigned$HALG.bin > run.out
    checkSuccess $?

    echo "Load the signing key under the primary key, at 80000002"
    ${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
    checkSuccess $?

    echo "Start a policy session"
    ${PREFIX}startauthsession -se p > run.out
    checkSuccess $?

    echo "Sign a digest - policy, should fail"
    ${PREFIX}sign -hk 80000002 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
    checkFailure $?

    echo "Policy signed, sign with PEM key - $HALG"
    ${PREFIX}policysigned -hk 80000001 -ha 03000000 -sk policies/rsaprivkey.pem -halg $HALG -pwdk rrrr > run.out
    checkSuccess $?

    echo "Get policy digest, should be f877 ..."
    ${PREFIX}policygetdigest -ha 03000000 -of tmppol.bin > run.out
    checkSuccess $?

    echo "Sign a digest - policy signed"
    ${PREFIX}sign -hk 80000002 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
    checkSuccess $?

    echo "Policy restart, set back to zero"
    ${PREFIX}policyrestart -ha 03000000 > run.out 
    checkSuccess $?

    echo "Sign just expiration (uint32_t 4 zeros) with openssl - $HALG"
    openssl dgst -$HALG -sign policies/rsaprivkey.pem -passin pass:rrrr -out pssig.bin policies/zero4.bin

    echo "Policy signed, signature generated externally - $HALG"
    ${PREFIX}policysigned -hk 80000001 -ha 03000000 -halg $HALG -is pssig.bin > run.out
    checkSuccess $?

    echo "Sign a digest - policy signed"
    ${PREFIX}sign -hk 80000002 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
    checkSuccess $?

    echo "Start a policy session - save nonceTPM"
    ${PREFIX}startauthsession -se p -on noncetpm.bin > run.out
    checkSuccess $?

    echo "Policy signed with nonceTPM and expiration, create a ticket - $HALG"
    ${PREFIX}policysigned -hk 80000001 -ha 03000000 -sk policies/rsaprivkey.pem -halg $HALG -pwdk rrrr -in noncetpm.bin -exp -200 -tk tkt.bin -to to.bin > run.out
    checkSuccess $?

    echo "Sign a digest - policy signed"
    ${PREFIX}sign -hk 80000002 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
    checkSuccess $?

    echo "Start a policy session"
    ${PREFIX}startauthsession -se p > run.out
    checkSuccess $?

    echo "Policy ticket"
    ${PREFIX}policyticket -ha 03000000 -to to.bin -na ${TPM_DATA_DIR}/h80000001.bin -tk tkt.bin > run.out
    checkSuccess $?

    echo "Sign a digest - policy ticket"
    ${PREFIX}sign -hk 80000002 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
    checkSuccess $?

    echo "Flush the verification public key"
    ${PREFIX}flushcontext -ha 80000001 > run.out
    checkSuccess $?

    echo "Flush the signing key"
    ${PREFIX}flushcontext -ha 80000002 > run.out
    checkSuccess $?

done

# getcapability  -cap 1 -pr 80000000
# getcapability  -cap 1 -pr 02000000
# getcapability  -cap 1 -pr 03000000

# exit 0

echo ""
echo "Policy Secret with Platform Auth"
echo ""

# 4000000c platform
# 80000000 primary key
# 80000001 signing key with policy
# 03000000 policy session
# 02000001 hmac session

echo "Change platform hierarchy auth"
${PREFIX}hierarchychangeauth -hi p -pwdn ppp > run.out
checkSuccess $?

echo "Create a signing key under the primary key - policy secret using platform auth"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policysecretp.bin > run.out
checkSuccess $?

echo "Load the signing key under the primary key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p -on noncetpm.bin > run.out
checkSuccess $?

echo "Sign a digest - policy, should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkFailure $?

echo "Policy Secret with PWAP session, create a ticket"
${PREFIX}policysecret -ha 4000000c -hs 03000000 -pwde ppp -in noncetpm.bin -exp -200 -tk tkt.bin -to to.bin > run.out
checkSuccess $?

echo "Sign a digest - policy secret"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p -on noncetpm.bin > run.out
checkSuccess $?

echo "Policy Secret using primary key, create a ticket"
${PREFIX}policysecret -ha 4000000c -hs 03000000 -pwde ppp -in noncetpm.bin -exp -200 -tk tkt.bin -to to.bin > run.out
checkSuccess $?

echo "Sign a digest - policy secret"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p > run.out
checkSuccess $?

echo "Policy ticket"
${PREFIX}policyticket -ha 03000000 -to to.bin -hi p -tk tkt.bin > run.out
checkSuccess $?

echo "Sign a digest - policy ticket"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p -on noncetpm.bin > run.out
checkSuccess $?

echo "Start an HMAC session"
${PREFIX}startauthsession -se h > run.out
checkSuccess $?

echo "Policy Secret with HMAC session"
${PREFIX}policysecret -ha 4000000c -hs 03000000 -pwde ppp -se0 02000001 0 > run.out
checkSuccess $?

echo "Sign a digest - policy secret"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkSuccess $?

echo "Change platform hierarchy auth back to null"
${PREFIX}hierarchychangeauth -hi p -pwda ppp > run.out
checkSuccess $?

echo "Flush the signing key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

echo ""
echo "Policy Secret with NV Auth"
echo ""

# Name is 
# 00 0b e0 65 10 81 c2 fc da 30 69 93 da 43 d1 de 
# 5b 24 be 42 6e 2d 61 90 7b 42 83 54 69 13 6c 97 
# 68 1f 

# Policy is
# c6 93 f9 b0 ef 1a b7 1e ca ae 00 af 1f 0b f4 88 
# 37 9e ab 16 c1 f8 0d 9f f9 6d 90 41 4e 2f c6 b3 

echo "NV Define Space 0100000"
${PREFIX}nvdefinespace -hi p -ha 01000000 -pwdn nnn -sz 16 -pwdn nnn > run.out
checkSuccess $?

echo "Create a signing key under the primary key - policy secret NV auth"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policysecretnv.bin > run.out
checkSuccess $?

echo "Load the signing key under the primary key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p -on noncetpm.bin > run.out
checkSuccess $?

echo "Sign a digest - policy, should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkFailure $?

echo "Policy Secret with PWAP session"
${PREFIX}policysecret -ha 01000000 -hs 03000000 -pwde nnn -in noncetpm.bin > run.out
checkSuccess $?

echo "Sign a digest - policy secret"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkSuccess $?

echo "Flush the signing key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

echo "NV Undefine Space 0100000"
${PREFIX}nvundefinespace -hi p -ha 01000000 > run.out
checkSuccess $?


echo ""
echo "Policy Secret with Object"
echo ""

# Use a externally generated object so that the Name is known and thus
# the policy can be precalculated

# Name
# 00 0b 64 ac 92 1a 03 5c 72 b3 aa 55 ba 7d b8 b5 
# 99 f1 72 6f 52 ec 2f 68 20 42 fc 0e 0d 29 fa e8 
# 17 99 

# 000001151 plus the above name as text, add a blank line for empty policyRef
# to create policies/policysecretsha256.txt
# 00000151000b64ac921a035c72b3aa55ba7db8b599f1726f52ec2f682042fc0e0d29fae81799

# 4b 7f ca c2 b7 c3 ac a2 7c 5c da 9c 71 e6 75 28 
# 63 d2 87 d2 33 ec 49 0e 7a be 88 f1 ef 94 5d 5c 

echo "Load the RSA openssl key pair in the NULL hierarchy 80000001"
${PREFIX}loadexternal -rsa -ider policies/rsaprivkey.der -pwdk rrrr > run.out
checkSuccess $?

echo "Create a signing key under the primary key - policy secret of object 80000001"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -uwa -pol policies/policysecretsha256.bin > run.out
checkSuccess $?

echo "Load the signing key under the primary key 80000002"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Sign a digest - password auth - should fail"
${PREFIX}sign -hk 80000002 -if policies/aaa -pwdk sig > run.out
checkFailure $?

echo "Start a policy session 03000000"
${PREFIX}startauthsession -se p > run.out
checkSuccess $?

echo "Policy Secret with PWAP session"
${PREFIX}policysecret -ha 80000001 -hs 03000000 -pwde rrrr > run.out
checkSuccess $?

echo "Sign a digest - policy secret"
${PREFIX}sign -hk 80000002 -if msg.bin -se0 03000000 1 > run.out
checkSuccess $?

echo "Flush the policysecret key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

echo "Load the RSA openssl key pair in the NULL hierarchy, userWithAuth false 80000001"
${PREFIX}loadexternal -rsa -ider policies/rsaprivkey.der -pwdk rrrr -uwa > run.out
checkSuccess $?

echo "Policy Secret with PWAP session - should fail"
${PREFIX}policysecret -ha 80000001 -hs 03000000 -pwde rrrr > run.out
checkFailure $?

echo "Flush the policysecret key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

echo "Flush the signing key"
${PREFIX}flushcontext -ha 80000002 > run.out
checkSuccess $?

echo "Flush the session"
${PREFIX}flushcontext -ha 03000000 > run.out
checkSuccess $?

echo ""
echo "Policy Authorize"
echo ""

# 80000000 primary
# 80000001 verification public key, openssl
# 80000002 signing key
# 03000000 policy session

# Name for 80000001 0004 4234 c24f c1b9 de66 93a6 2453 417d 2734 d753 8f6f
#
# policyauthorize.txt
# 0000016a00044234c24fc1b9de6693a62453417d2734d7538f6f
#
# (need blank line for policyRef)
#
# > policymaker -if policies/policyauthorize.txt -of policies/policyauthorize.bin -pr
#
# 46 d4 8c 7e 17 0a 71 ca 9e 1f c7 e1 77 e5 7b 53 
# 75 df c4 3a 44 c9 65 4b 18 97 ce b1 92 e0 21 50 

echo "Create a signing key with policy authorize"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policyauthorize.bin > run.out
checkSuccess $?

echo "Load external just the public part of PEM authorizing key"
${PREFIX}loadexternal -hi p -halg sha1 -nalg sha1 -ipem policies/rsapubkey.pem > run.out
checkSuccess $?

echo "Load the signing key under the primary key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p > run.out
checkSuccess $?

echo "Get policy digest, should be zero"
${PREFIX}policygetdigest -ha 03000000 -of policyapproved.bin > run.out
checkSuccess $?

echo "Policy command code - sign"
${PREFIX}policycommandcode -ha 03000000 -cc 15d > run.out
checkSuccess $?

echo "Get policy digest, should be policy to approve, aHash input"
${PREFIX}policygetdigest -ha 03000000 -of policyapproved.bin > run.out
checkSuccess $?

echo "Openssl generate and sign aHash (empty policyRef)"
openssl dgst -sha1 -sign policies/rsaprivkey.pem -passin pass:rrrr -out pssig.bin policyapproved.bin

echo "Verify the signature to generate ticket"
${PREFIX}verifysignature -hk 80000001 -halg sha1 -if policyapproved.bin -is pssig.bin -raw -tk tkt.bin > run.out
checkSuccess $?

echo "Policy authorize using the ticket"
${PREFIX}policyauthorize -ha 03000000 -appr policyapproved.bin -skn ${TPM_DATA_DIR}/h80000001.bin -tk tkt.bin > run.out
checkSuccess $?

echo "Get policy digest, should be policy authorize"
${PREFIX}policygetdigest -ha 03000000 -of policyapproved.bin > run.out
checkSuccess $?

echo "Sign a digest"
${PREFIX}sign -hk 80000002 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkSuccess $?

echo "Flush the verification public key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

echo "Flush the signing key"
${PREFIX}flushcontext -ha 80000002 > run.out
checkSuccess $?

# getcapability  -cap 1 -pr 80000000
# getcapability  -cap 1 -pr 02000000
# getcapability  -cap 1 -pr 03000000

# exit 0

echo ""
echo "Set Primary Policy"
echo ""

echo "Platform policy empty"
${PREFIX}setprimarypolicy -hi p > run.out
checkSuccess $?

echo "Platform policy empty, bad password"
${PREFIX}setprimarypolicy -hi p -pwda ppp > run.out
checkFailure $?

echo "Set platform hierarchy auth"
${PREFIX}hierarchychangeauth -hi p -pwdn ppp > run.out
checkSuccess $?

echo "Platform policy empty, bad password"
${PREFIX}setprimarypolicy -hi p > run.out
checkFailure $?

echo "Platform policy empty"
${PREFIX}setprimarypolicy -hi p -pwda ppp > run.out
checkSuccess $?

echo "Platform policy to policy secret platform auth"
${PREFIX}setprimarypolicy -hi p -pwda ppp -halg sha256 -pol policies/policysecretp.bin > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p > run.out
checkSuccess $?

echo "Policy Secret with PWAP session"
${PREFIX}policysecret -ha 4000000c -hs 03000000 -pwde ppp > run.out
checkSuccess $?

echo "Change platform hierarchy auth to null with policy secret"
${PREFIX}hierarchychangeauth -hi p -se0 03000000 0 > run.out
checkSuccess $?

echo ""
echo "Policy PCR no select"
echo ""

# create AND term for policy PCR
# > policymakerpcr -halg sha1 -bm 0 -v -pr -of policies/policypcr.txt
# 0000017f00000001000403000000da39a3ee5e6b4b0d3255bfef95601890afd80709

# convert to binary policy
# > policymaker -halg sha1 -if policies/policypcr.txt -of policies/policypcrbm0.bin -pr -v

# 6d 38 49 38 e1 d5 8b 56 71 92 55 94 3f 06 69 66 
# b6 fa 2c 23 

echo "Create a signing key with policy PCR no select"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -nalg sha1 -pol policies/policypcrbm0.bin > run.out
checkSuccess $?

echo "Load the signing key under the primary key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -halg sha1 -se p > run.out
checkSuccess $?

echo "Policy PCR, update with the correct digest"
${PREFIX}policypcr -ha 03000000 -halg sha1 -bm 0 > run.out
checkSuccess $?

echo "Policy get digest - should be 6d 38 49 38 ... "
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Sign, should succeed"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkSuccess $?

echo "Policy restart, set back to zero"
${PREFIX}policyrestart -ha 03000000 > run.out 
checkSuccess $?

echo "Policy PCR, update with the correct digest"
${PREFIX}policypcr -ha 03000000 -halg sha1 -bm 0 > run.out
checkSuccess $?

echo "PCR extend PCR 0, updates pcr counter"
${PREFIX}pcrextend -ha 0 -halg sha1 -ic policies/aaa > run.out
checkSuccess $?

echo "Sign, should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Flush the policy session"
${PREFIX}flushcontext -ha 03000000 > run.out
checkSuccess $?

echo "Flush the key"
${PREFIX}flushcontext -ha 80000001 > run.out 
checkSuccess $?

echo ""
echo "Policy PCR 16"
echo ""

# policypcr0.txt has 20 * 00

# create AND term for policy PCR
# > policymakerpcr -halg sha1 -bm 010000 -if policies/policypcr0.txt -v -pr -of policies/policypcr.txt
# 0000017f000000010004030000016768033e216468247bd031a0a2d9876d79818f8f

# convert to binary policy
# > policymaker -halg sha1 -if policypcr.txt -of policypcr.bin -pr -v

# 85 33 11 83 19 03 12 f5 e8 3c 60 43 34 6f 9f 37
# 21 04 76 8e

echo "Create a signing key with policy PCR PCR 16 zero"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -nalg sha1 -pol policies/policypcr.bin > run.out
checkSuccess $?

echo "Load the signing key under the primary key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Reset PCR 16 back to zero"
${PREFIX}pcrreset -ha 16 > run.out
checkSuccess $?

echo "Read PCR 16, should be 00 00 00 00 ..."
${PREFIX}pcrread -ha 16 -halg sha1 > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?

echo "Sign, policy not satisfied - should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkFailure $?

echo "Policy PCR, update with the correct digest"
${PREFIX}policypcr -ha 03000000 -halg sha1 -bm 10000 > run.out
checkSuccess $?

echo "Policy get digest - should be 85 33 11 83 ..."
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Sign, should succeed"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkSuccess $?

echo "PCR extend PCR 16"
${PREFIX}pcrextend -ha 16 -halg sha1 -ic policies/aaa > run.out
checkSuccess $?

echo "Read PCR 0, should be 1d 47 f6 8a ..."
${PREFIX}pcrread -ha 16 -halg sha1 > run.out
checkSuccess $?

echo "Start a policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?

echo "Policy PCR, update with the wrong digest"
${PREFIX}policypcr -ha 03000000 -halg sha1 -bm 10000 > run.out
checkSuccess $?

echo "Policy get digest - should be 66 dd e5 e3"
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Sign - should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 0 > run.out
checkFailure $?

echo "Flush the policy session"
${PREFIX}flushcontext -ha 03000000 > run.out
checkSuccess $?

echo "Flush the key"
${PREFIX}flushcontext -ha 80000001 > run.out 
checkSuccess $?

# 01000000 authorizing ndex
# 01000001 authorized index
# 03000000 policy session
#
# 4 byte NV index
# policynv.txt
# policy CC_PolicyNV || args || Name
#
# policynvargs.txt (binary)
# args = hash of 0000 0000 0000 0000 | 0000 | 0000 (eight bytes of zero | offset | op ==)
# hash -hi n -halg sha1 -if policynvargs.txt -v
# openssl dgst -sha1 policynvargs.txt
# 2c513f149e737ec4063fc1d37aee9beabc4b4bbf
#
# NV authorizing index
#
# after defining index and NV write to set written, use 
# ${PREFIX}nvreadpublic -ha 01000000 -nalg sha1
# to get name
# 00042234b8df7cdf8605ee0a2088ac7dfe34c6566c5c
#
# append Name to policynvnv.txt
#
# convert to binary policy
# > policymaker -halg sha1 -if policynvnv.txt -of policynvnv.bin -pr -v
# bc 9b 4c 4f 7b 00 66 19 5b 1d d9 9c 92 7e ad 57 e7 1c 2a fc 
#
# file zero8.bin has 8 bytes of hex zero

echo ""
echo "Policy NV, NV index authorizing"
echo ""

echo "Define a setbits index, authorizing index"
${PREFIX}nvdefinespace -hi p -nalg sha1 -ha 01000000 -pwdn nnn -ty b > run.out
checkSuccess $?

echo "NV Read public, get Name, not written"
${PREFIX}nvreadpublic -ha 01000000 -nalg sha1 > run.out
checkSuccess $?

echo "NV setbits to set written"
${PREFIX}nvsetbits -ha 01000000 -pwdn nnn > run.out
checkSuccess $?

echo "NV Read public, get Name, written"
${PREFIX}nvreadpublic -ha 01000000 -nalg sha1 > run.out
checkSuccess $?

echo "NV Read, should be zero"
${PREFIX}nvread -ha 01000000 -pwdn nnn -sz 8 > run.out
checkSuccess $?

echo "Define an ordinary index, authorized index, policyNV"
${PREFIX}nvdefinespace -hi p -nalg sha1 -ha 01000001 -pwdn nnn -sz 2 -ty o -pol policies/policynvnv.bin > run.out
checkSuccess $?

echo "NV Read public, get Name, not written"
${PREFIX}nvreadpublic -ha 01000001 -nalg sha1 > run.out
checkSuccess $?

echo "NV write to set written"
${PREFIX}nvwrite -ha 01000001 -pwdn nnn -ic aa > run.out
checkSuccess $?

echo "Start policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?
 
echo "NV write, policy not satisfied  - should fail"
${PREFIX}nvwrite -ha 01000001 -ic aa -se0 03000000 1 > run.out
checkFailure $?

echo "Policy get digest, should be 0"
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Policy NV to satisfy the policy"
${PREFIX}policynv -ha 01000000 -pwda nnn -hs 03000000 -if policies/zero8.bin -op 0 > run.out
checkSuccess $?

echo "Policy get digest, should be bc 9b 4c 4f ..."
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "NV write, policy satisfied"
${PREFIX}nvwrite -ha 01000001 -ic aa -se0 03000000 1 > run.out
checkSuccess $?

echo "Set bit in authorizing NV index"
${PREFIX}nvsetbits -ha 01000000 -pwdn nnn -bit 0 > run.out
checkSuccess $?

echo "NV Read, should be 1"
${PREFIX}nvread -ha 01000000 -pwdn nnn -sz 8 > run.out
checkSuccess $?

echo "Policy NV to satisfy the policy - should fail"
${PREFIX}policynv -ha 01000000 -pwda nnn -hs 03000000 -if policies/zero8.bin -op 0 > run.out
checkFailure $?

echo "Policy get digest, should be 00 00 00 00 ..."
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "NV Undefine authorizing index"
${PREFIX}nvundefinespace -hi p -ha 01000000 > run.out
checkSuccess $?

echo "NV Undefine authorized index"
${PREFIX}nvundefinespace -hi p -ha 01000001 > run.out 
checkSuccess $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out  
checkSuccess $?

echo ""
echo "Policy NV Written"
echo ""

echo "Define an ordinary index, authorized index, policyNV"
${PREFIX}nvdefinespace -hi p -nalg sha1 -ha 01000000 -pwdn nnn -sz 2 -ty o -pol policies/policywrittenset.bin > run.out  
checkSuccess $?

echo "NV Read public, get Name, not written"
${PREFIX}nvreadpublic -ha 01000000 -nalg sha1 > run.out  
checkSuccess $?

echo "Start policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?
 
echo "NV write, policy not satisfied  - should fail"
${PREFIX}nvwrite -ha 01000000 -ic aa -se0 03000000 1 > run.out  
checkFailure $?

echo "Policy NV Written no, does not satisfy policy"
${PREFIX}policynvwritten -hs 03000000 -ws n > run.out  
checkSuccess $?

echo "NV write, policy not satisfied - should fail"
${PREFIX}nvwrite -ha 01000000 -ic aa -se0 03000000 1 > run.out  
checkFailure $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out  
checkSuccess $?

echo "Start policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?

echo "Policy NV Written yes, satisfy policy"
${PREFIX}policynvwritten -hs 03000000 -ws y > run.out
checkSuccess $?

echo "NV write, policy satisfied but written clear - should fail"
${PREFIX}nvwrite -ha 01000000 -ic aa -se0 03000000 1 > run.out
checkFailure $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out  
checkSuccess $?

echo "NV write using password, set written"
${PREFIX}nvwrite -ha 01000000 -ic aa -pwdn nnn > run.out
checkSuccess $?

echo "Start policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?

echo "Policy NV Written yes, satisfy policy"
${PREFIX}policynvwritten -hs 03000000 -ws y > run.out
checkSuccess $?

echo "NV write, policy satisfied"
${PREFIX}nvwrite -ha 01000000 -ic aa -se0 03000000 1 > run.out
checkSuccess $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out  
checkSuccess $?

echo "Start policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?

echo "Policy NV Written no"
${PREFIX}policynvwritten -hs 03000000 -ws n > run.out
checkSuccess $?

echo "Policy NV Written yes - should fail"
${PREFIX}policynvwritten -hs 03000000 -ws y > run.out
checkFailure $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out  
checkSuccess $?

echo "NV Undefine authorizing index"
${PREFIX}nvundefinespace -hi p -ha 01000000 > run.out
checkSuccess $?

# test using clockrateadjust
# policycphashhash.txt is (hex) 00000130 4000000c 000
# hash -if policycphashhash.txt -oh policycphashhash.bin -halg sha1 -v
# openssl dgst -sha1 policycphashhash.txt
# cpHash is
# b5f919bbc01f0ebad02010169a67a8c158ec12f3
# append to policycphash.txt 00000163 + cpHash
# policymaker -halg sha1 -if policycphash.txt -of policycphash.bin -pr
#  06 e4 6c f9 f3 c7 0f 30 10 18 7c a6 72 69 b0 84 b4 52 11 6f 

echo ""
echo "Policy cpHash"
echo ""

echo "Set the platform policy to policy cpHash"
${PREFIX}setprimarypolicy -hi p -pol policies/policycphash.bin -halg sha1 > run.out
checkSuccess $?

echo "Clockrate adjust using wrong password - should fail"
${PREFIX}clockrateadjust -hi p -pwdp ppp -adj 0 > run.out 
checkFailure $?

echo "Start policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out 
checkSuccess $?

echo "Clockrate adjust, policy not satisfied - should fail"
${PREFIX}clockrateadjust -hi p -pwdp ppp -adj 0 -se0 03000000 1 > run.out
checkFailure $?

echo "Policy cpHash, satisfy policy"
${PREFIX}policycphash -ha 03000000 -cp policies/policycphashhash.bin > run.out
checkSuccess $?
 
echo "Policy get digest, should be 06 e4 6c f9"
${PREFIX}policygetdigest -ha 03000000 > run.out 
checkSuccess $?

echo "Clockrate adjust, policy satisfied but bad command params - should fail"
${PREFIX}clockrateadjust -hi p -pwdp ppp -adj 1 -se0 03000000 1 > run.out 
checkFailure $?

echo "Clockrate adjust, policy satisfied"
${PREFIX}clockrateadjust -hi p -pwdp ppp -adj 0 -se0 03000000 1 > run.out 
checkSuccess $?

echo "Clear the platform policy"
${PREFIX}setprimarypolicy -hi p > run.out 
checkSuccess $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out 
checkSuccess $?

# test using clockrateadjust and platform policy

# operand A time is 64 bits at offset 0, operation GT (2)
# policycountertimerargs.txt (binary)
# args = hash of operandB | offset | operation
# 0000 0000 0000 0000 | 0000 | 0002
# hash -hi n -halg sha1 -if policycountertimerargs.txt -v
# openssl dgst -sha1 policycountertimerargs.txt
# 7a5836fe287e11ac39ee88d3c0794916d50b73c3
# 
# policycountertimer.txt 
# policy CC_PolicyCounterTimer || args
# 0000016d + args
# convert to binary policy
# > policymaker -halg sha1 -if policycountertimer.txt -of policycountertimer.bin -pr -v
# e6 84 81 27 55 c0 39 d3 68 63 21 c8 93 50 25 dd aa 26 42 9a 

echo ""
echo "Policy Counter Timer"
echo ""

echo "Set the platform policy to policy "
${PREFIX}setprimarypolicy -hi p -pol policies/policycountertimer.bin -halg sha1 > run.out
checkSuccess $?

echo "Clockrate adjust using wrong password - should fail"
${PREFIX}clockrateadjust -hi p -pwdp ppp -adj 0 > run.out
checkFailure $?

echo "Start policy session"
${PREFIX}startauthsession -se p -halg sha1 > run.out
checkSuccess $?

echo "Clockrate adjust, policy not satisfied - should fail"
${PREFIX}clockrateadjust -hi p -adj 0 -se0 03000000 1 > run.out
checkFailure $?

echo "Policy counter timer, zero operandB, op EQ satisfy policy - should fail"
${PREFIX}policycountertimer -ha 03000000 -if policies/zero8.bin -op 0 > run.out
checkFailure $?
 
echo "Policy counter timer, zero operandB, op GT satisfy policy"
${PREFIX}policycountertimer -ha 03000000 -if policies/zero8.bin -op 2 > run.out 
checkSuccess $?
 
echo "Policy get digest, should be e6 84 81 27"
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Clockrate adjust, policy satisfied"
${PREFIX}clockrateadjust -hi p -adj 0 -se0 03000000 1 > run.out 
checkSuccess $?

echo "Clear the platform policy"
${PREFIX}setprimarypolicy -hi p > run.out 
checkSuccess $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out 
checkSuccess $?


# policyccsign.txt  0000016c 0000015d (policy command code | sign)
# policyccquote.txt 0000016c 00000158 (policy command code | quote)
#
# > policymaker -if policyccsign.txt -of policyccsign.bin -pr -v
# cc6918b226273b08f5bd406d7f10cf160f0a7d13dfd83b7770ccbcd1aa80d811
#
# > policymaker -if policyccquote.txt -of policyccquote.bin -pr -v
# a039cad5fe68870688f8233c3e3ee3cf27aac9e2efe3486aeb4e304c0e90cd27
#
# policyor.txt is CC_PolicyOR || digests
# 00000171 | cc69 ... | a039 ...
# > policymaker -if policyor.txt -of policyor.bin -pr -v
# 6b fe c2 3a be 57 b0 2a ce 39 dd 13 bb 60 fa 39 
# 4d ac 7b 38 96 56 57 84 b3 73 fc 61 92 94 29 db 

echo ""
echo "PolicyOR"
echo ""

echo "Create an unrestricted signing key, policy command code sign or quote"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr tmppriv.bin -opu tmppub.bin -pwdp pps -pwdk sig -pol policies/policyor.bin > run.out
checkSuccess $?

echo "Load the signing key"
${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu tmppub.bin -pwdp pps > run.out
checkSuccess $?

echo "Start policy session"
${PREFIX}startauthsession -se p > run.out
checkSuccess $?

echo "Policy get digest"
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Sign a digest - should fail"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkFailure $?

echo "Quote - should fail"
${PREFIX}quote -hp 0 -hk 80000001 -se0 03000000 1 > run.out
checkFailure $?

echo "Get time - should fail, policy not set"
${PREFIX}gettime -hk 80000001 -qd policies/aaa -se1 03000000 1 > run.out
checkFailure $?

echo "Policy OR - should fail"
${PREFIX}policyor -ha 03000000 -if policies/policyccsign.bin -if policies/policyccquote.bin > run.out
checkFailure $?

echo "Policy Command code - sign"
${PREFIX}policycommandcode -ha 03000000 -cc 0000015d > run.out
checkSuccess $?

echo "Policy get digest, should be cc 69 18 b2"
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Policy OR"
${PREFIX}policyor -ha 03000000 -if policies/policyccsign.bin -if policies/policyccquote.bin > run.out
checkSuccess $?

echo "Policy get digest, should be 6b fe c2 3a"
${PREFIX}policygetdigest -ha 03000000 > run.out
checkSuccess $?

echo "Sign with policy OR"
${PREFIX}sign -hk 80000001 -if msg.bin -os sig.bin -se0 03000000 1 > run.out
checkSuccess $?

echo "Policy Command code - sign"
${PREFIX}policycommandcode -ha 03000000 -cc 0000015d > run.out
checkSuccess $?

echo "Policy OR"
${PREFIX}policyor -ha 03000000 -if policies/policyccsign.bin -if policies/policyccquote.bin > run.out
checkSuccess $?

echo "Quote - should fail, wrong command code"
${PREFIX}quote -hp 0 -hk 80000001 -se0 03000000 1 > run.out
checkFailure $?

echo "Policy restart, set back to zero"
${PREFIX}policyrestart -ha 03000000 > run.out 
checkSuccess $?

echo "Policy Command code - quote, digest a0 39 ca d5"
${PREFIX}policycommandcode -ha 03000000 -cc 00000158 > run.out
checkSuccess $?

echo "Policy OR, digest 6b fe c2 3a"
${PREFIX}policyor -ha 03000000 -if policies/policyccsign.bin -if policies/policyccquote.bin > run.out
checkSuccess $?

echo "Quote with policy OR"
${PREFIX}quote -hp 0 -hk 80000001 -se0 03000000 1 > run.out
checkSuccess $?

echo "Policy Command code - gettime 7a 3e bd aa"
${PREFIX}policycommandcode -ha 03000000 -cc 0000014c > run.out
checkSuccess $?

echo "Policy OR, gettime not an AND term - should fail"
${PREFIX}policyor -ha 03000000 -if policies/policyccsign.bin -if policies/policyccquote.bin > run.out
checkFailure $?

echo "Flush policy session"
${PREFIX}flushcontext -ha 03000000 > run.out
checkSuccess $?

echo "Flush signing key"
${PREFIX}flushcontext -ha 80000001 > run.out
checkSuccess $?

rm -f tmppol.bin
rm -r tmppriv.bin
rm -r tmppub.bin

# ${PREFIX}getcapability -cap 1 -pr 80000000
# ${PREFIX}getcapability -cap 1 -pr 01000000
# ${PREFIX}getcapability -cap 1 -pr 02000000
# ${PREFIX}getcapability -cap 1 -pr 03000000
