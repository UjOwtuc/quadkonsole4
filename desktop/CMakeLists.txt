install(FILES quadkonsole4.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES quadkonsole4ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/quadkonsole4)

install(FILES qkremote.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES qkremote_shell.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/qkremote)

install(FILES qkremote_part.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES qkremote_part.rc DESTINATION  ${KXMLGUI_INSTALL_DIR}/qkremote)
