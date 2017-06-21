#!/bin/sh
aclocal
autoheader
automake -aci --foreign
autoconf
rm -f config.h.in~
./configure "$@"
rm -f ../np2tool/np2tool.d88
( cd ../np2tool && unzip -j -o np2tool.zip )
make maintainer-clean > /dev/null 2>&1
