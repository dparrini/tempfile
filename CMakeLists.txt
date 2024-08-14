cmake_minimum_required(VERSION 3.16)
project(tempfile LANGUAGES CXX)

add_library(tempfile src/tempfile.cpp)
target_include_directories(tempfile PUBLIC include PRIVATE src)

