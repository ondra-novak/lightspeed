################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/actions/InfParallelExecutor.cpp \
../src/lightspeed/base/actions/dispatcher.cpp \
../src/lightspeed/base/actions/distributor.cpp \
../src/lightspeed/base/actions/job.cpp \
../src/lightspeed/base/actions/message.cpp \
../src/lightspeed/base/actions/msgQueue.cpp \
../src/lightspeed/base/actions/parallelExecutor.cpp \
../src/lightspeed/base/actions/promise.cpp \
../src/lightspeed/base/actions/scheduler.cpp \
../src/lightspeed/base/actions/simpleMsgQueue.cpp 

OBJS += \
./src/lightspeed/base/actions/InfParallelExecutor.o \
./src/lightspeed/base/actions/dispatcher.o \
./src/lightspeed/base/actions/distributor.o \
./src/lightspeed/base/actions/job.o \
./src/lightspeed/base/actions/message.o \
./src/lightspeed/base/actions/msgQueue.o \
./src/lightspeed/base/actions/parallelExecutor.o \
./src/lightspeed/base/actions/promise.o \
./src/lightspeed/base/actions/scheduler.o \
./src/lightspeed/base/actions/simpleMsgQueue.o 

CPP_DEPS += \
./src/lightspeed/base/actions/InfParallelExecutor.d \
./src/lightspeed/base/actions/dispatcher.d \
./src/lightspeed/base/actions/distributor.d \
./src/lightspeed/base/actions/job.d \
./src/lightspeed/base/actions/message.d \
./src/lightspeed/base/actions/msgQueue.d \
./src/lightspeed/base/actions/parallelExecutor.d \
./src/lightspeed/base/actions/promise.d \
./src/lightspeed/base/actions/scheduler.d \
./src/lightspeed/base/actions/simpleMsgQueue.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/actions/%.o: ../src/lightspeed/base/actions/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


