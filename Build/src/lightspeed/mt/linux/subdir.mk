################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/mt/linux/IOSleep.cpp \
../src/lightspeed/mt/linux/process.cpp \
../src/lightspeed/mt/linux/thread.cpp \
../src/lightspeed/mt/linux/threadSleeper.cpp 

OBJS += \
./src/lightspeed/mt/linux/IOSleep.o \
./src/lightspeed/mt/linux/process.o \
./src/lightspeed/mt/linux/thread.o \
./src/lightspeed/mt/linux/threadSleeper.o 

CPP_DEPS += \
./src/lightspeed/mt/linux/IOSleep.d \
./src/lightspeed/mt/linux/process.d \
./src/lightspeed/mt/linux/thread.d \
./src/lightspeed/mt/linux/threadSleeper.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/mt/linux/%.o: ../src/lightspeed/mt/linux/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


