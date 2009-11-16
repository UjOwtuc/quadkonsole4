set(quadkonsole4_SRCS
   quadkonsole.cpp
   main.cpp
   konsole.cpp
 )

kde4_add_executable(quadkonsole4 ${quadkonsole4_SRCS})

target_link_libraries(quadkonsole4 ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} )

install(TARGETS quadkonsole4 ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( FILES quadkonsole4.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR} )
