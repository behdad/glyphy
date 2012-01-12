#!/bin/sh
aclocal --force
libtoolize --install --copy --force
/usr/bin/autoconf --force
automake --add-missing --copy --force-missing
./configure
