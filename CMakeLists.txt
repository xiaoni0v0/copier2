cmake_minimum_required(VERSION 3.23)
project(copier_cpp)

set(CMAKE_CXX_STANDARD 14)

link_libraries(ws2_32)

add_executable(copier_cpp copier.cpp tools.cpp configparser/ini_parser.cpp network/network_socket.cpp)
