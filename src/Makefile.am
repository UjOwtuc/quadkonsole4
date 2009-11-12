# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# these are the headers for your project
noinst_HEADERS = quadkonsole.h mousemovefilter.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/quadkonsole.pot

KDE_ICON = AUTO

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = quadkonsole

# the application source, library search path, and link libraries
quadkonsole_SOURCES = main.cpp quadkonsole.cpp mousemovefilter.cpp
quadkonsole_LDFLAGS = $(KDE_RPATH) $(all_libraries)
quadkonsole_LDADD = $(LIB_KDEUI) $(LIB_KPARTS)

# this is where the desktop file will go 
shelldesktopdir   = $(kde_appsdir)/Utilities
shelldesktop_DATA = quadkonsole.desktop
