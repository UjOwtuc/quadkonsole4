# quadkonsole4
set(quadkonsole4_SRCS
	qkapplication.cpp
	quadkonsole.cpp
	main.cpp
	mousemovefilter.cpp
	closedialog.cpp
	qkstack.cpp
	qkview.cpp
	prefsviews.cpp
	qkbrowseriface.cpp
	qkurlhandler.cpp
	qkhistory.cpp
)

ki18n_wrap_ui(quadkonsole4_SRCS
	prefs_base.ui
	prefs_shutdown.ui
	prefs_views.ui
	detach_processes.ui
)

kconfig_add_kcfg_files(quadkonsole4_SRCS
	settings.kcfgc
)

qt5_generate_dbus_interface(quadkonsole.h de.spambri.quadkonsole4.QuadKonsole.xml OPTIONS -m -p)
qt5_generate_dbus_interface(qkapplication.h de.spambri.quadkonsole4.QKApplication.xml OPTIONS -m -p)
qt5_add_dbus_adaptor(quadkonsole4_SRCS
	${CMAKE_CURRENT_BINARY_DIR}/de.spambri.quadkonsole4.QuadKonsole.xml
	quadkonsole.h
	QuadKonsole
)
qt5_add_dbus_adaptor(quadkonsole4_SRCS
	${CMAKE_CURRENT_BINARY_DIR}/de.spambri.quadkonsole4.QKApplication.xml
	qkapplication.h
	QKApplication
)

# set(quadkonsole4_LIBS ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS})
# if (LIBKONQ_FOUND)
# 	set(quadkonsole4_LIBS ${quadkonsole4_LIBS} ${LIBKONQ_LIBRARY})
# endif()

add_executable(quadkonsole4 ${quadkonsole4_SRCS})
target_link_libraries(quadkonsole4
	KF5::Parts
	KF5::DBusAddons
)

install(TARGETS quadkonsole4 ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES quadkonsole4.kcfg DESTINATION ${KCFG_INSTALL_DIR})

# qkremote
set(qkremote_SRCS
	qkremotemain.cpp
	qkremote.cpp
	${quadkonsoleadaptor_SRCS}
)

add_executable(qkremote ${qkremote_SRCS})
target_link_libraries(qkremote KF5::Parts)

install(TARGETS qkremote ${INSTALL_TARGETS_DEFAULT_ARGS})

# qkremotepart
set(qkremote_PART_SRCS
	qkremote_part.cpp
)

ki18n_wrap_ui(qkremote_PART_SRCS
	qkremotewidget.ui
)

qt5_add_dbus_interface(qkremote_PART_SRCS
	${CMAKE_CURRENT_BINARY_DIR}/de.spambri.quadkonsole4.QuadKonsole.xml
	quadkonsoleinterface
)
qt5_add_dbus_interface(qkremote_PART_SRCS
	${CMAKE_CURRENT_BINARY_DIR}/de.spambri.quadkonsole4.QKApplication.xml
	qkapplicationinterface
)

add_library(qkremotepart MODULE ${qkremote_PART_SRCS})
target_link_libraries(qkremotepart KF5::Parts)

install(TARGETS qkremotepart DESTINATION ${PLUGIN_INSTALL_DIR})
