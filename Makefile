LIBNAME=liblightspeed.a
LIBDEPS=liblightspeed.deps
PLATFORM=src/lightspeed/platform.h

CPP_SRCS := 
include $(shell find -name .sources.mk)

ifeq "$(MAKECMDGOALS)" "debug"
	CXXFLAGS += -O0 -g3 -fPIC -Wall -Wextra -DDEBUG -D_DEBUG
	CFGNAME := cfg.debug
else
	CXXFLAGS += -O3 -g3 -fPIC -Wall -Wextra -DNDEBUG
	CFGNAME := cfg.release
endif


OBJS := ${CPP_SRCS:.cpp=.o}
DEPS := ${CPP_SRCS:.cpp=.deps}
clean_list := $(OBJS)  ${CPP_SRCS:.cpp=.deps} testfn testbuildin $(LIBNAME) $(LIBDEPS) $(PLATFORM) cfg.debug cfg.release

all: $(LIBNAME)

.PHONY: debug all clean

debug: $(LIBNAME) 

.INTERMEDIATE : deprun

deprun:
	@echo "Updating dependencies..."; touch deprun;

$(CFGNAME):
	@rm -f cfg.debug cfg.release
	@touch $@	
	@echo Forced rebuild for CXXFLAGS=$(CXXFLAGS)

testfn:
	@echo 'echo "void $$1(); int main() {$$1();return 0;}" > testfn.c' >$@
	@echo 'if gcc -o testfn.out testfn.c 2> /dev/null; then echo "#define $$2 1"; else echo "#defined $$2 0"; fi' >>$@
	@echo 'rm -f testfn.out testfn.c' >>$@
	@chmod +x $@

testbuildin:
	@echo 'echo "int main() {$$1;return 0;}"  > testfn.c' >$@
	@echo 'if gcc -o testfn.out testfn.c 2> /dev/null; then echo "#define $$2 1"; else echo "#defined $$2 0"; fi' >>$@
	@echo 'rm -f testfn.out testfn.c' >>$@
	@chmod +x $@


$(PLATFORM): testfn testbuildin
	@echo Detecting features...
	@./testfn pipe2 HAVE_PIPE2 > $@
	@./testfn vfork HAVE_VFORK >> $@
	@./testbuildin __atomic_compare_exchange HAVE_DECL___ATOMIC_COMPARE_EXCHANGE >> $@

%.deps: %.cpp deprun $(PLATFORM) 
	@$(CPP) $(CPPFLAGS) -MM $*.cpp | sed -e 's~^\(.*\)\.o:~$(@D)/\1.deps $(@D)/\1.o:~' > $@


%.o: %.cpp $(CFGNAME)
	@echo $(*F).cpp  
	@$(CXX) $(CXXFLAGS) -o $@ -c $*.cpp
	

$(LIBDEPS):
	@echo "Generating library dependencies..."
	@PWD=`pwd`;find $$PWD "(" -name "*.h" -or -name "*.tcc" ")" -and -printf '%p \\\n' > $@
	@for K in $(abspath $(CPP_SRCS)); do echo "$$K \\" >> $@;done

$(LIBNAME): $(OBJS) $(LIBDEPS)
	@echo "Creating library $@ ..."		
	@$(AR) -r $@ $(OBJS) 2> /dev/null
	
print-%  : ; @echo $* = $($*)

clean: 
	$(RM) $(clean_list)

ifneq "$(MAKECMDGOALS)" "clean"
-include ${DEPS}
endif
	
