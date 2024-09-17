@echo off
if not exist build mkdir build
cd build
cmake ../ -A Win32
cd ..
@echo on