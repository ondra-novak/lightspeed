################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/text/textFormat.cpp \
../src/lightspeed/base/text/textIn.cpp \
../src/lightspeed/base/text/textInBuffer.cpp \
../src/lightspeed/base/text/textParser.cpp \
../src/lightspeed/base/text/toString.cpp 

OBJS += \
./src/lightspeed/base/text/textFormat.o \
./src/lightspeed/base/text/textIn.o \
./src/lightspeed/base/text/textInBuffer.o \
./src/lightspeed/base/text/textParser.o \
./src/lightspeed/base/text/toString.o 

CPP_DEPS += \
./src/lightspeed/base/text/textFormat.d \
./src/lightspeed/base/text/textIn.d \
./src/lightspeed/base/text/textInBuffer.d \
./src/lightspeed/base/text/textParser.d \
./src/lightspeed/base/text/toString.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/text/%.o: ../src/lightspeed/base/text/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


