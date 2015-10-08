LIBNAME=liblightspeed.a

ALLCOMPILE := $(wildcard */.sources.mk) $(wildcard */*/.sources.mk) $(wildcard */*/*/.sources.mk) 
CPP_SRCS := 
include $(ALLCOMPILE)

CXXFLAGS += -O3 -g3 -fPIC -Wall -Wextra

OBJS := ${CPP_SRCS:.cpp=.o}
DEPS := ${CPP_SRCS:.cpp=.deps}
clean_list := $(OBJS)  ${CPP_SRCS:.cpp=.deps}

	
%.deps: %.cpp
	@echo Updating deps file for $*.cpp
	$(CPP) $(CPPFLAGS) -MM $*.cpp | sed -e 's@^\(.*\)\.o:@\1.deps \1.o:@' > $@

%.o: %.cpp
	@echo Compiling $*.cpp
	@$(CXX) $(CXXFLAGS) -c -o $@ $*.cpp 

all: $(LIBNAME) 

$(LIBNAME): $(OBJS)
	@$(AR) -r $(LIBNAME) $(OBJS)
  
print-%  : ; @echo $* = $($*)


clean: 
	$(RM) $(clean_list)

ifneq "$(MAKECMDGOALS)" "clean"
-include ${DEPS}
endif
	