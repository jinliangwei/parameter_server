#
# CMakeLists.txt  top-level cmake file for parameter_server
# 08-Oct-2017  chuck@ece.cmu.edu
#


cmake_minimum_required (VERSION 3.0)
project (parameter_server C CXX)
enable_testing ()

# link shared lib with full rpath
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# setup cache variables for ccmake
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release
         CACHE STRING "Choose the type of build." FORCE)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS 
                  "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif ()

set (DEBUG_SANITIZER OFF CACHE STRING "Sanitizer for debug builds")
set_property (CACHE DEBUG_SANITIZER PROPERTY STRINGS
              "OFF" "Address" "Thread")
set (CMAKE_PREFIX_PATH "" CACHE STRING "External dependencies path")
set (BUILD_SHARED_LIBS "OFF" CACHE BOOL "Build a shared library")

#
# project config
#
set (PETUUM_UMBRELLA "/usr/local" CACHE PATH "petuum umbrella prefix")
set (PETUUM_MAX_NUM_CLIENTS 64 CACHE STRING "petuum_ps server's max clients")
set (PETUUM_CHECK_DENSE_STORAGE_RANGE OFF
        CACHE BOOLEAN "check dense storage range")
set (PETUUM_STATS ON CACHE BOOLEAN "petuum stats")
set (PETUUM_NUMA ON CACHE BOOLEAN "petuum numa support")
set (PETUUM_COMP_IMPORTANCE OFF CACHE BOOLEAN "comp importance")

#
# sanitizer config (XXX: does not probe compiler to see if sanitizer flags
# are supported... )
#
set (as_flags "-fsanitize=address -O1 -fno-omit-frame-pointer")
set (ts_flags "-fsanitize=thread  -O1 -fno-omit-frame-pointer")
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    if (${DEBUG_SANITIZER} STREQUAL "Address")
        set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${as_flags}")
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${as_flags}")
    elseif (${DEBUG_SANITIZER} STREQUAL "Thread")
        set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${ts_flags}")
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${ts_flags}")
    endif ()
endif ()

# Find required packages
find_package (Threads REQUIRED)

add_subdirectory (src)
