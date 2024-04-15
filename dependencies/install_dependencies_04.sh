#!/bin/bash

# This file deals with installing QMP and QIO

# ---------------- QMP

wget https://usqcd-software.github.io/downloads/qmp/qmp-2.5.1.tar.gz
tar -xvzf qmp-2.5.1.tar.gz
mkdir qmp
cd qmp/
QMP_DIR=`pwd`
cd ..
cd qmp-2.5.1/
./configure --prefix=$QMP_DIR --with-qmp-comms-type=MPI CC=mpicc
make -j 4
make install
cd ..

# ---------------- QIO

wget https://usqcd-software.github.io/downloads/qio/qio-2.5.0.tar.gz

tar -xvzf qio-2.5.0.tar.gz
mkdir qio
cd qio/
QIO_DIR=`pwd`
cd ..
cd qio-2.5.0/
./configure --prefix=$QIO_DIR --with-qmp=$QMP_DIR
make -j 4
make install
cd ..

#rm -Rf qmp-2.1.7
#rm qmp-2.1.7.tar.gz
