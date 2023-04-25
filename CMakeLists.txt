cmake_minimum_required(VERSION 3.23)
project(copier_cpp)

set(CMAKE_CXX_STANDARD 14)

add_executable(copier_cpp copier.cpp tools.cpp configparser/ini_parser.cpp hashfile/hash.cpp)
