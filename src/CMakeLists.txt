#
# CMakeLists.txt  cmake file for the source directory
#

include (CheckCXXCompilerFlag)

#
# require correct c++ standard
#
set (CMAKE_CXX_STANDARD 11)
set (CXX_STANDARD_REQUIRED ON)

#
# translate some of the cache variables to compile definitions
# for this directory and below ...
#
add_definitions (-DPETUUM_MAX_NUM_CLIENTS=${PETUUM_MAX_NUM_CLIENTS})
if (PETUUM_CHECK_DENSE_STORAGE_RANGE)
  add_definitions (-DPETUUM_CHECK_DENSE_STORAGE_RANGE)
endif ()
if (PETUUM_STATS)
  add_definitions (-DPETUUM_STATS)
endif ()
if (PETUUM_NUMA)
  add_definitions (-DPETUUM_NUMA)
endif ()
if (PETUUM_COMP_IMPORTANCE)
  add_definitions (-DPETUUM_COMP_IMPORTANCE)
endif ()

#
# set some general global compile options (this is a directory property,
# so all subdirs will get it)
#
set (petuum-try-cxxflags -Wall -Wno-sign-compare -fno-builtin-malloc
        -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free)

# check and add if ok
foreach (lcv ${petuum-try-cxxflags})
    CHECK_CXX_COMPILER_FLAG (${lcv} flag${lcv})
    if (${flag${lcv}})
        add_compile_options (${lcv})
    endif ()
endforeach ()


add_subdirectory (petuum_ps_common)
add_subdirectory (petuum_ps)
add_subdirectory (ml)
