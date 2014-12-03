################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/debug/LogProvider.cpp \
../src/lightspeed/base/debug/dbglog.cpp \
../src/lightspeed/base/debug/livelog.cpp \
../src/lightspeed/base/debug/progress.cpp \
../src/lightspeed/base/debug/stdlogoutput.cpp 

OBJS += \
./src/lightspeed/base/debug/LogProvider.o \
./src/lightspeed/base/debug/dbglog.o \
./src/lightspeed/base/debug/livelog.o \
./src/lightspeed/base/debug/progress.o \
./src/lightspeed/base/debug/stdlogoutput.o 

CPP_DEPS += \
./src/lightspeed/base/debug/LogProvider.d \
./src/lightspeed/base/debug/dbglog.d \
./src/lightspeed/base/debug/livelog.d \
./src/lightspeed/base/debug/progress.d \
./src/lightspeed/base/debug/stdlogoutput.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/debug/%.o: ../src/lightspeed/base/debug/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


