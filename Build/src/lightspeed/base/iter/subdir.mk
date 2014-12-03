################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/iter/generator.cpp 

OBJS += \
./src/lightspeed/base/iter/generator.o 

CPP_DEPS += \
./src/lightspeed/base/iter/generator.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/iter/%.o: ../src/lightspeed/base/iter/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


