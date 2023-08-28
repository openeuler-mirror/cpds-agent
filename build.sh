__='
   Copyright 2023 CPDS Author
   
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   
        https://www.apache.org/licenses/LICENSE-2.0
   
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. 
'

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
    mkdir -p /var/log/cpds/agent
    cd ./build && make install
elif [ "$1" == "uninstall" ]; then
    cd ./build && xargs rm -v < install_manifest.txt
else
    echo "build para error"
    echo "usage: ./build.sh [install|uninstall]"
fi

cd ${CURRENT_DIR}
