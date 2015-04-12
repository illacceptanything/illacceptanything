cmake_minimum_required(VERSION 2.8.11)
project(illacceptanything)
file(GLOB_RECURSE ALL_FILES *.*)
add_executable(illacceptanything ${ALL_FILES})
