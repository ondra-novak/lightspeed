################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/exceptions/exception.cpp \
../src/lightspeed/base/exceptions/messages.cpp \
../src/lightspeed/base/exceptions/throws.cpp 

OBJS += \
./src/lightspeed/base/exceptions/exception.o \
./src/lightspeed/base/exceptions/messages.o \
./src/lightspeed/base/exceptions/throws.o 

CPP_DEPS += \
./src/lightspeed/base/exceptions/exception.d \
./src/lightspeed/base/exceptions/messages.d \
./src/lightspeed/base/exceptions/throws.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/exceptions/%.o: ../src/lightspeed/base/exceptions/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


