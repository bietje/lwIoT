SET(LWIOT_PORT_DIR ports/win32)

find_package(PythonLibs 2.7 REQUIRED)
include(${CMAKE_SOURCE_DIR}/cmake/FindMatplotlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/FindNumPy.cmake)

SET(LWIOT_PORT_SRCS
	${LWIOT_PORT_DIR}/win32.c
	soc/win32.cpp
	io/gpio/hostedgpiochip.cpp
)

SET(LWIOT_PORT_HEADERS ${LWIOT_PORT_DIR}/lwiot_arch.h)
SET(LWIOT_PORT_INCLUDE_DIRECTORIES
	${PROJECT_SOURCE_DIR}/source/ports/win32
	${PROJECT_SOURCE_DIR}/include/asm/hosted
	${PYTHON_INCLUDE_DIRS}
	${PYTHON_NUMPY_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/external/matplotlib-cpp
)

find_package(Threads REQUIRED)
SET(LWIOT_SYSTEM_LIBS ${LWIOT_SYSTEM_LIBS} ${CMAKE_THREAD_LIBS_INIT})

SET(HAVE_JSON True CACHE BOOL "Build JSON library")
SET(HAVE_NETWORKING True)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
