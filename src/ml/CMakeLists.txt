#
# CMakeLists.txt  cmake file for the petuum ml lib
#

set (PETUUM_ML_SRC disk_stream/byte_buffer.cpp
    disk_stream/disk_reader.cpp disk_stream/multi_buffer.cpp
    util/data_loading.cpp util/math_util.cpp util/metafile_reader.cpp)

add_library (petuum_ml ${PETUUM_ML_SRC})

#
# "make install" rules
#
install (TARGETS petuum_ml EXPORT petuum_ml-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
