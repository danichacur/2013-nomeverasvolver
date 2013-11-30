################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/personaje.c 

OBJS += \
./src/personaje.o 

C_DEPS += \
./src/personaje.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
<<<<<<< HEAD
	gcc -I"/home/utnso/git/tp-2013-2c-nomeverasvolver/commons" -Ipthread -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
=======
	gcc -I"../../commons" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
>>>>>>> d82f5ceb70874c9c3818d1e51577ed850cef809b
	@echo 'Finished building: $<'
	@echo ' '


