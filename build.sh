#!/bin/bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
CURRENT_DIR=`pwd`

cd ${SCRIPT_DIR}

if [ ! -d ./build ]; then
    mkdir ./build
fi

if [ -z "$1" ]; then
    rm -rf ./build/*
    cd ./build && cmake .. && make
elif [ "$1" == "install" ]; then
    cd ./build && make install
elif [ "$1" == "uninstall" ]; then
    cd ./build && xargs rm -v < install_manifest.txt
else
    echo "build para error"
    echo "usage: ./build.sh [install|uninstall]"
fi

cd ${CURRENT_DIR}
