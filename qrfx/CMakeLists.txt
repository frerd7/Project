project(QRFX)
set(USC_VERSION_MAJOR 0)
set(USC_VERSION_MINOR 9)
set(USC_VERSION_PATCH 0)
set(USC_VERSION "${USC_VERSION_MAJOR}.${USC_VERSION_MINOR}.${USC_VERSION_PATCH}")

cmake_minimum_required(VERSION 3.9)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

# Set location of outputs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_definitions(-D_REENTRANT -DQT_DEPRECATED_WARNINGS -DQT_CORE_LIB)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Use C++11 and warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -fPIC")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRIENT_DIR ON)

find_package(PkgConfig)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)

pkg_check_modules(GL REQUIRED gl)
pkg_check_modules(QT5CORE REQUIRED Qt5Core)

add_subdirectory(src/)
add_subdirectory(lib/qrc)

install(PROGRAMS
  ${CMAKE_BINARY_DIR}/bin/qrfx
  DESTINATION /lib/qrfx
  PERMISSIONS OWNER_READ OWNER_WRITE WORLD_EXECUTE GROUP_EXECUTE WORLD_READ GROUP_READ
)

install(PROGRAMS
  ${CMAKE_BINARY_DIR}/bin/qrfxp
  DESTINATION /lib/qrfx
  PERMISSIONS OWNER_READ OWNER_WRITE WORLD_EXECUTE GROUP_EXECUTE WORLD_READ GROUP_READ
)

install(FILES
  ${CMAKE_BINARY_DIR}/lib/libqrc.so
  DESTINATION /lib/qrfx
)

install(FILES
  ${CMAKE_SOURCE_DIR}/systemd/qrfx-recovery.service
  ${CMAKE_SOURCE_DIR}/systemd/qrfx-recovery.target
  DESTINATION /lib/systemd/system
)

install(FILES
  ${CMAKE_SOURCE_DIR}/systemd/qrfx-recovery
  DESTINATION /lib/systemd/system-generators
  PERMISSIONS OWNER_READ OWNER_WRITE WORLD_EXECUTE GROUP_EXECUTE WORLD_READ GROUP_READ
)

