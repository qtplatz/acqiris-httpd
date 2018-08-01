
include ( version )
include_directories( ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/linux/drivers )

add_subdirectory( libs )
add_subdirectory( httpd )

set( CPACK_PACKAGE_VERSION ${VERSION} )
set( CMAKE_INSTALL_NAME_DIR ./bin )

# "Checking html/bootstrap"
if ( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/html/bower_components )
  execute_process( COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bower.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
    )
endif()

