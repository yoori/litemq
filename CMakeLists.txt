cmake_minimum_required(VERSION 2.6)

project(HashEchoServer1)

macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

SET(GCC_ROOT ""
  CACHE
  STRING "Custom GCC root. Define CMAKE_CXX_COMPILER and CMAKE_C_COMPILER appropriately!"
  )
SET(GCC_VER ""
  CACHE
  STRING "Version of custom GCC. Must be specified if GCC_ROOT is defined!"
  )
SET(CXX11 TRUE
  CACHE
  BOOL "Use C++11 standard"
  )
SET(GCC_EXTRA_OPTIONS ""
  CACHE
  STRING "Extra flags used by the compiler during all builds"
  )

IF (GCC_ROOT AND NOT GCC_VER)
    MESSAGE(SEND_ERROR "GCC_VER must be specified if GCC_ROOT is defined")
ENDIF()

#Set compiler binary paths if they are not cached yet and GCC_ROOT is set.
IF (GCC_ROOT AND NOT CMAKE_CXX_COMPILER)
    SET(CMAKE_CXX_COMPILER "${GCC_ROOT}/bin/c++" CACHE INTERNAL "C++ Compiler. Generated.")
ENDIF()
IF (GCC_ROOT AND NOT CMAKE_C_COMPILER)
    SET(CMAKE_C_COMPILER "${GCC_ROOT}/bin/gcc" CACHE INTERNAL "C Compiler. Generated.")
ENDIF()

use_cxx11()
SET(CMAKE_CXX_FLAGS "" CACHE INTERNAL "Compiler flags. Generated.")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE INTERNAL "Compiler DEBUG flags. Generated.")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE INTERNAL "Compiler RELEASE flags. Generated.")
SET(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE INTERNAL "Compiler MINSIZEREL flags. Generated.")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE INTERNAL "Compiler RELWITHDEBINFO flags. Generated.")
SET(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS} CACHE INTERNAL "Compiler flags. Generated.")
SET(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} CACHE INTERNAL "Compiler DEBUG flags. Generated.")
SET(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} CACHE INTERNAL "Compiler RELEASE flags. Generated.")
SET(CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_MINSIZEREL} CACHE INTERNAL "Compiler MINSIZEREL flags. Generated.")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} CACHE INTERNAL "Compiler RELWITHDEBINFO flags. Generated.")

SET(CMAKE_CXX_FLAGS "-Wall ${GCC_EXTRA_OPTIONS}")
IF (CXX11)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated -DBOOST_SYSTEM_NO_DEPRECATED")
ENDIF()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

MESSAGE(STATUS "cxx flags = " ${CMAKE_CXX_FLAGS})

# additional cmake modules location path
set(
  CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules
  CACHE
  PATH "CMake modules search path"
)

# all compiled executables and libraries
# will be created inside "${BUILD_DIR}/bin"
# and "${BUILD_DIR}/lib_objects" directories
set(
  CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/bin
)
set(
  CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/lib_objects
)

# commonly used packages

if (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE DEBUG)
endif()

# projects common include directory path(s)
set(GCC_INCLUDE_DIR)
IF (${GCC_ROOT})
    set(GCC_INCLUDE_DIR ${GCC_ROOT}/include ${GCC_ROOT}/include/c++/${GCC_VER})
ENDIF()

SET(CMAKE_INSTALL_PREFIX /)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

add_subdirectory(gears)
add_subdirectory(src)
add_subdirectory(litemq)

add_dependencies(LiteMQ Gears)
add_dependencies(ServerGearsHashServer Gears)
add_dependencies(ServerGearsHashServer LiteMQ)

install(DIRECTORY etc/ DESTINATION etc/
  FILES_MATCHING
  PATTERN "*.template"
  PATTERN "*.txt"
  PATTERN ".*" EXCLUDE)

install(DIRECTORY bin/ DESTINATION bin/
  FILES_MATCHING
  PATTERN "*.sh")
