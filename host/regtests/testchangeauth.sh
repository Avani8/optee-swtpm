#!/bin/bash
#

#################################################################################
#										#
#			TPM2 regression test					#
#			     Written by Ken Goldman				#
#		       IBM Thomas J. Watson Research Center			#
#	$Id: testchangeauth.sh 979 2017-04-04 17:57:18Z kgoldman $ 		#
#										#
# (c) Copyright IBM Corporation 2015, 2016					#
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

echo ""
echo "Object Change Auth"
echo ""

for BIND in "" "-bi 80000001 -pwdb sig"
do

    for SESS in "" "-se0 02000000 1"
    do

	echo "Load the signing key under the primary key"
	${PREFIX}load -hp 80000000 -ipr signpriv.bin -ipu signpub.bin -pwdp pps > run.out
	checkSuccess $?

	echo "Start an HMAC session ${BIND}"
	${PREFIX}startauthsession -se h ${BIND} > run.out
	checkSuccess $?

	echo "Object change auth, change password to xxx ${SESS}"
	${PREFIX}objectchangeauth -ho 80000001 -pwdo sig -pwdn xxx -hp 80000000 -opr tmppriv.bin ${SESS} > run.out
	checkSuccess $?

	echo "Load the signing key with the changed auth ${SESS}"
	${PREFIX}load -hp 80000000 -ipr tmppriv.bin -ipu signpub.bin -pwdp pps ${SESS} > run.out
	checkSuccess $?

	echo "Sign a digest with the original key ${SESS}"
	${PREFIX}sign -hk 80000001 -halg sha1 -if policies/aaa -os sig.bin -pwdk sig ${SESS} > run.out
	checkSuccess $?

	echo "Sign a digest with the changed key"
	${PREFIX}sign -hk 80000002 -halg sha1 -if policies/aaa -os sig.bin -pwdk xxx > run.out
	checkSuccess $?

	echo "Flush the key"
	${PREFIX}flushcontext -ha 80000001 > run.out
	checkSuccess $?

	echo "Flush the key"
	${PREFIX}flushcontext -ha 80000002 > run.out
	checkSuccess $?

	echo "Flush the auth session"
	${PREFIX}flushcontext -ha 02000000 > run.out
	checkSuccess $?

    done
done

# ${PREFIX}getcapability  -cap 1 -pr 80000000
# ${PREFIX}getcapability  -cap 1 -pr 02000000

# ${PREFIX}flushcontext -ha 80000001
# ${PREFIX}flushcontext -ha 80000002
# ${PREFIX}flushcontext -ha 02000000
