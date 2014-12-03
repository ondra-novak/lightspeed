################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/containers/arrayref.cpp \
../src/lightspeed/base/containers/arrayt.cpp \
../src/lightspeed/base/containers/avltreenode.cpp \
../src/lightspeed/base/containers/constStr.cpp \
../src/lightspeed/base/containers/deque.cpp \
../src/lightspeed/base/containers/resourcePool.cpp \
../src/lightspeed/base/containers/string.cpp \
../src/lightspeed/base/containers/stringBase.cpp \
../src/lightspeed/base/containers/stringpool.cpp 

OBJS += \
./src/lightspeed/base/containers/arrayref.o \
./src/lightspeed/base/containers/arrayt.o \
./src/lightspeed/base/containers/avltreenode.o \
./src/lightspeed/base/containers/constStr.o \
./src/lightspeed/base/containers/deque.o \
./src/lightspeed/base/containers/resourcePool.o \
./src/lightspeed/base/containers/string.o \
./src/lightspeed/base/containers/stringBase.o \
./src/lightspeed/base/containers/stringpool.o 

CPP_DEPS += \
./src/lightspeed/base/containers/arrayref.d \
./src/lightspeed/base/containers/arrayt.d \
./src/lightspeed/base/containers/avltreenode.d \
./src/lightspeed/base/containers/constStr.d \
./src/lightspeed/base/containers/deque.d \
./src/lightspeed/base/containers/resourcePool.d \
./src/lightspeed/base/containers/string.d \
./src/lightspeed/base/containers/stringBase.d \
./src/lightspeed/base/containers/stringpool.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/containers/%.o: ../src/lightspeed/base/containers/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


