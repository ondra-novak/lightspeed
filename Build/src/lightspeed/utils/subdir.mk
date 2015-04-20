################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/utils/base64.cpp \
../src/lightspeed/utils/configParser.cpp \
../src/lightspeed/utils/eventdb.cpp \
../src/lightspeed/utils/queryParser.cpp \
../src/lightspeed/utils/sendmail.cpp \
../src/lightspeed/utils/urlencode.cpp \
../src/lightspeed/utils/xmlparser.cpp 

OBJS += \
./src/lightspeed/utils/base64.o \
./src/lightspeed/utils/configParser.o \
./src/lightspeed/utils/eventdb.o \
./src/lightspeed/utils/queryParser.o \
./src/lightspeed/utils/sendmail.o \
./src/lightspeed/utils/urlencode.o \
./src/lightspeed/utils/xmlparser.o 

CPP_DEPS += \
./src/lightspeed/utils/base64.d \
./src/lightspeed/utils/configParser.d \
./src/lightspeed/utils/eventdb.d \
./src/lightspeed/utils/queryParser.d \
./src/lightspeed/utils/sendmail.d \
./src/lightspeed/utils/urlencode.d \
./src/lightspeed/utils/xmlparser.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/utils/%.o: ../src/lightspeed/utils/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


