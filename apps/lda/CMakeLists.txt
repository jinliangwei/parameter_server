#
# CMakeLists.txt  main cmake file for matrixfact
# 22-Oct-2017  chuck@ece.cmu.edu
#

cmake_minimum_required (VERSION 3.0)
project (lda C CXX)

# link shared libs with full rpath
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package (petuum_ps CONFIG REQUIRED)

#
# lda_main
#
add_executable (lda_main src/context.cpp src/corpus.cpp
                src/fast_doc_sampler.cpp src/lda_engine.cpp
                src/lda_main.cpp src/lda_stats.cpp src/utils.cpp)
target_link_libraries (lda_main petuum_ps)

# XXX: lda_sn_main: add -DPETUUM_SINGLE_NODE and link with petuum_ps_sn

#
# data_preprocessor
#
add_executable (data_preprocessor src/data_preprocessor.cpp)
target_link_libraries (data_preprocessor petuum_ps)

#
# row_test
#
add_executable (row_test src/row_test.cpp)
target_link_libraries (row_test petuum_ps)

#
# "make install" rules
#
install (TARGETS lda_main data_preprocessor row_test RUNTIME DESTINATION bin)

