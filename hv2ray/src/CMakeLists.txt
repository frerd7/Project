include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT5CORE_INCLUDE_DIRS}
  ${QT5GUI_INCLUDE_DIRS}
  ${GL_INCLUDE_DIRS}
  ${LIBNOTIFY_INCLUDE_DIRS}
)

add_executable(hv2ray
   about.cpp
   hv2ray.cpp
   about.h
   qplaintext.h
   hv2ray.h
   main.cpp
   qplaintext.cpp
   notifyservice.cpp
   hv2ray.ui
   about.ui
   resource.qrc
)

target_link_libraries(hv2ray
  ${QT5CORE_LDFLAGS}
  ${QT5GUI_LDFLAGS}
  Qt5::Widgets
  ${GL_LDFLAGS}
  ${LIBNOTIFY_LDFLAGS}
)


