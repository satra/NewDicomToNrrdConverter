project (DWIConvert)

#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8.0)
#-----------------------------------------------------------------------------

enable_testing()

find_package(ITK 4 REQUIRED)

find_package(DCMTK REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_MODULE_PATH})

# add libraries possibly missing from library list based
# on incomplete FindDCMTK.cmake
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

# SlicerExecutionModel
find_package(SlicerExecutionModel REQUIRED GenerateCLP)
include(${GenerateCLP_USE_FILE})

find_package(LIBICONV REQUIRED)

set(DWIConvert_Support_SRC
  FSLToNrrd.cxx
  NrrdToFSL.cxx
  itkDCMTKImageIO.cxx
  itkDCMTKImageIOFactory.cxx
  itkDCMTKSeriesFileNames.cxx
  itkDCMTKFileReader.cxx
  )

# several files needed down in ExtenededTesting
set(DWIConvertTest_SRC)
foreach(f ${DWIConvert_Support_SRC})
  list(APPEND DWIConvertTest_SRC
    ${CMAKE_CURRENT_LIST_DIR}/${f})
endforeach()

set(CLP DWIConvert)

set ( ${CLP}_SOURCE ${CLP}.cxx
  ${DWIConvert_Support_SRC}
)

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