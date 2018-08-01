cmake_minimum_required (VERSION 2.8.9)

project( httpd CXX )

if ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )

  find_package( Threads )

  if ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
    find_package( Threads )
    configure_file( "${PROJECT_SOURCE_DIR}/httpd.sh.in" "${PROJECT_BINARY_DIR}/${PROJECT_NAME}" @ONLY )
    configure_file( "${PROJECT_SOURCE_DIR}/postinst.in" "${PROJECT_BINARY_DIR}/postinst" @ONLY )
    configure_file( "${PROJECT_SOURCE_DIR}/prerm.in" "${PROJECT_BINARY_DIR}/prerm" @ONLY )
  endif()
  
endif()

configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

message( STATUS ${CMAKE_INSTALL_DIR} )

add_definitions( -DPID_NAME="/var/run/${PROJECT_NAME}.pid" -DDOC_ROOT="${CMAKE_INSTALL_PREFIX}/html" )

if ( QTDIR )
  add_definitions( -DHAVE_QT5=1 )
  set ( __guilib acqiris-gui )
endif()

include_directories(
  ${PROJECT_BINARY_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}
  )

add_definitions( -D_LINUX -D_ACQIRIS -DVERSION="${VERSION}")

add_executable( ${PROJECT_NAME}
  connection.cpp
  connection_manager.cpp
  constants.hpp
  endian_decoder_t.hpp
  main.cpp
  mime_types.cpp
  reply.cpp
  request_handler.cpp
  request_parser.cpp
  server.cpp
  log.cpp
  log.hpp
  datastorage.cpp
  datastorage.hpp
  page_handler.cpp
  page_handler.hpp
  acqiris/task.cpp
  acqiris/task.hpp
  acqiris/digitizer.cpp
  acqiris/digitizer.hpp
  )

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

target_link_libraries( ${PROJECT_NAME} LINK_PUBLIC
  ${__guilib}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  ${AqDrv4_LIBRARIES}
  )

install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT httpd )
install( FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME} DESTINATION /etc/init.d COMPONENT httpd )
