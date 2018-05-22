CFG_TEE_TA_LOG_LEVEL ?= 4
CPPFLAGS += -DCFG_TEE_TA_LOG_LEVEL=$(CFG_TEE_TA_LOG_LEVEL)

# The UUID for the Trusted Application
BINARY=9269fadd-99d5-4afb-a1dc-ee3e9c61b04c

include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

CCFLAGS = -Wall  			\
	-Wmissing-declarations -Wmissing-prototypes -Wnested-externs \
	-Werror -Wsign-compare \
	 -c -ggdb -O0 			\
	-DTPM_POSIX			\
	-D_POSIX_			\
	-DTPM_NUVOTON			\
	-I../utils			\
        -fdump-rtl-expand               \
	-I.

LNFLAGS = -ggdb 			\
	-DTPM_POSIX			\
	-DTPM_NUVOTON			\
	-lcrypto			\
	-lpthread			\
	-I.

all:	tpm_server

CRYPTO_SUBSYSTEM = openssl

include makefile-common

OBJFILES += TcpServerPosix.o

TcpServerPosix.o	: $(HEADERS)

.PHONY:		clean
.PRECIOUS:	%.o

tpm_server:	$(OBJFILES)
		$(CC) $(OBJFILES) $(LNFLAGS) -o tpm_server
		./tpm_server &
clean:		
		rm -f *.o tpm_server *~

%.o:		%.c
		$(CC) $(CCFLAGS) $< -o $@

