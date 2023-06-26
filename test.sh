#!/bin/sh
mkdir -p ./bin
mkdir -p ./src
STUB=./bin/stub
ENCRYPTOR="./bin/encryptor"

echo "\n######################### COMPILING stub #########################\n"

g++ -o $STUB ./src/stub.cpp && objdump -d -j .secure -M intel $STUB

readelf -t $STUB | grep "Section Headers" -A 4
readelf -t $STUB | grep secure -A 3

echo "\n######################### COMPILING ENCRYPTOR ######################\n"

g++ --no-warnings -o $ENCRYPTOR ./src/crypter.cpp
$ENCRYPTOR $STUB


echo "\n######################### EXECUTING stub    ######################\n"
objdump -d -j .secure -M intel $STUB
$stub