####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = small

## INCLUDES were found outside kdevelop specific part

small_SOURCES = StyleSettings.ui ColorPicker.cc StyleEngine.cc SignalPlotter.cc objhandler.cpp ROHCProfileTable.cpp ROHCInstanceTable.cpp ROHCContextTable.cpp proccom.cpp parser.cpp saken.ui small.cpp main.cpp 
small_LDADD   =  $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)


EXTRA_DIST = main.cpp small.cpp small.h small.desktop lo32-app-small.png lo16-app-small.png saken.ui objhandler.h parser.cpp parser.h proccom.cpp proccom.h ROHCContextTable.cpp ROHCContextTable.h ROHCInstanceTable.cpp ROHCInstanceTable.h ROHCProfileTable.cpp ROHCProfileTable.h objhandler.cpp proctest SignalPlotter.cc SignalPlotter.h StyleEngine.cc StyleEngine.h ColorPicker.cc ColorPicker.h StyleSettings.ui 

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/small.desktop $(kde_appsdir)/Applications/small.desktop
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/lo32-app-small.png $(kde_icondir)/locolor/32x32/apps/small.png
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/lo16-app-small.png $(kde_icondir)/locolor/16x16/apps/small.png

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/small.desktop
	-rm -f $(kde_icondir)/locolor/32x32/apps/small.png
	-rm -f $(kde_icondir)/locolor/16x16/apps/small.png

####### kdevelop will overwrite this part!!! (end)############
# These paths are KDE specific. Use them:
# kde_appsdir         Where your application's menu entry (.desktop) should go to.
# kde_icondir         Where your icon should go to - better use KDE_ICON.
# kde_sounddir        Where your sounds should go to.
# kde_htmldir         Where your docs should go to. (contains lang subdirs)
# kde_datadir         Where you install application data. (Use a subdir)
# kde_locale          Where translation files should go to. (contains lang subdirs)
# kde_cgidir          Where cgi-bin executables should go to.
# kde_confdir         Where config files should go to (system-wide ones with default values).
# kde_mimedir         Where mimetypes .desktop files should go to.
# kde_servicesdir     Where services .desktop files should go to.
# kde_servicetypesdir Where servicetypes .desktop files should go to.
# kde_toolbardir      Where general toolbar icons should go to (deprecated, use KDE_ICON).
# kde_wallpaperdir    Where general wallpapers should go to.
# kde_templatesdir    Where templates for the "New" menu (Konqueror/KDesktop) should go to.
# kde_bindir          Where executables should go to. Use bin_PROGRAMS or bin_SCRIPTS.
# kde_libdir          Where shared libraries should go to. Use lib_LTLIBRARIES.
# kde_moduledir       Where modules (e.g. parts) should go to. Use kde_module_LTLIBRARIES.
# kde_styledir        Where Qt/KDE widget styles should go to (new in KDE 3).
# kde_designerdir     Where Qt Designer plugins should go to (new in KDE 3).

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

METASOURCES = AUTO

# the library search path. 
small_LDFLAGS = $(all_libraries) $(KDE_RPATH)

# Uncomment the following two lines if you add a ui.rc file for your application to make use of
# KDEs XML GUI builing
#rcdir = $(kde_datadir)/small
#rc_DATA = smallui.rc

#WARNING: if you use a ui.rc file above, use:

# messages: rc.cpp

# instead of 

# messages:

messages: 
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/small.pot; \
	fi

