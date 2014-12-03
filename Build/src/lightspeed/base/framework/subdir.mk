################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/framework/TCPServer.cpp \
../src/lightspeed/base/framework/app.cpp \
../src/lightspeed/base/framework/cmdLineIterator.cpp \
../src/lightspeed/base/framework/serviceapp.cpp \
../src/lightspeed/base/framework/services.cpp 

OBJS += \
./src/lightspeed/base/framework/TCPServer.o \
./src/lightspeed/base/framework/app.o \
./src/lightspeed/base/framework/cmdLineIterator.o \
./src/lightspeed/base/framework/serviceapp.o \
./src/lightspeed/base/framework/services.o 

CPP_DEPS += \
./src/lightspeed/base/framework/TCPServer.d \
./src/lightspeed/base/framework/app.d \
./src/lightspeed/base/framework/cmdLineIterator.d \
./src/lightspeed/base/framework/serviceapp.d \
./src/lightspeed/base/framework/services.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/framework/%.o: ../src/lightspeed/base/framework/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


