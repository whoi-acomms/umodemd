#!/bin/sh
#
# run libtoolize before first bootstrap
#

aclocal -I config
autoheader
automake --add-missing --copy
autoconf
