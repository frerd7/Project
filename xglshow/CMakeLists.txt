project(Xglshow)
set(USC_VERSION_MAJOR 0)
set(USC_VERSION_MINOR 1)
set(USC_VERSION_PATCH 0)
set(USC_VERSION "${USC_VERSION_MAJOR}.${USC_VERSION_MINOR}.${USC_VERSION_PATCH}")

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

# Set location of outputs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_definitions(-DUSC_VERSION="${USC_VERSION}")

find_package(PkgConfig)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GDKPIXBUF REQUIRED gdk-pixbuf-2.0)
pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(GLU REQUIRED glu)
pkg_check_modules(GL REQUIRED gl)

add_subdirectory(src/)
