﻿# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("OpenSSL_101")

find_package(OpenSSL)

# Include sub-projects.
add_subdirectory ("client")
add_subdirectory ("server")
