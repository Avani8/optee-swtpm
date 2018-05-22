################################################################################
# Android FIM makefile                                                         #
################################################################################
LOCAL_PATH := $(call my-dir)

CFG_TEEC_PUBLIC_INCLUDE = $(LOCAL_PATH)/../optee_client/public
CFG_OPENSSL_PUBLIC_INCLUDE = $(LOCAL_PATH)/includes

################################################################################
# Build TEST functions                                                                    #
################################################################################
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += -Wall

LOCAL_SRC_FILES += host/main.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ta \
		$(CFG_OPENSSL_PUBLIC_INCLUDE) \
		$(CFG_TEEC_PUBLIC_INCLUDE) \

LOCAL_SHARED_LIBRARIES := libteec
LOCAL_MODULE := tee_ossec
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/ta/Android.mk
