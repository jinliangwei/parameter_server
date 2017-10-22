#
# CMakeLists.txt  cmake file for the petuum ml lib
# 21-Oct-2017  chuck@ece.cmu.edu
#

# list of soruce files
set (petuum_ml_src disk_stream/byte_buffer.cpp
    disk_stream/disk_reader.cpp disk_stream/multi_buffer.cpp
    util/data_loading.cpp util/math_util.cpp util/metafile_reader.cpp)

#
# configure/load in standard modules we plan to use and probe the enviroment
#
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (petuum_ml_pkgloc "share/cmake/petuum_ml")

#
# library version set here (e.g. for shared libs).
#
set (PETUMM_ML_VERSION_MAJOR 1)
set (PETUMM_ML_VERSION_MINOR 0)
set (PETUMM_ML_VERSION_PATCH 0)
set (petuum_ml_vers "${PETUMM_ML_VERSION_MAJOR}.${PETUMM_ML_VERSION_MINOR}")
set (PETUMM_ML_VERSION "${petuum_ml_vers}.${PETUMM_ML_VERSION_PATCH}")

#
# create library target (user can specify shared vs. static using
# BUILD_SHARED_LIBS).  configure include files and targets that we
# depend on ...
#
add_library (petuum_ml ${petuum_ml_src})

# libs we use that are exposed to our users (so they need -I, etc.)
# this will pull in all of petuum_ps_common's public libs
target_link_libraries (petuum_ml PUBLIC petuum_ps_common)

# includes for installed lib, and for when building
target_include_directories (petuum_ml PUBLIC
                            $<INSTALL_INTERFACE:include>)
target_include_directories (petuum_ml BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

if (CMAKE_THREAD_LIBS_INIT)
  target_link_libraries (petuum_ml PUBLIC "${CMAKE_THREAD_LIBS_INIT}")
endif ()

set_target_properties(petuum_ml
                      PROPERTIES VERSION ${PETUMM_ML_VERSION}
                      SOVERSION ${PETUMM_ML_VERSION_MAJOR})

#
# installation stuff (packaging and install commands)
#
write_basic_package_version_file(
    "petuum_ml-config-version.cmake"
    VERSION ${PETUMM_ML_VERSION}
    COMPATIBILITY AnyNewerVersion)

# generate our config file for find_package()
configure_file (petuum_ml-config.cmake.in
                petuum_ml-config.cmake @ONLY)

#
# "make install" rules
#
install (TARGETS petuum_ml EXPORT petuum_ml-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (EXPORT petuum_ml-targets
         DESTINATION ${petuum_ml_pkgloc}
         FILE "petuum_ml-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/petuum_ml-config.cmake"
         "${CMAKE_CURRENT_BINARY_DIR}/petuum_ml-config-version.cmake"
         DESTINATION ${petuum_ml_pkgloc} )
install (DIRECTORY ../petuum_ml
         DESTINATION include
         FILES_MATCHING PATTERN "*.hpp")
