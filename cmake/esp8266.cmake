set(CMAKE_SYSTEM_NAME "Generic")

include(CMakeForceCompiler)

set(XTENSA_GCC_COMPILER "xtensa-lx106-elf-gcc${CMAKE_EXECUTABLE_SUFFIX}")
set(XTENSA_GXX_COMPILER "xtensa-lx106-elf-g++${CMAKE_EXECUTABLE_SUFFIX}")

if(NOT DEFINED ESP8266_TOOLCHAIN_PATH)
    # Check if GCC is reachable.
    find_path(ESP8266_TOOLCHAIN_PATH bin/${XTENSA_GCC_COMPILER})

    if(NOT ESP8266_TOOLCHAIN_PATH)
        # Set default path.
        set(ESP8266_TOOLCHAIN_PATH /opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf)
        message(STATUS "GCC not found, default path will be used: ${ESP8266_TOOLCHAIN_PATH}")
    endif()
else()
    message(STATUS "Toolchain path: ${ESP8266_TOOLCHAIN_PATH}")
endif()

SET(CMAKE_C_COMPILER ${XTENSA_GCC_COMPILER} )
SET(CMAKE_CXX_COMPILER ${XTENSA_GXX_COMPILER} )

#cmake_force_c_compiler(${ESP8266_TOOLCHAIN_PATH}/bin/${XTENSA_GCC_COMPILER} GNU)
#cmake_force_cxx_compiler(${ESP8266_TOOLCHAIN_PATH}/bin/${XTENSA_GXX_COMPILER} GNU)

set(CMAKE_OBJCOPY ${ESP8266_TOOLCHAIN_PATH}/bin/xtensa-lx106-elf-objcopy CACHE PATH "")

#########################
### ESP8266 SDK setup ###
#########################

if(NOT DEFINED ESP8266_SDK_PATH)
    set(ESP8266_SDK_PATH /opt/Espressif/esp-rtos-sdk)
    message(STATUS "Default SDK location will be used: ${ESP8266_SDK_PATH}")
else()
    message(STATUS "ESP8266 SDK path: ${ESP8266_SDK_PATH}")
endif()

set(CMAKE_LIBRARY_PATH ${ESP8266_SDK_PATH}/lib/)

set(LWIOT_PORT_INCLUDE_DIRECTORIES
        ${ESP8266_SDK_PATH}/extra_include
        ${ESP8266_SDK_PATH}/include
        ${ESP8266_SDK_PATH}/include/freertos
        ${ESP8266_SDK_PATH}/include/lwip
        ${ESP8266_SDK_PATH}/include/lwip/ipv4
        ${ESP8266_SDK_PATH}/include/lwip/ipv6
        ${ESP8266_SDK_PATH}/include/espressif/
        ${ESP8266_TOOLCHAIN_PATH}/lib/gcc/xtensa-lx106-elf/4.8.5/include/
        ${PROJECT_SOURCE_DIR}/source/ports/freertos
)

SET(LWIOT_PORT_DIR ${PROJECT_SOURCE_DIR}/source/ports/freertos)
SET(LWIOT_PORT_SRCS
        ${LWIOT_PORT_DIR}/rtos.c
        ports/thread.cpp
        )

SET(LWIOT_PORT_HEADERS
        ${LWIOT_PORT_DIR}/lwiot_arch.h)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-comment -Wno-pointer-sign -fno-builtin -Wno-implicit-function-declaration \
    -Wl,-EL,--gc-sections -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals \
    -ffunction-sections" CACHE FORCE "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -Wno-comment -fno-builtin -Wl,-EL,--gc-sections -fno-inline-functions \
    -nostdlib -mlongcalls -mtext-section-literals -ffunction-sections" CACHE FORCE "")

SET(HAVE_RTOS True)
