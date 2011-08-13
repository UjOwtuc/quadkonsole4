set(quadkonsole4_SRCS
	quadkonsole.cpp
	main.cpp
	mousemovefilter.cpp
	closedialog.cpp
	qkstack.cpp
	qkview.cpp
	prefsviews.cpp
)

kde4_add_ui_files(quadkonsole4_SRCS
	prefs_base.ui
	prefs_shutdown.ui
	prefs_views.ui
	detach_processes.ui
)

kde4_add_kcfg_files(quadkonsole4_SRCS
	settings.kcfgc
)

kde4_add_executable(quadkonsole4 ${quadkonsole4_SRCS})
target_link_libraries(quadkonsole4 ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS quadkonsole4 ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES quadkonsole4.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES quadkonsole4.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(FILES quadkonsole4ui.rc DESTINATION  ${DATA_INSTALL_DIR}/quadkonsole4)
