################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/memory/clusterAlloc.cpp \
../src/lightspeed/base/memory/dynobject.cpp \
../src/lightspeed/base/memory/nodeAlloc.cpp \
../src/lightspeed/base/memory/refCntPtr.cpp \
../src/lightspeed/base/memory/singleton.cpp 

OBJS += \
./src/lightspeed/base/memory/clusterAlloc.o \
./src/lightspeed/base/memory/dynobject.o \
./src/lightspeed/base/memory/nodeAlloc.o \
./src/lightspeed/base/memory/refCntPtr.o \
./src/lightspeed/base/memory/singleton.o 

CPP_DEPS += \
./src/lightspeed/base/memory/clusterAlloc.d \
./src/lightspeed/base/memory/dynobject.d \
./src/lightspeed/base/memory/nodeAlloc.d \
./src/lightspeed/base/memory/refCntPtr.d \
./src/lightspeed/base/memory/singleton.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/memory/%.o: ../src/lightspeed/base/memory/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


