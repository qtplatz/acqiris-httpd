cmake_minimum_required( VERSION 3.5 )
project( acqiris-gui )
cmake_policy(SET CMP0022 NEW)
cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0028 NEW)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB UIs *.ui)
file(GLOB Resources *.qrc)

if ( QTDIR )
  find_package( Qt5 OPTIONAL_COMPONENTS Core Xml Widgets Svg QUIET HINTS ${QTDIR} )
  find_package( qwt )
endif()

add_definitions( -D_LINUX -D_ACQIRIS -DVERSION="${VERSION}")

include_directories(
  ${QWT_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  )

set( qwt_dependent_sources
  waveformview.cpp
  waveformview.hpp
  xyseriesdata.cpp
  xyseriesdata.hpp
  )


add_library(
  ${PROJECT_NAME} SHARED
  app.cpp
  app.hpp
  document.cpp
  document.hpp
  mainwindow.cpp
  mainwindow.hpp
  ${aqdrv4_sources}
  outputwidget.cpp
  outputwidget.hpp
  ${qwt_dependent_sources}
  ${Resources}
  )

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14 )

target_link_libraries( ${PROJECT_NAME} LINK_PUBLIC
  adportable
  acqrscontrols
  acqrswidgets
  ${AqDrv4_LIBRARIES}
  ${QWT_LIBRARIES}
  ${Boost_LIBRARIES}
  Qt5::Widgets
  Qt5::Svg
  Threads::Threads
  ${CMAKE_DL_LIBS}
  )

install( TARGETS ${PROJECT_NAME} 
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin COMPONENT applications
  LIBRARY DESTINATION lib/qtplatz COMPONENT libraries 
  ARCHIVE DESTINATION lib/qtplatz COMPONENT libraries
  )
