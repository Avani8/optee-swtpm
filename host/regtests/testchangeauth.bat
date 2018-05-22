REM #############################################################################
REM #										#
REM #			TPM2 regression test					#
REM #			     Written by Ken Goldman				#
REM #		       IBM Thomas J. Watson Research Center			#
REM #		$Id: testchangeauth.bat 480 2015-12-29 22:41:45Z kgoldman $		#
REM #										#
REM # (c) Copyright IBM Corporation 2015					#
REM # 										#
REM # All rights reserved.							#
REM # 										#
REM # Redistribution and use in source and binary forms, with or without	#
REM # modification, are permitted provided that the following conditions are	#
REM # met:									#
REM # 										#
REM # Redistributions of source code must retain the above copyright notice,	#
REM # this list of conditions and the following disclaimer.			#
REM # 										#
REM # Redistributions in binary form must reproduce the above copyright		#
REM # notice, this list of conditions and the following disclaimer in the	#
REM # documentation and/or other materials provided with the distribution.	#
REM # 										#
REM # Neither the names of the IBM Corporation nor the names of its		#
REM # contributors may be used to endorse or promote products derived from	#
REM # this software without specific prior written permission.			#
REM # 										#
REM # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS	#
REM # "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		#
REM # LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	#
REM # A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT	#
REM # HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	#
REM # SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		#
REM # LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	#
REM # DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	#
REM # THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT	#
REM # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	#
REM # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.	#
REM #										#
REM #############################################################################

setlocal enableDelayedExpansion

echo ""
echo "Object Change Auth"
echo ""

for %%B in ("" "-bi 80000001 -pwdb sig") do (

    for %%S in ("" "-se0 02000000 1") do (

	echo "Load the signing key under the primary key"
	%TPM_EXE_PATH%load -hp 80000000 -ipr signpriv.bin -ipu signpub.bin -pwdp pps > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Start an HMAC session %%~B"
	%TPM_EXE_PATH%startauthsession -se h %%~B > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Object change auth, change password to xxx %%~S"
	%TPM_EXE_PATH%objectchangeauth -ho 80000001 -pwdo sig -pwdn xxx -hp 80000000 -opr tmppriv.bin %%~S > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Load the signing key with the changed auth %%~S"
	%TPM_EXE_PATH%load -hp 80000000 -ipr tmppriv.bin -ipu signpub.bin -pwdp pps %%~S > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Sign a digest with the original key %%~S"
	%TPM_EXE_PATH%sign -hk 80000001 -halg sha1 -if policies/aaa -os sig.bin -pwdk sig %%~S > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Sign a digest with the changed key"
	%TPM_EXE_PATH%sign -hk 80000002 -halg sha1 -if policies/aaa -os sig.bin -pwdk xxx > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Flush the key"
	%TPM_EXE_PATH%flushcontext -ha 80000001 > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Flush the key"
	%TPM_EXE_PATH%flushcontext -ha 80000002 > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	echo "Flush the auth session"
	%TPM_EXE_PATH%flushcontext -ha 02000000 > run.out
	IF !ERRORLEVEL! NEQ 0 (
	   exit /B 1
	   )

	)
)

exit /B 0

REM getcapability  -cap 1 -pr 80000000
REM getcapability  -cap 1 -pr 02000000

REM flushcontext -ha 80000001
REM flushcontext -ha 80000002
REM flushcontext -ha 02000000
