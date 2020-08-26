cmake_minimum_required(VERSION 3.16)
project(OpenAstra)

set(CMAKE_CXX_STANDARD 14)

add_executable(OpenAstra candump.c lib.h lib.c include/socketcan/can.h)