#
# petuum_leveldb.cmake  generate a petuum_leveldb target for us
# 22-Oct-2017  chuck@ece.cmu.edu
#
include (FindPackageHandleStandardArgs)

#
# make an imported interface for leveldb support
#

find_path (PETUUM_LEVELDB_INCLUDE leveldb/db.h)
find_library (PETUUM_LEVELDB_LIB leveldb)

find_package_handle_standard_args (petuum_leveldb REQUIRED_VARS
    PETUUM_LEVELDB_INCLUDE PETUUM_LEVELDB_LIB)

mark_as_advanced (PETUUM_LEVELDB_INCLUDE PETUUM_LEVELDB_LIB)

#
# generate target if not already defined!
#
if (NOT TARGET petuum_numa)
    add_library (petuum_numa INTERFACE IMPORTED)
    set_target_properties (petuum_numa PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PETUUM_LEVELDB_INCLUDE}"
        INTERFACE_LINK_LIBRARIES "${PETUUM_LEVELDB_LIB}")
endif ()
