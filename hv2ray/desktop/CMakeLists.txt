# Install data files

install(FILES 
  org.freedesktop.policykit.pkexec.hv2ray.policy
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/polkit-1/actions
)

install(PROGRAMS
  starthv2ray
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES 
  hv2ray.desktop
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(FILES 
  icon-hv2ray.png
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
)

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "i686" )
     install(PROGRAMS
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_32/v2ctl
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_32/v2ray
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/corev2ray
  )
    install(FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_32/geosite.dat
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_32/geoip.dat
   DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/corev2ray)

elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
      install(PROGRAMS
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_64/v2ctl
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_64/v2ray
  DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/corev2ray)

  install(FILES 
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_64/geosite.dat
  ${CMAKE_CURRENT_SOURCE_DIR}/v2ray_64/geoip.dat
   DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/corev2ray)
else()
   message(FATAL_ERROR "ERROR")
endif()


