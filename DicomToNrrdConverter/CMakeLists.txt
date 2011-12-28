project (DicomToNrrdConverter)

#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.0)
#-----------------------------------------------------------------------------

enable_testing()

find_package(ITK REQUIRED)

find_package(DCMTK REQUIRED)

find_package(ZLIB REQUIRED)

#message("DCMTK_INCLUDE_DIRS=${DCMTK_INCLUDE_DIRS}")
#message("DCMTK_LIBRARIES=${DCMTK_LIBRARIES}")

include_directories(${DCMTK_INCLUDE_DIRS})
include_directories(${DCMTK_DIR}/include)

set (CLP DicomToNrrdConverter)
set ( ${CLP}_SOURCE ${CLP}.cxx)

# SlicerExecutionModel
find_package(SlicerExecutionModel REQUIRED GenerateCLP)
include(${GenerateCLP_USE_FILE})

find_library(ICONV_LIBRARY iconv "IConv library needed by DCMTK")

if(${ICONV_LIBRARY} STREQUAL "ICONV_LIB-NOTFOUND")
  message(FATAL_ERROR "Can't locate IConv library")
endif()

generateclp(${CLP}_SOURCE ${CLP}.xml)

link_directories(${DCMTK_DIR}/lib)

add_executable(${CLP} ${${CLP}_SOURCE})

message("DCMTK Lib dir ${DCMTK_DIR}/lib}")
target_link_libraries (${CLP}
  ${ITK_LIBRARIES}
  ${DCMTK_LIBRARIES}
  ${ICONV_LIBRARY}
  oflog ${ZLIB_LIBRARIES}
  )
set_target_properties(${CLP} PROPERTIES LABELS ${CLP})

set(BRAINS_CMAKE_HELPER_DIR ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(ExtendedTesting)