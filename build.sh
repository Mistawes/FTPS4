#!/bin/bash

set -e

pushd tool
make
popd

make

tool/bin2js ps4-ftp-vtx.bin > exploit/payload.js
