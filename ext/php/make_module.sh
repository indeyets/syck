#!/bin/sh
phpize
./configure --with-syck
make
make install
