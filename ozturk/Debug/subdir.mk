################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ozturk.c \
../ozturk.mod.c 

O_SRCS += \
../ozturk.mod.o \
../ozturk.o 

OBJS += \
./ozturk.o \
./ozturk.mod.o 

C_DEPS += \
./ozturk.d \
./ozturk.mod.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


