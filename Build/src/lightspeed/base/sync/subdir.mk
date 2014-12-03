################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/sync/tls.cpp \
../src/lightspeed/base/sync/tlsalloc.cpp 

OBJS += \
./src/lightspeed/base/sync/tls.o \
./src/lightspeed/base/sync/tlsalloc.o 

CPP_DEPS += \
./src/lightspeed/base/sync/tls.d \
./src/lightspeed/base/sync/tlsalloc.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/sync/%.o: ../src/lightspeed/base/sync/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


