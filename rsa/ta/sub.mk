#global-incdirs-y += include
#global-incdirs-y += ../host/include
srcs-y += TA.c
srcs-y += crypto.c
srcs-y += test.c

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
