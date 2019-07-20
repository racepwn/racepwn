#!/bin/sh

rm -rf build; mkdir build && cd build
DOPENSSL_ROOT_DIR="$(brew --prefix openssl)"
[[ -z "$DOPENSSL_ROOT_DIR" ]] && { echo "Error: DOPENSSL_ROOT_DIR not found. See https://github.com/udacity/CarND-PID-Control-Project/issues/2"; exit 1;}
cmake -DOPENSSL_ROOT_DIR=$DOPENSSL_ROOT_DIR -DOPENSSL_LIBRARIES=$DOPENSSL_ROOT_DIR+'/lib' .. && make $@
