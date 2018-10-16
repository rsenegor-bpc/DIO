#!/bin/bash

cd Desktop
cd powerdaq-3.6.26

echo "Installing drivers..."
echo password | sudo -S make install
clear

echo "Compiling tests..."
make
make test
clear

echo "Navigating to test..."
cd examples
cd SingleDIO
./SingleDIO
