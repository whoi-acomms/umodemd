#!/bin/sh
# no libtoolize needed

aclocal -I config
autoheader
automake --add-missing --copy
autoconf
