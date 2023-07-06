#!/bin/bash

echo "Creating package profile - conan_profile"
conan profile new conan_profile --detect
conan profile update env.CXX=clang++-9 conan_profile
conan profile show conan_profile
