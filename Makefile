SHELL := /bin/bash # Use bash syntax

include ./make.conf
DIRS = portd

DIRS += $(SUBLIBS)

export PORTD_LIBS
export EXCLUDE_FILE

ifdef DC_USERDISK_PATH
INSTALL_PATH=$(DC_USERDISK_PATH)
CFLAGS += -DCROSS
CPU = ARM
else
INSTALL_PATH=${shell pwd}/target
CFLAGS += -g
CPU = ${shell uname -i}
endif

ifndef DC_BUILD_DATE
DC_BUILD_DATE=${shell date +%y%m%d%H}
endif

ifdef PROFILE
$(eval ${shell fgrep "MASTER_MAJOR=" $(DC_TOP)/profile/$(PROFILE)/BuildInfo.inc})
$(eval ${shell fgrep "MASTER_MINOR=" $(DC_TOP)/profile/$(PROFILE)/BuildInfo.inc})
$(eval ${shell fgrep "MASTER_CV_BETA=" $(DC_TOP)/profile/$(PROFILE)/BuildInfo.inc})
else
MASTER_MAJOR = 1
MASTER_MINOR = 0
MASTER_CV_BETA = 0
endif


CFLAGS += -DBUILD_DATE=$(DC_BUILD_DATE)
CFLAGS += -DVERSION_MAJOR=$(MASTER_MAJOR)
CFLAGS += -DVERSION_MINOR=$(MASTER_MINOR)
CFLAGS += -DVERSION_CV_BETA=$(MASTER_CV_BETA)
CFLAGS += $(DC_TARGET_CFLAGS)
ifdef DC_USERDISK_PATH
CFLAGS += -Werror -fno-strict-aliasing 
endif

CFLAGS += $(MAKE_SUPPORT_FLAG) -D$(TARGET_MODEL)

TOP = ${shell pwd}
LIBPATH=$(TOP)/target/lib

INCLUDE = -I$(TOP)/include
INCLUDE += -I$(TOP)/../wpa_supplicant/wpa_supplicant -I$(TOP)/../wpa_supplicant/src
ifdef DC_KERNEL_PATH
INCLUDE += -L$(TOP)/../openssl -I$(TOP)/../openssl/include
INCLUDE += -I$(DC_KERNEL_PATH)/include
else
INCLUDE += -I/usr/src/kernels/${shell uname -r}/include
endif


.PHONYL: all dep depend clean install check

ifndef DC_FW_MODEL
    DC_FW_MODEL=PC
endif
MODEL = ${shell find ./dep | grep $(DC_FW_MODEL)}

all:
	make clean
	@mkdir -p dep
	@if [ "$(MODEL)" == "" ]; then \
		rm -f dep/*; \
		touch dep/$(DC_FW_MODEL); \
		make clean; \
	fi
	for i in $(DIRS); do \
		INCLUDE='$(INCLUDE)' LD_PATH=$(LIBPATH) CFLAGS='$(CFLAGS)' TOP=$(TOP) make -C $$i all || exit 1; \
	done

depend dep:
	@for i in $(DIRS); do \
		INCLUDE='$(INCLUDE)' CFLAGS='$(CFLAGS)' TOP=$(TOP) make -C $$i depend || exit 1; \
	done

clean:
	@for i in $(DIRS); do \
		TOP=$(TOP) make -C $$i clean; \
	done
	@if [ -d $(INSTALL_PATH) ]; then \
		rm -rf $(INSTALL_PATH); \
	fi


install:
	@if [ ! -d $(INSTALL_PATH)/bin ]; then \
		mkdir -p $(INSTALL_PATH)/bin || exit 1; \
	fi
	@if [ ! -d $(INSTALL_PATH)/lib ]; then \
		mkdir -p $(INSTALL_PATH)/lib || exit 1; \
	fi
	@for i in $(DIRS); do \
		TOP=$(TOP) INSTALL_PATH=$(INSTALL_PATH) make -C $$i install; \
	done
	chmod a+x $(INSTALL_PATH)/bin/*

disk:
	rm -rf portd*.tar.gz tmp/
	mkdir -p tmp/portd
	cp portd/portd tcp_srv.conf readme.txt version.txt tmp/portd

ifeq ($(MASTER_CV_BETA), 0)
		tar zcvf portd_ver$(MASTER_MAJOR).$(MASTER_MINOR)_build_$(DC_BUILD_DATE).tar.gz -C tmp portd
else
		tar zcvf portd_ver$(MASTER_MAJOR).$(MASTER_MINOR).$(MASTER_CV_BETA)_build_$(DC_BUILD_DATE).tar.gz -C tmp portd
endif