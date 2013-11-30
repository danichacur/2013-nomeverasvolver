################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/enemigo.c \
../src/interbloqueo.c \
../src/nivel.c 

OBJS += \
./src/enemigo.o \
./src/interbloqueo.o \
./src/nivel.o 

C_DEPS += \
./src/enemigo.d \
./src/interbloqueo.d \
./src/nivel.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/git/tp-2013-2c-nomeverasvolver/commons" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


