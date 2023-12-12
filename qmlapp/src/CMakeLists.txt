# -*- Mode: CMake; indent-tabs-mode: nil; tab-width: 2 -*-
#
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

include_directories(
  ${Qt5Quick_INCLUDE_DIRS}
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Qml_INCLUDE_DIRS}
  ${QtCore_INCLUDE_DIRS}
  ${Qt5DBus_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(qmlapp
  object.cpp 
  qmlapp.cpp
  qmlapp.h
  parser.cpp
  appsettings.h
  appsettings.cpp
)

target_link_libraries(qmlapp
  Qt5::Core
  Qt5::Gui
  Qt5::DBus
  Qt5::Qml
  Qt5::Quick
)

install(TARGETS qmlapp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


