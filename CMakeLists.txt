cmake_minimum_required(VERSION 2.8.12)
project(quadkonsole4)

set(QT_MIN_VERSION "5.0")

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
	Widgets
)

find_package(KF5 REQUIRED COMPONENTS
	Parts
	DBusAddons
	DocTools
)

if(CMAKE_BUILD_TYPE STREQUAL "debug")
	set(ENABLE_DEBUG "On")
endif()

option(ENABLE_DEBUG "Enable debugging code")
if(ENABLE_DEBUG)
	add_definitions(-DDEBUG)
	include_directories(${LIBKONQ_INCLUDE_DIR})
endif()

include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
git_describe(QUADKONSOLE4_VERSION)
if (NOT "${QUADKONSOLE4_VERSION}" STREQUAL HEAD-HASH-NOTFOUND)
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h" @ONLY)
endif()

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(i18n)
add_subdirectory(desktop)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/build/;/.git/;/.kdev4/;.*.kdev4;.*.swp")
set(CPACK_PACKAGE_VERSION_MAJOR ${QUADKONSOLE4_VERSION})
set(CPACK_SOURCE_PACKAGE_FILE_NAME "quadkonsole4-${QUADKONSOLE4_VERSION}")
include(CPack)
