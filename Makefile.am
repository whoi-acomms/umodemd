
#man3_MANS = umodemd.3c

bin_PROGRAMS = umodemd

foo_SOURCES = umodemd.c

foo_CPPFLAGS = -Wall

foo_LDFLAGS = -Wl,--rpath -Wl,/usr/local/lib -lnmea


doc::
	doxygen Doxyfile
