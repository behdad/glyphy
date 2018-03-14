#!/bin/bash
g++ demo-simple.cc \
    -I../src \
    -I/usr/include/freetype2 \
    -I../demo \
    -L../src/.libs \
    -lglyphy \
    -lfreetype \
    -o demo-simple

LD_LIBRARY_PATH=../src/.libs ./demo-simple
