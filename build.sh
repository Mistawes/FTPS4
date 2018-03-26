#!/bin/bash

set -e

pushd tool
make
popd

make

tool/bin2js FTPS4.bin > exploit/payload.js
