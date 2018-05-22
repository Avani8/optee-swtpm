CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

OBJS = main.o test.o crypto.o

# CFLAGS += -Wall -I../ta -I$(TEEC_EXPORT)/include
CFLAGS += -Wall -I../ta -I$(TEEC_EXPORT)/include -I/usr/include

# copied opensslconf.h from /usr/include/x86_64-linux-gnu/openssl => /usr/install/openssl

LDADD += -lteec -L$(TEEC_EXPORT)/lib
LDFLAGS += ../../../optee_test/host/lib/armv7/libcrypto.a
# Add/link other required libraries here
# LDADD += -lteec -L$(TEEC_EXPORT)/lib -lcrypto -lssl

BINARY=tpm_rsa

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(LDADD) -o $@ $< test.o crypto.o $(LDFLAGS) -ldl

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY) 
