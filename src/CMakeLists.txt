# quadkonsole4
set(quadkonsole4_SRCS
	quadkonsole.cpp
	main.cpp
	mousemovefilter.cpp
	closedialog.cpp
	qkstack.cpp
	qkview.cpp
	prefsviews.cpp
	qkbrowseriface.cpp
	qkurlhandler.cpp
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

set(quadkonsole4_LIBS ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS})
if (LIBKONQ_FOUND)
	set(quadkonsole4_LIBS ${quadkonsole4_LIBS} ${LIBKONQ_LIBRARY})
endif()

kde4_add_executable(quadkonsole4 ${quadkonsole4_SRCS})
target_link_libraries(quadkonsole4 ${quadkonsole4_LIBS})

install(TARGETS quadkonsole4 ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES quadkonsole4.kcfg DESTINATION ${KCFG_INSTALL_DIR})

# qkremote
set(qkremote_SRCS
	qkremotemain.cpp
	qkremote.cpp
)

kde4_add_executable(qkremote ${qkremote_SRCS})
target_link_libraries(qkremote ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS qkremote ${INSTALL_TARGETS_DEFAULT_ARGS})

# qkremotepart
set(qkremote_PART_SRCS
	qkremote_part.cpp
)

kde4_add_ui_files(qkremote_PART_SRCS
	qkremotewidget.ui
)

kde4_add_plugin(qkremotepart ${qkremote_PART_SRCS})
target_link_libraries(qkremotepart ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS})

install(TARGETS qkremotepart DESTINATION ${PLUGIN_INSTALL_DIR})
