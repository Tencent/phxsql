#!/bin/bash

set -e  # exit immediately on error
set -x  # display all commands

git submodule update --init --recursive

(cd third_party && ./autoinstall.sh)

if [ ! -f percona/sql/binlog.cc ]; then
    wget https://www.percona.com/downloads/Percona-Server-5.6/Percona-Server-5.6.31-77.0/source/tarball/percona-server-5.6.31-77.0.tar.gz

    rm percona -rf

    tar -xvf percona-server-5.6.31-77.0.tar.gz
    mv percona-server-5.6.31-77.0 percona
fi

./autoinstall.sh

make

make install
