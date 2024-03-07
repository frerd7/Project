include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT5CORE_INCLUDE_DIRS}
  ${GL_INCLUDE_DIRS}
)

add_executable(qrfxp
  qrfxp.cpp
)

target_link_libraries(qrfxp
  ${QT5CORE_LDFLAGS}
)

add_executable(qrfx
   actions.cpp
   console_widget.cpp 
   console_widget.h
   info.cpp
   info.h
   info.ui 
   main.cpp   
   path.cpp
   qrfx.cpp
   qrfx.h
   toolbar.cpp
)

target_link_libraries(qrfx
  ${QT5CORE_LDFLAGS}
  ${GL_LDFLAGS}
  Qt5::Widgets
  Qt5::Gui
)
