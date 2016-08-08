# dettings
# APPNAME - name of application. Can contain path, for example bin/appname
# CONFIG - optional name of configuration file. You can define rule to create this file
# NEEDLIBS - list of libraries (path to lib root) that is needed by this library. They need to  
#             implement same make-interface and generate ldeps.mk 
# DONTREBUILDLIBS - if "1", force rebuild will not rebuild depend libraries

# goals
#
# all - build all release
# debug - build debug version
# force-rebuild - requests rebuild, next all or debug will rebuild 




TARGETFILE=$(APPNAME)
PROGRESSPREFIX=$(APPNAME)


include $(dir $(lastword $(MAKEFILE_LIST)))/common.mk

LDLIBS+=$(LDOTHERLIBS)

$(APPNAME): $(OBJS) $(LIBPATHS)  $(CFGNAME)
	@echo "$(APPNAME) Linking ... $(LDLIBS)"
ifneq "$(notdir $(APPNAME))" "$(APPNAME)"		
	@mkdir -p $(@D)
endif
	@$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) 
	

clean:
	@echo $(APPNAME): cleaning  
ifneq "$(clean_list)" ""
	$(RM) $(clean_list)
endif
	$(RM) -r $(BUILDDIR)
	for X in $(NEEDLIBS); do $(MAKE) -C $$X clean; done
	


ifneq "$(MAKECMDGOALS)" "clean"
-include ${DEPS}
endif
	
