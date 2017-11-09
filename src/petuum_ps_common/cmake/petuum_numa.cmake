#
# petuum_numa.cmake  generate a petuum_numa target for us
# 22-Oct-2017  chuck@ece.cmu.edu
#
include (FindPackageHandleStandardArgs)

#
# make an imported interface for numa support
#

find_path (PETUUM_NUMA_INCLUDE numa.h)
find_library (PETUUM_NUMA_LIB numa)

find_package_handle_standard_args (petuum_numa REQUIRED_VARS
    PETUUM_NUMA_INCLUDE PETUUM_NUMA_LIB)

mark_as_advanced (PETUUM_NUMA_INCLUDE PETUUM_NUMA_LIB)

#
# generate target if not already defined!
#
if (NOT TARGET petuum_numa)
    add_library (petuum_numa INTERFACE IMPORTED)

    # address bug that bites on cray, see cmake issue #17413
    if ("${PETUUM_NUMA_INCLUDE}" STREQUAL "/usr/include")
        set (PETUUM_NUMA_INCLUDE "")
    endif ()

    set_target_properties (petuum_numa PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PETUUM_NUMA_INCLUDE}"
        INTERFACE_LINK_LIBRARIES "${PETUUM_NUMA_LIB}")
endif ()
