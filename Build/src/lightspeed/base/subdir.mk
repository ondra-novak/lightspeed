################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/datetime.cpp \
../src/lightspeed/base/objmanip.cpp 

OBJS += \
./src/lightspeed/base/datetime.o \
./src/lightspeed/base/objmanip.o 

CPP_DEPS += \
./src/lightspeed/base/datetime.d \
./src/lightspeed/base/objmanip.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/%.o: ../src/lightspeed/base/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


