##### Project #####

PROJECT			?= karifa
# The path for generated files
BUILD_DIR		= Build

MCU_TYPE = PY32F002Ax5

##### Options #####

# Enable printf float %f support, y:yes, n:no
ENABLE_PRINTF_FLOAT	?= n
# Programmer, jlink or pyocd
FLASH_PROGRM	?= pyocd

##### Toolchains #######

ARM_TOOLCHAIN	?= /usr/bin

# path to JLinkExe
JLINKEXE		?= /opt/SEGGER/JLink/JLinkExe
# path to PyOCD
PYOCD_EXE		?= pyocd

##### Paths ############

# C and CPP source folders
CDIRS		:= Src
# Single C and CPP source files
CFILES	 	+= $(wildcard Drivers/PY32F0xx_HAL_Driver/Src/*_ll_*.c)
CPPFILES	:= 

# ASM source folders
ADIRS		:= Src
# Single ASM source files
AFILES		:= platformio/startup_py32f002a.s

# Include paths
INCLUDES	:= Drivers/PY32F0xx_HAL_Driver/Inc \
 			Drivers/CMSIS/Include \
			Drivers/CMSIS/Device \
			$(CDIRS)

##### Library Paths ############

# Library flags
LIB_FLAGS		= $(MCU_TYPE)
LIB_FLAGS   	+= USE_FULL_LL_DRIVER
# JLink device (Uppercases)
JLINK_DEVICE	?= $(shell echo $(MCU_TYPE) | tr '[:lower:]' '[:upper:]')
# PyOCD device (Lowercases)
PYOCD_DEVICE	?= $(shell echo $(MCU_TYPE) | tr '[:upper:]' '[:lower:]')
# Link descript file: 
LDSCRIPT		= platformio/$(PYOCD_DEVICE).ld

include ./platformio/rules.mk
