# Copyright © 2021 - 2022 - 2023
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Frederick Valdez

project(FQMLAPP)
set(USC_VERSION_MAJOR 0)
set(USC_VERSION_MINOR 2)
set(USC_VERSION_PATCH 0)
set(USC_VERSION "${USC_VERSION_MAJOR}.${USC_VERSION_MINOR}.${USC_VERSION_PATCH}")

cmake_minimum_required(VERSION 3.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)

# Set location of outputs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_definitions(-DUSC_VERSION="${USC_VERSION}" -D_REENTRANT -DQT_QUICK_LIB -DQT_GUI_LIB -DQT_QML_LIB -DQT_CORE_LIB)

set(CMAKE_AUTOMOC ON)

# Use C++11 and warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -W  -fPIC -pipe -lGL -lpthread ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -W  -fPIC -pipe -lGL -lpthread")

find_package(PkgConfig)
find_package(Qt5 COMPONENTS Core Gui DBus Qml Quick REQUIRED)
add_subdirectory(src/)
add_subdirectory(qml/)
