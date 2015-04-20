################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/lightspeed/utils/linux/subdir.mk
-include src/lightspeed/utils/json/subdir.mk
-include src/lightspeed/utils/impl/subdir.mk
-include src/lightspeed/utils/subdir.mk
-include src/lightspeed/mt/linux/subdir.mk
-include src/lightspeed/mt/exceptions/subdir.mk
-include src/lightspeed/mt/subdir.mk
-include src/lightspeed/base/text/subdir.mk
-include src/lightspeed/base/sync/subdir.mk
-include src/lightspeed/base/streams/subdir.mk
-include src/lightspeed/base/memory/subdir.mk
-include src/lightspeed/base/linux/subdir.mk
-include src/lightspeed/base/iter/subdir.mk
-include src/lightspeed/base/framework/subdir.mk
-include src/lightspeed/base/exceptions/subdir.mk
-include src/lightspeed/base/debug/subdir.mk
-include src/lightspeed/base/containers/subdir.mk
-include src/lightspeed/base/actions/subdir.mk
-include src/lightspeed/base/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: liblightspeed.a

# Tool invocations
liblightspeed.a: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC Archiver'
	ar -r "liblightspeed.a" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(ARCHIVES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) liblightspeed.a
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
