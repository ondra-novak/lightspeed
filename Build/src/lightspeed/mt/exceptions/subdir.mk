################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/mt/exceptions/mt_messages.cpp 

OBJS += \
./src/lightspeed/mt/exceptions/mt_messages.o 

CPP_DEPS += \
./src/lightspeed/mt/exceptions/mt_messages.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/mt/exceptions/%.o: ../src/lightspeed/mt/exceptions/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


