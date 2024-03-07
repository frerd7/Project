include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT5CORE_INCLUDE_DIRS}
)

add_library(qrc SHARED
   src/rc.cpp
   src/rc.h
   src/rc_global.h
   resources/resource.qrc
)

target_link_libraries(qrc
  ${QT5CORE_LDFLAGS}
)


