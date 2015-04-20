################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../src/lightspeed/base/streams/utf.cc 

CPP_SRCS += \
../src/lightspeed/base/streams/abstractFileIOService.cpp \
../src/lightspeed/base/streams/bufferedio.cpp \
../src/lightspeed/base/streams/compressNumb.cpp \
../src/lightspeed/base/streams/filehlp.cpp \
../src/lightspeed/base/streams/fileiobuff.cpp \
../src/lightspeed/base/streams/memfile.cpp \
../src/lightspeed/base/streams/tcpHandler.cpp 

CC_DEPS += \
./src/lightspeed/base/streams/utf.d 

OBJS += \
./src/lightspeed/base/streams/abstractFileIOService.o \
./src/lightspeed/base/streams/bufferedio.o \
./src/lightspeed/base/streams/compressNumb.o \
./src/lightspeed/base/streams/filehlp.o \
./src/lightspeed/base/streams/fileiobuff.o \
./src/lightspeed/base/streams/memfile.o \
./src/lightspeed/base/streams/tcpHandler.o \
./src/lightspeed/base/streams/utf.o 

CPP_DEPS += \
./src/lightspeed/base/streams/abstractFileIOService.d \
./src/lightspeed/base/streams/bufferedio.d \
./src/lightspeed/base/streams/compressNumb.d \
./src/lightspeed/base/streams/filehlp.d \
./src/lightspeed/base/streams/fileiobuff.d \
./src/lightspeed/base/streams/memfile.d \
./src/lightspeed/base/streams/tcpHandler.d 


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


