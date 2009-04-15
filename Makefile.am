
#man3_MANS = umodemd.3c

bin_PROGRAMS = umodemd

umodemd_SOURCES = umodemd.c

umodemd_CPPFLAGS = -Wall

umodemd_CFLAGS =  -g -O0

umodemd_LDFLAGS = -Wl,--rpath -Wl,/usr/local/lib -lnmea


doc::
	doxygen Doxyfile
