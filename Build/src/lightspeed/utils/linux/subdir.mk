################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/utils/linux/FilePath.cpp 

OBJS += \
./src/lightspeed/utils/linux/FilePath.o 

CPP_DEPS += \
./src/lightspeed/utils/linux/FilePath.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/utils/linux/%.o: ../src/lightspeed/utils/linux/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


