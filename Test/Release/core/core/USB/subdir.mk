################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino\USB\CDC.cpp \
C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino\USB\PluggableUSB.cpp \
C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino\USB\USBCore.cpp 

AR_OBJ += \
.\core\core\USB\CDC.cpp.o \
.\core\core\USB\PluggableUSB.cpp.o \
.\core\core\USB\USBCore.cpp.o 

CPP_DEPS += \
.\core\core\USB\CDC.cpp.d \
.\core\core\USB\PluggableUSB.cpp.d \
.\core\core\USB\USBCore.cpp.d 


# Each subdirectory must supply rules for building sources it contributes
core\core\USB\CDC.cpp.o: C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino\USB\CDC.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\eclipse\/arduinoPlugin/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-g++" -c -g -Os -Wall -Wextra -std=gnu++11 -ffunction-sections -fdata-sections -nostdlib -fno-threadsafe-statics --param max-inline-insns-single=500 -fno-rtti -fno-exceptions -Dprintf=iprintf -mcpu=cortex-m3 -mthumb -DF_CPU=84000000L -DARDUINO=10802 -DARDUINO_SAM_DUE -DARDUINO_ARCH_SAM -D__SAM3X8E__ -mthumb -DUSB_VID=0x2341 -DUSB_PID=0x003e -DUSBCON "-DUSB_MANUFACTURER=\"Arduino LLC\"" "-DUSB_PRODUCT=\"Arduino Due\"" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/libsam" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/CMSIS/CMSIS/Include/" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/CMSIS/Device/ATMEL/"  -I"C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino" -I"C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\variants\arduino_due_x" -I"G:\Electric\Arduino\libraries\extEEPROM" -I"G:\Electric\Arduino\libraries\extEEPROM\src" -I"G:\Electric\Arduino\libraries\DS3232" -I"G:\Electric\Arduino\libraries\DS3232\src" -I"G:\Electric\Arduino\libraries\WireSam" -I"G:\Electric\Arduino\libraries\WireSam\src" -I"G:\Electric\Arduino\libraries\FreeRTOS_ARM" -I"G:\Electric\Arduino\libraries\FreeRTOS_ARM\src" -I"G:\Electric\Arduino\libraries\Arduino-Due-RTC-Library" -I"G:\Electric\Arduino\libraries\Arduino-Due-RTC-Library\src" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"
	@echo 'Finished building: $<'
	@echo ' '

core\core\USB\PluggableUSB.cpp.o: C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino\USB\PluggableUSB.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\eclipse\/arduinoPlugin/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-g++" -c -g -Os -Wall -Wextra -std=gnu++11 -ffunction-sections -fdata-sections -nostdlib -fno-threadsafe-statics --param max-inline-insns-single=500 -fno-rtti -fno-exceptions -Dprintf=iprintf -mcpu=cortex-m3 -mthumb -DF_CPU=84000000L -DARDUINO=10802 -DARDUINO_SAM_DUE -DARDUINO_ARCH_SAM -D__SAM3X8E__ -mthumb -DUSB_VID=0x2341 -DUSB_PID=0x003e -DUSBCON "-DUSB_MANUFACTURER=\"Arduino LLC\"" "-DUSB_PRODUCT=\"Arduino Due\"" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/libsam" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/CMSIS/CMSIS/Include/" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/CMSIS/Device/ATMEL/"  -I"C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino" -I"C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\variants\arduino_due_x" -I"G:\Electric\Arduino\libraries\extEEPROM" -I"G:\Electric\Arduino\libraries\extEEPROM\src" -I"G:\Electric\Arduino\libraries\DS3232" -I"G:\Electric\Arduino\libraries\DS3232\src" -I"G:\Electric\Arduino\libraries\WireSam" -I"G:\Electric\Arduino\libraries\WireSam\src" -I"G:\Electric\Arduino\libraries\FreeRTOS_ARM" -I"G:\Electric\Arduino\libraries\FreeRTOS_ARM\src" -I"G:\Electric\Arduino\libraries\Arduino-Due-RTC-Library" -I"G:\Electric\Arduino\libraries\Arduino-Due-RTC-Library\src" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"
	@echo 'Finished building: $<'
	@echo ' '

core\core\USB\USBCore.cpp.o: C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino\USB\USBCore.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\eclipse\/arduinoPlugin/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/bin/arm-none-eabi-g++" -c -g -Os -Wall -Wextra -std=gnu++11 -ffunction-sections -fdata-sections -nostdlib -fno-threadsafe-statics --param max-inline-insns-single=500 -fno-rtti -fno-exceptions -Dprintf=iprintf -mcpu=cortex-m3 -mthumb -DF_CPU=84000000L -DARDUINO=10802 -DARDUINO_SAM_DUE -DARDUINO_ARCH_SAM -D__SAM3X8E__ -mthumb -DUSB_VID=0x2341 -DUSB_PID=0x003e -DUSBCON "-DUSB_MANUFACTURER=\"Arduino LLC\"" "-DUSB_PRODUCT=\"Arduino Due\"" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/libsam" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/CMSIS/CMSIS/Include/" "-IC:/Users/kulakov/AppData/Local/Arduino15/packages/arduino/hardware/sam/1.6.11/system/CMSIS/Device/ATMEL/"  -I"C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\cores\arduino" -I"C:\Users\kulakov\AppData\Local\Arduino15\packages\arduino\hardware\sam\1.6.11\variants\arduino_due_x" -I"G:\Electric\Arduino\libraries\extEEPROM" -I"G:\Electric\Arduino\libraries\extEEPROM\src" -I"G:\Electric\Arduino\libraries\DS3232" -I"G:\Electric\Arduino\libraries\DS3232\src" -I"G:\Electric\Arduino\libraries\WireSam" -I"G:\Electric\Arduino\libraries\WireSam\src" -I"G:\Electric\Arduino\libraries\FreeRTOS_ARM" -I"G:\Electric\Arduino\libraries\FreeRTOS_ARM\src" -I"G:\Electric\Arduino\libraries\Arduino-Due-RTC-Library" -I"G:\Electric\Arduino\libraries\Arduino-Due-RTC-Library\src" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"
	@echo 'Finished building: $<'
	@echo ' '


