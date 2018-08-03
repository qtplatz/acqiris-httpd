
cmake_minimum_required ( VERSION 3.7 )  # for GREATER_EQUAL
cmake_policy( SET CMP0020 NEW )
cmake_policy( SET CMP0042 NEW )
cmake_policy( SET CMP0053 NEW )

project (acqiris-httpd)

set ( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if (NOT DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
endif()

list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" )
include ( version )

set ( VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )

find_path( QTPLATZ NAME "qtplatz-config.cmake" HINTS "/opt/qtplatz" )
if ( NOT QTPLATZ )
  message( FATAL "QTPLATZ install directory not find" )
endif()

include ( ${QTPLATZ}/qtplatz-config.cmake )  # import libraries
set ( CMAKE_INSTALL_PREFIX "/opt/${PROJECT_NAME}" )   # override qtplatz
set ( CPACK_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" )
set ( HTML_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}" )    # ${CMAKE_INSTALL_PREFIX}/html/index.html

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/lib ) # static libs, dll's import lib/exp
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/lib ) #plugins
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/bin )  # .exe | .dll

if ( QTDIR )
  find_package( Qt5 OPTIONAL_COMPONENTS Core Xml Widgets Svg QUIET HINTS ${QTDIR} )
  find_package( qwt )
endif()

find_package( Threads REQUIRED )
find_package( AqDrv4 )
if ( NOT AqDrv4_FOUND )
  message( FATAL "${AqDrv4_FOUND}" )
endif()

include_directories(
  ${QTPLATZ_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/libs
  )

message( STATUS "QTDIR = " ${QTDIR} )

find_package( Boost 1.62 REQUIRED COMPONENTS
  filesystem
  program_options
  system
  timer
  serialization
  wserialization
  )

#message( STATUS "QTPLATZ_LIBRARY_DIRS=" ${QTPLATZ_LIBRARY_DIRS} )
set( CMAKE_INSTALL_RPATH
  "${CMAKE_INSTALL_PREFIX}/lib"
  "${Boost_LIBRARY_DIRS}"
  "${QTPLATZ_LIBRARY_DIRS}"
  "${QTPLATZ_LIBRARY_DIRS}/qtplatz"
  )

set( CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE )

add_subdirectory ( src )

set( CPACK_PACKAGE_VENDOR "MS-Cheminformatics LLC" )
set( CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK} )
set( CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR} )
set( CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR} )
set( CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH} )
set( CPACK_GENERATOR "DEB" )
#set( CPACK_DEB_COMPONENT_INSTALL 1 )
set( CPACK_PACKAGE_NAME ${PROJECT_NAME} )
set( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst;${CMAKE_BINARY_DIR}/prerm" )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER "T. Hondo <toshi.hondo@qtplatz.com>" )

include( CPack )
