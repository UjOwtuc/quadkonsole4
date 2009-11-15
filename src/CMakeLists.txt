set(quadkonsole_SRCS
   quadkonsole.cpp
   main.cpp
   konsole.cpp
 )

kde4_add_executable(quadkonsole ${quadkonsole_SRCS})

target_link_libraries(quadkonsole ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} )

install(TARGETS quadkonsole ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES quadkonsole.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
