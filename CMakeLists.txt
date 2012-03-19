cmake_minimum_required(VERSION 2.8)

project(DICOM2NRRD)

include(ExternalProject)

set(PREREQS ${CMAKE_CURRENT_BINARY_DIR}/Prereqs)

if(APPLE OR UNIX)
set(LIB_PREFIX lib)
set(LIB_SUFFIX .a)
endif()

set(COMMON_CMAKE_FLAGS
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
  -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
  -DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}
  -DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
  -DBUILD_TESTING:BOOL=OFF
  -DBUILD_SHARED_LIBS:BOOL=OFF
)

# Set CMake OSX variable to pass down the external project
set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
if(APPLE)
  list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
endif()

if(NOT USE_SYSTEM_ITK)
  set(ITK_REPOSITORY git://itk.org/ITK.git)
  set(ITK_TAG_COMMAND GIT_TAG v4.1.0)
  ExternalProject_Add(ITK
    GIT_REPOSITORY ${ITK_REPOSITORY}
    ${ITK_TAG_COMMAND}
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    SOURCE_DIR ITK
    BINARY_DIR ITK-build
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
      ${COMMON_CMAKE_FLAGS}
      -DBUILD_TESTING:BOOL=OFF
      -DBUILD_EXAMPLES:BOOL=OFF
      -DITK_LEGACY_REMOVE:BOOL=ON
      -DITK_BUILD_ALL_MODULES:BOOL=ON
      -DITK_USE_REVIEW:BOOL=ON
      -DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}
      -DINSTALL_WRAP_ITK_COMPATIBILITY:BOOL=OFF
    )
  set(ITK_DIR ${CMAKE_CURRENT_BINARY_DIR}/ITK-build)
else()
  ExternalProject_Add(ITK
    SOURCE_DIR ITK
    BINARY_DIR ITK-build
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    DEPENDS
    ${ITKv4_DEPENDENCIES}
    )
  find_package(ITK 4 REQUIRED)
endif()

ExternalProject_add(SlicerExecutionModel
  GIT_REPOSITORY git://github.com/Chaircrusher/SlicerExecutionModel.git
  GIT_TAG "origin/master"
  UPDATE_COMMAND ""
  BINARY_DIR SlicerExecutionModel-build
  CMAKE_ARGS
  ${COMMON_EXTERNAL_PROJECT_ARGS}
  ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  ${COMMON_CMAKE_FLAGS}
  -DITK_DIR:PATH=${ITK_DIR}
  INSTALL_COMMAND ""
  DEPENDS ITK
  )
set(SlicerExecutionModel_DIR ${CMAKE_BINARY_DIR}/SlicerExecutionModel-build)

ExternalProject_add(libjpeg
  URL http://www.ijg.org/files/jpegsrc.v8c.tar.gz
  URL_MD5 a2c10c04f396a9ce72894beb18b4e1f9
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PREREQS}
  --enable-static=yes
  --enable-shared=no
  CC=${CMAKE_C_COMPILER}
  CXX=${CMAKE_CXX_COMPILER}
  )

ExternalProject_add(libtiff
  URL ftp://dicom.offis.de/pub/dicom/offis/software/dcmtk/dcmtk360/support/tiff-3.9.4.tar.gz
  URL_MD5 2006c1bdd12644dbf02956955175afd6
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PREREQS} CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER}
  DEPENDS libjpeg
  )

if(NOT APPLE)
ExternalProject_add(libiconv
  URL http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.14.tar.gz
  URL_MD5 e34509b1623cec449dfeb73d7ce9c6c6
#  GIT_REPOSITORY git://git.savannah.gnu.org/libiconv.git
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${PREREQS}
  --enable-static=yes
  --enable-shared=no
  CC=${CMAKE_C_COMPILER}
  CXX=${CMAKE_CXX_COMPILER}
  )
  set(ICONV_LIB_FLAGS -DICONV_LIBRARY:FILEPATH=${PREREQS}/lib/libiconv.a)
  set(ICONV_DEP libiconv)
endif(NOT APPLE)

ExternalProject_add(dcmtk
  GIT_REPOSITORY "git://git.dcmtk.org/dcmtk.git"
  GIT_TAG 140ab83172d639066e757682ce252303196c6900
  UPDATE_COMMAND ""
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PREREQS}
  ${COMMON_CMAKE_FLAGS}
  -DDCMTK_WITH_XML:BOOL=ON
  -DDCMTK_WITH_PRIVATE_TAGS:BOOL=ON
  -DJPEG_DIR:PATH=${PREREQS}/lib
  -DTIFF_INCLUDE_DIR:PATH=${PREREQS}/include
  -DTIFF_LIBRARY:FILEPATH=${PREREQS}/lib/${LIB_PREFIX}tiff${LIB_SUFFIX}
  DEPENDS libtiff
)


set(dcmtkPatchScript ${CMAKE_CURRENT_LIST_DIR}/dcmtkPatchScript.cmake)
set(dcmtkGenConfig ${CMAKE_CURRENT_BINARY_DIR}/dcmtk-prefix/src/dcmtk/CMake/GenerateDCMTKConfigure.cmake)
set(dcmtk3rdParty ${CMAKE_CURRENT_BINARY_DIR}/dcmtk-prefix/src/dcmtk/CMake/3rdparty.cmake)
ExternalProject_Add_Step(dcmtk fixGenerateConfig
  COMMENT "One step of the configuration is broke with respect to strerror_r"
  DEPENDEES download
  DEPENDERS configure
  COMMAND ${CMAKE_COMMAND}
  -Ddcmtk3rdParty=${dcmtk3rdParty}
  -DdcmtkGenConfig=${dcmtkGenConfig}
  -P ${dcmtkPatchScript}
)

set(proj DicomToNrrdConverter)

ExternalProject_add(${proj}
  DEPENDS dcmtk SlicerExecutionModel ITK ${ICONV_DEP}
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND  ""
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${proj}
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${proj}-build
  CMAKE_ARGS
  -DRUN_EXTENDED_LOCAL_TESTING:BOOL=ON
  -DITK_DIR:PATH=${ITK_DIR}
  -DDCMTK_DIR:PATH=${PREREQS}
  ${ICONV_LIB_FLAGS}
  ${COMMON_CMAKE_FLAGS}
  -DSlicerExecutionModel_DIR:PATH=${SlicerExecutionModel_DIR}
)

