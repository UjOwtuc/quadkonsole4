project(quadkonsole)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (FindKDE4Internal)

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )

add_subdirectory( doc )
add_subdirectory( src )
add_subdirectory( icons )
