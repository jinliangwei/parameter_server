#
# CMakeLists.txt  main cmake file for matrixfact
# 22-Oct-2017  chuck@ece.cmu.edu
#

cmake_minimum_required (VERSION 3.0)
project (matrixfact C CXX)

# link shared libs with full rpath
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package (petuum_ps CONFIG REQUIRED)

set (program_list data_split duplicate matrixfact matrixfact_adarevision
                  matrixfact_data_verify matrixfact_split process_snapshot
                  randomize)
# XXXCDC: matrixfact_split16 does not compile properly

foreach (prog ${program_list})
    add_executable (${prog} src/${prog}.cpp)
    target_link_libraries (${prog} petuum_ps)
endforeach ()

#
# "make install" rules
#
install (TARGETS ${program_list} RUNTIME DESTINATION bin)
