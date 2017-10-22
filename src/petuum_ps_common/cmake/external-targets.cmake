#
# external-targets.cmake  external targets we need for the petuum common lib
# 20-Oct-2017  chuck@ece.cmu.edu
#

#
# find required cmake packages
#
find_package (Eigen3 CONFIG REQUIRED)
find_package (float16_compressor CONFIG REQUIRED)
find_package (gflags CONFIG REQUIRED)
find_package (glog CONFIG REQUIRED)
find_package (Snappy CONFIG REQUIRED)
find_package (zmqhpp CONFIG REQUIRED)

#
# find pkgconfig packages
#
include (${CMAKE_CURRENT_LIST_DIR}/xpkg-import.cmake)
xpkg_import_module (yaml-cpp REQUIRED yaml-cpp)

#
# targets that we generate ourselves
#
include (${CMAKE_CURRENT_LIST_DIR}/petuum_boost.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/libcuckoo.cmake)
