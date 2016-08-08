
BUILDDIR=tmp

all: $(TARGETFILE)
	@echo "Finished $(TARGETFILE)"
debug: $(TARGETFILE)
	@echo "Finished $(TARGETFILE) (debug)"


CPP_SRCS := 
OBJS := 
clean_list :=
include $(shell find $(SOURCES) -name .sources.mk)


libdeps%.mk:
	flock $(@D) -c "$(MAKE) -C $(@D) deps"

ifeq "$(MAKECMDGOALS)" "debug"
FORCE_DEBUG ?= 1
else
FORCE_DEBUG ?= 0
endif 

ifeq "$(FORCE_DEBUG)" "1"
CXXFLAGS += -O0 -g3 -fPIC -Wall -Wextra -DDEBUG -D_DEBUG $(INCLUDES)
CFGNAME := $(BUILDDIR)/cfg.debug
LIBMAKEGOALS ?= debug
OBJCACHE=debug
else 
CXXFLAGS += -O3 -g3 -fPIC -Wall -Wextra -DNDEBUG $(INCLUDES)
CFGNAME := $(BUILDDIR)/cfg.release
LIBMAKEGOALS ?= all
OBJCACHE=release
endif

-include $(CFGNAME)

ifneq "$(MAKECMDGOALS)" "clean"
NEEDLIBSDEPS=$(addsuffix /libdeps.mk,$(NEEDLIBS))
include  $(NEEDLIBSDEPS)
endif




ROOT_DIR:=$(dir $(abspath $(lastword $(MAKEFILE_LIST))))
OBJS += $(patsubst %,$(BUILDDIR)/$(OBJCACHE)/%,${CPP_SRCS:.cpp=.o})
DEPS := $(patsubst %,$(BUILDDIR)/$(OBJCACHE)/%,${CPP_SRCS:.cpp=.deps})


.PHONY: debug all debug clean force-rebuild deps 

force-rebuild: 
	@echo $(PROGRESSPREFIX): Requested rebuild
	@rm -f $(BUILDDIR)/cfg.debug $(BUILDDIR)/cfg.release $(TARGETFILE)
	
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)	

$(BUILDDIR)/$(OBJCACHE): $(BUILDDIR)
	@mkdir -p $(BUILDDIR)	
	
$(CFGNAME): | $(BUILDDIR)
	@rm -f $(BUILDDIR)/cfg.debug $(BUILDDIR)/cfg.release
	@touch $@	
	@echo $(PROGRESSPREFIX): Forced rebuild for CXXFLAGS=$(CXXFLAGS)


$(BUILDDIR)/$(OBJCACHE)/%.o: %.cpp  $(CONFIG)  
	@set -e
	@mkdir -p $(@D) 
	@echo $(PROGRESSPREFIX): $(*F).cpp  
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ -c $*.cpp -MMD -MF $(BUILDDIR)/$(OBJCACHE)/$*.deps

