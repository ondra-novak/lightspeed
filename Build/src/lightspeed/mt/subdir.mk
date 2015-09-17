################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/mt/apcthread.cpp \
../src/lightspeed/mt/dispatcher.cpp \
../src/lightspeed/mt/msgthread.cpp \
../src/lightspeed/mt/mutex.cpp \
../src/lightspeed/mt/notifier.cpp \
../src/lightspeed/mt/scheduler.cpp \
../src/lightspeed/mt/shareExclusiveLock.cpp \
../src/lightspeed/mt/syncPt.cpp \
../src/lightspeed/mt/threadHook.cpp \
../src/lightspeed/mt/threadMinimal.cpp 

OBJS += \
./src/lightspeed/mt/apcthread.o \
./src/lightspeed/mt/dispatcher.o \
./src/lightspeed/mt/msgthread.o \
./src/lightspeed/mt/mutex.o \
./src/lightspeed/mt/notifier.o \
./src/lightspeed/mt/scheduler.o \
./src/lightspeed/mt/shareExclusiveLock.o \
./src/lightspeed/mt/syncPt.o \
./src/lightspeed/mt/threadHook.o \
./src/lightspeed/mt/threadMinimal.o 

CPP_DEPS += \
./src/lightspeed/mt/apcthread.d \
./src/lightspeed/mt/dispatcher.d \
./src/lightspeed/mt/msgthread.d \
./src/lightspeed/mt/mutex.d \
./src/lightspeed/mt/notifier.d \
./src/lightspeed/mt/scheduler.d \
./src/lightspeed/mt/shareExclusiveLock.d \
./src/lightspeed/mt/syncPt.d \
./src/lightspeed/mt/threadHook.d \
./src/lightspeed/mt/threadMinimal.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/mt/%.o: ../src/lightspeed/mt/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


