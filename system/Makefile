DIRS =

##################################
# Output Mode.
##################################
MODE=share


##################################
# Include library
##################################
# include share library
INC_SHARELIBS = libsio.so
# include static library
INC_STATICLIBS = 

EXTOBJS = version

version:
	$(CC) $(EXTFLAGS) -c $(CFLAGS) $(INCLUDE) version.c -o version.o || exit 1;


include $(TOP)/make.dep

