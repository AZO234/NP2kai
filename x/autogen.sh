#!/bin/sh
aclocal
autoheader
automake -aci --foreign
autoconf
rm -f config.h.in~
./configure "$@"
make maintainer-clean > /dev/null 2>&1
