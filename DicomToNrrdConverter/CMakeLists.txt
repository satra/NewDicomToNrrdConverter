project (DicomToNrrdConverter)

#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.0)
#-----------------------------------------------------------------------------

enable_testing()

find_package(ITK 4 REQUIRED)

find_package(DCMTK REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_MODULE_PATH})

foreach(lib oflog ofstd )
  find_library(DCMTK_${lib}_LIBRARY
    ${lib}
    PATHS ${DCMTK_DIR}/lib
    )
  if(DCMTK_${lib}_LIBRARY)
    list(APPEND DCMTK_LIBRARIES ${DCMTK_${lib}_LIBRARY})
  endif()
endforeach()

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

find_package(LIBICONV REQUIRED)

#message("DCMTK_LIBRARIES=${DCMTK_LIBRARIES}")
#message("LIBICONV_LIBRARIES ${LIBICONV_LIBRARIES} LIB_ICONV_INCLUDE_DIRS ${LIBICONV_INCLUDE_DIRS}")

generateclp(${CLP}_SOURCE ${CLP}.xml)

link_directories(${DCMTK_DIR}/lib)

add_executable(${CLP} ${${CLP}_SOURCE})

target_link_libraries (${CLP}
  ${ITK_LIBRARIES}
  ${DCMTK_LIBRARIES}
  ${LIBICONV_LIBRARIES}
  ${ZLIB_LIBRARIES}
  )
set_target_properties(${CLP} PROPERTIES LABELS ${CLP})

add_subdirectory(ExtendedTesting)