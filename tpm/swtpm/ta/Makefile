CFG_TEE_TA_LOG_LEVEL ?= 4
CPPFLAGS += -DCFG_TEE_TA_LOG_LEVEL=$(CFG_TEE_TA_LOG_LEVEL)

# The UUID for the Trusted Application
BINARY=9269fadd-99d5-4afb-a1dc-ee3e9c61b04c

include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk



CRYPTO_SUBSYSTEM = openssl

include makefile-common

OBJFILES += TcpServerPosix.o

TcpServerPosix.o	: $(HEADERS)

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
