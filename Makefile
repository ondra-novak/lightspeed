LIBNAME=lightspeed
LIBINCLUDES=src
CONFIG=src/lightspeed/platform.h
CXX=clang++

include building/build_lib.mk
include building/testfns.mk

$(CONFIG): testfn testbuildin
	@echo Detecting features...
	@./testfn pipe2 HAVE_PIPE2 > $@
	@./testfn vfork HAVE_VFORK >> $@
	@./testbuildin __atomic_compare_exchange HAVE_DECL___ATOMIC_COMPARE_EXCHANGE >> $@
	
