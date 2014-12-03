################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/lightspeed/base/streams/compressNumb.cpp \
../src/lightspeed/base/streams/filehlp.cpp \
../src/lightspeed/base/streams/fileiobuff.cpp \
../src/lightspeed/base/streams/memfile.cpp 

CC_SRCS += \
../src/lightspeed/base/streams/utf.cc 

OBJS += \
./src/lightspeed/base/streams/compressNumb.o \
./src/lightspeed/base/streams/filehlp.o \
./src/lightspeed/base/streams/fileiobuff.o \
./src/lightspeed/base/streams/memfile.o \
./src/lightspeed/base/streams/utf.o 

CC_DEPS += \
./src/lightspeed/base/streams/utf.d 

CPP_DEPS += \
./src/lightspeed/base/streams/compressNumb.d \
./src/lightspeed/base/streams/filehlp.d \
./src/lightspeed/base/streams/fileiobuff.d \
./src/lightspeed/base/streams/memfile.d 


# Each subdirectory must supply rules for building sources it contributes
src/lightspeed/base/streams/%.o: ../src/lightspeed/base/streams/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/lightspeed/base/streams/%.o: ../src/lightspeed/base/streams/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(GPP) -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


