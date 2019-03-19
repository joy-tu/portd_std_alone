DIRS += $(PORTD_LIBS)

##################################
# Output Mode
##################################
CFLAGS += -pthread -lcrypt
MODE=exe

##################################
# Include library
##################################
# include share library
INC_SHARELIBS = libpwd.so libconfig.so libsio.so libsystem.so libssl.so libcrypto.so libupgradeFirmware.so
# include static library
#INC_STATICLIBS = rfc2217/rfc2217.a aspp/aspp.a raw_tcp/raw_tcp.a udp/udp.a
#INC_STATICLIBS += pair/pair.a


TEMP_LIBS = $(patsubst %, %.a, $(DIRS))
TEMP = $(addsuffix /, $(DIRS))
INC_STATICLIBS = $(join $(TEMP), $(TEMP_LIBS))
include $(TOP)/make.dep

