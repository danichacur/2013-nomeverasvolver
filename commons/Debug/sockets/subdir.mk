################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../sockets/estructuras.c \
../sockets/mensajes.c \
../sockets/sockets.c 

OBJS += \
./sockets/estructuras.o \
./sockets/mensajes.o \
./sockets/sockets.o 

C_DEPS += \
./sockets/estructuras.d \
./sockets/mensajes.d \
./sockets/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
sockets/%.o: ../sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


