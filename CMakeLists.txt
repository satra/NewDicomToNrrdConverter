cmake_minimum_required(VERSION 2.8)

project(DICOM2NRRD)

include(ExternalProject)

set(PREREQS ${CMAKE_CURRENT_BINARY_DIR}/Prereqs)

if(APPLE OR UNIX)
set(LIB_PREFIX lib)
set(LIB_SUFFIX .a)
endif()

# Set CMake OSX variable to pass down the external project
set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
if(APPLE)
  list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

find_package(ITK 4 REQUIRED)

ExternalProject_add(SlicerExecutionModel
  GIT_REPOSITORY git://github.com/Chaircrusher/SlicerExecutionModel.git
  GIT_TAG "origin/master"
  UPDATE_COMMAND ""
  BINARY_DIR ${proj}-build
  CMAKE_ARGS
  ${COMMON_EXTERNAL_PROJECT_ARGS}
  ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  -DBUILD_TESTING:BOOL=OFF
  -DITK_DIR:PATH=${ITK_DIR}
  INSTALL_COMMAND ""
  )
set(SlicerExecutionModel_DIR ${CMAKE_BINARY_DIR}/${proj}-build)

ExternalProject_add(libtiff
  URL ftp://dicom.offis.de/pub/dicom/offis/software/dcmtk/dcmtk360/support/tiff-3.9.4.tar.gz
  URL_MD5 2006c1bdd12644dbf02956955175afd6
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PREREQS}
  )

ExternalProject_add(dcmtk
  GIT_REPOSITORY "git://git.dcmtk.org/dcmtk.git"
  GIT_TAG 085525e643cab5ac82
  UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PREREQS}
  -DDCMTK_WITH_XML:BOOL=ON
  -DDCMTK_WITH_PRIVATE_TAGS:BOOL=ON
  -DTIFF_INCLUDE_DIR:PATH=${PREREQS}/include
  -DTIFF_LIBRARY:FILEPATH=${PREREQS}/lib/${LIB_PREFIX}tiff${LIB_SUFFIX}
  DEPENDS libtiff
)


set(proj DicomToNRRDConverter)
ExternalProject_add(${proj}
  DEPENDS dcmtk SlicerExecutionModel
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND  ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${proj}
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${proj}-build
  CMAKE_ARGS
  -DRUN_EXTENDED_LOCAL_TESTING:BOOL=ON
  -DITK_DIR:PATH=${ITK_DIR}
  -DDCMTK_DIR:PATH=${PREREQS}
  -DSlicerExecutionModel_DIR:PATH=${SlicerExecutionModel_DIR}
)

set(BuildStamp ${CMAKE_CURRENT_BINARY_DIR}/${proj}-prefix/src/${proj}-stamp/${proj}-build)

ExternalProject_Add_Step(${proj} force-build
  COMMAND ${CMAKE_COMMAND} -E remove ${BuildStamp}
  DEPENDEES build done
  ALWAYS 1
)