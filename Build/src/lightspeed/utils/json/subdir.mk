################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/utils/json/json.cpp \
../src/lightspeed/utils/json/jsonbuilder.cpp \
../src/lightspeed/utils/json/jsonfast.cpp 

OBJS += \
./src/lightspeed/utils/json/json.o \
./src/lightspeed/utils/json/jsonbuilder.o \
./src/lightspeed/utils/json/jsonfast.o 

CPP_DEPS += \
./src/lightspeed/utils/json/json.d \
./src/lightspeed/utils/json/jsonbuilder.d \
./src/lightspeed/utils/json/jsonfast.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/utils/json/%.o: ../src/lightspeed/utils/json/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


