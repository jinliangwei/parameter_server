#
# libcuckoo.cmake  temp hack to make a libcuckoo target
# 20-Oct-2017  chuck@ece.cmu.edu
#

#
# XXXCDC: tmp hack for libcuckoo, since libcuckoo doesn't provide it.
# I sent in a patch to libcuckoo to add cmake support.  this lib is
# all .h files (so no .a/.so's to worry about)
#

find_path (LIBCUCKOO_INCLUDE libcuckoo/cuckoohash_map.hh)
if (NOT LIBCUCKOO_INCLUDE)
    message (FATAL_ERROR "unable to find libcuckoo in include path")
endif ()

if (NOT TARGET libcuckoo)
    add_library (libcuckoo INTERFACE IMPORTED)
    set_target_properties (libcuckoo PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LIBCUCKOO_INCLUDE}")
endif ()
