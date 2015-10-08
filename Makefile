LIBNAME=liblightspeed.a
LIBDEPS=liblightspeed.deps

ALLCOMPILE := $(wildcard src/lightspeed/*/.sources.mk) $(wildcard src/lightspeed/*/*/.sources.mk) $(wildcard src/lightspeed/*/*/*/.sources.mk) $(wildcard src/lightspeed/*/*/*/*/.sources.mk)  
CPP_SRCS := 
include $(ALLCOMPILE)

#CXXFLAGS += -O3 -g3 -fPIC -Wall -Wextra
CXXFLAGS += -O0 -g3 -fPIC -Wall -Wextra

OBJS := ${CPP_SRCS:.cpp=.o}
DEPS := ${CPP_SRCS:.cpp=.deps}
clean_list := $(OBJS)  ${CPP_SRCS:.cpp=.deps} testfn testbuildin $(LIBDEPS)

all: $(LIBNAME) 

.INTERMEDIATE : deprun

deprun:
	@echo "Updating dependencies..."; touch deprun;

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


platform.h: testfn testbuildin
	@echo Detecting features...
	@./testfn pipe2 HAVE_PIPE2 > $@
	@./testfn vfork HAVE_VFORK >> $@
	@./testbuildin __atomic_compare_exchange HAVE_DECL___ATOMIC_COMPARE_EXCHANGE >> $@

%.deps: %.cpp deprun platform.h
	@$(CPP) $(CPPFLAGS) -MM $*.cpp | sed -e 's~^\(.*\)\.o:~$(@D)/\1.deps $(@D)/\1.o:~' > $@


$(LIBDEPS):
	@echo "Generating library dependencies..."
	@PWD=`pwd`;find $$PWD "(" -name "*.h" -or -name "*.tcc" ")" -and -printf '%p \\\n' > $@
	@for K in $(abspath $(CPP_SRCS)); do echo "$$K \\" >> $@;done

$(LIBNAME): $(OBJS) $(LIBDEPS)
	@$(AR) -r $(LIBNAME) $(OBJS)
	
print-%  : ; @echo $* = $($*)

clean: 
	$(RM) $(clean_list)

ifneq "$(MAKECMDGOALS)" "clean"
-include ${DEPS}
endif
	
