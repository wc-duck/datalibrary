@echo off
mkdir build & pushd build
cmake -G "Visual Studio 15 2017 Win64" ..
popd
cmake --build build --config Release