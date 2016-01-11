ifneq "$(MAKECMDGOALS)" "runtests"
LIBNAME=lightspeed
BUILDTYPE=lib
SOURCES=src/lightspeed
else
APPNAME=tmp/lightspeed_test
BUILDTYPE=app
LDOTHERLIBS:=-lpthread
SOURCES=src
endif

LIBINCLUDES=src

CONFIG=src/lightspeed/platform.h
CXX=clang++

include building/build_$(BUILDTYPE).mk
include building/testfns.mk


$(CONFIG): testfn testbuildin
	@echo Detecting features...
	@./testfn pipe2 HAVE_PIPE2 > $@
	@./testfn vfork HAVE_VFORK >> $@
	@./testbuildin __atomic_compare_exchange HAVE_DECL___ATOMIC_COMPARE_EXCHANGE >> $@
	

runtests: $(APPNAME) tmp/lightspeed_test
	tmp/lightspeed_test
	