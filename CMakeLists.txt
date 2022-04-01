cmake_minimum_required(VERSION 2.8.12)
project(turbopfor_hdf5plugin)

# options
set(PLUGIN_INSTALL_PATH "./build" CACHE PATH
      "Where to install the dynamic HDF5-plugin")

set(CMAKE_INSTALL_PREFIX ./)


# sources
set(SOURCES turbopfor_h5plugin.c)
set(PLUGIN_SOURCES turbopfor_h5plugin.c)

# dependencies
if(MSVC)
    # FindHDF5.cmake does not find Windows installations. Try to
    # use an environment variable instead until the official "find"
    # file can be updated for Windows.
    #
    # Note that you have to set this environment variable by hand.
    file(TO_CMAKE_PATH "$ENV{HDF5_DIR}" HDF5_HINT)
    set(HDF5_DIR ${HDF5_HINT} CACHE STRING "Path to HDF5 CMake config directory.")
    find_package(HDF5 REQUIRED HINTS ${HDF5_DIR})
else(MSVC)
    find_package(HDF5 REQUIRED)
endif(MSVC)
include_directories(${HDF5_INCLUDE_DIRS})

find_package(MPI REQUIRED)

##
## Change here for a different place of TurboPFor-Integer-Compression
##
set(turbopfor_ROOT_DIR $ENV{TurboPFor_HOME})
find_path(turbopfor_INCLUDE_DIRS
  NAMES vp4.h
  HINTS ${turbopfor_ROOT_DIR}/)

find_library(turbopfor_LIBRARIES
  NAMES ic
  HINTS ${turbopfor_ROOT_DIR}/)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(turbopfor DEFAULT_MSG turbopfor_LIBRARIES turbopfor_INCLUDE_DIRS)

mark_as_advanced(
  turbopfor_LIBRARIES
  turbopfor_INCLUDE_DIRS)

if(turbopfor_FOUND AND NOT (TARGET turbopfor::turbopfor))
  add_library (turbopfor::turbopfor UNKNOWN IMPORTED)
  set_target_properties(turbopfor::turbopfor
    PROPERTIES
      IMPORTED_LOCATION ${turbopfor_LIBRARIES}
      INTERFACE_INCLUDE_DIRECTORIES ${turbopfor_INCLUDE_DIRS})
endif()

include_directories(SYSTEM ${MPI_INCLUDE_PATH})
include_directories(SYSTEM ${turbopfor_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS_RELEASE "-O3")


#target_link_libraries(my_mpi_target ${MPI_C_LIBRARIES})

# HDF5 plugin as shared library
add_library(turbopfor_h5_plugin_shared SHARED ${PLUGIN_SOURCES})
set_target_properties(turbopfor_h5_plugin_shared PROPERTIES OUTPUT_NAME H5Zturbopfor)
target_link_libraries(turbopfor_h5_plugin_shared  ${HDF5_LIBRARIES} ${MPI_C_LIBRARIES} ${turbopfor_LIBRARIES} )
install(TARGETS turbopfor_h5_plugin_shared  DESTINATION  ${PLUGIN_INSTALL_PATH} COMPONENT HDF5_FILTER_DEV)
