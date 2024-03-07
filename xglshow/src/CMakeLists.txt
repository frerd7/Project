include_directories(
  ${GL_INCLUDE_DIRS}
  ${GLU_INCLUDE_DIRS}
  ${GDKPIXBUF_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GLFW3_INCLUDE_DIRS}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-write-strings")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")

add_executable(
 xglshowf
 xglshowf.cpp
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

add_executable(
 xglshow
 xglshow.cpp
 xglshow.h
)

target_link_libraries(xglshow
 ${GL_LDFLAGS}
 ${GLU_LDFLAGS}
 ${GDKPIXBUF_LDFLAGS}
 ${GLIB_LDFLAGS}
 ${GLFW3_LDFLAGS}
)




