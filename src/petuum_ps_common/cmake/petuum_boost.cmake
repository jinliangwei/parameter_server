#
# petuum_boost.cmake  generate a petuum_boost target for us
# 20-Oct-2017  chuck@ece.cmu.edu
#
include (FindPackageHandleStandardArgs)

#
# XXX: the cmake FindBoost.cmake varies depending on what version of
# cmake you have (some generate targets, some don't)
#
# avoid version issues by rolling our own simple "petuum_boost" target..
#

find_path (PETUUM_BOOST_INCLUDE boost/version.hpp)
find_library (PETUUM_BOOST_DATE_TIME_LIB boost_date_time)
find_library (PETUUM_BOOST_PROGRAM_OPTIONS_LIB boost_program_options)
find_library (PETUUM_BOOST_SYSTEM_LIB boost_system)
find_library (PETUUM_BOOST_THREAD_LIB boost_thread)

find_package_handle_standard_args (petuum_boost DEFAULT_MSG
    PETUUM_BOOST_INCLUDE PETUUM_BOOST_DATE_TIME_LIB
    PETUUM_BOOST_PROGRAM_OPTIONS_LIB PETUUM_BOOST_SYSTEM_LIB
    PETUUM_BOOST_THREAD_LIB)

mark_as_advanced (PETUUM_BOOST_INCLUDE PETUUM_BOOST_DATE_TIME_LIB
                  PETUUM_BOOST_PROGRAM_OPTIONS_LIB PETUUM_BOOST_SYSTEM_LIB
                  PETUUM_BOOST_THREAD_LIB)

set (PETUUM_BOOST_LIBLIST ${PETUUM_BOOST_DATE_TIME_LIB}
                          ${PETUUM_BOOST_PROGRAM_OPTIONS_LIB}
                          ${PETUUM_BOOST_SYSTEM_LIB}
                          ${PETUUM_BOOST_THREAD_LIB})

#
# generate target if not already defined!
#
if (NOT TARGET petuum_boost)
    add_library (petuum_boost INTERFACE IMPORTED)
    set_target_properties (petuum_boost PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PETUUM_BOOST_INCLUDE}"
        INTERFACE_LINK_LIBRARIES "${PETUUM_BOOST_LIBLIST}")
endif ()
