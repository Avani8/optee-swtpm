#!/bin/bash
#

#################################################################################
#										#
#			TPM2 regression test					#
#			     Written by Ken Goldman				#
#		       IBM Thomas J. Watson Research Center			#
#		$Id: initkeys.sh 1047 2017-07-20 18:27:34Z kgoldman $		#
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

echo -n "1234567890123456" > msg.bin
touch zero.bin

# try to undefine any NV index left over from a previous test.  Do not check for errors.
${PREFIX}nvundefinespace -hi p -ha 01000000 > run.out
${PREFIX}nvundefinespace -hi p -ha 01000000 -pwdp ppp > run.out
${PREFIX}nvundefinespace -hi p -ha 01000001 > run.out
${PREFIX}nvundefinespace -hi o -ha 01000002 > run.out
# same for persistent objects
${PREFIX}evictcontrol -ho 81800000 -hp 81800000 -hi p > run.out

echo ""
echo "Initialize Regression Test Keys"
echo ""

echo "Create a platform primary storage key"
${PREFIX}createprimary -hi p -pwdk pps -tk pritk.bin -ch prich.bin > run.out
checkSuccess $?

echo "Create an RSA storage key under the primary key"
${PREFIX}create -hp 80000000 -st -kt f -kt p -opr storepriv.bin -opu storepub.bin -tk stotk.bin -ch stoch.bin -pwdp pps -pwdk sto > run.out
checkSuccess $?

echo "Create an ECC storage key under the primary key"
${PREFIX}create -hp 80000000 -ecc nistp256 -st -kt f -kt p -opr storeeccpriv.bin -opu storeeccpub.bin -pwdp pps -pwdk sto > run.out
checkSuccess $?

echo "Create an unrestricted RSA signing key under the primary key"
${PREFIX}create -hp 80000000 -si -kt f -kt p -opr signpriv.bin -opu signpub.bin -opem signpub.pem -pwdp pps -pwdk sig > run.out
checkSuccess $?

echo "Create an unrestricted ECC signing key under the primary key"
${PREFIX}create -hp 80000000 -ecc nistp256 -si -kt f -kt p -opr signeccpriv.bin -opu signeccpub.bin -opem signeccpub.pem -pwdp pps -pwdk sig > run.out
checkSuccess $?

echo "Create a restricted RSA signing key under the primary key"
${PREFIX}create -hp 80000000 -sir -kt f -kt p -opr signrpriv.bin -opu signrpub.bin -opem signrpub.pem -pwdp pps -pwdk sig > run.out
checkSuccess $?

echo "Create an RSA decryption key under the primary key"
${PREFIX}create -hp 80000000 -den -kt f -kt p -opr derpriv.bin -opu derpub.bin -pwdp pps -pwdk dec > run.out
checkSuccess $?

echo "Create a symmetric cipher key under the primary key"
${PREFIX}create -hp 80000000 -des -kt f -kt p -opr despriv.bin -opu despub.bin -pwdp pps -pwdk aes > run.out
RC=$?
checkWarning $RC "Symmetric cipher key may not support sign attribute"

if [ $RC -ne 0 ]; then
    echo "Create a rev 116 symmetric cipher key under the primary key"
    ${PREFIX}create -hp 80000000 -des -116 -kt f -kt p -opr despriv.bin -opu despub.bin -pwdp pps -pwdk aes > run.out
    checkSuccess $?
fi

for HALG in sha1 sha256 sha384

do

    echo "Create a ${HALG} keyed hash key under the primary key"
    ${PREFIX}create -hp 80000000 -kh -kt f -kt p -opr khpriv${HALG}.bin -opu khpub${HALG}.bin -pwdp pps -pwdk khk -halg ${HALG} > run.out
    checkSuccess $?

done

exit ${WARN}
